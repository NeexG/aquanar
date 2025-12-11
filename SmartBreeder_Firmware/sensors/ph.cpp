#include "ph.h"
#include "../config/config.h"

PHSensor::PHSensor(int pin) : pin(pin), ph7Voltage(2.50), ph4Voltage(1.50), 
                               sampleIndex(0), bufferFilled(false) {
  // Calculate initial slope: (7.0 - 4.0) / (ph7Voltage - ph4Voltage)
  slope = 3.0 / (ph7Voltage - ph4Voltage);
  offset = 0.0;
  
  // Initialize sample buffer
  for (int i = 0; i < PH_MEDIAN_SAMPLES; i++) {
    samples[i] = 7.0;
  }
}

void PHSensor::begin() {
  pinMode(pin, INPUT);
  loadCalibration();
  Serial.println("pH Sensor initialized");
}

void PHSensor::loadCalibration() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  ph7Voltage = prefs.getFloat(PREF_PH7_KEY, 2.50);
  ph4Voltage = prefs.getFloat(PREF_PH4_KEY, 1.50);
  prefs.end();
  
  // Recalculate slope
  if (ph7Voltage != ph4Voltage) {
    slope = 3.0 / (ph7Voltage - ph4Voltage);
  }
  
  Serial.printf("pH calibration loaded: 7.00=%.3fV, 4.00=%.3fV, slope=%.3f\n", 
                ph7Voltage, ph4Voltage, slope);
}

float PHSensor::calculateMedian() {
  float sorted[PH_MEDIAN_SAMPLES];
  for (int i = 0; i < PH_MEDIAN_SAMPLES; i++) {
    sorted[i] = samples[i];
  }
  
  // Bubble sort
  for (int i = 0; i < PH_MEDIAN_SAMPLES - 1; i++) {
    for (int j = 0; j < PH_MEDIAN_SAMPLES - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  
  return sorted[PH_MEDIAN_SAMPLES / 2];
}

float PHSensor::read() {
  int adcValue = analogRead(pin);
  float voltage = (adcValue / 4095.0) * 3.3; // ESP32 12-bit ADC, 3.3V reference
  
  // Calculate pH using two-point calibration
  // pH = 7.0 + ((ph7Voltage - voltage) / slope)
  float phValue = 7.0 + ((ph7Voltage - voltage) / slope) + offset;
  
  // Clamp to valid range
  if (phValue < 0) phValue = 0;
  if (phValue > 14) phValue = 14;
  
  // Add to median filter buffer
  samples[sampleIndex] = phValue;
  sampleIndex = (sampleIndex + 1) % PH_MEDIAN_SAMPLES;
  
  if (sampleIndex == 0 && !bufferFilled) {
    bufferFilled = true;
  }
  
  // Return median if buffer is filled, otherwise return current value
  if (bufferFilled) {
    return calculateMedian();
  }
  
  return phValue;
}

void PHSensor::calibrate7() {
  int adcValue = analogRead(pin);
  ph7Voltage = (adcValue / 4095.0) * 3.3;
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat(PREF_PH7_KEY, ph7Voltage);
  prefs.end();
  
  // Recalculate slope
  if (ph7Voltage != ph4Voltage) {
    slope = 3.0 / (ph7Voltage - ph4Voltage);
  }
  
  Serial.printf("pH 7.00 calibrated: %.3fV\n", ph7Voltage);
}

void PHSensor::calibrate4() {
  int adcValue = analogRead(pin);
  ph4Voltage = (adcValue / 4095.0) * 3.3;
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat(PREF_PH4_KEY, ph4Voltage);
  prefs.end();
  
  // Recalculate slope
  if (ph7Voltage != ph4Voltage) {
    slope = 3.0 / (ph7Voltage - ph4Voltage);
  }
  
  Serial.printf("pH 4.00 calibrated: %.3fV\n", ph4Voltage);
}

void PHSensor::setCalibration(float ph7, float ph4) {
  ph7Voltage = ph7;
  ph4Voltage = ph4;
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat(PREF_PH7_KEY, ph7Voltage);
  prefs.putFloat(PREF_PH4_KEY, ph4Voltage);
  prefs.end();
  
  if (ph7Voltage != ph4Voltage) {
    slope = 3.0 / (ph7Voltage - ph4Voltage);
  }
}

bool PHSensor::isSafe() {
  float currentPH = read();
  return (currentPH >= PH_MIN_SAFE && currentPH <= PH_MAX_SAFE);
}

