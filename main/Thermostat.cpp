#include "Arduino.h"
#include "WiFi.h"
#include "app.htm"
#include "app_script.js"
#include "Device.h"
#include "Thermostat.h"
#include "IOT.h"
#include "Log.h"

using namespace CLASSICDIY;

const char *ModeStrings[] = {"off", "off", "heat"};

IOT _iot = IOT();

Thermostat::Thermostat() {}

void Thermostat::Setup() {
   Init();
   _iot.Init(this);
   _current_mode = undefined;
   _thermometer.Init();
   _tft.Init(this);
   _tft.TargetTemperature(_mode, getTargetTemperature());
   auto &server = _iot.getWebServer();
   server.on(
       "/control", HTTP_POST,
       [this](AsyncWebServerRequest *request) {
          // This callback is called after the body is processed
          request->send(200, "application/json", "{\"status\":\"ok\"}");
       },
       NULL, // file upload handler (not used here)
       [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
          JsonDocument doc;
          DeserializationError err = deserializeJson(doc, data, len);
          if (err) {
             loge("JSON parse failed!");
             return;
          }
          String jsonString;
          serializeJson(doc, jsonString);

          String command = doc["command"];
          logd("/control: %s => %s", jsonString.c_str(), command);
          if (command == "heat") {
             _mode = _current_mode == heat ? off : heat;
          } else if (command == "up") {
             Up();
          } else if (command == "down") {
             Down();
          }
       });
}

void Thermostat::Up() {
   if (_targetTemperature < MAX_TEMPERATURE) {
      _targetTemperature += TEMP_PRECISION;
      setTargetTemperature(_targetTemperature);
   }
}

void Thermostat::Down() {
   if (_targetTemperature > MIN_TEMPERATURE) {
      _targetTemperature -= TEMP_PRECISION;
      setTargetTemperature(_targetTemperature);
   }
}

void Thermostat::ToggleMode() { _mode = _current_mode == heat ? off : heat; }

void Thermostat::actionHeater() {
   logd("_heating_element_on: %d", _heating_element_on);
   _tft.Element(_heating_element_on);
   digitalWrite(SSR_PIN, _heating_element_on ? HIGH : LOW);
}

void Thermostat::runHeater() {
   _iot.Run();
   Run(); // base class
   _currentTemperature = _thermometer.Temperature();
   _tft.CurrentTemperature(_currentTemperature);
   if (_current_mode != _mode) {
      _current_mode = _mode;
      _heating_element_on = false;
      actionHeater();
      _tft.TargetTemperature(_mode, getTargetTemperature());
   }
   float currentTemperature = _thermometer.Temperature();
   currentTemperature = roundf(currentTemperature * 10.0f) / 10.0f; // round to one decimal place
   if (abs(_lastTemperatureReading - currentTemperature) > 0.2)     // publish changes greater than .2 degrees in temperature
   {
      _lastTemperatureReading = currentTemperature;
   }
   if (_current_mode == heat) {
      if (_targetTemperature > _lastTemperatureReading) {
         if (_heating_element_on == false) {
            _heating_element_on = true;
            actionHeater();
         }
      } else {
         if (_heating_element_on) {
            _heating_element_on = false;
            actionHeater();
         }
      }
   }
   uint32_t now = millis();
   if ((now - _lastPublishTimeStamp) > PUBLISH_RATE_LIMIT) {
      _lastPublishTimeStamp = now;
      JsonDocument doc;
      doc["mode"] = ModeStrings[_current_mode];
      doc["target_temperature"] = getTargetTemperature();
      doc["temperature"] = currentTemperature;
      doc["element"] = _heating_element_on;
      String s;
      serializeJson(doc, s);
      if (_lastMessagePublished != s) {
         _lastMessagePublished = s;
         _iot.PostWeb(s);
#ifdef Has_TFT
         // _tft.Update(doc);
#endif
#ifdef HasMQTT
         if (_discoveryPublished) { // wait until MQTT is connected and discovery is published
            String topic = _iot.getRootTopicPrefix() + "/stat";
            _iot.PublishMessage(topic.c_str(), s.c_str(), false);
         }
#endif
      }
   }
}

void Thermostat::onSocketPong() {
   _lastPublishTimeStamp = 0;
   _lastMessagePublished.clear(); // force a broadcast
}

void Thermostat::onNetworkState(NetworkState state) {
   _networkState = state;
}

void Thermostat::onSaveSetting(JsonDocument &doc) {
   doc["_mode"] = _mode;
   doc["_targetTemperature"] = getTargetTemperature();
}

