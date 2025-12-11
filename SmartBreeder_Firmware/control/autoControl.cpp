#include "autoControl.h"
#include "../sensors/ph.h"
#include "../sensors/temp.h"
#include "../control/fan.h"
#include "../control/phControl.h"

AutoControl::AutoControl(PHSensor* ph, TempSensor* temp, FanControl* fan, PHControl* phCtrl) {
  phSensor = ph;
  tempSensor = temp;
  fanControl = fan;
  phControl = phCtrl;
  lastTempCheck = 0;
  lastPHCheck = 0;
}

void AutoControl::checkEmergency() {
  float temp = tempSensor->read();
  float ph = phSensor->read();
  
  // Emergency: Temperature too high
  if (temp > TEMP_MAX_SAFE) {
    fanControl->emergencyOn();
    Serial.println("EMERGENCY: Temperature too high - Fan forced ON");
  }
  
  // Emergency: pH out of safe range
  if (ph < PH_MIN_SAFE || ph > PH_MAX_SAFE) {
    phControl->emergencyStop();
    Serial.println("EMERGENCY: pH out of safe range - Pumps stopped");
  }
}

void AutoControl::checkTemperature() {
  if (activeFishType == FISH_NONE) return;
  if (fanControl->isManual()) return; // Don't override manual control
  
  float temp = tempSensor->read();
  const FishProfile& profile = FISH_PROFILES[activeFishType];
  
  if (temp < profile.tempMin) {
    // Too cold - turn on heater (if available)
    // For now, just turn off fan
    fanControl->set(false, false);
    Serial.printf("Temp too low (%.1f < %.1f) - Fan OFF\n", temp, profile.tempMin);
  } else if (temp > profile.tempMax) {
    // Too hot - turn on fan
    fanControl->set(true, false);
    Serial.printf("Temp too high (%.1f > %.1f) - Fan ON\n", temp, profile.tempMax);
  } else {
    // Normal temperature
    fanControl->set(false, false);
  }
}

void AutoControl::checkPH() {
  if (activeFishType == FISH_NONE) return;
  if (phControl->isManual()) return; // Don't override manual control
  if (!phControl->canDose()) return; // In cooldown period
  
  float ph = phSensor->read();
  const FishProfile& profile = FISH_PROFILES[activeFishType];
  
  if (ph < profile.phMin) {
    // Too acidic - add base
    phControl->setBase(true, false);
    phControl->setAcid(false, false);
    Serial.printf("pH too low (%.2f < %.2f) - Adding base\n", ph, profile.phMin);
  } else if (ph > profile.phMax) {
    // Too alkaline - add acid
    phControl->setAcid(true, false);
    phControl->setBase(false, false);
    Serial.printf("pH too high (%.2f > %.2f) - Adding acid\n", ph, profile.phMax);
  } else {
    // Normal pH - turn off pumps
    phControl->setAcid(false, false);
    phControl->setBase(false, false);
  }
}

void AutoControl::update() {
  unsigned long now = millis();
  
  // Check emergency conditions first
  checkEmergency();
  
  // Update control timers
  phControl->update();
  fanControl->update();
  
  // Check temperature
  if (now - lastTempCheck >= TEMP_CHECK_INTERVAL) {
    checkTemperature();
    lastTempCheck = now;
  }
  
  // Check pH
  if (now - lastPHCheck >= PH_CHECK_INTERVAL) {
    checkPH();
    lastPHCheck = now;
  }
}

