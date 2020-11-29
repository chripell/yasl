#!/usr/bin/env python3

import yasl.buspirate as bp
import yasl.vendored_tsl2591 as tsl2591
import asyncio


async def main():
    i2c = bp.I2C("/dev/ttyUSB0", bp.I2C_SPEED_50KHZ)
    tsl = tsl2591.TSL2591(i2c)
    await tsl.open()
    await tsl.set_gain(tsl2591.GAIN_HIGH)
    await tsl.set_integration_time(tsl2591.INTEGRATIONTIME_500MS)
    while True:
        lux = await tsl.lux()
        print(f"Lux: {lux}")
        await asyncio.sleep(1)


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    loop.run_until_complete(main())
