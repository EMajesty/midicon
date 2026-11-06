import struct,time
try: import serial
except ImportError: serial=None
from .serialization import to_binary,from_binary
def crc16(data):
    c=0xffff
    for b in data:
        c^=b
        for _ in range(8):c=(c>>1)^0xA001 if c&1 else c>>1
    return c
def frame(command,payload=b""):
    body=bytes((command,))+struct.pack("<H",len(payload))+payload
    return b"\xA5"+body+struct.pack("<H",crc16(body))
class Device:
    def __init__(self,port,timeout=2):
        if serial is None:raise RuntimeError("install pyserial")
        self.serial=serial.Serial(port,115200,timeout=timeout);time.sleep(.1)
    def command(self,cmd,payload=b""):
        self.serial.write(frame(cmd,payload));h=self.serial.read(3)
        if len(h)!=3 or h[0]!=0x5a or h[1]!=cmd:raise TimeoutError("invalid/no device response")
        if h[2]:raise RuntimeError(f"device error {h[2]}")
    def upload(self,c):self.command(3,to_binary(c))
    def download(self):
        self.serial.write(frame(2));data=self.serial.read(1634)
        h=self.serial.read(3)
        if h!=b"\x5a\x02\x00":raise TimeoutError("invalid/no download acknowledgement")
        return from_binary(data)
    def save(self):self.command(4)
    def reload(self):self.command(5)
