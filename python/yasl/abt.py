import asyncio
from collections import deque
from dbus_next import BusType
from dbus_next.aio import MessageBus

NUS_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e'
NUS_CHARACTERISTIC_TX = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'
NUS_CHARACTERISTIC_RX = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'


class NUS:

    def __init__(self):
        self.bus = None
        self.root_obj = None
        self.object_manager = None
        self.all = None
        self.prefix = None
        self.data = deque()
        self.data_lock = asyncio.Lock()
        self.data_cond = asyncio.Condition(lock=self.data_lock)

    async def find(self, mac: str = '', name: str = '') -> bool:
        self.bus = await MessageBus(bus_type=BusType.SYSTEM).connect()
        introspection = await self.bus.introspect('org.bluez', '/')
        self.root_obj = self.bus.get_proxy_object(
            'org.bluez', '/', introspection)
        self.object_manager = self.root_obj.get_interface(
            'org.freedesktop.DBus.ObjectManager')
        self.all = await self.object_manager.call_get_managed_objects()
        self.prefix = None
        for k, v in self.all.items():
            if 'org.bluez.Device1' in v:
                dev = v['org.bluez.Device1']
                if 'UUIDs' not in dev:
                    continue
                if NUS_SERVICE_UUID not in dev['UUIDs'].value:
                    continue
                if 'Address' not in dev:
                    continue
                mac_got = dev['Address'].value
                if mac != '' and mac.upper() != mac_got:
                    continue
                if 'Name' not in dev:
                    continue
                name_got = dev['Name'].value
                if name != '' and name not in name_got:
                    continue
                self.prefix = k
                break
        if self.prefix is None:
            return False
        introspection = await self.bus.introspect('org.bluez', self.prefix)
        self.dev_obj = self.bus.get_proxy_object(
            'org.bluez', self.prefix, introspection)
        self.dev = self.dev_obj.get_interface(
            'org.bluez.Device1')
        return True

    async def find_uart(self) -> bool:
        tx = None
        rx = None
        for k, v in self.all.items():
            if tx is not None and rx is not None:
                break
            if ('org.bluez.GattCharacteristic1' in v and
                    self.prefix is not None and k.startswith(self.prefix)):
                val = v['org.bluez.GattCharacteristic1']
                if 'UUID' not in val:
                    continue
                car_uuid = val['UUID'].value
                if NUS_CHARACTERISTIC_RX == car_uuid:
                    rx = k
                if NUS_CHARACTERISTIC_TX == car_uuid:
                    tx = k
        if tx is None or rx is None:
            return False
        introspection = await self.bus.introspect('org.bluez', tx)
        self.tx_obj = self.bus.get_proxy_object(
            'org.bluez', tx, introspection)
        self.tx = self.tx_obj.get_interface(
            'org.bluez.GattCharacteristic1')
        introspection = await self.bus.introspect('org.bluez', rx)
        self.rx_obj = self.bus.get_proxy_object(
            'org.bluez', rx, introspection)
        self.rx = self.rx_obj.get_interface(
            'org.bluez.GattCharacteristic1')
        self.rx_prop = self.rx_obj.get_interface(
            'org.freedesktop.DBus.Properties')
        self.rx_prop.on_properties_changed(
            lambda iface, props, inv: asyncio.create_task(
                self._new_data(props['Value'].value))
            if 'Value' in props else None)
        await self.rx.call_start_notify()
        return True

    async def write(self, data: bytes):
        await self.tx.call_write_value(data, {})

    async def read(self) -> bytes:
        async with self.data_cond:
            await self.data_cond.wait_for(self._has_data)
            return self.data.popleft()

    def _has_data(self) -> bool:
        return len(self.data) > 0

    async def has_data(self) -> bool:
        async with self.data_lock:
            return self._has_data()

    async def _new_data(self, data):
        async with self.data_lock:
            self.data.append(data)
            self.data_cond.notify_all()

    async def flush(self):
        async with self.data_lock:
            self.data.clear()

    async def is_connected(self) -> bool:
        return await self.dev.get_connected()

    async def connect(self):
        await self.dev.call_connect()

    async def disconnect(self):
        await self.dev.call_disconnect()


class I2C:

    def __init__(self, nus):
        self.nus = nus

    def _check_response(self, resp):
        resp = resp.decode('ascii', 'backslashreplace')
        if resp[0] == ':':
            chunks = (resp[i:i + 2] for i in range(1, len(resp), 2))
            return [int(x, 16) for x in chunks]
        if resp[0] == "!":
            ret = int(resp[1:], 16)
            if ret == 0:
                return []
            raise IOError("I2C failed: 0x%04x" % ret)
        raise IOError("I2C failed, invalid response: %s" % resp)

    async def read_byte_data(self, addr, register):
        await self.nus.write(b'|%02x0101%02x' % (
            addr, register))
        resp = await self.nus.read()
        return self._check_response(resp)[0]

    async def read_word_data(self, addr, register):
        await self.nus.write(b'|%02x0102%02x' % (
            addr, register))
        resp = await self.nus.read()
        r = self._check_response(resp)
        return r[0] + r[1] * 256

    async def write_byte_data(self, addr, reg, val):
        await self.nus.write(b'>%02x02%02x%02x' % (
            addr, reg, val))
        resp = await self.nus.read()
        self._check_response(resp)

    async def read_i2c_block_data(self, addr, reg, n):
        await self.nus.write(b'|%02x01%02x%02x' % (
            addr, n, reg))
        resp = await self.nus.read()
        return self._check_response(resp)
