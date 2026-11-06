#include "Persistence.h"
#include <EEPROM.h>
namespace midicon {
struct __attribute__((packed)) Header{uint32_t magic,sequence;uint16_t size,crc;uint8_t committed,reserved[3];};
constexpr uint32_t MAGIC=0x3144494d; constexpr int SLOT=2048;
static bool readSlot(uint8_t s,Header&h,Configuration&c){int a=s*SLOT;EEPROM.get(a,h);if(h.magic!=MAGIC||h.committed!=0xA5||h.size!=sizeof(c))return false;EEPROM.get(a+sizeof(h),c);return h.crc==crc16((uint8_t*)&c,sizeof(c))&&validate(c);}
bool Persistence::load(Configuration&c){Header a{},b{};Configuration ca{},cb{};bool va=readSlot(0,a,ca),vb=readSlot(1,b,cb);if(!va&&!vb)return false;c=(vb&&(!va||b.sequence>a.sequence))?cb:ca;return true;}
bool Persistence::save(const Configuration&c){if(!validate(c))return false;Header a{},b{};Configuration t{};bool va=readSlot(0,a,t),vb=readSlot(1,b,t);uint8_t s=(!va)?0:(!vb?1:(a.sequence<=b.sequence?0:1));uint32_t seq=max(va?a.sequence:0UL,vb?b.sequence:0UL)+1;int base=s*SLOT;Header h{MAGIC,seq,sizeof(c),crc16((const uint8_t*)&c,sizeof(c)),0,{0,0,0}};EEPROM.put(base,h);EEPROM.put(base+sizeof(h),c);h.committed=0xA5;EEPROM.update(base+offsetof(Header,committed),h.committed);return true;}
}
