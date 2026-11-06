"""Host-testable reference logic shared by tests and protocol tooling."""
class ModifierState:
    def __init__(self): self.owner=None; self.consumed=False; self.page=0
    def function(self,index,pressed):
        if pressed and self.owner is None:self.owner=index;self.consumed=False
        elif not pressed and self.owner==index:
            changed=None if self.consumed else index;self.owner=None;return changed
    def matrix_press(self,index):
        if self.owner is None:return None
        self.consumed=True
        return (self.owner,index)
def pickup(previous,physical_values,tolerance=2):
    """Return first emitted value after crossing/touching the old logical value."""
    last=physical_values[0]
    for value in physical_values:
        if abs(value-previous)<=tolerance or (last<previous<value) or (value<previous<last):return value
        last=value
    return None
class SysExMerger:
    def __init__(self):self.sysex=False;self.pending=[]
    def incoming(self,b):
        if b==0xf0:self.sysex=True
        if b==0xf7:self.sysex=False
        out=[b]
        if not self.sysex and self.pending:out+=self.pending;self.pending=[]
        return out
    def local(self,message):
        if self.sysex:self.pending+=message;return []
        return list(message)
