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
  unsigned long pumpStartTime;
  unsigned long cooldownStartTime;
  bool inCooldown;
  
  const unsigned long PUMP_DURATION = 3000;      // 3 seconds
  const unsigned long COOLDOWN_DURATION = 60000; // 1 minute (60 seconds)
  
public:
  PHControl(int acidPin, int basePin);
  void begin();
  void setAcid(bool on);
  void setBase(bool on);
  void stopAll();
  bool getAcidState() { return acidState; }
  bool getBaseState() { return baseState; }
  bool canDose() { return !inCooldown; }
  unsigned long getCooldownRemaining();
  void update();
};

#endif
