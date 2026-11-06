import unittest
from configurator.models import defaults
from configurator.serialization import to_binary,from_binary
from configurator.protocol import crc16,frame
from configurator.validation import validate
from configurator.logic import ModifierState,pickup,SysExMerger
class ConfigTests(unittest.TestCase):
 def test_defaults(self):self.assertEqual(validate(defaults()),[])
 def test_binary_roundtrip(self):
  c=defaults();b=to_binary(c);self.assertEqual(len(b),1634);self.assertEqual(to_binary(from_binary(b)),b)
 def test_crc_vector(self):self.assertEqual(crc16(b"123456789"),0x4b37)
 def test_frame(self):self.assertEqual(frame(1)[:4],b"\xa5\x01\x00\x00")
 def test_modifier_consumption(self):
  s=ModifierState();s.function(3,True);self.assertEqual(s.matrix_press(15),(3,15));self.assertIsNone(s.function(3,False))
 def test_modifier_tap(self):
  s=ModifierState();s.function(1,True);self.assertEqual(s.function(1,False),1)
 def test_pickup_crossing(self):
  self.assertEqual(pickup(64,[20,40,63]),63);self.assertIsNone(pickup(64,[20,30,40]))
 def test_sysex_merge(self):
  m=SysExMerger();self.assertEqual(m.incoming(0xf0),[0xf0]);self.assertEqual(m.local([0x90,60,100]),[]);self.assertEqual(m.incoming(0xf8),[0xf8]);self.assertEqual(m.incoming(0xf7),[0xf7,0x90,60,100])
if __name__=="__main__":unittest.main()
