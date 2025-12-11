#ifndef PH_CONTROL_H
#define PH_CONTROL_H

#include <Arduino.h>
#include "config/config.h"

class PHControl {
private:
  int acidPin;
  int basePin;
  bool acidState;
  bool baseState;
  
  bool manualOverride;
  unsigned long overrideTime;
  unsigned long pumpStartTime;
  unsigned long cooldownStartTime;
  bool inCooldown;
  
  const unsigned long MANUAL_OVERRIDE_TIMEOUT = 30000; // 30 seconds
  const unsigned long PUMP_MAX_DURATION = 3000; // 3 seconds
  const unsigned long COOLDOWN_DURATION = 5UL * 60UL * 1000UL; // 5 minutes
  
public:
  PHControl(int acidPin, int basePin);
  void begin();
  void setAcid(bool on, bool manual = false);
  void setBase(bool on, bool manual = false);
  void stopAll();
  bool getAcidState() { return acidState; }
  bool getBaseState() { return baseState; }
  bool isManual() { return manualOverride; }
  bool canDose() { return !inCooldown; }
  unsigned long getCooldownRemaining();
  void update(); // Check timers and safety
  void emergencyStop();
};

#endif

