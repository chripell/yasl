# Yet another sensors library

## as726xfw_update

This directory explains how to update the firmware on these
chips. Note that the command interface is totally different.

## buspirate

Is a simple library to use buspirate in I2C mode under Linux. Update
buspirate to the latest community firmware available at
https://github.com/BusPirate/Bus_Pirate . The python interface to it
is in `python/yasl/buspirate.py`.

## puremodules

This directory contains an improved (less buggy) version of the
original Pure Modules firmware. Also, it exports a remote I2C protocol
(see the documentation in `puremodules/protocol.c`) to allow quicker
development of sensors drivers. Check the README in the directory for
full information.

## python

Various Python libraries:

* `python/test_anus.py`, `python/yasl/abt.py` and
  `python/yasl/vendored_bme280` interface to a Nordic NUS using
  dbus/Bluetooth/asyncio. It connects to a Puremodule board and reads
  the BME280 sensor on it.

* `python/test_nus.py` and `python/yasl/bt.py` interfaces to a Nordic
  NUS using dbus/Bluetooth/glib.

* `test_rfduino.py` and `python/yasl/vendored_tsl2561` connect to a
  remote light sensor on a RFDuino.

* `python/test_as726x.py` and `python/yasl/as726x.py` is a driver for
  AMS AS726x.

* `test_tcs34725.py`, `python/yasl/buspirate.py` and
  `python/yasl/tcs34725.py` read information from a TCS34725 connected
  via buspirate.

* `test_tigard_ssd1306.py` and `python/yasl/vendored_ssd1306` connect,
  using the SPI interface on the
  [Tigard](https://github.com/tigard-tools/tigard), to a SSD1306 OLED
  display.

* `test_tsl2591.py` and `python/yasl/vendored_tsl2591` connect to a
  TSL2591 light sensor via the I2C interface on the Buspirate.

## RFDuino

Implementation of the remote I2C protocol (see
`puremodules/protocol.c`) for the RFDuino.

## HomerGW and HomerWEB

* `homergw` is my personal home sensor hub.

* `homerweb` is a Web interface to `homergw`.
