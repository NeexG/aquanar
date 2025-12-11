#ifndef LCD_UI_H
#define LCD_UI_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "../config/config.h"

enum LCDPage {
  PAGE_READINGS = 0,
  PAGE_FISH_TYPE,
  PAGE_STATUS,
  PAGE_DOSING_TIMER,
  PAGE_WIFI_IP,
  PAGE_COUNT
};

class LCDUI {
private:
  LiquidCrystal_I2C* lcd;
  LCDPage currentPage;
  unsigned long lastPageChange;
  unsigned long lastUpdate;
  bool startupComplete;
  unsigned long startupStart;
  
  void showStartupAnimation();
  void showReadings(float ph, float temp, String phState, String tempState);
  void showFishType();
  void showStatus(float ph, float temp, bool fan, bool acid, bool base);
  void showDosingTimer(unsigned long remaining);
  void showWiFiIP(String ip);
  
public:
  LCDUI();
  void begin();
  void update(float ph, float temp, String phState, String tempState, 
              bool fan, bool acid, bool base, unsigned long cooldownRemaining, String wifiIP);
  void nextPage();
  void setPage(LCDPage page);
  void showMessage(String line1, String line2, unsigned long duration = 2000);
};

#endif

