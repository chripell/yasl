import asyncio
import hrgw
import argparse


class Impl(hrgw.Producer, hrgw.RunnerMixin):

    NAME="52 Pi Sensor"

    def __init__(self):
        pass

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--sensor52-refresh-time", type=float, default=1,
                         help="Seconds between rereading values.")

    def start(self, args, coll: hrgw.Collector):
        self.args = args
        self.coll = coll
        self.temp = 1.0         # DELME
        self.run_task(self.args.sensor52_refresh_time)

    async def _action(self):
        self.coll.push("T_52pi", self.temp)
        self.temp += 1
        return True

    def stop(self):
        self.cancel_task()
