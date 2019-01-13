#!/usr/bin/python3

import yasl.buspirate as bp
import yasl.tcs34725 as dev


i2c = bp.I2C("/dev/ttyUSB0", bp.I2C_SPEED_50KHZ)
tcs34725 = dev.TCS34725(i2c)
tcs34725.power_on()
tcs34725.gain(2)
tcs34725.integration_ms(100)
tcs34725.start()
tcs34725.dump()
while True:
    print("{0[0]:0>5d} {0[1]:0>5d} {0[2]:0>5d} {0[3]:0>5d}".format(
        tcs34725.read_crgb()), end="\r")
