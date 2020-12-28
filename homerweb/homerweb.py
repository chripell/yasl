from flask import Flask, g, Blueprint, render_template, request, jsonify
import sqlite3
from scipy.interpolate import interp1d
import numpy as np
import time

ROOT = "/homer"
DATABASE = "/tmp/DELME.db"
CONFIG = [
    {
        "name": "Internet throughput",
        "vars": ["RXB_enp1s0u1", "TXB_enp1s0u1"],
        "labels": ["RX MB/s", "TX MB/s"],
        "yaxis": "MB/s"
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


def resample(data, maxpoints=1000):
    if len(data) <= maxpoints:
        return data
    uz = list(zip(*data))
    F = interp1d(uz[0], uz[1], fill_value='extrapolate')
    T = np.linspace(data[0][0], data[-1][0], maxpoints)
    D = F(T)
    T *= 1000
    ndata = list(zip(T.tolist(), D.tolist()))
    return ndata


@bp.route("/graphs")
def graphs():
    return render_template('graphs.html', cfg=CONFIG)


@bp.route("/get_data")
def get_data():
    name = request.args.get("name", "", type=str)
    start = request.args.get("start", int(time.time()) - 86400, type=int)
    finish = request.args.get("finish", int(time.time()), type=int)
    maxpoints = request.args.get("maxpoints", 1000, type=int)
    conf = None
    for c in CONFIG:
        if c["name"] == name:
            conf = c
            break
    if conf is None:
        return jsonify(result={"error": "Invalid name"})
    series = []
    for v, l in zip(conf["vars"], conf["labels"]):
        data = query_db(
            r"select cast(strftime('%s', jd) as decimal), data from samples where name = ? and jd > julianday(?, 'unixepoch') and jd < julianday(?, 'unixepoch') order by jd limit 100000",
            (v, start, finish))
        series.append({
            "data": resample(data, maxpoints),
            "name": l,
        })
    return jsonify(result={
        "name": name,
        "series": series,
        "yaxis": conf["yaxis"],
    })


app.register_blueprint(bp, url_prefix="/homer")


if __name__ == "__main__":
    app.run(host="localhost")
