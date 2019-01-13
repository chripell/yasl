from time import sleep


class TCS34725:

    def __init__(self, i2c):
        self.addr = 0x29
        self.i2c = i2c
        self.i2c.set_pin(0, 0)

    def light(self, on: int):
        if on != 0:
            on = 1
        self.i2c.set_pin(on, on)

    def write_reg(self, reg: int, val: int):
        self.i2c.send(self.addr, [0xa0 | reg, val])

    def read_reg(self, reg: int, n: int):
        return self.i2c.cmd_recv(self.addr, 0xa0 | reg, n)

    def power_on(self):
        self.write_reg(0, 1)
        sleep(0.0024)
        self.write_reg(0xd, 0)

    def start(self):
        self.write_reg(0, 3)

    def integration_ms(self, t: int):
        t = int(t / 2.4)
        if t > 255:
            t = 255
        self.write_reg(1, 255 - t)

    def gain(self, g: int):
        if g < 0:
            g = 0
        if g > 3:
            g = 3
        self.write_reg(0xf, g)

    def read_crgb(self):
        status = self.read_reg(0x13, 1)
        while status[0] & 1 == 0:
            status = self.read_reg(0x13, 1)
        data = self.read_reg(0x14, 8)
        return [
            data[0] + data[1] * 256,
            data[2] + data[3] * 256,
            data[4] + data[5] * 256,
            data[6] + data[7] * 256,
        ]

    def dump(self):
        data = self.read_reg(0, 0x1b + 1)
        for i, v in enumerate(data):
            print("0x{:0>2x}=0x{:0>2x}".format(i, v))
