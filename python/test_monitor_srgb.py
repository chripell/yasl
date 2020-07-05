#!/usr/bin/env python3

import argparse
import csv
import lcms2
from collections import namedtuple
import gi
from typing import List
import yasl.as726x as as726x
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk  # type: ignore # NOQA


Color = namedtuple("Color", "sR sG sB mR mG mB X Y Z x y")


def c8(c: int) -> float:
    return c/255.0


def calc_colors(fname: str, step: int) -> List[Color]:
    srgb_profile = lcms2.cmsCreate_sRGBProfile()
    xyz_profile = lcms2.cmsCreateXYZProfile()
    if fname == '':
        monitor_profile = lcms2.cmsCreate_sRGBProfile()
    else:
        monitor_profile = lcms2.cmsOpenProfileFromFile(fname)

    srgb2monitor = lcms2.cmsCreateTransform(
        srgb_profile, lcms2.TYPE_RGB_8,
        monitor_profile, lcms2.TYPE_RGB_8,
        renderingIntent=lcms2.INTENT_RELATIVE_COLORIMETRIC)
    srgb2xyz = lcms2.cmsCreateTransform(
        srgb_profile, lcms2.TYPE_RGB_8,
        xyz_profile, lcms2.TYPE_XYZ_DBL,
        renderingIntent=lcms2.INTENT_RELATIVE_COLORIMETRIC)

    colors = []
    endc = 254 + step
    for r in range(0, endc, step):
        for g in range(0, endc, step):
            for b in range(0, endc, step):
                if r > 255:
                    r = 255
                if g > 255:
                    g = 255
                if b > 255:
                    b = 255
                xyz = [0.0, 0.0, 0.0, 0.0, lcms2.COLOR_DBL]
                rgb = [0, 0, 0, 0, lcms2.COLOR_BYTE]
                lcms2.cmsDoTransform(
                    srgb2xyz,
                    [r, g, b, 0, lcms2.COLOR_BYTE],
                    xyz)
                lcms2.cmsDoTransform(
                    srgb2monitor, [r, g, b, 0, lcms2.COLOR_BYTE], rgb)
                s = xyz[0] + xyz[1] + xyz[2]
                if s != 0.0:
                    x = xyz[0] / s
                    y = xyz[1] / 2
                else:
                    x = 0
                    y = 0
                colors.append(
                    Color(r, g, b,
                          rgb[0], rgb[1], rgb[2],
                          xyz[0], xyz[1], xyz[2],
                          x, y))
    return colors


class ColorShow:

    def __init__(self):
        self.color = None
        self.cb = None

    def create(self):
        self.widget = Gtk.DrawingArea()
        self.widget.connect("draw", self.draw)
        window = Gtk.Window(title="sRGB tester")
        window.add(self.widget)
        window.show_all()
        window.connect("destroy", Gtk.main_quit)

    def draw(self, w, cr):
        if not self.color:
            return
        width = w.get_allocated_width()
        height = w.get_allocated_height()
        cr.set_source_rgb(c8(self.color.mR),
                          c8(self.color.mG),
                          c8(self.color.mB))
        cr.rectangle(0, 0, width, height)
        cr.fill()
        if self.cb:
            self.cb(self.color)

    def set_color(self, c, cb=None):
        self.color = c
        self.cb = cb
        self.widget.queue_draw()


class Sequencer:

    def __init__(self, show, colors, out, dev1=None, dev2=None):
        self.show = show
        self.colors = colors
        self.dev1 = dev1
        self.dev2 = dev2
        self.out = out

    def start(self):
        self.i = 0
        self.res = []
        self.next()

    def next(self):
        if self.i >= len(self.colors):
            self.finish()
            return
        self.show.set_color(self.colors[self.i], self.measure)
        self.i += 1

    def measure(self, color):
        r = list(color)
        # TODO: adapt integration time to lux?
        if self.dev1 is not None:
            r.append(self.dev1.get_temperature())
            r.extend(self.dev1.measure())
            r.append(self.dev1.get_lux())
            r.append(self.dev1.get_cct())
            r.extend(self.dev1.get_xy())
            r.extend(self.dev1.get_uv())
            r.append(self.dev1.get_duv())
        else:
            r.extend([0]*11)
        if self.dev2 is not None:
            r.append(self.dev2.get_temperature())
            r.extend(self.dev2.measure())
        else:
            r.extend([0]*7)
        self.res.append(r)
        self.next()

    def finish(self):
        with open(self.out, 'w', newline='') as csvfile:
            wr = csv.writer(csvfile)
            for r in self.res:
                wr.writerow(r)
        Gtk.main_quit()


def open_device(d, args):
    dev = as726x.AS726x_SERIAL(d)
    dev.set_bulb_current(0)
    dev.set_indicator_current(0)
    dev.set_bulb(0)
    dev.set_indicator(0)
    dev.set_gain(args.gain)
    dev.set_integration(args.int_time)
    return dev


def main():
    parser = argparse.ArgumentParser(description='sRGB monitor tester.')
    parser.add_argument(
        '--icc', dest='icc', action='store', default='',
        help='ICC profile for the monitor')
    parser.add_argument(
        '--step', dest='step', action='store', default=64,
        help='Sample step for the color space')
    parser.add_argument(
        '--deva', dest='deva', action='store', default='',
        help='First device for acquisition')
    parser.add_argument(
        '--devb', dest='devb', action='store', default='',
        help='Second device for acquisition')
    parser.add_argument(
        '--out', dest='out', action='store', default='out.csv',
        help='CSV file with results')
    parser.add_argument(
        '--gain', dest='gain', action='store', default=0,
        help='gain 0-3')
    parser.add_argument(
        '--int-time', dest='int_time', action='store', default=30,
        help='integration time 1-255')
    args = parser.parse_args()
    dev1 = None
    dev2 = None
    if args.deva != '':
        deva = open_device(args.deva, args)
        if deva.get_version() == as726x.AS7261:
            dev1 = deva
        else:
            dev2 = deva
    if args.devb != '':
        devb = open_device(args.deva, args)
        if deva.get_version() == as726x.AS7261:
            dev1 = devb
        else:
            dev2 = devb
    colors = calc_colors(args.icc, args.step)
    show = ColorShow()
    show.create()
    seq = Sequencer(show, colors, args.out, dev1, dev2)
    seq.start()
    Gtk.main()


if __name__ == "__main__":
    main()
