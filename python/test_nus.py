#!/usr/bin/env python3

import argparse
import os
import signal
import sys
import termios
import tty
from yasl.bt import NUS
from gi.repository import GLib

parser = argparse.ArgumentParser(description='Simple BT NUS terminal.')
parser.add_argument(
    '--mac', dest='mac', action='store', default='',
    help='MAC address of the device we want to connect')
parser.add_argument(
    '--name', dest='name', action='store', default='',
    help='Name of the device we want to connect')
args = parser.parse_args()
nus = NUS()


def on_nus_rx(b: bytes):
    print(b.decode('ascii'), end='')
    sys.stdout.flush()
    return True


def on_stdin(fobj, cond):
    nus.write(
        os.read(fobj.fileno(), 8))
    return True


if not nus.find(mac=args.mac, name=args.name):
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
nus.start_read(on_nus_rx)
loop = GLib.MainLoop()
GLib.unix_signal_add(GLib.PRIORITY_DEFAULT, signal.SIGINT, lambda: loop.quit())
stdin = sys.stdin.fileno()
tattr = termios.tcgetattr(stdin)
tty.setcbreak(stdin, termios.TCSANOW)
new = termios.tcgetattr(stdin)
new[3] &= ~termios.ECHO
termios.tcsetattr(stdin, termios.TCSANOW, new)
sys.stdin.flush()
GLib.io_add_watch(sys.stdin, GLib.IO_IN, on_stdin)
loop.run()
termios.tcsetattr(stdin, termios.TCSANOW, tattr)
