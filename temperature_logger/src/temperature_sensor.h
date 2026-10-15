/*
 * temperature_sensor.h
 *
 *  Created on: Sep. 25, 2021
 *      Author: Danny
 */

#ifndef TEMPERATURE_SENSOR_H_
#define TEMPERATURE_SENSOR_H_

#include <OneWire.h>
#include <DallasTemperature.h>

class TemperatureSensor
{
  OneWire oneWire;            // oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
  DallasTemperature DS18B20;  // must pass oneWire reference to Dallas Temperature

  bool readingReady;
  float lastReading;

  void getReadingAndReady();

public:
  TemperatureSensor(int sensorpin);

  void setup();

  void askForReading();
  bool isReadingReady();
  float getReading();
};



#endif /* TEMPERATURE_SENSOR_H_ */
