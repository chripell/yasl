import hrgw
import argparse
import smbus
import time
import datetime
import julian  # type: ignore

DEVICE_BUS = 1
DEVICE_ADDR = 0x17

TEMP_REG = 0x01
LIGHT_REG_L = 0x02
LIGHT_REG_H = 0x03
STATUS_REG = 0x04
ON_BOARD_TEMP_REG = 0x05
ON_BOARD_HUMIDITY_REG = 0x06
ON_BOARD_SENSOR_ERROR = 0x07
BMP280_TEMP_REG = 0x08
BMP280_PRESSURE_REG_L = 0x09
BMP280_PRESSURE_REG_M = 0x0A
BMP280_PRESSURE_REG_H = 0x0B
BMP280_STATUS = 0x0C
HUMAN_DETECT = 0x0D


class Impl(hrgw.Producer, hrgw.SleeperMixin):

    NAME = "52 Pi Sensor"

    def __init__(self):
        self.running = True
        self.bus = smbus.SMBus(DEVICE_BUS)

    def register_args(self, arg: argparse.ArgumentParser):
        arg.add_argument("--sensor52-refresh-time", type=float, default=60,
                         help="Seconds between rereading values.")

    async def start(self, args, coll: hrgw.Collector):
        refresh = time.time() + args.sensor52_refresh_time
        presence = 0.0
        while self.running:
            await self.sleep(1.0)
            presence += self.bus.read_byte_data(DEVICE_ADDR, HUMAN_DETECT)
            if time.time() < refresh:
                continue
            now = julian.to_jd(datetime.datetime.now())
            refresh = time.time() + args.sensor52_refresh_time
            await coll.push("PR_52pi", presence, now)
            presence = 0
            aReceiveBuf = [0]
            for i in range(TEMP_REG, HUMAN_DETECT):
                aReceiveBuf.append(
                    self.bus.read_byte_data(DEVICE_ADDR, i))
            if aReceiveBuf[STATUS_REG] != 0:
                await coll.push("S1_52pi", aReceiveBuf[STATUS_REG], now)
            if aReceiveBuf[STATUS_REG] & 0x3 == 0:
                await coll.push("T1_52pi", aReceiveBuf[TEMP_REG], now)
            if aReceiveBuf[STATUS_REG] & 0xc == 0:
                await coll.push(
                    "LUX_52pi",
                    aReceiveBuf[LIGHT_REG_H] << 8 | aReceiveBuf[LIGHT_REG_L],
                    now)
            if aReceiveBuf[ON_BOARD_SENSOR_ERROR] != 0:
                await coll.push("S2_52pi",
                                aReceiveBuf[ON_BOARD_SENSOR_ERROR], now)
            await coll.push("T2_52pi", aReceiveBuf[ON_BOARD_TEMP_REG], now)
            await coll.push("HU_52pi",
                            aReceiveBuf[ON_BOARD_HUMIDITY_REG], now)
            if aReceiveBuf[BMP280_STATUS] == 0:
                await coll.push("T3_52pi", aReceiveBuf[BMP280_TEMP_REG], now)
                await coll.push("PR_52pi", aReceiveBuf[BMP280_PRESSURE_REG_L] |
                                aReceiveBuf[BMP280_PRESSURE_REG_M] << 8 |
                                aReceiveBuf[BMP280_PRESSURE_REG_H] << 16, now)
            else:
                await coll.push("S3_52pi", aReceiveBuf[BMP280_STATUS], now)

    async def stop(self):
        self.running = False
