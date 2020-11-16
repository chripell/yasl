#!/usr/bin/env python

import asyncio
import sys
from yasl.abt import RFDuino, I2C
from yasl.vendored_tsl2561 import TSL2561


async def main():
    nus = RFDuino()
    if not await nus.find(mac='C0:B2:DD:04:80:9C'):
        print('RFDuino not found')
        sys.exit(1)
    if not await nus.find_uart():
        print('NUS missing UART UUIDs')
        sys.exit(1)
    await nus.write('|3901018a'.encode())
    ret = await nus.read()
    print(ret.decode(errors="ignore"))
    i2c = I2C(nus)
    tsl = TSL2561(i2c)
    await tsl.open()
    print('Lux', await tsl.lux())

loop = asyncio.get_event_loop()
loop.run_until_complete(main())
