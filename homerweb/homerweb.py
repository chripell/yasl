from flask import (Flask, g, Blueprint, render_template, request,
                   jsonify, escape, abort)
import sqlite3
import pandas as pd
import time
import os
import paho.mqtt.client as mqttc
import threading
import json
from collections import defaultdict


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
        "vars": ["T1_52pi", "T2_52pi", "T_3d", "T_cu", "T_ca", "SB_T_0"],
        "labels": ["Pi", "Ext Pi", "3D printer", "Cucina", "Camera",
                   "Esterno"],
        "yaxis": "° C",
    },
    {
        "name": "Humidity",
        "vars": ["HU_3d", "HU_cu", "HU_ca", "SB_HUM_0"],
        "labels": ["3D printer", "Cucina", "Camera", "Esterno"],
        "yaxis": "%",
    },
    {
        "name": "Ping 8.8.8.8",
        "vars": ["PL_8.8.8.8", "P%_8.8.8.8"],
        "labels": ["Latency", "Loss"],
        "yaxis": "ms/%",
    },
]

GET = 1 << 0
SET = 1 << 1
CMDS = [
    {
        "name": "Lampadina PC",
        "id": "lampadina_pc",
        "cmds": [
            {
                "name": "state",
                "desc": "State ON/OFF/TOGGLE",
                "prop": GET + SET,
            },
            {
                "name": "brightness",
                "desc": "Brigtness 0-254",
                "prop": GET + SET,
            },
            {
                "name": "color_temp",
                "desc": "Color temperature 150-500",
                "prop": GET + SET,
            },
            {
                "name": "color",
                "desc": "Color R,G,B",
                "prop": SET,
                "fields": ["r", "g", "b"],
            },
            {
                "name": "linkquality",
                "desc": "Link Quality 0-255",
                "prop": GET,
            },
        ]
    }
]


app = Flask(__name__, static_url_path="/homer/static")
bp = Blueprint("homer", __name__)


class MQTT:

    def __init__(self, props):
        self._props = props
        server = os.getenv("MQTT_SERVER")
        if server is None:
            server = "localhost"
        self._client = mqttc.Client(client_id="homerweb")
        self._client.connect(server)
        for dev in props:
            self._client.subscribe(f"zigbee2mqtt/{dev}")
        self._data = defaultdict(lambda: defaultdict(dict))
        self._lock = threading.Lock()
        self._client.on_message = self.on_message
        self._client.loop_start()
        self._refreshed = 0
        self._max_wait = 1
        self.refresh()

    def close(self):
        self._client.disconnect()
        self._client.loop_stop()

    def on_message(self, client, userdata, message):
        dev_l = message.topic.split("/")
        if len(dev_l) != 2:
            app.logger.warning("Invalid topic received: " + message.topic)
        dev = dev_l[1]
        if dev not in self._props:
            app.logger.warning("Invalid data received: " + message.topic)
        d = json.loads(message.payload)
        now = time.time()
        with self._lock:
            for k, v in d.items():
                if isinstance(v, dict):
                    for idx, vv in v.items():
                        self._data[dev][(k, idx)] = (vv, now)
                else:
                    self._data[dev][k] = (v, now)

    def refresh(self):
        self._refreshed = time.time()
        for dev, ps in self._props.items():
            payload = json.dumps({k: "" for k in ps})
            self._client.publish(f"zigbee2mqtt/{dev}/get", payload)

    def get(self, dev, var):
        deadline = time.time() + self._max_wait
        val = None
        while time.time() < deadline:
            with self._lock:
                if dev in self._data:
                    val = self._data[dev].get(var, None)
            if val is None or val[1] < self._refreshed:
                continue
            return val
        if val is None:
            return "NONE", 0
        return val

    def set(self, dev, var):
        payload = json.dumps(var)
        print("DELME", f"zigbee2mqtt/{dev}/set", payload)
        self._client.publish(f"zigbee2mqtt/{dev}/set", payload)


def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(DATABASE)
    return db


def get_mqtt():
    mqtt = getattr(g, '_mqtt', None)
    if mqtt is None:
        cmds = {}
        for dev in CMDS:
            cmds[dev["id"]] = [
                i["name"] for i in dev["cmds"] if i["prop"] & GET != 0]
        mqtt = g._mqtt = MQTT(cmds)
    return mqtt


@app.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()
    mqtt = getattr(g, '_mqtt', None)
    if mqtt is not None:
        mqtt.close()


def query_db(query, args=(), one=False):
    cur = get_db().execute(query, args)
    rv = cur.fetchall()
    cur.close()
    return (rv[0] if rv else None) if one else rv


@bp.route("/")
def main():
    return render_template('main.html', cfg=CONFIG)


@bp.route("/graphs")
def graphs():
    return render_template('graphs.html', cfg=CONFIG)


@bp.route("/control/<id>", methods=['GET', 'POST'])
def control(id):
    mqtt = get_mqtt()
    if id is None or len(id) == 0:
        abort(404)
    ids = [i for i in CMDS if i["id"] == id]
    if len(ids) == 0:
        abort(404)
    dev = ids[0]
    if request.method == "POST":
        action = request.form["action"]
        for e in dev["cmds"]:
            if e["name"] != action:
                continue
            new_text = request.form[action]
            if "fields" in e:
                new_vals = (i.strip() for i in new_text.split(","))
                cmd = {k: v for k, v in zip(e["fields"], new_vals)}
            else:
                cmd = new_text.strip()
            mqtt.set(dev["id"], {e["name"]: cmd})
        time.sleep(1)
    now = time.time()
    mqtt.refresh()
    elements = []
    for e in dev["cmds"]:
        stale = False
        if e["prop"] & GET != 0:
            if "fields" in e:
                vals = []
                for f in e["fields"]:
                    vv = mqtt.get(dev["id"], (e["name"], f))
                    vals.append(vv[0])
                    if vv[1] < now:
                        stale = True
                val = ", ".join(str(v) for v in vals)
            else:
                vv = mqtt.get(dev["id"], e["name"])
                val = str(vv[0])
                if vv[1] < now:
                    stale = True
        else:
            val = ""
        elements.append({
            "id": e["name"],
            "label": e["desc"],
            "val": val,
            "stale": stale,
            "submit": e["prop"] & SET != 0,
        })
    return render_template('control.html', name=dev["name"],
                           id=dev["id"], elements=elements)


@bp.route("/get_data/<name>/<int:start>/<int:finish>/<int:maxpoints>")
def get_data(name, start, finish, maxpoints):
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
