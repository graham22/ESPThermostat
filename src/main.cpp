#include <Arduino.h>
#include <ArduinoJson.h>
#include <ThreadController.h>
#include <Thread.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <IotWebConf.h>
#include <PubSubClient.h>
#include <Thermometer.h>

// PIN assignments
#define THERMISTOR_SENSOR_PIN 35
#define THERMISTOR_POWER_PIN 25
#define SSR_PIN 5
#define TFT_LED_PIN 32
#define WIFI_STATUS_PIN 17 //First it will light up (kept LOW), on Wifi connection it will blink, when connected to the Wifi it will turn off (kept HIGH).
#define WIFI_AP_PIN 16	 // pull down to force WIFI AP mode

// display coordinates
#define CURRENT_TEMPERATURE_X 20
#define CURRENT_TEMPERATURE_Y 90
#define SET_TEMPERATURE_X 20
#define SET_TEMPERATURE_Y 200
#define ARROW_X 260
#define ARROW_Y 20
#define POWER_LED_X 30
#define POWER_LED_Y 20
#define WIFI_LED_X 50
#define WIFI_LED_Y 10

// Limits
#define DEFAULT_TARGET_TEMPERATURE 21.0
#define MAX_TEMPERATURE 32
#define MIN_TEMPERATURE 10
#define DISPLAY_TIMOUT 60
#define STR_LEN 64 // general string buffer size
#define AP_TIMEOUT 30000
#define WATCHDOG_TIMER 60000 //time in ms to trigger the watchdog
#define TEMP_PRECISION 0.5

enum Mode
{
	undefined,
	off,
	heat
};

//WIFI
// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "V1.3"
#define HOME_ASSISTANT_PREFIX "homeassistant" // MQTT prefix used in autodiscovery

DNSServer _dnsServer;
WebServer _webServer(80);
HTTPUpdateServer _httpUpdater;
IotWebConf _iotWebConf("ESPThermostat", &_dnsServer, &_webServer, "ESPThermostat", CONFIG_VERSION);

char _mqttServer[IOTWEBCONF_WORD_LEN];
char _mqttPort[5];
char _mqttUserName[IOTWEBCONF_WORD_LEN];
char _mqttUserPassword[IOTWEBCONF_WORD_LEN];
char _mqttTemperatureCmndTopic[STR_LEN];
char _mqttModeCmndTopic[STR_LEN];
char _savedTargetTemperature[5];
char _savedMode[5];
char _mqttRootTopic[STR_LEN];

IotWebConfSeparator seperatorParam = IotWebConfSeparator("MQTT");
IotWebConfParameter mqttServerParam = IotWebConfParameter("MQTT server", "mqttServer", _mqttServer, IOTWEBCONF_WORD_LEN);
IotWebConfParameter mqttPortParam = IotWebConfParameter("MQTT port", "mqttSPort", _mqttPort, 5, "text", NULL, "8080");
IotWebConfParameter mqttUserNameParam = IotWebConfParameter("MQTT user", "mqttUser", _mqttUserName, IOTWEBCONF_WORD_LEN);
IotWebConfParameter mqttUserPasswordParam = IotWebConfParameter("MQTT password", "mqttPass", _mqttUserPassword, IOTWEBCONF_WORD_LEN, "password");
IotWebConfParameter savedTargetTemperatureParam = IotWebConfParameter("Target temperature", "savedTargetTemperature", _savedTargetTemperature, 5, "number", NULL, NULL, "min='10' max='27'", false);
IotWebConfParameter savedModeParam = IotWebConfParameter("Current Mode", "savedMode", _savedMode, 5, "number", NULL, NULL, NULL, false);

hw_timer_t *timer = NULL;
ThreadController _controller = ThreadController();
Thread *_workerThreadTFT = new Thread();
Thread *_workerThreadHeat = new Thread();
Thread *_workerThreadMQTT = new Thread();
WiFiClient _EspClient;
PubSubClient _MqttClient(_EspClient);
TFT_eSPI _tft = TFT_eSPI();
float _targetTemperature = DEFAULT_TARGET_TEMPERATURE;
float _lastTargetTemperature = 0;
float _lastTemperatureReading = 0;
Thermometer _thermometer(THERMISTOR_SENSOR_PIN, THERMISTOR_POWER_PIN, 3.38);
boolean _heating_element_on = false;
Mode _requested_mode = undefined;
Mode _current_mode = undefined;
boolean _wifi_on = false;
u_int _display_timer;
u_int _uniqueId;
const char S_JSON_COMMAND_NVALUE[] PROGMEM = "{\"%s\":%d}";
const char S_JSON_COMMAND_LVALUE[] PROGMEM = "{\"%s\":%lu}";
const char S_JSON_COMMAND_SVALUE[] PROGMEM = "{\"%s\":\"%s\"}";
const char S_JSON_COMMAND_HVALUE[] PROGMEM = "{\"%s\":\"#%X\"}";

