import hrgw
import argparse
import typing
import json
import collections
import asyncio_mqtt


class Impl(hrgw.Producer, hrgw.SleeperMixin):

    NAME = "Mqtt"

    def __init__(self):
        self.topics: typing.Dict[
            str, typing.List] = collections.defaultdict(list)

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--getmqtt-server", type=str, default="localhost",
                         help="Mqtt server.")
        arg.add_argument("--getmqtt-filter", type=str, default="zigbee2mqtt/#",
                         help="Filter for mqtt.")
        arg.add_argument("--getmqtt-topics", type=str, default="",
                         help=("Comma separated list of: " +
                               "[topic]:[field]:[db name]]."))

    async def start(self, args, coll: hrgw.Collector):
        if args.getmqtt_topics == "":
            return
        topics = [t.strip() for t in args.getmqtt_topics.split(",")]
        for t, f, d in [t.split(":") for t in topics]:
            self.topics[t].append((f, d))
        async with asyncio_mqtt.Client(args.getmqtt_server) as client:
            async with client.filtered_messages(
                    args.getmqtt_filter) as messages:
                for t in self.topics.keys():
                    await client.subscribe(t)
                async for msg in messages:
                    await self.process_msg(msg, coll)

    async def process_msg(self, msg, coll):
        if msg.topic not in self.topics:
            print(f"Got spurious topic {msg.topic}")
            return
        decoded = json.loads(msg.payload)
        for (field, name) in self.topics[msg.topic]:
            await coll.push(name, float(decoded[field]))
