#!/usr/bin/env python3

from yasl.bt import NUS
import sys
import argparse
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import GLib  # nopep8


parser = argparse.ArgumentParser(description='Simple BT NUS terminal.')
parser.add_argument(
    '--mac', dest='mac', action='store', default='',
    help='MAC address of the device we want to connect')
parser.add_argument(
    '--name', dest='name', action='store', default='',
    help='Name of the device we want to connect')
args = parser.parse_args()


class I2C:

    def __init__(self, mac: str, name: str):
        self.nus = NUS()
        if not self.nus.find(mac=args.mac, name=args.name):
            print('NUS not found')
            sys.exit(1)
        if not self.nus.is_connected():
            self.nus.connect()
            if not self.nus.is_connected():
                print('Cannot connect to NUS')
                sys.exit(1)
        if not self.nus.find_uart():
            print('NUS missing UART UUIDs')
            sys.exit(1)

    def cb(self, b: bytes):
        print(b.decode('ascii'))
        sys.stdout.flush()
        return True

    def testf(self):
        self.nus.start_read(self.cb)
        cmd = "|770101D0"
        print(cmd)
        self.nus.write(cmd.encode('utf-8'))

i2c = I2C(args.mac, args.name)
loop = GLib.MainLoop()
GLib.idle_add(i2c.testf)
loop.run()
