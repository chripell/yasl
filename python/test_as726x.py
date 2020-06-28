#!/usr/bin/env python3

import time
import yasl.as726x as dev


as726x = dev.AS726x_SERIAL("/dev/ttyUSB0")
as726x.set_bulb_current(0)
as726x.set_indicator_current(0)
as726x.set_bulb(0)
as726x.set_indicator(0)
as726x.set_gain(0)
as726x.set_integration(30)
print("Temperature ", as726x.get_temperature())
n = 10
tot_t = 0.0
if as726x.get_version() == as726x.AS7261:
    for i in range(n):
        print("Measure ", i)
        start_t = time.time()
        m = as726x.measure()
        tot_t = tot_t + (time.time() - start_t)
        print("raw", m)
        print("allc", as726x.get_all_values(cal=False))
        if m is None:
            continue
        print("XYZ", as726x.get_XYZ())
        print("LUX", as726x.get_lux())
        print("CCT", as726x.get_cct())
        print("xy", as726x.get_xy())
        print("UV", as726x.get_uv())
        print("DUV", as726x.get_duv())
    print("Time per read in ms: %d" % (
        tot_t / n * 1000.0))
else:
    print(as726x.get_all_values())
#as726x.set_bulb(0)
#as726x.set_indicator(0)
