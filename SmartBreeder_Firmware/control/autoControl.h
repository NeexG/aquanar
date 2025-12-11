#ifndef AUTO_CONTROL_H
#define AUTO_CONTROL_H

#include <Arduino.h>
#include "../config/config.h"

// Forward declarations
class PHSensor;
class TempSensor;
class FanControl;
class PHControl;

class AutoControl {
private:
  PHSensor* phSensor;
  TempSensor* tempSensor;
  FanControl* fanControl;
  PHControl* phControl;
  
  unsigned long lastTempCheck;
  unsigned long lastPHCheck;
  const unsigned long TEMP_CHECK_INTERVAL = 5000; // 5 seconds
  const unsigned long PH_CHECK_INTERVAL = 10000;  // 10 seconds
  
  void checkEmergency();
  void checkTemperature();
  void checkPH();
  
public:
  AutoControl(PHSensor* ph, TempSensor* temp, FanControl* fan, PHControl* phCtrl);
  void update();
};

#endif

