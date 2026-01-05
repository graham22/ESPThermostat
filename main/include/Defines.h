#pragma once

#include "GPIO_pins.h"

#define PUBLISH_RATE_LIMIT 200

#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3700
// the value of the 'other' resistor
#define SERIESRESISTOR 9995
// #define SensorVoltageMin 540 // Mininum output voltage from Sensor in mV (135 * .004 = 540)
// #define SensorVoltageMax 2700 // Maximum output voltage from Sensor in mV (135 * .02 = 2700).