import hrgw
import argparse
from dbus_next import BusType, errors
from dbus_next.aio import MessageBus
import asyncio
import struct
from typing import Dict
import julian  # type: ignore
import datetime


def get_signed_short(b, i):
    return struct.unpack("<h", b[i: i+2])[0]


def get_unsigned_short(b, i):
    return struct.unpack("<H", b[i: i+2])[0]


def get_unsigned_long(b, i):
    return struct.unpack("<L", b[i: i+4])[0]


class Impl(hrgw.Producer):

    NAME = "Sensor Blue"

    def __init__(self):
        self.running = True

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--sb-list", type=str, default="",
                         help="Comma separated list of label@mac")

    async def start(self, args, coll: hrgw.Collector):
        if args.sb_list == "":
            return
        self.coll = coll
        await self.bt_init("hci0")
        self.macs: Dict[str, str] = {}
        for ent in (s.strip() for s in args.sb_list.split(",")):
            label, mac = ent.split("@")
            mac = mac.lower()
            self.macs[mac] = label
            try:
                await self.scan(mac)
            except errors.InterfaceNotFoundError as e:
                print(f"Exception while adding SB {mac}:", e)

    async def stop(self):
        self.running = False
        await self.dev.call_stop_discovery()

    async def bt_init(self, hci_name_short: str):
        self.bus = await MessageBus(bus_type=BusType.SYSTEM).connect()
        self.hci_name = "/org/bluez/" + hci_name_short
        introspection = await self.bus.introspect('org.bluez', self.hci_name)
        dev_obj = self.bus.get_proxy_object(
            'org.bluez', self.hci_name, introspection)
        self.dev = dev_obj.get_interface('org.bluez.Adapter1')
        await self.dev.set_powered(True)
        await self.dev.call_start_discovery()

    async def get_sb(self, mac_addr: str):
        sb_name = (
            self.hci_name + "/dev_" + mac_addr.replace(':', '_').upper())
        introspection = await self.bus.introspect('org.bluez', sb_name)
        return self.bus.get_proxy_object('org.bluez', sb_name, introspection)

    async def scan(self, mac_addr: str):
        sb_obj = await self.get_sb(mac_addr)
        sb = sb_obj.get_interface('org.freedesktop.DBus.Properties')
        sb.on_properties_changed(
            lambda iface, props, inv: asyncio.create_task(
                self.new_adv(props['ManufacturerData'].value))
            if 'ManufacturerData' in props else None)

    async def new_adv(self, prop):
        if not self.running:
            return
        for key, val in prop.items():
            data = val.value
            mac = ":".join(f"{i:02x}" for i in data[7:1:-1])
            mac = mac.lower()
            if mac not in self.macs:
                print(f"MAC {mac} not found in {self.macs}")
                continue
            label = self.macs[mac]
            if len(data) != 18:
                continue
            now = julian.to_jd(datetime.datetime.now())
            await self.coll.push(
                f"SB_BAT_{label}", get_unsigned_short(data, 8), now)
            await self.coll.push(
                f"SB_T_{label}", get_signed_short(data, 10) / 16.0, now)
            if key in (16, 17, 18):
                await self.coll.push(
                    f"SB_HUM_{label}", get_signed_short(data, 12) / 16.0, now)
