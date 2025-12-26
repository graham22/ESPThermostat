#pragma once

#include "Arduino.h"
#include "ArduinoJson.h"
#include "ThreadController.h"

#include <ESPAsyncWebServer.h>
#include "IOTCallbackInterface.h"
#include "IDisplayServiceInterface.h"
#include "Enumerations.h"
#include "Log.h"
#include "Thermometer.h"
#include "Device.h"

namespace CLASSICDIY
{
    class Thermostat : public Device, public IOTCallbackInterface, public IThermostatControl
    {
    public:
        Thermostat();
        void Setup();
#ifdef HasMQTT
        void onMqttConnect(esp_mqtt_client_handle_t &client);
        void onMqttMessage(char *topic, char *payload);
#endif
        void onNetworkState(NetworkState state);
        void onSaveSetting(JsonDocument &doc);
        void onLoadSetting(JsonDocument &doc);
        String appTemplateProcessor(const String &var);
#ifdef Has_TFT
        IDisplayServiceInterface &getDisplayInterface() override { return _tft; };
        void runTFT() { _tft.runTFT(_current_mode); }
#endif
        void Up();
        void Down();
        void ToggleMode();
        void setTargetTemperature(float v)
        {
            _targetTemperature = v;
        }
        float getTargetTemperature()
        {
            return _targetTemperature;
        }
        float getCurrentTemperature()
        {
            return _currentTemperature;
        }
        void setMode(Mode v, boolean persist = true); // persist true saves to eeprom
        Mode getMode()
        {
            return _current_mode;
        }

        void runHeater();

    protected:
#ifdef HasMQTT
        boolean PublishDiscoverySub();
#endif
    private:
        void showTargetTemperature();
        void actionHeater();
        float _currentTemperature;
        float _targetTemperature = DEFAULT_TARGET_TEMPERATURE;
        float _lastTargetTemperature = 0;
        float _lastTemperatureReading = 0;
        boolean _heating_element_on = false;
        Mode _mode = undefined;
        Mode _current_mode = undefined;

        Thermometer _thermometer = Thermometer(THERMISTOR_SENSOR_PIN, THERMISTOR_POWER_PIN, ESP_VOLTAGE_REFERENCE);
        boolean _discoveryPublished = false;
        String _lastMessagePublished;
        unsigned long _lastPublishTimeStamp = 0;
        String _bodyBuffer;
    };
} // namespace CLASSICDIY