const uint8_t wifi_Symbol[33] PROGMEM = { // WIFI symbol
	0x00, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0x1C, 0x38,
	0x07, 0x60, 0xE1, 0xC7, 0x78, 0x1E, 0x0C, 0x30,
	0x80, 0x01, 0xE0, 0x07, 0x30, 0x0C, 0x00, 0x00,
	0x80, 0x01, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00};

void IRAM_ATTR resetModule()
{
	ets_printf("watchdog timer expired - rebooting\n");
	esp_restart();
}

void init_watchdog()
{
	if (timer == NULL)
	{
		timer = timerBegin(0, 80, true);					  //timer 0, div 80
		timerAttachInterrupt(timer, &resetModule, true);	  //attach callback
		timerAlarmWrite(timer, WATCHDOG_TIMER * 1000, false); //set time in us
		timerAlarmEnable(timer);							  //enable interrupt
	}
}

void feed_watchdog()
{
	if (timer != NULL)
	{
		timerWrite(timer, 0); // feed the watchdog
	}
}

void initTFT()
{ // Initialise the TFT screen
	_tft.init();
	_tft.setRotation(1); // Set the rotation before we calibrate
	uint16_t calData[5] = {452, 3097, 460, 3107, 4};
	_tft.setTouch(calData);
	pinMode(TFT_LED_PIN, OUTPUT);
	digitalWrite(TFT_LED_PIN, HIGH);
	_display_timer = DISPLAY_TIMOUT;
	_tft.fillScreen(TFT_BLACK);
	int xpos = ARROW_X;
	int ypos = ARROW_Y;
	_tft.fillTriangle(
		xpos, ypos,			  // peak
		xpos - 40, ypos + 60, // bottom left
		xpos + 40, ypos + 60, // bottom right
		TFT_RED);
	ypos += 200;
	_tft.fillTriangle(
		xpos, ypos,			  // peak
		xpos - 40, ypos - 60, // bottom left
		xpos + 40, ypos - 60, // bottom right
		TFT_BLUE);
}

void publish(const char *topic, const char *value, boolean retained = false)
{
	if (_MqttClient.connected())
	{
		char buf[64];
		sprintf(buf, "%s/stat/%s", _mqttRootTopic, topic);
		_MqttClient.publish(buf, value, retained);
		Serial.print("Topic: ");
		Serial.print(buf);
		Serial.print(" Data: ");
		Serial.println(value);
	}
}

void publish(const char *topic, const char *subtopic, float value, boolean retained = false)
{
	char str_temp[6];
	dtostrf(value, 2, 1, str_temp);
	char buf[256];
	snprintf_P(buf, sizeof(buf), S_JSON_COMMAND_SVALUE, subtopic, str_temp);
	publish(topic, buf);
}

void showTargetTemperature()
{
	_tft.setTextSize(4);
	_tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
	if (_current_mode == heat)
	{
		_tft.drawFloat(_targetTemperature, 1, SET_TEMPERATURE_X, SET_TEMPERATURE_Y);
		publish("TEMPERATURE", "SET_TEMPERATURE", _targetTemperature, true);
	}
	else
	{
		_tft.drawString("Off   ", SET_TEMPERATURE_X, SET_TEMPERATURE_Y);
	}
}

void up()
{
	if (_targetTemperature < MAX_TEMPERATURE)
	{
		_targetTemperature += TEMP_PRECISION;
		showTargetTemperature();
	}
}

void down()
{
	if (_targetTemperature > MIN_TEMPERATURE)
	{
		_targetTemperature -= TEMP_PRECISION;
		showTargetTemperature();
	}
}

void wakeScreen()
{
	digitalWrite(TFT_LED_PIN, HIGH);
	_display_timer = DISPLAY_TIMOUT;
}

