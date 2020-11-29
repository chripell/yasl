import ctypes
from typing import List, Sequence

I2C_SPEED_400KHZ = 0x03
I2C_SPEED_100KHZ = 0x02
I2C_SPEED_50KHZ = 0x01
I2C_SPEED_5KHZ = 0x00

I2C_USE_CS = 0x10
I2C_USE_AUX = 0x20
I2C_PIN_LOW = 0x00
I2C_PIN_HIGH = 0x01
I2C_PIN_HIZ = 0x02
I2C_PIN_READ = 0x03

lib = ctypes.CDLL("libyasl_buspirate.so")
lib.i2c_new.argtypes = [ctypes.c_char_p, ctypes.c_int]
lib.i2c_new.restype = ctypes.c_void_p
lib.i2c_cmd_recv.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_ubyte,
                             ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
lib.i2c_send.argtypes = [ctypes.c_void_p, ctypes.c_int,
                         ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
lib.i2c_recv.argtypes = [ctypes.c_void_p, ctypes.c_int,
                         ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
lib.i2c_pin.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
lib.i2c_fast.argtypes = [ctypes.c_void_p, ctypes.c_int]


class I2C:

    def __init__(self, port: str, speed: int):
        self.b = lib.i2c_new(port.encode('ascii', 'ignore'), speed)

    def cmd_recv(self, addr: int, cmd: int, n: int) -> List[int]:
        data = (ctypes.c_ubyte * n)()
        lib.i2c_cmd_recv(self.b, addr, cmd, data, n)
        return data[:]

    def send(self, addr: int, data: Sequence[int]):
        n = len(data)
        raw = (ctypes.c_ubyte * n)()
        for i, v in enumerate(data):
            raw[i] = data[i]
        lib.i2c_send(self.b, addr, raw, n)

    def recv(self, addr: int, n: int) -> List[int]:
        data = (ctypes.c_ubyte * n)()
        lib.i2c_recv(self.b, addr, data, n)
        return data[:]

    def set_pin(self, aux: int, cs: int):
        lib.i2c_pin(self.b, aux, cs)

    def set_fast(self, fast: int):
        lib.i2c_fast(self.b, fast)

    async def read_i2c_block_data(self, addr, register, n):
        data = (ctypes.c_ubyte * n)()
        lib.i2c_cmd_recv(self.b, addr, register, data, n)
        return bytes(data)

    async def read_byte_data(self, addr, register):
        return await self.read_i2c_block_data(addr, register, 1)

    async def read_word_data(self, addr, register):
        return await self.read_i2c_block_data(addr, register, 2)

    async def write_i2c_block_data(self, addr, register, data):
        self.send(addr, [register] + data)
