import asyncio
import hrgw
import argparse
import time
import datetime
import re
import typing
import julian  # type: ignore


class Sample(typing.NamedTuple):
    rx_bytes: int
    rx_packets: int
    tx_bytes: int
    tx_packets: int


class Impl(hrgw.Producer):

    NAME = "Bandwidth monitor"
    PARSE_LINE = re.compile(
        r"(\w+):\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)" +
        r"\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)" +
        r"\s+(\d+)\s+(\d+)")

    def __init__(self):
        self.ifaces = []

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--bwe-refresh-time", type=float, default=10,
                         help="Seconds between rereading values.")
        arg.add_argument("--bwe-ifaces", type=str, default="",
                         help="Comma separated list of interfaces.")

    def sample(self):
        r = {}
        with open("/proc/net/dev") as f:
            for ll in f:
                m = self.PARSE_LINE.search(ll)
                if m is None:
                    continue
                if m[1] not in self.ifaces:
                    continue
                r[m[1]] = Sample(int(m[2]), int(m[3]), int(m[10]), int(m[11]))
        return r

    async def start(self, args, coll: hrgw.Collector):
        if args.bwe_ifaces == "":
            return
        self.ifaces = [i.strip() for i in args.bwe_ifaces.split("'")]
        prev = self.sample()
        prev_t = time.time()
        MB = 1024 * 1024
        while True:
            await asyncio.sleep(args.bwe_refresh_time)
            now = self.sample()
            now_t = time.time()
            delta_t = now_t - prev_t
            jd = julian.to_jd(datetime.datetime.now())
            for i in self.ifaces:
                await coll.push(
                    f"RXB_{i}",
                    (now[i].rx_bytes - prev[i].rx_bytes) / delta_t / MB,
                    jd)
                await coll.push(
                    f"RXP_{i}",
                    (now[i].rx_packets - prev[i].rx_packets) / delta_t,
                    jd)
                await coll.push(
                    f"TXB_{i}",
                    (now[i].tx_bytes - prev[i].tx_bytes) / delta_t / MB,
                    jd)
                await coll.push(
                    f"TXP_{i}",
                    (now[i].tx_packets - prev[i].tx_packets) / delta_t,
                    jd)
            prev = now
            prev_t = now_t
