#include "Config.h"
#include <string.h>
namespace midicon {
static Mapping map(uint8_t a, uint8_t b, uint8_t n, uint16_t initial = 0) {
    return {a, b, 1, n, 0, 127, 1, 0, initial, 50};
}
void makeDefaults(Configuration &c) {
    memset(&c, 0, sizeof(c));
    c.schema = kSchemaVersion;
    c.hardware = kHardwareId;
    const char *names[4] = {"PADS", "MIX", "SYNTH", "USER"};
    for (uint8_t p = 0; p < 4; p++) {
        strncpy(c.pageNames[p], names[p], 8);
        for (uint8_t i = 0; i < 16; i++)
            c.mappings[p][i] = map(1, 1, 36 + i);
        for (uint8_t i = 0; i < 4; i++)
            c.mappings[p][16 + i] = map(2, 4, 16 + p * 4 + i, 64);
        for (uint8_t i = 0; i < 4; i++)
            c.mappings[p][20 + i] = map(2, 1, 80 + i);
        for (uint8_t i = 0; i < 4; i++)
            c.mappings[p][24 + i] = map(2, 4, 7 + i, 0);
        for (uint8_t i = 0; i < 4; i++)
            c.mappings[p][28 + i] = map(2, 4, 1 + i, 64);
    }
    for (auto &a : c.analog)
        a = {0, 1023, 512};
    c.pickupTolerance = 2;
    c.analogFilter = 4;
}
bool validate(const Configuration &c) {
    if (c.schema != kSchemaVersion || c.hardware != kHardwareId ||
        c.pickupTolerance > 16)
        return false;
    for (const auto &a : c.analog)
        if (a.maximum <= a.minimum + 16 || a.center < a.minimum ||
            a.center > a.maximum)
            return false;
    for (const auto &p : c.mappings)
        for (const auto &m : p)
            if (m.action > 4 || m.behavior > 6 || m.channel < 1 ||
                m.channel > 16 || m.minimum > m.maximum)
                return false;
    return true;
}
uint16_t crc16(const uint8_t *d, size_t n) {
    uint16_t c = 0xffff;
    while (n--) {
        c ^= *d++;
        for (uint8_t i = 0; i < 8; i++)
            c = (c & 1) ? (c >> 1) ^ 0xA001 : c >> 1;
    }
    return c;
}
} // namespace midicon
