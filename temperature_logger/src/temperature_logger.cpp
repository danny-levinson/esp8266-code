/*
 * temperature_logger.cpp
 *
 *  Created on: Sep. 7, 2021
 *      Author: Danny
 */

#include <Arduino.h>

#include "temperature_logger.h"


#define SENSOR_PIN D1

TemperatureLogger the_temperature_logger(SENSOR_PIN);

TemperatureLogger::TemperatureLogger(int pin) : lastTemperatureRead(0.0),
		waitingForReading(false), lastAskTime(0), lastCheckTime(0),
		lastRecordingTime(0), lastDelayTime(0), temperatureIsValid(false),
		tempSensor(pin) {
}

void TemperatureLogger::setupTemperatureLogger() {
	tempSensor.setup();
}


void TemperatureLogger::loopTemperatureLogger() {
	takeTemperatureReading();			// every second or so
	recordTemperatureReading();			// once per minute

}

/*
 * Note: DS18B20 thermometer device takes between 93.75 & 750 msecs to convert temperature
 * depending on number of bits of precision (see table on p3 of datasheet).
 */

static const int minAskWaitTime = 2000;			// minimum of 2 seconds between readings
static const int minCheckWaitTime = 50;			// minimum of 50 milliseconds wait before checking
static const int minCheckIntervalTime = 20;
static const int recordingInterval = 60000;		// make one record per minute

void TemperatureLogger::takeTemperatureReading() {
	if (! waitingForReading) {
		if (millis() - lastAskTime < minAskWaitTime) return;
		tempSensor.askForReading();
		lastAskTime = lastCheckTime = millis();
		waitingForReading = true;
		return;
	}
	if (millis() - lastAskTime < minCheckWaitTime) return;
	if (millis() - lastCheckTime < minCheckIntervalTime) return;
	lastCheckTime = millis();
	if (! tempSensor.isReadingReady()) return;
	lastTemperatureRead = tempSensor.getReading();
	temperatureIsValid = true;
	lastDelayTime = millis() - lastAskTime;
	waitingForReading = false;
}

// this will handle 50 day wrap around (tested it)
// see: http://www.gammon.com.au/millis or https://arduino.stackexchange.com/questions/33572/arduino-countdown-without-using-delay/33577#33577

void TemperatureLogger::recordTemperatureReading() {
	if (! temperatureIsValid) return;
	if (millis() - lastRecordingTime < recordingInterval) return;
	lastRecordingTime = millis();
	temperatureStorage.pushReading(lastTemperatureRead);
}


TemperatureStore &TemperatureLogger::getStore() {
	return temperatureStorage;
}

unsigned int TemperatureLogger::getLastDelayTime() {
	return lastDelayTime;
}

float TemperatureLogger::getLastTemperature() {
	return lastTemperatureRead;
}

// returns a static; use at once!

char *TemperatureLogger::getLastTemperatureAsString(float offset) {
	static char temperatureString[7];
	dtostrf(lastTemperatureRead+offset, 2, 2, temperatureString);
	return temperatureString;
}
