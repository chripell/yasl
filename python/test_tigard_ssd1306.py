#!/usr/bin/env python

from pyftdi.spi import SpiController
from os import environ
from yasl.vendored_ssd1306 import SSD1306_128_64
from PIL import Image


# from pyftdi.ftdi import Ftdi
# Ftdi.show_devices()


# 1 BD0 SCK	 -> CLK
# 2 BD1 COPI -> DAT
# 3 BD2 CIPO ->
# 4 BD3 CS	 -> CS
# 5 BD4 TRST -> RST
# 6 BD5 SRST -> DC

ftdi_url = environ.get('FTDI_DEVICE', 'ftdi://ftdi:2232:TG10004a/2')
spi = SpiController()
spi.configure(ftdi_url)
gpio = spi.get_gpio()
gpio.set_direction(0x30, 0x30)

disp = SSD1306_128_64(rst=0x10, dc=0x20, gpio=gpio, spi=spi)

im = Image.open("Pirate.bmp")
im = im.resize((128, 64))
im = im.convert("1")
disp.begin()
disp.image(im)
disp.display()
disp.dim(False)
disp.set_contrast(255)
