import asyncio
import typing
import abc
import argparse
import datetime
import contextlib
import traceback
import os
import julian  # type: ignore


class Data(typing.NamedTuple):
    jd: float
    name: str
    data: float


class Collector(abc.ABC):

    @abc.abstractmethod
    def push(self, name: str, data: float, now: float = -1):
        pass


class Producer(abc.ABC):

    @abc.abstractmethod
    def register_args(self, arg: argparse.ArgumentParser):
        pass

    @abc.abstractmethod
    async def start(self, args, coll: Collector):
        pass

    async def stop(self) -> bool:
        return True


class Consumer(abc.ABC):

    @abc.abstractmethod
    def register_args(self, arg: argparse.ArgumentParser):
        pass

    @abc.abstractmethod
    async def start(self, args):
        pass

    @abc.abstractmethod
    async def push(self, p: Data):
        pass

    @abc.abstractmethod
    async def stop(self):
        pass


class Hub(Collector):

    def __init__(self):
        self.consumers: typing.List[Consumer] = []
        self.producers: typing.List[Producer] = []

    def register_consumer(self, c: Consumer):
        self.consumers.append(c)

    def register_producer(self, p: Producer):
        self.producers.append(p)

    def register_args(self, arg: argparse.ArgumentParser):
        for i in self.consumers:
            i.register_args(arg)
        for j in self.producers:
            j.register_args(arg)

    async def start(self, args):
        self.consumer_tasks = []
        self.producer_tasks = []
        for i in self.consumers:
            self.consumer_tasks.append(
                asyncio.create_task(run_consumer(i, args), name=i.NAME))
        for i in self.producers:
            self.producer_tasks.append(
                asyncio.create_task(run_producer(i, args, self), name=i.NAME))
        try:
            await asyncio.gather(*self.producer_tasks, *self.consumer_tasks)
        except Exception as e:
            print("Exception:", e)
            print(traceback.format_exc())
            try:
                await self.stop()
            except Exception as e:
                print("Stop failed with exception, emergency exit!", e,
                      flush=True)
                print(traceback.format_exc(), flush=True)
                os._exit(1)
            return

    async def stop(self):
        for i, t in zip(self.consumers, self.consumer_tasks):
            if await i.stop():
                t.cancel()
        for i, t in zip(self.producers, self.producer_tasks):
            if await i.stop():
                t.cancel()

    async def push(self, name: str, data: float, now: float = -1):
        if now < 0.0:
            now = julian.to_jd(datetime.datetime.now())
        p = Data(now, name, data)
        for i in self.consumers:
            await i.push(p)


async def run_consumer(cons, args):
    with contextlib.suppress(asyncio.exceptions.CancelledError):
        await cons.start(args)


async def run_producer(cons, args, coll):
    with contextlib.suppress(asyncio.exceptions.CancelledError):
        await cons.start(args, coll)


class SleeperMixin:

    running = False

    async def sleep(self, secs: float):
        while secs > 0.001 and self.running:
            sl = min(secs, 1.0)
            await asyncio.sleep(sl)
            secs = secs - sl
