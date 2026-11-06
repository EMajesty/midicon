#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>
#include "Config.h"
namespace midicon {
class Controller {
 public: Controller(); void begin(); void tick();
 private:
  enum class Mode:uint8_t{Performance,System,LoadConfirm,Calibration,AssignSelect,AssignEdit,Message};
  struct Debounce{bool stable=1,sample=1;uint32_t at=0;bool update(bool v,uint32_t now);};
  Configuration config_{}; bool dirty_=false; uint8_t page_=0; Mode mode_=Mode::Performance;
  LiquidCrystal lcd_; Adafruit_NeoPixel leds_; Debounce matrix_[16],encPb_[4],functions_[4];
  uint8_t encState_[4]{},functionOwner_=255; bool functionConsumed_=false,shiftOnly_[16]{};
  int8_t encAccum_[4]{}; uint16_t raw_[8]{},filtered_[8]{},logical_[4][8]{}; bool pickup_[4][4]{};
  bool toggle_[4][24]{},noteHeld_[16]{},transport_=false; uint32_t clockAt_=0,lastClock_=0,lastUi_=0;
  float bpm_=0; char lower_[21]{}; uint8_t systemItem_=0,selected_=255; Mapping edit_{}; uint8_t editGroup_=0;
  uint32_t messageUntil_=0; char message_[21]{}; uint16_t triggerOff_[16]{}; uint32_t triggerAt_[16]{};
  void scan(uint32_t); void matrixEvent(uint8_t,bool); void functionEvent(uint8_t,bool); void encoderEvent(uint8_t,int8_t); void encoderButton(uint8_t,bool); void analog(uint32_t);
  void dispatchButton(uint8_t,bool); void dispatchEncoder(uint8_t,int8_t); void dispatchAnalog(uint8_t,uint8_t);
  void send3(uint8_t,uint8_t,uint8_t); void send2(uint8_t,uint8_t); void releaseNotes(); void changePage(uint8_t);
  void midiIn(uint32_t); void render(uint32_t,bool=false); void ledRender(); void show(const char*); void systemAccept(); void assignmentAdjust(uint8_t,int8_t); void protocol();
  uint8_t scaled(uint8_t,uint16_t)const; Mapping& mapping(uint8_t index){return config_.mappings[page_][index];}
}; }
