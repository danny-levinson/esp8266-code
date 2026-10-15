/*
 * temperature_sensor.cpp
 *
 *  Created on: Sep. 25, 2021
 *      Author: Danny
 */

//#include <OneWire.h>			// included in header to avoid a hidden _impl class
//#include <DallasTemperature.h>
#include <Arduino.h>

//#include "loghandler.h"		// don't do message logging for now
#define MSGV(x)

#include "temperature_sensor.h"

#define SENSOR_PRESENT true


TemperatureSensor::TemperatureSensor(int sensorpin) : oneWire(sensorpin), DS18B20(&oneWire),
			readingReady(false), lastReading(0.0) {}

void TemperatureSensor::setup() {
#if SENSOR_PRESENT
	MSGV("Starting real temperature sensor");
	DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
#else
	MSGV("Starting fake temperature sensor");
#endif
}

void TemperatureSensor::askForReading() {
#if SENSOR_PRESENT
	DS18B20.requestTemperatures();
#endif
	readingReady = false;
}

bool TemperatureSensor::isReadingReady() {
	getReadingAndReady();
	return readingReady;
}

float TemperatureSensor::getReading() {		// only valid after readingReady returns true
	return lastReading;
}

void TemperatureSensor::getReadingAndReady() {
	float tempC = 0.0;

	//    do {
	//      DS18B20.requestTemperatures();          // !!! doing these together slows http responses
	//      tempC = DS18B20.getTempCByIndex(0);
	//      delay(100);
	//    } while (tempC == 85.0 || tempC == (-127.0));
	//    lastReading = tempC;
	//    readingReady = true;

#if SENSOR_PRESENT
	tempC = DS18B20.getTempCByIndex(0);
#else
	if (tempC < 1.0) tempC = 20.0;
	tempC += (random(100)-50)/70.0;
#endif
	if (tempC == 85.0 || tempC == (-127.0)) return;		// not yet ready

	readingReady = true;
	lastReading = tempC;
}



