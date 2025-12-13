#include "autoControl.h"
#include "sensors/ph.h"
#include "sensors/temp.h"
#include "control/fan.h"
#include "control/phControl.h"
#include "config/config.h"

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
    digitalWrite(REL_WATER_HEATER, getRelayLevel(false));
  }
  
  // Emergency: pH extremely dangerous (only stop if < 1.0 or > 13.0)
  if (ph < 1.0 || ph > 13.0) {
    phControl->stopAll();
  }
}

void AutoControl::checkTemperature() {
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  bool useCustom = prefs.getBool("use_custom_profile", false);
  prefs.end();
  
  if (activeFishType == FISH_NONE && !useCustom) {
    fanControl->set(false, false);
    digitalWrite(REL_WATER_HEATER, getRelayLevel(false));
    // Air pump OFF when no fish selected
    digitalWrite(REL_AIR_PUMP, getRelayLevel(false));
    return;
  }
  
  // Air pump ON when any fish is selected
  digitalWrite(REL_AIR_PUMP, getRelayLevel(true));
  
  float temp = tempSensor->read();
  FishProfile profile = getActiveFishProfile();
  bool fanManual = fanControl->isManual();
  
  if (temp > profile.tempMax) {
    if (!fanManual) {
      fanControl->set(true, false);
    }
    digitalWrite(REL_WATER_HEATER, getRelayLevel(false));
  } else if (temp < profile.tempMin) {
    digitalWrite(REL_WATER_HEATER, getRelayLevel(true));
    if (!fanManual) {
      fanControl->set(false, false);
    }
  } else {
    if (!fanManual) {
      fanControl->set(false, false);
    }
    digitalWrite(REL_WATER_HEATER, getRelayLevel(false));
  }
}

void AutoControl::checkPH() {
  // CRITICAL: Always ensure base pump is OFF first (safety check)
  // This prevents any accidental activation
  if (!phControl->getBaseState()) {
    // Base pump should be OFF - double check by forcing it OFF
    // This is a safety measure to prevent GPIO23 from accidentally going LOW
    digitalWrite(REL_ALKALI_PUMP, HIGH);
  }
  
  // Check if fish is selected
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  bool useCustom = prefs.getBool("use_custom_profile", false);
  prefs.end();
  
  if (activeFishType == FISH_NONE && !useCustom) {
    phControl->setAcid(false);
    phControl->setBase(false);
    // Extra safety: Force GPIO23 HIGH
    digitalWrite(REL_ALKALI_PUMP, HIGH);
    return;
  }
  
  // Check cooldown - CRITICAL: This must pass before any pump activation
  if (!phControl->canDose()) {
    // In cooldown - ensure pumps are OFF
    phControl->setAcid(false);
    phControl->setBase(false);
    // Extra safety: Force GPIO23 HIGH during cooldown
    digitalWrite(REL_ALKALI_PUMP, HIGH);
    return;
  }
  
  // Read pH and get profile
  float ph = phSensor->read();
  FishProfile profile = getActiveFishProfile();
  
  // pH Control Logic:
  // pH > max → Run ACID pump (reduce pH)
  // pH < min → Run ALKALINE pump (increase pH)
  // pH in range → Both pumps OFF
  
  if (ph > profile.phMax) {
    phControl->setBase(false);
    phControl->setAcid(true);
    // Ensure GPIO23 is HIGH when base pump should be OFF
    digitalWrite(REL_ALKALI_PUMP, HIGH);
  } else if (ph < profile.phMin) {
    phControl->setAcid(false);
    // Only activate base pump if pH is really low AND cooldown passed
    if (phControl->canDose()) {
      phControl->setBase(true);
    } else {
      phControl->setBase(false);
      digitalWrite(REL_ALKALI_PUMP, HIGH);
    }
  } else {
    phControl->setAcid(false);
    phControl->setBase(false);
    // Extra safety: Force GPIO23 HIGH when pH is in range
    digitalWrite(REL_ALKALI_PUMP, HIGH);
  }
}

void AutoControl::update() {
  unsigned long now = millis();
  
  // Check emergency conditions first
  checkEmergency();
  
  // Update control timers
  phControl->update();
  fanControl->update();
  
  // Check temperature every 5 seconds
  if (now - lastTempCheck >= TEMP_CHECK_INTERVAL) {
    checkTemperature();
    lastTempCheck = now;
  }
  
  // Check pH every 1 minute
  if (now - lastPHCheck >= PH_CHECK_INTERVAL) {
    checkPH();
    lastPHCheck = now;
  }
}
