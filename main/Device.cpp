#include <Arduino.h>
#include <memory>
#include "Wire.h"
#include <ArduinoJson.h>
#ifdef UseLittleFS
#include <LittleFS.h>
#endif
#include "Log.h"
#include "Device.h"

void Device::InitCommon() {
   Wire.begin(I2C_SDA, I2C_SCL);
#ifdef UseLittleFS
   if (!LittleFS.begin()) {
      loge("LittleFS mount failed");
   }
#endif
}

void Device::Init() {
   InitCommon();
   pinMode(KCT_CS, OUTPUT); // disable MAX6675 if installed, using Thermistor
   digitalWrite(KCT_CS, HIGH);
   pinMode(SSR_PIN, OUTPUT);
   digitalWrite(SSR_PIN, LOW);
}

void Device::Run() {}
