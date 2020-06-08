#!/usr/bin/env python3

import sys
import time
from yasl.bt import NUS
from gi.repository import GLib


def rx(nus):
    for d in nus.read():
        print(d, end='')
    return True


nus = NUS()
if not nus.find(mac='ED:A7:C5:B0:7B:D4'):
    print('NUS not found')
    sys.exit(1)
if not nus.is_connected():
    nus.connect()
    if not nus.is_connected():
        print('Cannot connect to NUS')
        sys.exit(1)
if not nus.find_uart():
    print('NUS missing UART UUIDs')
    sys.exit(1)
GLib.timeout_add(1000, lambda: rx(nus))
nus.start_read()
for b in [b'5' b'7' b'9' b'b' b'd' b'f']:
    nus.write(b)
loop = GLib.MainLoop()
loop.run()
