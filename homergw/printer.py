import hrgw
import argparse


class Impl(hrgw.Collector, hrgw.SleeperMixin):

    NAME = "Print to stdout for debugging"

    def __init__(self):
        self.running = True

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--printer", type=bool, default=False,
                         help="Enable printing of data.")

    async def start(self, args):
        print("Printer started.")

    async def push(self, p: hrgw.Data):
        print(p.jd, p.name, p.data)

    async def stop(self):
        print("Printer stopped.")
