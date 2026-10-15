/*
 * temperature_logger.h
 *
 *  Created on: Sep. 29, 2026
 *      Author: Danny
 */

#ifndef TEMPERATURE_LOGGER_H_
#define TEMPERATURE_LOGGER_H_

#include "temperature_sensor.h"
#include "store_temps.h"

class TemperatureLogger
{
	float lastTemperatureRead;
	bool waitingForReading, temperatureIsValid;
	unsigned long lastAskTime, lastCheckTime;
	unsigned long lastDelayTime;
	unsigned long lastRecordingTime;
	TemperatureSensor tempSensor;
	TemperatureStore temperatureStorage;

public:
	TemperatureLogger(int pin);

	void setupTemperatureLogger();
	void loopTemperatureLogger();

	void takeTemperatureReading();
	void recordTemperatureReading();
// utilities
	char *getLastTemperatureAsString(float offset = 0);
	float getLastTemperature();
	unsigned int getLastDelayTime();

	TemperatureStore &getStore();

private:
	void handleTemperatureReading();
};

extern TemperatureLogger the_temperature_logger;


#endif /* TEMPERATURE_LOGGER_H_ */
