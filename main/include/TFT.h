
#pragma once
#ifdef Has_TFT
#include <TFT_eSPI.h> // Graphics library
#include "IDisplayServiceInterface.h"
#include "IThermostatControl.h"
#include "Enumerations.h"

using namespace CLASSICDIY;

// Fonts
#define FF17 &FreeSans9pt7b
#define FF18 &FreeSans12pt7b
#define FF19 &FreeSans18pt7b
#define FF20 &FreeSans24pt7b

#define FSSB9 &FreeSansBold9pt7b
#define FSSB12 &FreeSansBold12pt7b
#define FSSB18 &FreeSansBold18pt7b
#define FSSB24 &FreeSansBold24pt7b

// display coordinates
#define CURRENT_TEMPERATURE_X 20
#define CURRENT_TEMPERATURE_Y 90
#define SET_TEMPERATURE_X 20
#define SET_TEMPERATURE_Y 200
#define ARROW_X 260
#define ARROW_Y 20
#define POWER_LED_X 305
#define POWER_LED_Y 10
#define WIFI_LED_X 275
#define WIFI_LED_Y 1

// Limits
#define DISPLAY_TIMOUT 240
#define DEFAULT_TARGET_TEMPERATURE 21.0

#define LEVEL_FONT 5
#define STATUS_FONT 2
#define HDR_FONT 2
#define MODE_FONT 2
#define DETAIL_FONT 1
#define NumChar(font) SCREEN_WIDTH / (6 * font)

struct TFTHeaderCache {
   String hdr1;
   String detail1;
   String hdr2;
   String detail2;
   int count = -1; // use -1 to indicate “no previous count”
   String status;
   String levelStr;
};

class TFT : public IDisplayServiceInterface {
 public:
   void Init(IThermostatControl *thermostat);
   void Display(const char *hdr1, const char *detail1, const char *hdr2, const char *detail2);
   void Display(const char *hdr1, const char *detail1, const char *hdr2, int count);
   void Element(boolean state);
   void runTFT(Mode mode);
   void TargetTemperature(Mode mode, float targetTemperature);
   void CurrentTemperature(float temperature);

 private:
   u_int _display_timer;
   uint16_t _hSplit = 70;
   float ltx = 0;                 // Saved x coord of bottom of needle
   uint16_t osx = 120, osy = 120; // Saved x & y coords
   int value[6] = {0, 0, 0, 0, 0, 0};
   int old_value[6] = {-1, -1, -1, -1, -1, -1};
   int old_analog = -999; // Value last displayed
   void drawIfChanged(const String &newVal, String &oldVal, int x, int y, uint16_t color);
   TFTHeaderCache _headerCache;
   boolean _wifi_on = false;
   IThermostatControl *_thermostat;
};

#endif