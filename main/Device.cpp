#include <Arduino.h>
#include <memory>
#include "Wire.h"
#include <ArduinoJson.h>
#ifdef UseLittleFS
#include <LittleFS.h>
#endif
#include "Log.h"
#include "Device.h"

namespace CLASSICDIY
{

   void Device::InitCommon()
   {
      Wire.begin(I2C_SDA, I2C_SCL);
#ifdef UseLittleFS
      if (!LittleFS.begin())
      {
         loge("LittleFS mount failed");
      }
#endif
   }

   void Device::Init()
   {
      InitCommon();
      pinMode(KCT_CS, OUTPUT); // disable MAX6675 if installed, using Thermistor
      digitalWrite(KCT_CS, HIGH);
      pinMode(SSR_PIN, OUTPUT);
      digitalWrite(SSR_PIN, LOW);
   }

   void Device::Run()
   {
      // handle blink led, fast : NotConnected slow: AP connected On: Station connected
      if (_networkState != OnLine)
      {
         unsigned long binkRate = _networkState == ApState ? AP_BLINK_RATE : NC_BLINK_RATE;
         unsigned long now = millis();
         if (binkRate < now - _lastBlinkTime)
         {
            _blinkStateOn = !_blinkStateOn;
            _lastBlinkTime = now;
            digitalWrite(WIFI_STATUS_PIN, _blinkStateOn ? HIGH : LOW);
         }
      }
      else
      {
         digitalWrite(WIFI_STATUS_PIN, LOW);
      }
   }

} // namespace CLASSICDIY