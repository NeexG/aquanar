#include "temp.h"
#include "../config/config.h"

TempSensor::TempSensor(int pin) : offset(0.0), lastReading(25.0) {
  oneWire = new OneWire(pin);
  sensors = new DallasTemperature(oneWire);
}

void TempSensor::begin() {
  sensors->begin();
  loadCalibration();
  Serial.println("Temperature sensor initialized");
}

void TempSensor::loadCalibration() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  offset = prefs.getFloat(PREF_TEMP_OFFSET_KEY, 0.0);
  prefs.end();
  Serial.printf("Temperature offset loaded: %.2f°C\n", offset);
}

float TempSensor::read() {
  sensors->requestTemperatures();
  float temp = sensors->getTempCByIndex(0);
  
  if (temp == DEVICE_DISCONNECTED_C) {
    Serial.println("Temperature sensor error - using last reading");
    return lastReading;
  }
  
  lastReading = temp + offset;
  return lastReading;
}

void TempSensor::setOffset(float newOffset) {
  offset = newOffset;
  
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, false);
  prefs.putFloat(PREF_TEMP_OFFSET_KEY, offset);
  prefs.end();
  
  Serial.printf("Temperature offset set: %.2f°C\n", offset);
}

bool TempSensor::isSafe() {
  float currentTemp = read();
  return (currentTemp <= TEMP_MAX_SAFE);
}

