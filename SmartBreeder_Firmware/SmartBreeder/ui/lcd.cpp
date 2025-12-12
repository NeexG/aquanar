#include "lcd.h"
#include "config/config.h"

LCDUI::LCDUI() : currentPage(PAGE_READINGS), lastPageChange(0), 
                 lastUpdate(0), startupComplete(false), startupStart(0),
                 lastProjectInfoShow(0), showingProjectInfo(false),
                 wifiConnected(false), wifiStatusShown(false), wifiConnectedTime(0) {
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

void LCDUI::showWiFiConnecting() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("WiFi Connecting");
  lcd->setCursor(0, 1);
  lcd->print("Please wait...");
}

void LCDUI::showWiFiConnected() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("WiFi Connected");
  lcd->setCursor(0, 1);
  lcd->print("Success!");
}

void LCDUI::showWiFiIP(String ip) {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("Connected");
  lcd->setCursor(0, 1);
  // Print full IP address - LCD has 16 chars, IP is max 15 chars
  lcd->print(ip);
}

void LCDUI::update(float ph, float temp, String phState, String tempState,
                   bool fan, bool acid, bool base, unsigned long cooldownRemaining, 
                   bool wifiConnected, String wifiIP) {
  unsigned long now = millis();
  
  // Show startup animation
  if (!startupComplete) {
    showStartupAnimation();
    return;
  }
  
  // Handle WiFi connection status display sequence
  if (!wifiConnected && !wifiStatusShown) {
    // Show "WiFi Connecting" while not connected
    if (now - lastUpdate >= LCD_UPDATE_INTERVAL) {
      showWiFiConnecting();
      lastUpdate = now;
    }
    return;
  }
  
  // WiFi just connected - show "WiFi Connected" then IP
  if (wifiConnected && !this->wifiConnected) {
    this->wifiConnected = true;
    wifiConnectedTime = now;
    wifiStatusShown = false;
    currentPage = PAGE_READINGS;
    lastPageChange = now;
  }
  
  // Show WiFi connection sequence: Connected → IP → normal pages
  if (wifiConnected && !wifiStatusShown) {
    unsigned long elapsed = now - wifiConnectedTime;
    if (elapsed < 2000) {
      // Show "WiFi Connected" for 2 seconds
      if (now - lastUpdate >= LCD_UPDATE_INTERVAL) {
        showWiFiConnected();
        lastUpdate = now;
      }
      return;
    } else if (elapsed < 5000) {
      // Show IP address for 3 seconds
      if (now - lastUpdate >= LCD_UPDATE_INTERVAL) {
        showWiFiIP(wifiIP);
        lastUpdate = now;
      }
      return;
    } else {
      // Done showing WiFi status, now show project info immediately
      wifiStatusShown = true;
      showingProjectInfo = true;
      lastProjectInfoShow = now; // Start 3-minute timer from now
      currentPage = PAGE_PROJECT_NAME; // Start with project name
      lastPageChange = now;
    }
  }
  
  // Check if 3 minutes (180000ms) have passed to show project info again
  const unsigned long PROJECT_INFO_INTERVAL = 3UL * 60UL * 1000UL; // 3 minutes
  const unsigned long PROJECT_INFO_DURATION = 5UL * 1000UL; // 5 seconds per project page
  
  // Only check for next project info cycle if WiFi status is already shown
  if (wifiStatusShown && !showingProjectInfo && now - lastProjectInfoShow >= PROJECT_INFO_INTERVAL) {
    // Time to show project info again
    showingProjectInfo = true;
    lastProjectInfoShow = now;
    // Start with project name page
    currentPage = PAGE_PROJECT_NAME;
    lastPageChange = now;
  }
  
  // If showing project info, cycle through project pages
  if (showingProjectInfo) {
    if (now - lastPageChange >= PROJECT_INFO_DURATION) {
      // Move to next project info page
      switch (currentPage) {
        case PAGE_PROJECT_NAME:
          currentPage = PAGE_APP_NAME;
          break;
        case PAGE_APP_NAME:
          currentPage = PAGE_TEAM_NAME;
          break;
        case PAGE_TEAM_NAME:
          currentPage = PAGE_TEAM_LEADER;
          break;
        case PAGE_TEAM_LEADER:
          currentPage = PAGE_SOFTWARE_DEV;
          break;
        case PAGE_SOFTWARE_DEV:
          currentPage = PAGE_HARDWARE_DEV;
          break;
        case PAGE_HARDWARE_DEV:
          // Done showing project info, return to normal pages
          showingProjectInfo = false;
          currentPage = PAGE_READINGS;
          break;
        default:
          showingProjectInfo = false;
          currentPage = PAGE_READINGS;
          break;
      }
      lastPageChange = now;
    }
  } else {
    // Normal operation: Always show pH and Temperature (no page cycling)
    // Force to READINGS page if not showing project info
    if (currentPage != PAGE_READINGS && currentPage < PAGE_PROJECT_NAME) {
      currentPage = PAGE_READINGS;
    }
  }
  
  // Update display based on current page
  if (now - lastUpdate >= LCD_UPDATE_INTERVAL) {
    switch (currentPage) {
      case PAGE_READINGS:
        // Always show pH and Temperature in normal operation
        showReadings(ph, temp, phState, tempState);
        break;
      case PAGE_PROJECT_NAME:
        showProjectName();
        break;
      case PAGE_APP_NAME:
        showAppName();
        break;
      case PAGE_TEAM_NAME:
        showTeamName();
        break;
      case PAGE_TEAM_LEADER:
        showTeamLeader();
        break;
      case PAGE_SOFTWARE_DEV:
        showSoftwareDev();
        break;
      case PAGE_HARDWARE_DEV:
        showHardwareDev();
        break;
      default:
        // Fallback to readings if unknown page
        showReadings(ph, temp, phState, tempState);
        break;
    }
    lastUpdate = now;
  }
}

void LCDUI::nextPage() {
  // Normal operation always shows READINGS page (pH and Temperature)
  // Only project info pages cycle during their display period
  if (!showingProjectInfo) {
    currentPage = PAGE_READINGS;
  } else {
    currentPage = (LCDPage)((currentPage + 1) % PAGE_COUNT);
  }
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

void LCDUI::showProjectName() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("Project:");
  lcd->setCursor(0, 1);
  lcd->print("Fish Breeding");
}

void LCDUI::showAppName() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("App Name:");
  lcd->setCursor(0, 1);
  lcd->print("Smart Breeder");
}

void LCDUI::showTeamName() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("Team:");
  lcd->setCursor(0, 1);
  lcd->print("Team AquaNAR");
}

void LCDUI::showTeamLeader() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("Team Leader:");
  lcd->setCursor(0, 1);
  lcd->print("Md Naim Islam");
}

void LCDUI::showSoftwareDev() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("Software Dev:");
  lcd->setCursor(0, 1);
  lcd->print("Md Abu Hosain");
}

void LCDUI::showHardwareDev() {
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print("Hardware Dev:");
  lcd->setCursor(0, 1);
  lcd->print("Rakibul Hasan");
}

