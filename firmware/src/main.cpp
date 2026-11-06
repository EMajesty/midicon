#include <Arduino.h>
#include "Controller.h"
midicon::Controller controller;
void setup(){controller.begin();}
void loop(){controller.tick();}
