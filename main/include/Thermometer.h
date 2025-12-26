#pragma once
#include <Arduino.h>
#include "defines.h"
#ifdef MAX6675
#include <max6675.h>
#endif
class Thermometer
{
	#define THERMISTORNOMINAL 10000 
	// temp. for nominal resistance (almost always 25 C)
	#define TEMPERATURENOMINAL 25
	// The beta coefficient of the thermistor (usually 3000-4000)
	#define BCOEFFICIENT 3700
	// the value of the 'other' resistor
	#define SERIESRESISTOR 9995

	int _sensorPin; //Defines the pin that the thermistor is connected to
	int _vRefPin; 
	float _offset;  //This constant is the offset of the MCP9700 in mv @ 0C
	float _voltRef;  //arduino +5, esp 3.3
	float _rollingSum;
	int _numberOfSummations;
	#ifdef MAX6675
	MAX6675 _ktc;
	#endif

public:
	#ifdef MAX6675
	Thermometer(int ktcCLK, int ktcCS, int ktcSO, float offset = 0.0);
	#else
	Thermometer(int sensorPin, int vRefPin, float voltRef);
	#endif
	~Thermometer();
	void Init();
	float Temperature();

private:	
	double ReadOversampledADC();

};

