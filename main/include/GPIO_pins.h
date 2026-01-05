#pragma once
#include <Arduino.h>
#include "Log.h"

#ifdef ESP32_DEV_BOARD

#define ADC_Resolution 4095.0

// I2C
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

#define THERMISTOR_SENSOR_PIN 35
#define THERMISTOR_POWER_PIN 25
#define SSR_PIN 5
#define TFT_LED_PIN 32
#define KCT_CS 26
#define MAX_TEMPERATURE 32
#define MIN_TEMPERATURE 10
#define TEMP_PRECISION 0.5
#define ESP_VOLTAGE_REFERENCE 3.4

#endif