void trimSlashes(const char *input, char *result)
{
	int i, j = 0;
	for (i = 0; input[i] != '\0'; i++)
	{
		if (input[i] != '/' && input[i] != '\\')
		{
			result[j++] = input[i];
		}
	}
	result[j] = '\0';
}

void runTFT()
{
	uint16_t x = 0, y = 0;
	if (_tft.getTouch(&x, &y))
	{
		if (_display_timer > 0)
		{
			if (x > 200)
			{
				if (_current_mode == heat)
				{
					if (y < 120)
					{
						up();
					}
					else
					{
						down();
					}
				}
			}
			else if (y > 200)
			{
				_requested_mode = _current_mode == heat ? off : heat; // toggle on/off
			}
		}
		wakeScreen();
	}
	if (_wifi_on == false && WiFi.isConnected())
	{
		_wifi_on = true;
		_tft.drawXBitmap(WIFI_LED_X, WIFI_LED_Y, wifi_Symbol, 16, 16, TFT_GREEN);
	}
	else if (_wifi_on == true && !WiFi.isConnected())
	{
		_wifi_on = false;
		_tft.drawXBitmap(WIFI_LED_X, WIFI_LED_Y, wifi_Symbol, 16, 16, TFT_BLACK);
	}
}

void actionHeater()
{
	if (_heating_element_on)
	{
		digitalWrite(SSR_PIN, HIGH);
		_tft.fillCircle(POWER_LED_X, POWER_LED_Y, 10, TFT_RED);
		publish("ACTION", "heating");
	}
	else
	{
		digitalWrite(SSR_PIN, LOW);
		_tft.fillCircle(POWER_LED_X, POWER_LED_Y, 10, TFT_BLACK);
		publish("ACTION", _current_mode == heat ? "idle" : "off");
	}
}

