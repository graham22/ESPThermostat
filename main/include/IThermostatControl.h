#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

class IThermostatControl {
 public:
   virtual void Up() = 0;
   virtual void Down() = 0;
   virtual void ToggleMode() = 0;
};
