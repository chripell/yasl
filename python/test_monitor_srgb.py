#!/usr/bin/env python3

import sys
import lcms2
from collections import namedtuple
import gi
from typing import List
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GLib  # type: ignore # NOQA


Color = namedtuple("Color", "sR sG sB mR mG mB X Y Z x y")


def c8(c: int) -> float:
    return c/255.0


def calc_colors(fname: str) -> List[Color]:
    srgb_profile = lcms2.cmsCreate_sRGBProfile()
    xyz_profile = lcms2.cmsCreateXYZProfile()
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
    for r in range(0, 257, 64):
        for g in range(0, 257, 64):
            for b in range(0, 257, 64):
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
            self.cb()

    def set_color(self, c, cb=None):
        self.color = c
        self.cb = cb
        self.widget.queue_draw()


class Sequencer:

    def __init__(self, show, colors):
        self.show = show
        self.colors = colors
        self.i = 0

    def start(self):
        GLib.timeout_add(200, self.next)

    def next(self):
        self.show.set_color(self.colors[self.i])
        self.i += 1
        if self.i >= len(self.colors):
            self.i = 0
        return True


def main():
    colors = calc_colors(sys.argv[1])
    show = ColorShow()
    show.create()
    seq = Sequencer(show, colors)
    seq.start()
    Gtk.main()


if __name__ == "__main__":
    main()
