from flask import (Flask, g, Blueprint, render_template, request,
                   jsonify, escape)
import sqlite3
import pandas as pd
import time

ROOT = "/homer"
DATABASE = "/mnt/d/home.db"
CONFIG = [
    {
        "name": "Internet throughput",
        "vars": ["RXB_enp1s0u1", "TXB_enp1s0u1"],
        "labels": ["RX MB/s", "TX MB/s"],
        "yaxis": "MB/s",
    },
    {
        "name": "Temperatures",
        "vars": ["T1_52pi", "T2_52pi", "T_3d"],
        "labels": ["Pi", "Ext Pi", "3D printer"],
        "yaxis": "° C",
    },
    {
        "name": "Humidity",
        "vars": ["HU_52pi", "HU_3d"],
        "labels": ["Pi", "3D printer"],
        "yaxis": "%",
    },
    {
        "name": "Ping 8.8.8.8 latency",
        "vars": ["PL_8.8.8.8"],
        "labels": ["ping 8.8.8.8"],
        "yaxis": "ms",
    },
]

app = Flask(__name__, static_url_path="/homer/static")
bp = Blueprint("homer", __name__)


def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(DATABASE)
    return db


@app.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()


def query_db(query, args=(), one=False):
    cur = get_db().execute(query, args)
    rv = cur.fetchall()
    cur.close()
    return (rv[0] if rv else None) if one else rv


@bp.route("/graphs")
def graphs():
    return render_template('graphs.html', cfg=CONFIG)


@bp.route("/get_data")
def get_data():
    name = escape(request.args.get("name", "", type=str))
    start = request.args.get("start", int(time.time()) - 86400, type=int)
    finish = request.args.get("finish", int(time.time()), type=int)
    maxpoints = request.args.get("maxpoints", 1000, type=int)
    conf = None
    for c in CONFIG:
        if c["name"] == name:
            conf = c
            break
    if conf is None:
        return jsonify(result={"error": "Name not found"})
    series = []
    for v, l in zip(conf["vars"], conf["labels"]):
        df = pd.read_sql_query(
            "select strftime('%s', jd) as ts, data " +
            f"from samples where name='{v}' and " +
            f"jd > julianday({start}, 'unixepoch') and " +
            f"jd < julianday({finish}, 'unixepoch') " +
            "order by jd limit 100000",
            get_db(), parse_dates={"ts": "s"}, index_col="ts")
        n = len(df)
        if n > maxpoints:
            dt = df.index[-1] - df.index[0]
            df = df.resample(dt / (maxpoints-1), origin="start").mean().ffill()
        ts = df.index.astype(int)/1000000
        data = zip(ts.to_list(), df["data"].to_list())
        series.append({
            "data": list(data),
            "name": l,
            "num": n,
        })
    return jsonify(result={
        "name": name,
        "series": series,
        "yaxis": conf["yaxis"],
    })


app.register_blueprint(bp, url_prefix="/homer")


if __name__ == "__main__":
    app.run(host="localhost")
