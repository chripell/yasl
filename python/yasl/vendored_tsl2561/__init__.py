# The MIT License (MIT)
#
# Copyright (c) 2017 Carter Nelson for Adafruit Industries
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
"""
`adafruit_tsl2561`
====================================================

CircuitPython driver for TSL2561 Light Sensor.

* Author(s): Carter Nelson

Implementation Notes
--------------------

**Hardware:**

* Adafruit `TSL2561 Digital Luminosity/Lux/Light Sensor Breakout
  <https://www.adafruit.com/product/439>`_ (Product ID: 439)

* Adafruit `STEMMA - TSL2561 Digital Lux / Light Sensor
  <https://www.adafruit.com/product/3611>`_ (Product ID: 3611)

* Adafruit `Flora Lux Sensor - TSL2561 Light Sensor
  <https://www.adafruit.com/product/1246>`_ (Product ID: 1246)

**Software and Dependencies:**

* Adafruit CircuitPython firmware for the ESP8622 and M0-based boards:
  https://github.com/adafruit/circuitpython/releases
* Adafruit's Bus Device library: https://github.com/adafruit/Adafruit_CircuitPython_BusDevice
"""
__version__ = "0.0.0-auto.0"
__repo__ = "https://github.com/adafruit/Adafruit_CircuitPython_TSL2561.git"

_DEFAULT_ADDRESS = 0x39
_COMMAND_BIT = 0x80
_WORD_BIT = 0x20

_CONTROL_POWERON = 0x03
_CONTROL_POWEROFF = 0x00

_REGISTER_CONTROL = 0x00
_REGISTER_TIMING = 0x01
_REGISTER_TH_LOW = 0x02
_REGISTER_TH_HIGH = 0x04
_REGISTER_INT_CTRL = 0x06
_REGISTER_CHAN0_LOW = 0x0C
_REGISTER_CHAN1_LOW = 0x0E
_REGISTER_ID = 0x0A

_GAIN_SCALE = (16, 1)
_TIME_SCALE = (1 / 0.034, 1 / 0.252, 1)
_CLIP_THRESHOLD = (4900, 37000, 65000)


