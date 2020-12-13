import asyncio
import hrgw
import argparse


class Impl(hrgw.Collector, hrgw.RunnerMixin):

    def __init__(self):
        self.data = []

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--data-store-flush-time", type=float, default=5,
                         help="Seconds between flushes to storage.")

    def start(self, args):
        self.args = args
        self.run_task(self.args.data_store_flush_time)

    async def _action(self):
        for d in self.data:
            print(d)
        self.data = []
        return True

    def push(self, p: hrgw.Data):
        self.data.append(p)

    def stop(self):
        self.cancel_task()
        for d in self.data:
            print(d)