void Thermostat::onLoadSetting(JsonDocument &doc) {
   _mode = doc["_mode"].isNull() ? off : doc["_mode"].as<Mode>();
   setTargetTemperature(doc["_targetTemperature"].isNull() ? 10.0 : doc["_targetTemperature"].as<float>());
}

String Thermostat::appTemplateProcessor(const String &var) {
   if (var == "title") {
      return String(_iot.getThingName().c_str());
   }
   if (var == "version") {
      return String(APP_VERSION);
   }
   if (var == "home_html") {
      return String(home_html);
   }

   logd("Did not find app template for: %s", var.c_str());
   return String("");
}

#ifdef HasMQTT

void Thermostat::onMqttConnect(esp_mqtt_client_handle_t &client) {
   // Subscribe to command topics
   esp_mqtt_client_subscribe(client, (_iot.getRootTopicPrefix() + "/cmnd/MODE").c_str(), 0);
   esp_mqtt_client_subscribe(client, (_iot.getRootTopicPrefix() + "/cmnd/TEMPERATURE").c_str(), 0);
   if (!_discoveryPublished) {
      if (PublishDiscoverySub() == false) {
         return; // try later
      }
      _discoveryPublished = true;
   }
}

boolean Thermostat::PublishDiscoverySub() {
   String topic = HOME_ASSISTANT_PREFIX;
   topic += "/climate/";
   topic += String(_iot.getUniqueId());
   topic += "/config";

   JsonDocument payload;

   payload["name"] = _iot.getThingName();
   payload["unique_id"] = String(_iot.getUniqueId());

   // Base topic for "~"
   payload["~"] = _iot.getRootTopicPrefix();

   // Availability
   payload["avty_t"] = "~/tele/LWT";
   payload["pl_avail"] = "Online";
   payload["pl_not_avail"] = "Offline";

   // Temperature
   payload["temp_cmd_t"] = "~/cmnd/TEMPERATURE";
   payload["temp_cmd_tpl"] = "{{ value }}";
   payload["temp_stat_t"] = "~/stat";
   payload["temperature_state_template"] = "{{ value_json.target_temperature }}";

   // Current temperature
   payload["curr_temp_t"] = "~/stat";
   payload["current_temperature_template"] = "{{ value_json.temperature }}";

   // Mode
   payload["mode_cmd_t"] = "~/cmnd/MODE";
   payload["mode_cmd_tpl"] = "{{ value }}";
   payload["mode_stat_t"] = "~/stat";
   payload["mode_state_template"] = "{{ value_json.mode }}";

   // Action (heating/idle)
   payload["act_t"] = "~/stat";
   payload["action_template"] = "{% if value_json.element %}heating{% else %}idle{% endif %}";

   JsonArray modes = payload["modes"].to<JsonArray>();
   modes.add("off");
   modes.add("heat");

   // Limits
   payload["min_temp"] = MIN_TEMPERATURE;
   payload["max_temp"] = MAX_TEMPERATURE;
   payload["temp_step"] = TEMP_PRECISION;
   payload["precision"] = TEMP_PRECISION;
   payload["unit_of_measurement"] = "°C";
   // Device info
   JsonObject device = payload["device"].to<JsonObject>();
   device["name"] = _iot.getThingName();
   device["sw_version"] = APP_VERSION;
   device["manufacturer"] = "ClassicDIY";

   char buffer[STR_LEN];
   sprintf(buffer, "%s (%llX)", APP_LOG_TAG, _iot.getUniqueId());
   device["model"] = buffer;

   JsonArray identifiers = device["identifiers"].to<JsonArray>();
   sprintf(buffer, "%llX", _iot.getUniqueId());
   identifiers.add(buffer);

   logd("Discovery => topic: %s", topic.c_str());
   return _iot.PublishMessage(topic.c_str(), payload, true);
}

void Thermostat::onMqttMessage(char *topic, char *payload) {
   if (String(topic) == (_iot.getRootTopicPrefix() + "/cmnd/MODE")) {
      logd("Mode command received: %s", payload);
      String cmd(payload);

      if (cmd == "off") {
         _mode = off;
      }
      if (cmd == "heat") {
         _mode = heat;
      }
   }
   if (String(topic) == (_iot.getRootTopicPrefix() + "/cmnd/TEMPERATURE")) {
      logd("TEMPERATURE command received: %s", payload);
      float tmp = atof(payload);
      if (tmp > MIN_TEMPERATURE && tmp < MAX_TEMPERATURE) {
         setTargetTemperature(tmp);
      }
   }
}
#endif