class TSL2561:
    """Class which provides interface to TSL2561 light sensor."""

    def __init__(self, i2c, address=_DEFAULT_ADDRESS):
        self.buf = bytearray(3)
        self.i2c = i2c
        self.addr = address

    async def open(self):
        partno, revno = await self.chip_id()
        # data sheet says TSL2561 = 0001, reality says 0101
        if not partno == 5:
            raise RuntimeError(
                "Failed to find TSL2561! Part 0x%x Rev 0x%x" % (partno, revno)
            )
        await self.set_enabled(True)

    async def chip_id(self):
        """A tuple containing the part number and the revision number."""
        chip_id = await self._read_register(_REGISTER_ID)
        partno = (chip_id >> 4) & 0x0F
        revno = chip_id & 0x0F
        return (partno, revno)

    async def enabled(self):
        """The state of the sensor."""
        return (await self._read_register(_REGISTER_CONTROL) & 0x03) != 0

    async def set_enabled(self, enable):
        """Enable or disable the sensor."""
        if enable:
            await self._enable()
        else:
            await self._disable()

    async def lux(self):
        """The computed lux value or None when value is not computable."""
        return await self._compute_lux()

    async def broadband(self):
        """The broadband channel value."""
        return await self._read_broadband()

    async def infrared(self):
        """The infrared channel value."""
        return await self._read_infrared()

    async def luminosity(self):
        """The overall luminosity as a tuple containing the broadband
        channel and the infrared channel value."""
        return (await self.broadband(), await self.infrared())

    async def gain(self):
        """The gain. 0:1x, 1:16x."""
        return await self._read_register(_REGISTER_TIMING) >> 4 & 0x01

    async def set_gain(self, value):
        """Set the gain. 0:1x, 1:16x."""
        value &= 0x01
        value <<= 4
        current = await self._read_register(_REGISTER_TIMING)
        cmd = _COMMAND_BIT | _REGISTER_TIMING
        data = (current & 0xEF) | value
        await self.i2c.write_i2c_block_data(self.addr, cmd, [data])

    async def integration_time(self):
        """The integration time. 0:13.7ms, 1:101ms, 2:402ms, or 3:manual"""
        current = await self._read_register(_REGISTER_TIMING)
        return current & 0x03

    async def set_integration_time(self, value):
        """Set the integration time. 0:13.7ms, 1:101ms, 2:402ms, or 3:manual."""
        value &= 0x03
        current = await self._read_register(_REGISTER_TIMING)
        cmd = _COMMAND_BIT | _REGISTER_TIMING
        data = (current & 0xFC) | value
        await self.i2c.write_i2c_block_data(self.addr, cmd, [data])

    async def threshold_low(self):
        """The low light interrupt threshold level."""
        low, high = await self._read_register(_REGISTER_TH_LOW, 2)
        return high << 8 | low

    async def set_threshold_low(self, value):
        cmd = _COMMAND_BIT | _WORD_BIT | _REGISTER_TH_LOW
        buf = (value & 0xFF, (value >> 8) & 0xFF)
        await self.i2c.write_i2c_block_data(self.addr, cmd, buf)

    async def threshold_high(self):
        """The upper light interrupt threshold level."""
        low, high = await self._read_register(_REGISTER_TH_HIGH, 2)
        return high << 8 | low

    async def set_threshold_high(self, value):
        cmd = _COMMAND_BIT | _WORD_BIT | _REGISTER_TH_HIGH
        buf = (value & 0xFF, (value >> 8) & 0xFF)
        await self.i2c.write_i2c_block_data(self.addr, cmd, buf)

    async def cycles(self):
        """The number of integration cycles for which an out of bounds
           value must persist to cause an interrupt."""
        value = await self._read_register(_REGISTER_INT_CTRL)
        return value & 0x0F

    async def set_cycles(self, value):
        current = await self._read_register(_REGISTER_INT_CTRL)
        cmd = _COMMAND_BIT | _REGISTER_INT_CTRL
        data = current | (value & 0x0F)
        await self.i2c.write_i2c_block_data(self.addr, cmd, [data])

    async def interrupt_mode(self):
        """The interrupt mode selection.

        ==== =========================
        Mode Description
        ==== =========================
         0   Interrupt output disabled
         1   Level Interrupt
         2   SMBAlert compliant
         3   Test Mode
        ==== =========================

        """
        return (await self._read_register(_REGISTER_INT_CTRL) >> 4) & 0x03

    async def set_interrupt_mode(self, value):
        current = await self._read_register(_REGISTER_INT_CTRL)
        cmd = _COMMAND_BIT | _REGISTER_INT_CTRL
        data = (current & 0x0F) | ((value & 0x03) << 4)
        await self.i2c.write_i2c_block_data(self.addr, cmd, [data])

    async def clear_interrupt(self):
        """Clears any pending interrupt."""
        cmd = 0xC0
        await self.i2c.write_i2c_block_data(self.addr, cmd, [])

    async def _compute_lux(self):
        """Based on datasheet for FN package."""
        ch0, ch1 = await self.luminosity()
        if ch0 == 0:
            return None
        if ch0 > _CLIP_THRESHOLD[await self.integration_time()]:
            return None
        if ch1 > _CLIP_THRESHOLD[await self.integration_time()]:
            return None
        ratio = ch1 / ch0
        if 0 <= ratio <= 0.50:
            lux = 0.0304 * ch0 - 0.062 * ch0 * ratio ** 1.4
        elif ratio <= 0.61:
            lux = 0.0224 * ch0 - 0.031 * ch1
        elif ratio <= 0.80:
            lux = 0.0128 * ch0 - 0.0153 * ch1
        elif ratio <= 1.30:
            lux = 0.00146 * ch0 - 0.00112 * ch1
        else:
            lux = 0.0
        # Pretty sure the floating point math formula on pg. 23 of datasheet
        # is based on 16x gain and 402ms integration time. Need to scale
        # result for other settings.
        # Scale for gain.
        lux *= _GAIN_SCALE[await self.gain()]
        # Scale for integration time.
        lux *= _TIME_SCALE[await self.integration_time()]
        return lux

    async def _enable(self):
        await self._write_control_register(_CONTROL_POWERON)

    async def _disable(self):
        await self._write_control_register(_CONTROL_POWEROFF)

    async def _read_register(self, reg, count=1):
        cmd = _COMMAND_BIT | reg
        if count == 2:
            cmd |= _WORD_BIT
        ret = await self.i2c.read_i2c_block_data(self.addr, cmd, count)
        if count == 2:
            return ret
        return ret[0]

    async def _write_control_register(self, reg):
        cmd = _COMMAND_BIT | _REGISTER_CONTROL
        await self.i2c.write_i2c_block_data(self.addr, cmd, [reg])

    async def _read_broadband(self):
        low, high = await self._read_register(_REGISTER_CHAN0_LOW, 2)
        return high << 8 | low

    async def _read_infrared(self):
        low, high = await self._read_register(_REGISTER_CHAN1_LOW, 2)
        return high << 8 | low
