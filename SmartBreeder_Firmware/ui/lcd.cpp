#include "lcd.h"

LCDUI::LCDUI() : currentPage(PAGE_READINGS), lastPageChange(0), 
                 lastUpdate(0), startupComplete(false), startupStart(0) {
  lcd = new LiquidCrystal_I2C(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
}

void LCDUI::begin() {
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd->init();
  lcd->backlight();
  lcd->clear();
  startupStart = millis();
  Serial.println("LCD initialized");
}

void LCDUI::showStartupAnimation() {
  unsigned long elapsed = millis() - startupStart;
  
  if (elapsed < 1000) {
    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print("Smart Breeder");
    lcd->setCursor(0, 1);
    lcd->print("Starting...");
  } else if (elapsed < 2000) {
    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print("Initializing");
    lcd->setCursor(0, 1);
    lcd->print("Sensors...");
  } else if (elapsed < 3000) {
    lcd->clear();
    lcd->setCursor(0, 0);
    lcd->print("Connecting");
    lcd->setCursor(0, 1);
    lcd->print("WiFi...");
  } else {
    startupComplete = true;
  }
}

void LCDUI::showReadings(float ph, float temp, String phState, String tempState) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("pH:");
  lcd->print(ph, 2);
  lcd->print(" ");
  lcd->print(phState);
  
  lcd->setCursor(0, 1);
  lcd->print("T:");
  lcd->print(temp, 1);
  lcd->print("C ");
  lcd->print(tempState);
}

void LCDUI::showFishType() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("Fish Profile:");
  lcd->setCursor(0, 1);
  lcd->print(FISH_PROFILES[activeFishType].name);
}

void LCDUI::showStatus(float ph, float temp, bool fan, bool acid, bool base) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("F:");
  lcd->print(fan ? "ON " : "OFF");
  lcd->print(" A:");
  lcd->print(acid ? "ON" : "OFF");
  
  lcd->setCursor(0, 1);
  lcd->print("B:");
  lcd->print(base ? "ON " : "OFF");
  lcd->print(" pH:");
  lcd->print(ph, 1);
}

void LCDUI::showDosingTimer(unsigned long remaining) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("Next Dosing:");
  
  if (remaining == 0) {
    lcd->setCursor(0, 1);
    lcd->print("Ready");
  } else {
    unsigned long minutes = remaining / 60000;
    unsigned long seconds = (remaining % 60000) / 1000;
    lcd->setCursor(0, 1);
    lcd->print(minutes);
    lcd->print("m ");
    lcd->print(seconds);
    lcd->print("s");
  }
}

void LCDUI::showWiFiIP(String ip) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("WiFi Connected");
  lcd->setCursor(0, 1);
  lcd->print(ip);
}

void LCDUI::update(float ph, float temp, String phState, String tempState,
                   bool fan, bool acid, bool base, unsigned long cooldownRemaining, String wifiIP) {
  unsigned long now = millis();
  
  // Show startup animation
  if (!startupComplete) {
    showStartupAnimation();
    return;
  }
  
  // Auto-cycle pages every 5 seconds
  if (now - lastPageChange >= LCD_PAGE_DURATION) {
    nextPage();
    lastPageChange = now;
  }
  
  // Update display based on current page
  if (now - lastUpdate >= LCD_UPDATE_INTERVAL) {
    switch (currentPage) {
      case PAGE_READINGS:
        showReadings(ph, temp, phState, tempState);
        break;
      case PAGE_FISH_TYPE:
        showFishType();
        break;
      case PAGE_STATUS:
        showStatus(ph, temp, fan, acid, base);
        break;
      case PAGE_DOSING_TIMER:
        showDosingTimer(cooldownRemaining);
        break;
      case PAGE_WIFI_IP:
        showWiFiIP(wifiIP);
        break;
    }
    lastUpdate = now;
  }
}

void LCDUI::nextPage() {
  currentPage = (LCDPage)((currentPage + 1) % PAGE_COUNT);
}

void LCDUI::setPage(LCDPage page) {
  if (page < PAGE_COUNT) {
    currentPage = page;
    lastPageChange = millis();
  }
}

void LCDUI::showMessage(String line1, String line2, unsigned long duration) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print(line1);
  lcd->setCursor(0, 1);
  lcd->print(line2);
  delay(duration);
}

