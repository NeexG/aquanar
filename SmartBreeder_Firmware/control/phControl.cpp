#include "phControl.h"

PHControl::PHControl(int acidPin, int basePin) : 
  acidPin(acidPin), basePin(basePin), acidState(false), baseState(false),
  manualOverride(false), overrideTime(0), pumpStartTime(0), 
  cooldownStartTime(0), inCooldown(false) {}

void PHControl::begin() {
  pinMode(acidPin, OUTPUT);
  pinMode(basePin, OUTPUT);
  digitalWrite(acidPin, getRelayLevel(false));
  digitalWrite(basePin, getRelayLevel(false));
  Serial.println("pH Control initialized");
}

void PHControl::setAcid(bool on, bool manual) {
  if (inCooldown && on) {
    Serial.println("Pump blocked - in cooldown period");
    return;
  }
  
  acidState = on;
  digitalWrite(acidPin, getRelayLevel(on));
  
  if (on) {
    pumpStartTime = millis();
    Serial.println("Acid pump ON");
  } else {
    Serial.println("Acid pump OFF");
    if (!baseState) {
      // Both pumps off - start cooldown
      cooldownStartTime = millis();
      inCooldown = true;
    }
  }
  
  if (manual) {
    manualOverride = true;
    overrideTime = millis();
  }
}

void PHControl::setBase(bool on, bool manual) {
  if (inCooldown && on) {
    Serial.println("Pump blocked - in cooldown period");
    return;
  }
  
  baseState = on;
  digitalWrite(basePin, getRelayLevel(on));
  
  if (on) {
    pumpStartTime = millis();
    Serial.println("Base pump ON");
  } else {
    Serial.println("Base pump OFF");
    if (!acidState) {
      // Both pumps off - start cooldown
      cooldownStartTime = millis();
      inCooldown = true;
    }
  }
  
  if (manual) {
    manualOverride = true;
    overrideTime = millis();
  }
}

void PHControl::stopAll() {
  acidState = false;
  baseState = false;
  digitalWrite(acidPin, getRelayLevel(false));
  digitalWrite(basePin, getRelayLevel(false));
  cooldownStartTime = millis();
  inCooldown = true;
  Serial.println("All pumps stopped");
}

void PHControl::update() {
  unsigned long now = millis();
  
  // Check pump max duration
  if ((acidState || baseState) && pumpStartTime > 0) {
    unsigned long elapsed = now - pumpStartTime;
    if (elapsed >= PUMP_MAX_DURATION) {
      Serial.println("Pump max duration reached - stopping");
      stopAll();
    }
  }
  
  // Check cooldown
  if (inCooldown) {
    unsigned long elapsed = now - cooldownStartTime;
    if (elapsed >= COOLDOWN_DURATION) {
      inCooldown = false;
      Serial.println("Cooldown period ended");
    }
  }
  
  // Check manual override timeout
  if (manualOverride) {
    unsigned long elapsed = now - overrideTime;
    if (elapsed >= MANUAL_OVERRIDE_TIMEOUT) {
      manualOverride = false;
      Serial.println("pH control manual override expired");
    }
  }
}

unsigned long PHControl::getCooldownRemaining() {
  if (!inCooldown) return 0;
  unsigned long elapsed = millis() - cooldownStartTime;
  if (elapsed >= COOLDOWN_DURATION) return 0;
  return COOLDOWN_DURATION - elapsed;
}

void PHControl::emergencyStop() {
  stopAll();
  manualOverride = false;
  Serial.println("EMERGENCY: All pumps stopped");
}

