#pragma once
#include <Arduino.h>
#include "GPIO_pins.h"
#include "Enumerations.h"
#include "tft.h"

namespace CLASSICDIY
{
   class Device
   {
   protected:
      void Init();
      void InitCommon();
      void Run();
#ifdef Has_OLED
      Oled _oled = Oled();
#endif
#ifdef Has_TFT
      TFT _tft = TFT();
#endif
      NetworkState _networkState = Boot;
      unsigned long _lastBlinkTime = 0;
      bool _blinkStateOn = false;
      bool _running = false;
   };

} // namespace CLASSICDIY