void runHeater()
{
	float deg = _thermometer.Temperature();
	_tft.setTextSize(8);
	_tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	_tft.drawFloat(deg, 1, CURRENT_TEMPERATURE_X, CURRENT_TEMPERATURE_Y);
	if (_current_mode != _requested_mode)
	{
		_current_mode = _requested_mode;
		publish("MODE", _current_mode == heat ? "heat" : "off");
		_heating_element_on = false;
		actionHeater();
		showTargetTemperature();
	}
	if (_current_mode == heat)
	{
		if (_targetTemperature > deg)
		{
			if (_heating_element_on == false)
			{
				_heating_element_on = true;
				actionHeater();
			}
		}
		else
		{
			if (_heating_element_on)
			{
				_heating_element_on = false;
				actionHeater();
			}
		}
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
	feed_watchdog();
}

void MQTT_callback(char *topic, byte *payload, unsigned int data_len)
{
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (unsigned int i = 0; i < data_len; i++)
	{
		Serial.print((char)payload[i]);
	}
	Serial.println();
	char *data = (char *)payload;
	if (strcmp(_mqttTemperatureCmndTopic, topic) == 0)
	{
		if (strncmp(data, "UP", data_len) == 0)
		{
			up();
		}
		else if (strncmp(data, "DOWN", data_len) == 0)
		{
			down();
		}
		else
		{
			String inString = data;
			float target = inString.toFloat();
			if (target < MIN_TEMPERATURE)
			{
				_targetTemperature = MIN_TEMPERATURE;
			}
			else if (target > MAX_TEMPERATURE)
			{
				_targetTemperature = MAX_TEMPERATURE;
			}
			else
			{
				_targetTemperature = target;
			}
			showTargetTemperature();
		}
	}
	else if (strcmp(_mqttModeCmndTopic, topic) == 0)
	{
		if (strncmp(data, "heat", data_len) == 0)
		{
			_requested_mode = heat;
		}
		else if (strncmp(data, "off", data_len) == 0)
		{
			_requested_mode = off;
		}
	}
	wakeScreen();
}

void runMQTT()
{
	if (!_MqttClient.connected())
	{
		int port = atoi(_mqttPort);
		_MqttClient.setServer(_mqttServer, port);
		char willTopic[STR_LEN];
		sprintf(_mqttRootTopic, "%s/%X/climate", _iotWebConf.getThingName(), _uniqueId);
		sprintf(willTopic, "%s/tele/LWT", _mqttRootTopic);
		sprintf(_mqttTemperatureCmndTopic, "%s/cmnd/TEMPERATURE", _mqttRootTopic);
		sprintf(_mqttModeCmndTopic, "%s/cmnd/MODE", _mqttRootTopic);
		if (_MqttClient.connect(_iotWebConf.getThingName(), _mqttUserName, _mqttUserPassword, willTopic, 0, false, "Offline"))
		{
			Serial.println("MQTT connected");
			char buffer[STR_LEN];
			char configurationTopic[64];
			sprintf(configurationTopic, "%s/climate/%X/config", HOME_ASSISTANT_PREFIX, _uniqueId);
			_MqttClient.subscribe(_mqttTemperatureCmndTopic);
			_MqttClient.subscribe(_mqttModeCmndTopic);
			_MqttClient.setCallback(MQTT_callback);
			StaticJsonDocument<1024> doc; // MQTT discovery
			doc["name"] = _iotWebConf.getThingName();
			sprintf(buffer, "%X", _uniqueId);
			doc["unique_id"] = buffer;
			doc["mode_cmd_t"] = "~/cmnd/MODE";
			doc["mode_stat_t"] = "~/stat/MODE";
			doc["avty_t"] = "~/tele/LWT";
			doc["pl_avail"] = "Online";
			doc["pl_not_avail"] = "Offline";
			doc["temp_cmd_t"] = "~/cmnd/TEMPERATURE";
			doc["temp_stat_t"] = "~/stat/TEMPERATURE";
			doc["temp_stat_tpl"] = "{{value_json.SET_TEMPERATURE}}";
			doc["curr_temp_t"] = "~/stat/TEMPERATURE";
			doc["curr_temp_tpl"] = "{{value_json.CURRENT_TEMPERATURE}}";
			doc["action_topic"] = "~/stat/ACTION";
			doc["min_temp"] = MIN_TEMPERATURE;
			doc["max_temp"] = MAX_TEMPERATURE;
			doc["temp_step"] = TEMP_PRECISION;
			JsonArray array = doc.createNestedArray("modes");
			array.add("off");
			array.add("heat");
			JsonObject device = doc.createNestedObject("device");
			device["name"] = _iotWebConf.getThingName();
			device["sw_version"] = CONFIG_VERSION;
			device["manufacturer"] = "SkyeTracker";
			sprintf(buffer, "ESP32-Bit (%X)", _uniqueId);
			device["model"] = buffer;
			JsonArray identifiers = device.createNestedArray("identifiers");
			identifiers.add(_uniqueId);
			doc["~"] = _mqttRootTopic;
			String s;
			serializeJson(doc, s);
			if (_MqttClient.connected())
			{
				unsigned int msgLength = MQTT_MAX_HEADER_SIZE + 2 + strlen(configurationTopic) + s.length();
				if (msgLength > MQTT_MAX_PACKET_SIZE)
				{
					Serial.print("**** Configuration payload exceeds MAX Packet Size: ");
					Serial.println(msgLength);
				}
				else
				{
					_MqttClient.publish(configurationTopic, (const uint8_t *)s.c_str(), s.length(), true);
				}
				Serial.println(s.c_str());
			}
			_MqttClient.publish(willTopic, "Online");
			publish("TEMPERATURE", "CURRENT_TEMPERATURE", _thermometer.Temperature());
			publish("TEMPERATURE", "SET_TEMPERATURE", _targetTemperature, true);
		}
		else
		{
			Serial.println("Failed to connect to MQTT");
		}
	}
	else
	{
		float currentTemperature = _thermometer.Temperature();
		currentTemperature = roundf(currentTemperature * 10.0f) / 10.0f; // round to one decimal place
		if (abs(_lastTemperatureReading - currentTemperature) > 0.2)	 // publish changes greater than .2 degrees in temperature
		{
			publish("TEMPERATURE", "CURRENT_TEMPERATURE", currentTemperature);
			_lastTemperatureReading = currentTemperature;
		}
		if (_lastTargetTemperature != _targetTemperature) // save new target temperature into EEPROM if changed
		{
			_lastTargetTemperature = _targetTemperature;
			dtostrf(_targetTemperature, 2, 1, _savedTargetTemperature);
			_iotWebConf.configSave();
		}
	}
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
	// -- Let IotWebConf test and handle captive portal requests.
	if (_iotWebConf.handleCaptivePortal())
	{
		// -- Captive portal request were already served.
		return;
	}
	String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
	s += "<title>ESPThermostat</title></head><body>";
	s += _iotWebConf.getThingName();
	s += "<ul>";
	s += "<li>MQTT server: ";
	s += _mqttServer;
	s += "</ul>";
	s += "<ul>";
	s += "<li>MQTT port: ";
	s += _mqttPort;
	s += "</ul>";
	s += "<ul>";
	s += "<li>MQTT user: ";
	s += _mqttUserName;
	s += "</ul>";
	s += "<ul>";
	s += "<li>MQTT root topic: ";
	s += _mqttRootTopic;
	s += "</ul>";
	s += "Go to <a href='config'>configure page</a> to change values.";
	s += "</body></html>\n";

	_webServer.send(200, "text/html", s);
}

void wifiConnected()
{
	_workerThreadMQTT->onRun(runMQTT);
	_workerThreadMQTT->setInterval(5000);
	_controller.add(_workerThreadMQTT);
	init_watchdog();
}

void configSaved()
{
	Serial.println("Configuration was updated.");
}

boolean formValidator()
{
	boolean valid = true;
	int l = _webServer.arg(mqttServerParam.getId()).length();
	if (l < 3)
	{
		mqttServerParam.errorMessage = "Please provide at least 3 characters!";
		valid = false;
	}
	return valid;
}

void setup(void)
{
	Serial.begin(115200);
	while (!Serial)
	{
		; // wait for serial port to connect. Needed for native USB port only
	}
	Serial.println();
	Serial.println("Booting");
	pinMode(WIFI_AP_PIN, INPUT_PULLUP);
	pinMode(WIFI_STATUS_PIN, OUTPUT);
	// generate unique id from mac address NIC segment
	uint8_t chipid[6];
	esp_efuse_mac_get_default(chipid);
	_uniqueId = chipid[3] << 16;
	_uniqueId += chipid[4] << 8;
	_uniqueId += chipid[5];
	initTFT();
	pinMode(SSR_PIN, OUTPUT);
	digitalWrite(SSR_PIN, LOW);
	_current_mode = undefined;
	_iotWebConf.setStatusPin(WIFI_STATUS_PIN);
	_iotWebConf.setConfigPin(WIFI_AP_PIN);
	// setup EEPROM parameters
	_iotWebConf.addParameter(&savedTargetTemperatureParam);
	_iotWebConf.addParameter(&savedModeParam);
	_iotWebConf.addParameter(&seperatorParam);
	_iotWebConf.addParameter(&mqttServerParam);
	_iotWebConf.addParameter(&mqttPortParam);
	_iotWebConf.addParameter(&mqttUserNameParam);
	_iotWebConf.addParameter(&mqttUserPasswordParam);
	// setup callbacks for IotWebConf
	_iotWebConf.setConfigSavedCallback(&configSaved);
	_iotWebConf.setFormValidator(&formValidator);
	_iotWebConf.setWifiConnectionCallback(&wifiConnected);
	_iotWebConf.setupUpdateServer(&_httpUpdater);
	boolean validConfig = _iotWebConf.init();
	if (!validConfig)
	{
		Serial.println("!invalid configuration!");
		_mqttServer[0] = '\0';
		_mqttPort[0] = '\0';
		_mqttUserName[0] = '\0';
		_mqttUserPassword[0] = '\0';
		_targetTemperature = DEFAULT_TARGET_TEMPERATURE;
		dtostrf(_targetTemperature, 2, 1, _savedTargetTemperature);
		_requested_mode = off;
		ltoa(off, _savedMode, 10);
	}
	else
	{
		_iotWebConf.setApTimeoutMs(AP_TIMEOUT);
		_targetTemperature = atof(_savedTargetTemperature);
		_lastTargetTemperature = _targetTemperature;
		_requested_mode = (Mode)atoi(_savedMode);
	}
	// Set up required URL handlers on the web server.
	_webServer.on("/", handleRoot);
	_webServer.on("/config", [] { _iotWebConf.handleConfig(); });
	_webServer.onNotFound([]() { _iotWebConf.handleNotFound(); });
	// setup worker threads
	_thermometer.Init();
	_workerThreadTFT->onRun(runTFT);
	_workerThreadTFT->setInterval(250);
	_controller.add(_workerThreadTFT);
	_workerThreadHeat->onRun(runHeater);
	_workerThreadHeat->setInterval(1000);
	_controller.add(_workerThreadHeat);
	showTargetTemperature();
}

void loop(void)
{
	_iotWebConf.doLoop();
	_controller.run();
	if (WiFi.isConnected() && _MqttClient.connected())
	{
		_MqttClient.loop();
	}
}