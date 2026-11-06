from .models import *
def validate(c:Configuration)->list[str]:
    e=[]
    if c.schema!=SCHEMA_VERSION:e.append("unsupported schema")
    if c.hardware!=HARDWARE_ID:e.append("wrong hardware identifier")
    if len(c.pages)!=4:e.append("exactly four pages required")
    if len(c.analog)!=8:e.append("exactly eight calibration records required")
    for pi,p in enumerate(c.pages):
        if not p.name or len(p.name.encode("ascii","ignore"))>8:e.append(f"page {pi+1}: name must be 1..8 ASCII characters")
        if len(p.mappings)!=32:e.append(f"page {pi+1}: exactly 32 mappings required")
        for mi,m in enumerate(p.mappings):
            if not 0<=int(m.action)<=4 or not 1<=int(m.behavior)<=6:e.append(f"page {pi+1} mapping {mi+1}: invalid action/behavior")
            if not 1<=m.channel<=16 or not 0<=m.number<=127 or not 0<=m.minimum<=m.maximum<=127:e.append(f"page {pi+1} mapping {mi+1}: invalid MIDI range")
    for i,a in enumerate(c.analog):
        if not 0<=a.minimum<a.maximum<=1023 or a.maximum-a.minimum<=16 or not a.minimum<=a.center<=a.maximum:e.append(f"analog {i+1}: invalid calibration")
    return e
