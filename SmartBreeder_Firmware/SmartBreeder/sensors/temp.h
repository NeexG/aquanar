#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include "config/config.h"

class TempSensor {
private:
  OneWire* oneWire;
  DallasTemperature* sensors;
  float offset;
  float lastReading;
  
  void loadCalibration();
  
public:
  TempSensor(int pin);
  void begin();
  float read();
  void setOffset(float offset);
  float getOffset() { return offset; }
  bool isSafe();
};

#endif

