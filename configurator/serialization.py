import json,struct
from pathlib import Path
from .models import *
from .validation import validate
MAP=struct.Struct("<8B2H"); CAL=struct.Struct("<3H")
def to_binary(c:Configuration)->bytes:
    errors=validate(c)
    if errors: raise ValueError("; ".join(errors))
    out=bytearray(struct.pack("<HI",c.schema,c.hardware))
    for p in c.pages: out += p.name.encode("ascii")[:8].ljust(9,b"\0")
    for p in c.pages:
        for m in p.mappings: out += MAP.pack(m.action,m.behavior,m.channel,m.number,m.minimum,m.maximum,m.step,m.flags,m.initial,m.duration_ms)
    for a in c.analog: out += CAL.pack(a.minimum,a.maximum,a.center)
    out += bytes((c.pickup_tolerance,c.analog_filter))+bytes(6)
    return bytes(out)
def from_binary(data:bytes)->Configuration:
    if len(data)!=1634: raise ValueError(f"expected 1634 bytes, got {len(data)}")
    schema,hardware=struct.unpack_from("<HI",data); off=6; names=[]
    for _ in range(4): names.append(data[off:off+9].split(b"\0",1)[0].decode("ascii"));off+=9
    pages=[]
    for name in names:
        ms=[]
        for _ in range(32):
            v=MAP.unpack_from(data,off);off+=MAP.size;ms.append(Mapping(*v[:8],v[8],v[9]))
        pages.append(Page(name,ms))
    analog=[]
    for _ in range(8):analog.append(Calibration(*CAL.unpack_from(data,off)));off+=CAL.size
    c=Configuration(schema,hardware,pages,analog,data[off],data[off+1]);errors=validate(c)
    if errors:raise ValueError("; ".join(errors))
    return c
def load_json(path):return Configuration.from_dict(json.loads(Path(path).read_text()))
def save_json(c,path):Path(path).write_text(json.dumps(c.to_dict(),indent=2)+"\n")
