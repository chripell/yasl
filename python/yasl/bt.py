from pydbus import SystemBus
import pprint
from typing import List, Dict, Any

NUS_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e'
NUS_CHARACTERISTIC_RX = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'
NUS_CHARACTERISTIC_TX = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'


class NUS:

    def __init__(self):
        self.bus = SystemBus()
        self.tx = None
        self.rx = None
        self.dev = None
        self.notify = None
        self.prefix = None
        self.read_buffer = []

    def find(self, mac: str = '', name: str = '') -> bool:
        root = self.bus.get('org.bluez', '/')
        all = root.GetManagedObjects()
        # DELME START
        # pp = pprint.PrettyPrinter(indent=4)
        # pp.pprint(all)
        # DELME END
        self.prefix = None
        for k, v in all.items():
            if 'org.bluez.Device1' in v:
                if NUS_SERVICE_UUID not in v['org.bluez.Device1'].get(
                        'UUIDs', []):
                    continue
                if mac != '' and mac.upper() != v['org.bluez.Device1'].get(
                        'Address', ''):
                    continue
                if name != '' and name not in v['org.bluez.Device1'].get(
                        'Name', ''):
                    continue
                print("DELME {}".format(k))
                self.prefix = k
                break
        if self.prefix is None:
            return False
        self.dev = self.bus.get('org.bluez', self.prefix)
        return True

    def find_uart(self) -> bool:
        root = self.bus.get('org.bluez', '/')
        all = root.GetManagedObjects()
        tx = None
        rx = None
        for k, v in all.items():
            if tx is not None and rx is not None:
                break
            if ('org.bluez.GattCharacteristic1' in v and
                    self.prefix is not None and k.startswith(self.prefix)):
                if NUS_CHARACTERISTIC_RX == v[
                        'org.bluez.GattCharacteristic1'].get(
                            'UUID', ''):
                    rx = k
                if NUS_CHARACTERISTIC_TX == v[
                        'org.bluez.GattCharacteristic1'].get(
                            'UUID', ''):
                    tx = k
        if tx is None or rx is None:
            return False
        self.tx = self.bus.get('org.bluez', tx)
        self.rx = self.bus.get('org.bluez', rx)
        return True

    def is_connected(self) -> bool:
        if self.dev is None:
            raise RuntimeError
        return self.dev.Connected

    def connect(self):
        if self.dev is None:
            raise RuntimeError
        self.dev.Connect()

    def disconnect(self):
        if self.dev is None:
            raise RuntimeError
        self.dev.Disconnect()

    def write(self, data: bytes):
        if self.rx is None:
            raise RuntimeError
        self.rx.WriteValue(data, {})

    def read(self) -> List[bytes]:
        ret = self.read_buffer
        self.read_buffer = []
        return ret

    def start_read(self):
        if self.tx is None:
            raise RuntimeError
        self.notify = self.tx.PropertiesChanged.connect(self.on_read)
        self.tx.StartNotify()

    def stop_read(self):
        if self.notify() is not None:
            self.tx.StopNotify()
            self.notify.Disconnect()
            self.notify = None

    def on_read(self, iface: str, props: Dict[str, Any], lst: List[str]):
        v = props.get('Value', None)
        if v is not None:
            print(v)
            self.read_buffer.append(''.join([chr(i) for i in v]))
