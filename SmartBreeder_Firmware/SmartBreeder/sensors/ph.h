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
  void calculateSlope(); // Calculate slope from calibration points
  int getMedianADC(); // Median filter for ADC readings (calibration - more samples)
  int getFastMedianADC(); // Fast median filter for regular readings (optimized)
  
public:
  PHSensor(int pin);
  void begin();
  float read();
  void calibrate7();
  void calibrate4();
  float getCalibration7() { return ph7Voltage; }
  float getCalibration4() { return ph4Voltage; }
  float getOffset() { return offset; }
  void setCalibration(float ph7, float ph4);
  void setOffset(float off); // Set pH offset for fine-tuning (saves to preferences)
  void adjustOffsetForNormalWater(float targetPH = 7.0f); // Auto-adjust offset for normal water
  bool isSafe();
};

#endif

