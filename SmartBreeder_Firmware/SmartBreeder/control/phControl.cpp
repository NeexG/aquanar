#include "phControl.h"
#include "config/config.h"

PHControl::PHControl(int acidPin, int basePin) : 
  acidPin(acidPin), basePin(basePin), 
  acidState(false), baseState(false),
  pumpStartTime(0), cooldownStartTime(0), inCooldown(true) {
  // CRITICAL: Initialize pins IMMEDIATELY in constructor (runs before setup())
  // This prevents relay activation during boot phase
  // For Active-Low relays: HIGH = OFF, LOW = ON
  pinMode(basePin, OUTPUT);
  digitalWrite(basePin, HIGH); // Set to OFF immediately
  pinMode(acidPin, OUTPUT);
  digitalWrite(acidPin, HIGH); // Set to OFF immediately
}

void PHControl::begin() {
  // Note: Pins may already be initialized in setup(), but we ensure they're set correctly here
  pinMode(acidPin, OUTPUT);
  pinMode(basePin, OUTPUT);
  
  // CRITICAL: For Active-Low relays, explicitly set to HIGH (OFF) to prevent activation
  // Set multiple times to ensure stable state
  digitalWrite(acidPin, HIGH); // Explicit HIGH for Active-Low (OFF state)
  digitalWrite(basePin, HIGH);  // Explicit HIGH for Active-Low (OFF state)
  delay(10);
  digitalWrite(acidPin, HIGH); // Set again to ensure stability
  digitalWrite(basePin, HIGH);
  
  // Also use getRelayLevel for consistency
  digitalWrite(acidPin, getRelayLevel(false));
  digitalWrite(basePin, getRelayLevel(false));
  
  // Start with cooldown active - prevents immediate activation
  cooldownStartTime = millis();
  inCooldown = true;
}

void PHControl::setAcid(bool on) {
  // Block if in cooldown
  if (inCooldown && on) {
    return;
  }
  
  // Turn off base pump first
  if (on && baseState) {
    setBase(false);
  }
  
  acidState = on;
  digitalWrite(acidPin, getRelayLevel(on));
  
  if (on) {
    pumpStartTime = millis();
  }
}

void PHControl::setBase(bool on) {
  // Block if in cooldown
  if (inCooldown && on) {
    Serial.println("Base pump blocked - in cooldown period");
    return;
  }
  
  // CRITICAL SAFETY: If trying to turn OFF, explicitly set to HIGH (OFF for Active-Low)
  if (!on) {
    digitalWrite(basePin, HIGH); // Explicit HIGH to ensure OFF state
    delay(10);
  }
  
  // Turn off acid pump first
  if (on && acidState) {
    setAcid(false);
  }
  
  baseState = on;
  digitalWrite(basePin, getRelayLevel(on));
  
  // Double-check: If OFF, ensure it's really HIGH
  if (!on) {
    digitalWrite(basePin, HIGH);
  }
  
  if (on) {
    pumpStartTime = millis();
    Serial.println("Base pump activated");
  } else {
    Serial.println("Base pump deactivated");
  }
}

void PHControl::stopAll() {
  acidState = false;
  baseState = false;
  
  // CRITICAL: Explicitly set to HIGH (OFF for Active-Low relays)
  digitalWrite(acidPin, HIGH);
  digitalWrite(basePin, HIGH);
  delay(10);
  digitalWrite(acidPin, HIGH);
  digitalWrite(basePin, HIGH);
  
  // Also use getRelayLevel for consistency
  digitalWrite(acidPin, getRelayLevel(false));
  digitalWrite(basePin, getRelayLevel(false));
  
  // Start cooldown after pump stops
  if (pumpStartTime > 0) {
    cooldownStartTime = millis();
    inCooldown = true;
    pumpStartTime = 0;
  }
}

void PHControl::update() {
  unsigned long now = millis();
  
  // Stop pump after 3 seconds
  if ((acidState || baseState) && pumpStartTime > 0) {
    if (now - pumpStartTime >= PUMP_DURATION) {
      stopAll();
    }
  }
  
  // Check cooldown - wait 1 minute
  if (inCooldown && cooldownStartTime > 0) {
    if (now - cooldownStartTime >= COOLDOWN_DURATION) {
      inCooldown = false;
      cooldownStartTime = 0;
    }
  }
}

unsigned long PHControl::getCooldownRemaining() {
  if (!inCooldown || cooldownStartTime == 0) return 0;
  
  unsigned long now = millis();
  if (now < cooldownStartTime) return COOLDOWN_DURATION; // Overflow
  
  unsigned long elapsed = now - cooldownStartTime;
  if (elapsed >= COOLDOWN_DURATION) return 0;
  
  return COOLDOWN_DURATION - elapsed;
}
