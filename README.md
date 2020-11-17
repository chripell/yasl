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

* `yasl/abt.py` asyncio based Bluetooth library for connecting to
  RFDuino and Puremodules (and other Nordic UARTs).
  
* `yasl/as726x` driver for the as726x chips in serial mode and I2C
  mode. I use exclusively the formet, so the latter is not well
  tested.
  
* `yasl/bt.py` Glib based Bluetooth library for connecting to Nordic
  UARTs.
  
* `yasl/buspirate.py` library for interfacing with the I2C functions
  of Buspirate.
  
* `yasl/tcs34725.py` driver for the TCS34725.

* `yasl/vendored_bme280` vendored BME280 driver which uses asyncio.
  
* `yasl/vendored_tsl2561` vendored TSL2561 driver which uses asyncio.

## RFDuino

Implementation of the remote I2C protocol (see
`puremodules/protocol.c`) for the RFDuino.

