#include "Controller.h"
#include <Arduino.h>
midicon::Controller controller;
void setup() { controller.begin(); }
void loop() { controller.tick(); }
