import asyncio
import hrgw
import argparse


class Impl(hrgw.Producer, hrgw.SleeperMixin):

    NAME="52 Pi Sensor"

    def __init__(self):
        self.running = True

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--sensor52-refresh-time", type=float, default=1,
                         help="Seconds between rereading values.")

    async def start(self, args, coll: hrgw.Collector):
        temp = 1.0         # DELME
        while self.running:
            await self.sleep(args.sensor52_refresh_time)
            await coll.push("T_52pi", temp)
            temp += 1

    async def stop(self):
        self.running = False
