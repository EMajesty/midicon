import unittest
from configurator.models import defaults
from configurator.serialization import to_binary,from_binary
from configurator.protocol import crc16,frame
from configurator.validation import validate
class ConfigTests(unittest.TestCase):
 def test_defaults(self):self.assertEqual(validate(defaults()),[])
 def test_binary_roundtrip(self):
  c=defaults();b=to_binary(c);self.assertEqual(len(b),1634);self.assertEqual(to_binary(from_binary(b)),b)
 def test_crc_vector(self):self.assertEqual(crc16(b"123456789"),0x4b37)
 def test_frame(self):self.assertEqual(frame(1)[:4],b"\xa5\x01\x00\x00")
if __name__=="__main__":unittest.main()
