#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Enumerations.h"

namespace CLASSICDIY {
class IThermostatControl {
 public:
   virtual void Up() = 0;
   virtual void Down() = 0;
   virtual void ToggleMode() = 0;
};
} // namespace CLASSICDIY