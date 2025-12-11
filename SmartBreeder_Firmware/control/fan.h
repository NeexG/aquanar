#ifndef FAN_CONTROL_H
#define FAN_CONTROL_H

#include <Arduino.h>
#include "../config/config.h"

class FanControl {
private:
  int pin;
  bool state;
  bool manualOverride;
  unsigned long lastToggleTime;
  unsigned long overrideTime;
  const unsigned long MANUAL_OVERRIDE_TIMEOUT = 30000; // 30 seconds
  
public:
  FanControl(int pin);
  void begin();
  void set(bool on, bool manual = false);
  bool getState() { return state; }
  bool isManual() { return manualOverride; }
  void update(); // Check for expired overrides
  void emergencyOn(); // Force ON for emergency
};

#endif

