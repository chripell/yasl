#!/usr/bin/env python

import asyncio
import sys
from yasl.abt import RFDuino


async def main():
    nus = RFDuino()
    if not await nus.find(mac='C0:B2:DD:04:80:9C'):
        print('RFDuino not found')
        sys.exit(1)
    if not await nus.find_uart():
        print('NUS missing UART UUIDs')
        sys.exit(1)
    await nus.write('>39028003'.encode())
    ret = await nus.read()
    print(ret.decode(errors="ignore"))
    await nus.write('|3901018a'.encode())
    ret = await nus.read()
    print(ret.decode(errors="ignore"))
    await nus.write('|39010180'.encode())
    ret = await nus.read()
    print(ret.decode(errors="ignore"))
    await nus.write('>39028000'.encode())
    ret = await nus.read()
    print(ret.decode(errors="ignore"))
    await nus.write('|39010180'.encode())
    ret = await nus.read()
    print(ret.decode(errors="ignore"))
    # i2c = I2C(nus)


loop = asyncio.get_event_loop()
loop.run_until_complete(main())
