#ifdef Has_TFT
#include <WiFi.h>
#include "TFT.h"
#include "Log.h"
#include "defines.h"

TFT_eSPI tft = TFT_eSPI(); // Invoke library

const uint8_t wifi_Symbol[33] PROGMEM = { // WIFI symbol
    0x00, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x1C, 0x38,
    0x07, 0x60, 0xE1, 0xC7, 0x78, 0x1E, 0x0C, 0x30,
    0x80, 0x01, 0xE0, 0x07, 0x30, 0x0C, 0x00, 0x00,
    0x80, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00};

namespace CLASSICDIY
{
    void TFT::Init(IThermostatControl *thermostat)
    {
        _thermostat = thermostat;
        tft.init();
        tft.setRotation(1); // Landscape
        tft.fillScreen(TFT_BLACK);
        logd("Screen init: Width %d, Height %d", tft.width(), tft.height());
        uint16_t calData[5] = {452, 3097, 460, 3107, 4};
        tft.setTouch(calData);
        pinMode(TFT_LED_PIN, OUTPUT);
        digitalWrite(TFT_LED_PIN, HIGH);
        _display_timer = DISPLAY_TIMOUT;
        int xpos = ARROW_X;
        int ypos = ARROW_Y;
        tft.fillTriangle(
            xpos, ypos,           // peak
            xpos - 40, ypos + 60, // bottom left
            xpos + 40, ypos + 60, // bottom right
            TFT_RED);
        ypos += 200;
        tft.fillTriangle(
            xpos, ypos,           // peak
            xpos - 40, ypos - 60, // bottom left
            xpos + 40, ypos - 60, // bottom right
            TFT_BLUE);
    }

    void TFT::Display(const char *hdr1, const char *detail1, const char *hdr2, int count)
    {
        tft.setTextFont(1);
        int y = 0;
        // hdr1
        tft.setTextSize(HDR_FONT);
        drawIfChanged(String(hdr1), _headerCache.hdr1, 0, y, TFT_GREEN);
        y += 18;
        // detail1
        tft.setTextSize(DETAIL_FONT);
        drawIfChanged(String(detail1), _headerCache.detail1, 0, y, TFT_GREEN);
        y += 18;
        // hdr2 + count
        tft.setTextSize(MODE_FONT);
        String hdr2Full = String(hdr2);
        if (count > 0)
        {
            hdr2Full += ":" + String(count);
        }
        drawIfChanged(hdr2Full, _headerCache.hdr2, 0, y, TFT_GREEN);
        _headerCache.count = count;
    }

    void TFT::Display(const char *hdr1, const char *detail1, const char *hdr2, const char *detail2)
    {
        tft.setTextFont(1);
        int y = 0;
        // hdr1
        tft.setTextSize(HDR_FONT);
        drawIfChanged(String(hdr1), _headerCache.hdr1, 0, y, TFT_GREEN);
        y += 18;
        // detail1
        tft.setTextSize(DETAIL_FONT);
        drawIfChanged(String(detail1), _headerCache.detail1, 0, y, TFT_GREEN);
        y += 18;
        // hdr2
        tft.setTextSize(MODE_FONT);
        drawIfChanged(String(hdr2), _headerCache.hdr2, 0, y, TFT_GREEN);
        y += 18;
        // detail2
        tft.setTextSize(DETAIL_FONT);
        drawIfChanged(String(detail2), _headerCache.detail2, 0, y, TFT_GREEN);
    }

    void TFT::drawIfChanged(const String &newVal, String &oldVal, int x, int y, uint16_t color)
    {
        if (newVal != oldVal)
        {
            tft.setTextColor(TFT_BLACK, TFT_BLACK); // erases old text
            tft.drawString(oldVal, x, y);
            tft.setTextColor(color, TFT_BLACK); // new text
            tft.drawString(newVal, x, y);
            oldVal = newVal;
        }
    }

    void TFT::runTFT(Mode mode)
    {
        uint16_t x = 0, y = 0;
        if (tft.getTouch(&x, &y))
        {
            logd("Touch at %d-%d", x, y);
            if (_display_timer > 0)
            {
                if (x > 200)
                {
                    if (mode == heat)
                    {
                        if (y < 120)
                        {
                            _thermostat->Up();
                        }
                        else
                        {
                            _thermostat->Down();
                        }
                    }
                }
                else if (y > 200)
                {
                    _thermostat->ToggleMode(); // toggle on/off
                }
            }
            digitalWrite(TFT_LED_PIN, HIGH);
            _display_timer = DISPLAY_TIMOUT;
        }
        if (_wifi_on == false && WiFi.isConnected())
        {
            _wifi_on = true;
            tft.drawXBitmap(WIFI_LED_X, WIFI_LED_Y, wifi_Symbol, 16, 16, TFT_GREEN);
        }
        else if (_wifi_on == true && !WiFi.isConnected())
        {
            _wifi_on = false;
            tft.drawXBitmap(WIFI_LED_X, WIFI_LED_Y, wifi_Symbol, 16, 16, TFT_BLACK);
        }
        // screen saver timer
        if (_display_timer == 0)
        {
            digitalWrite(TFT_LED_PIN, LOW);
        }
        else
        {
            _display_timer--;
        }
    }

    void TFT::TargetTemperature(Mode mode, float targetTemperature)
    {
        tft.setTextSize(4);
        tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
        if (mode == heat)
        {
            tft.drawFloat(targetTemperature, 1, SET_TEMPERATURE_X, SET_TEMPERATURE_Y);
        }
        else
        {
            tft.drawString("Off   ", SET_TEMPERATURE_X, SET_TEMPERATURE_Y);
        }
        digitalWrite(TFT_LED_PIN, HIGH);
        _display_timer = DISPLAY_TIMOUT;
    }

    void TFT::CurrentTemperature(float temperature)
    {
        tft.setTextSize(8);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawFloat(temperature, 1, CURRENT_TEMPERATURE_X, CURRENT_TEMPERATURE_Y);
    }

    void TFT::Element(boolean state)
    {
        tft.fillCircle(POWER_LED_X, POWER_LED_Y, 8, state ? TFT_RED : TFT_BLACK);
    }
} // namespace CLASSICDIY

#endif