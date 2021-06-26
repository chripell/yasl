# Yet Another Sensors Library

YASL is a collection of software for managing various sensors.

* `as726xfw_update` contains firmware update for the AMS AS726x
  spectral sensors.

* `buspirate` is a small library for using the buspirate interface in
  I2C mode.

* `homergw` is my personal home sensor hub.

* `homerweb` is a Web interface to `homergw`.

* `puremodules` implements a remote I2C protocol over Bluetooth for
  the Puremodules boards.

* `rfduino` implements a remote I2C protocol over Bluetooth for the
  RFDuino boards.

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
