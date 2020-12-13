import asyncio
import typing
import abc
import argparse
import datetime
import julian


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
    def start(self, args, coll: Collector):
        pass

    @abc.abstractmethod
    def stop(self):
        pass


class Consumer(abc.ABC):

    @abc.abstractmethod
    def register_args(self, arg: argparse.ArgumentParser):
        pass

    @abc.abstractmethod
    def start(self, args):
        pass

    @abc.abstractmethod
    def push(self, p: Data):
        pass

    @abc.abstractmethod
    def stop(self):
        pass


class Hub(Collector):

    def __init__(self):
        self.consumers: List[Consumers] = []
        self.producers: List[Producers] = []

    def register_consumer(self, c: Consumer):
        self.consumers.append(c)

    def register_producer(self, p: Producer):
        self.producers.append(p)

    def register_args(self, arg: argparse.ArgumentParser):
        for i in self.consumers:
            i.register_args(arg)
        for i in self.producers:
            i.register_args(arg)

    def start(self, args):
        for i in self.consumers:
            i.start(args)
        for i in self.producers:
            i.start(args, self)

    def stop(self):
        for i in self.consumers:
            i.stop()
        for i in self.producers:
            i.stop()

    def push(self, name: str, data: float):
        p = Data(julian.to_jd(datetime.datetime.now()),
                 name, data)
        for i in self.consumers:
            i.push(p)

class RunnerMixin():

    NAME="unknown"

    def run_task(self, every: float):
        loop = asyncio.get_event_loop()
        self.__every = every
        self.__task = loop.create_task(self.__run())

    async def __run(self):
        active = True
        while active:
            try:
                await asyncio.sleep(self.__every)
                active = await self._action()
            except Exception as e:
                print(f"Task '{self.NAME}' died: {e}")
                active = False

    def cancel_task(self):
        self.__task.cancel()
