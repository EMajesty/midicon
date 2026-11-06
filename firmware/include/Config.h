#pragma once
#include <Arduino.h>

namespace midicon {
constexpr uint8_t kPageCount=4, kMappingsPerPage=32, kAnalogCount=8;
constexpr uint16_t kSchemaVersion=1; constexpr uint32_t kHardwareId=0x4D494431UL;
enum class Action:uint8_t { None, Note, CC, Program, PitchBend };
enum class Behavior:uint8_t { Momentary, Toggle, Trigger, Absolute, Relative, Stepped };
struct __attribute__((packed)) Mapping {
  uint8_t action, behavior, channel, number, minimum, maximum, step, flags;
  uint16_t initial, durationMs;
};
struct __attribute__((packed)) Calibration { uint16_t minimum, maximum, center; };
struct __attribute__((packed)) Configuration {
  uint16_t schema; uint32_t hardware; char pageNames[kPageCount][9];
  Mapping mappings[kPageCount][kMappingsPerPage]; Calibration analog[kAnalogCount];
  uint8_t pickupTolerance, analogFilter, reserved[6];
};
static_assert(sizeof(Mapping)==12,"mapping wire format");
static_assert(sizeof(Configuration)<1900,"EEPROM dual-slot budget");
void makeDefaults(Configuration& c);
bool validate(const Configuration& c);
uint16_t crc16(const uint8_t* data,size_t size);
}
