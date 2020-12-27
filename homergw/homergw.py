#!/usr/bin/env python

import asyncio
import hrgw
import argparse
import signal
import importlib


def handler(sig):
    print(f"Got signal: {sig!s}, shutting down.")
    loop.create_task(hub.stop())
    loop.remove_signal_handler(signal.SIGTERM)
    loop.add_signal_handler(signal.SIGINT, lambda: None)


def register(mod: str, consumer: bool = False):
    try:
        m = importlib.import_module(mod)
        if consumer:
            hub.register_consumer(m.Impl())
        else:
            hub.register_producer(m.Impl())
    except Exception as e:
        print(f"Cannot register '{mod}': {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    hub = hrgw.Hub()
    # Registration of consumers/producers:
    register("datastore", consumer=True)
    register("disphat", consumer=True)
    register("pinger")
    register("sensor52")
    register("getmqtt")
    register("bwe")
    # Registration finished.
    hub.register_args(parser)
    args = parser.parse_args()
    loop = asyncio.get_event_loop()
    for sig in (signal.SIGTERM, signal.SIGINT):
        loop.add_signal_handler(sig, handler, sig)
    loop.run_until_complete(hub.start(args))
    tasks = asyncio.all_tasks(loop=loop)
    for t in tasks:
        t.cancel()
    group = asyncio.gather(*tasks, return_exceptions=True)
    loop.run_until_complete(group)
    loop.close()
