#pragma once

#include "GPIO_pins.h"

#define TAG "ESPThermostat"

#define NTP_SERVER "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define HOME_ASSISTANT_PREFIX "homeassistant" //Home Assistant Auto discovery root topic

#define WATCHDOG_TIMEOUT 10 // time in seconds to trigger the watchdog reset
#define STR_LEN 64
#define EEPROM_SIZE 4096
#define LOG_BUFFER_SIZE 2048
#define AP_BLINK_RATE 600
#define NC_BLINK_RATE 100

// #define AP_TIMEOUT 1000 //set back to 30000 in production
#define AP_TIMEOUT 30000 
#define FLASHER_TIMEOUT 10000
#define GPIO0_FactoryResetCountdown 5000 // do a factory reset if GPIO0 is pressed for GPIO0_FactoryResetCountdown
#define WS_CLIENT_CLEANUP 5000
#define WIFI_CONNECTION_TIMEOUT 120000
#define DEFAULT_AP_PASSWORD "12345678"
#define ASYNC_WEBSERVER_PORT 80
#define DNS_PORT 53
#define PUBLISH_RATE_LIMIT 200 // delay between MQTT publishes
#define SAMPLESIZE 20
#define MODBUS_POLL_RATE 1000
#define MODBUS_RTU_TIMEOUT 2000
#define MODBUS_RTU_REQUEST_QUEUE_SIZE 64

#define INPUT_REGISTER_BASE_ADDRESS 0
#define COIL_BASE_ADDRESS 0
#define DISCRETE_BASE_ADDRESS 0
#define HOLDING_REGISTER_BASE_ADDRESS 0

#define SensorVoltageMin 540 // Mininum output voltage from Sensor in mV (135 * .004 = 540)
#define SensorVoltageMax 2700 // Maximum output voltage from Sensor in mV (135 * .02 = 2700).