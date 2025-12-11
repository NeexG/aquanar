#include "fan.h"
#include "config/config.h"

FanControl::FanControl(int pin) : pin(pin), state(false), manualOverride(false),
                                    lastToggleTime(0), overrideTime(0) {}

void FanControl::begin() {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, getRelayLevel(false));
  Serial.println("Fan control initialized");
}

void FanControl::set(bool on, bool manual) {
  unsigned long now = millis();
  
  // Prevent rapid toggling
  if (now - lastToggleTime < FAN_MIN_TOGGLE_INTERVAL && state != on) {
    Serial.println("Fan toggle too rapid - ignored");
    return;
  }
  
  state = on;
  digitalWrite(pin, getRelayLevel(on));
  lastToggleTime = now;
  
  if (manual) {
    manualOverride = true;
    overrideTime = now;
    Serial.printf("Fan manually set to %s\n", on ? "ON" : "OFF");
  } else {
    Serial.printf("Fan auto set to %s\n", on ? "ON" : "OFF");
  }
}

void FanControl::update() {
  if (manualOverride) {
    unsigned long elapsed = millis() - overrideTime;
    if (elapsed >= MANUAL_OVERRIDE_TIMEOUT) {
      manualOverride = false;
      Serial.println("Fan manual override expired");
    }
  }
}

void FanControl::emergencyOn() {
  state = true;
  manualOverride = false; // Emergency overrides manual
  digitalWrite(pin, getRelayLevel(true));
  Serial.println("FAN EMERGENCY ON");
}

