from __future__ import annotations
from dataclasses import asdict, dataclass, field
from enum import IntEnum

SCHEMA_VERSION=1; HARDWARE_ID=0x4D494431; PAGE_COUNT=4; MAPPINGS_PER_PAGE=32
class Action(IntEnum): NONE=0; NOTE=1; CC=2; PROGRAM=3; PITCH_BEND=4
class Behavior(IntEnum): MOMENTARY=1; TOGGLE=2; TRIGGER=3; ABSOLUTE=4; RELATIVE=5; STEPPED=6
@dataclass
class Mapping:
    action:int=Action.NONE; behavior:int=Behavior.ABSOLUTE; channel:int=1; number:int=0
    minimum:int=0; maximum:int=127; step:int=1; flags:int=0; initial:int=0; duration_ms:int=50
@dataclass
class Calibration: minimum:int=0; maximum:int=1023; center:int=512
@dataclass
class Page: name:str="PAGE"; mappings:list[Mapping]=field(default_factory=list)
@dataclass
class Configuration:
    schema:int=SCHEMA_VERSION; hardware:int=HARDWARE_ID; pages:list[Page]=field(default_factory=list)
    analog:list[Calibration]=field(default_factory=lambda:[Calibration() for _ in range(8)])
    pickup_tolerance:int=2; analog_filter:int=4
    def to_dict(self): return asdict(self)
    @classmethod
    def from_dict(cls,d):
        return cls(d.get("schema",1),d.get("hardware",HARDWARE_ID),[Page(p["name"],[Mapping(**m) for m in p["mappings"]]) for p in d["pages"]],[Calibration(**a) for a in d["analog"]],d.get("pickup_tolerance",2),d.get("analog_filter",4))
def defaults()->Configuration:
    pages=[]
    for p,name in enumerate(("PADS","MIX","SYNTH","USER")):
        ms=[Mapping(Action.NOTE,Behavior.MOMENTARY,1,36+i) for i in range(16)]
        ms += [Mapping(Action.CC,Behavior.ABSOLUTE,1,16+p*4+i,initial=64) for i in range(4)]
        ms += [Mapping(Action.CC,Behavior.MOMENTARY,1,80+i) for i in range(4)]
        ms += [Mapping(Action.CC,Behavior.ABSOLUTE,1,7+i) for i in range(4)]
        ms += [Mapping(Action.CC,Behavior.ABSOLUTE,1,1+i,initial=64) for i in range(4)]
        pages.append(Page(name,ms))
    return Configuration(pages=pages)
