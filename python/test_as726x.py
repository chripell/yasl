#!/usr/bin/env python3

import yasl.as726x as dev


as726x = dev.AS726x_SERIAL("/dev/ttyUSB0")
as726x.set_bulb_current(0)
as726x.set_indicator_current(0)
as726x.set_bulb(1)
as726x.set_indicator(1)
as726x.set_gain(3)
as726x.set_integration_ms(100)
print(as726x.measure())
print(as726x.get_temperature())
if as726x.get_version() == as726x.AS7261:
    print(as726x.get_XYZ())
    print(as726x.get_lux())
    print(as726x.get_cct())
    print(as726x.get_xy())
    print(as726x.get_uv())
    print(as726x.get_duv())
else:
    print(as726x.get_all_values())
as726x.set_bulb(0)
as726x.set_indicator(0)
