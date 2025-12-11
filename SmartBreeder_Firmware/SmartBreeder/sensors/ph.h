#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include <Arduino.h>
#include <Preferences.h>
#include "config/config.h"

class PHSensor {
private:
  int pin;
  float ph7Voltage;
  float ph4Voltage;
  float slope;
  float offset;
  
  float samples[PH_MEDIAN_SAMPLES];
  int sampleIndex;
  bool bufferFilled;
  
  float calculateMedian();
  void loadCalibration();
  
public:
  PHSensor(int pin);
  void begin();
  float read();
  void calibrate7();
  void calibrate4();
  float getCalibration7() { return ph7Voltage; }
  float getCalibration4() { return ph4Voltage; }
  void setCalibration(float ph7, float ph4);
  bool isSafe();
};

#endif

