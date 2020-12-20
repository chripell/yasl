import asyncio
import typing
import abc
import argparse
import datetime
import julian  # type: ignore


class Data(typing.NamedTuple):
    jd: float
    name: str
    data: float


class Collector(abc.ABC):

    @abc.abstractmethod
    def push(self, name: str, data: float):
        pass


class Producer(abc.ABC):

    @abc.abstractmethod
    def register_args(self, arg: argparse.ArgumentParser):
        pass

    @abc.abstractmethod
    async def start(self, args, coll: Collector):
        pass

    @abc.abstractmethod
    async def stop(self):
        pass


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
        tasks = []
        for i in self.consumers:
            tasks.append(i.start(args))
        for i in self.producers:
            tasks.append(i.start(args, self))
        await asyncio.gather(*tasks)

    async def stop(self):
        for i in self.consumers:
            await i.stop()
        for i in self.producers:
            await i.stop()

    async def push(self, name: str, data: float, now: float = -1):
        if now < 0.0:
            now = julian.to_jd(datetime.datetime.now())
        p = Data(now, name, data)
        for i in self.consumers:
            await i.push(p)


class SleeperMixin:

    running = False

    async def sleep(self, secs: float):
        while secs > 0.001 and self.running:
            sl = min(secs, 1.0)
            await asyncio.sleep(sl)
            secs = secs - sl
