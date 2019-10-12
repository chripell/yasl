#!/usr/bin/env python3

import sys
import lcms2  # type: ignore
from collections import namedtuple
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk  # type: ignore # NOQA

Color = namedtuple("Color", "R G B r g b X Y Z x y")

srgb_profile = lcms2.cmsCreate_sRGBProfile()
xyz_profile = lcms2.cmsCreateXYZProfile()
monitor_profile = lcms2.cmsOpenProfileFromFile(sys.argv[1])

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
            # print("%03d %03d %03d: %03d %03d %03d: %.3f %.3f %.3f: %.3f %.3f"
            #       % (
            #           r, g, b,
            #           rgb[0], rgb[1], rgb[2],
            #           xyz[0], xyz[1], xyz[2],
            #           x, y))

window = Gtk.Window(title="sRGB tester")
window.show()
window.connect("destroy", Gtk.main_quit)
Gtk.main()
