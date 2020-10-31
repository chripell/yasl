#!/usr/bin/env python

import asyncio
import sys
from yasl.abt import NUS, I2C
from yasl.vendored_bme280 import sample


async def main():
    nus = NUS()
    if not await nus.find(mac='ED:A7:C5:B0:7B:D4'):
        print('NUS not found')
        sys.exit(1)
    if not await nus.find_uart():
        print('NUS missing UART UUIDs')
        sys.exit(1)
    await nus.write('|770101d0'.encode())
    ret = await nus.read()
    print(ret.decode(errors="ignore"))
    # await nus.write(b'1')
    # while True:
    #     ret = await nus.read()
    #     print(ret.decode(errors="ignore"))
    i2c = I2C(nus)
    print(await sample(i2c))


loop = asyncio.get_event_loop()
loop.run_until_complete(main())
