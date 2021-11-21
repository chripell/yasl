import hrgw
import argparse


class Impl(hrgw.Collector, hrgw.SleeperMixin):

    NAME = "Print to stdout for debugging"

    def __init__(self):
        self.running = True
        self.active = False

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--printer", type=bool, default=False,
                         help="Enable printing of data.")

    async def start(self, args):
        self.active = args.printer
        if not self.active:
            return
        print("Printer started.")

    async def push(self, p: hrgw.Data):
        if not self.active:
            return
        print(p.jd, p.name, p.data)

    async def stop(self):
        if not self.active:
            return
        print("Printer stopped.")
