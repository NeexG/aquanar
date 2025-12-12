/*
  Smart Breeder - Production Ready Firmware
  Modular Architecture with Calibration, Safety, and Web Dashboard
  
  Hardware:
  - ESP32 Dev Module
  - LCD I2C (0x27) on GPIO 21/22
  - pH Sensor on GPIO 35 (ADC)
  - DS18B20 on GPIO 27
  - Relays (Active-Low) on GPIO 18, 23, 19
*/

// Include headers
#include "config/config.h"
#include "sensors/ph.h"
#include "sensors/temp.h"
#include "control/fan.h"
#include "control/phControl.h"
#include "control/autoControl.h"
#include "ui/lcd.h"
#include "wifi/server.h"

// Include implementations (Arduino IDE needs this)
#include "config/config.cpp"
#include "sensors/ph.cpp"
#include "sensors/temp.cpp"
#include "control/fan.cpp"
#include "control/phControl.cpp"
#include "control/autoControl.cpp"
#include "ui/lcd.cpp"
#include "wifi/server.cpp"

// ======================= GLOBAL OBJECTS =======================
PHSensor phSensor(PH_PIN);
TempSensor tempSensor(TEMP_PIN);
FanControl fanControl(REL_FAN);
PHControl phControl(REL_ACID_PUMP, REL_BASE_PUMP);
AutoControl autoControl(&phSensor, &tempSensor, &fanControl, &phControl);
LCDUI lcdUI;
SmartBreederServer wifiServer(&phSensor, &tempSensor, &fanControl, &phControl);

// ======================= STATE VARIABLES =======================
unsigned long lastSensorRead = 0;
float currentPH = 7.0;
float currentTemp = 25.0;
String phState = "Neutral";
String tempState = "Normal";

// ======================= HELPER FUNCTIONS =======================
String getPHState(float ph) {
  if (activeFishType == FISH_NONE) return "Neutral";
  const FishProfile& profile = FISH_PROFILES[activeFishType];
  if (ph < profile.phMin) return "Acidic";
  if (ph > profile.phMax) return "Alkaline";
  return "Neutral";
}

String getTempState(float temp) {
  if (activeFishType == FISH_NONE) return "Normal";
  const FishProfile& profile = FISH_PROFILES[activeFishType];
  if (temp < profile.tempMin) return "Cold";
  if (temp > profile.tempMax) return "Hot";
  return "Normal";
}

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("   Smart Breeder - Starting System");
  Serial.println("========================================\n");
  
  // Initialize LCD
  lcdUI.begin();
  
  // Initialize sensors
  phSensor.begin();
  tempSensor.begin();
  
  // Initialize controls
  fanControl.begin();
  phControl.begin();
  
  // Load saved settings
  loadCalibration();
  loadFishType();
  
  Serial.printf("Active Fish Type: %s\n", FISH_PROFILES[activeFishType].name.c_str());
  Serial.printf("pH Range: %.1f - %.1f\n", 
                FISH_PROFILES[activeFishType].phMin,
                FISH_PROFILES[activeFishType].phMax);
  Serial.printf("Temp Range: %.1f - %.1f°C\n",
                FISH_PROFILES[activeFishType].tempMin,
                FISH_PROFILES[activeFishType].tempMax);
  
  // Initialize WiFi and Web Server
  wifiServer.begin();
  
  Serial.println("\n========================================");
  Serial.println("   System Ready!");
  Serial.println("========================================\n");
  
  if (wifiServer.isConnected()) {
    Serial.print("Dashboard: http://");
    Serial.println(wifiServer.getIP());
    Serial.println("Or: http://smartbreeder.local");
  }
}

// ======================= MAIN LOOP =======================
void loop() {
  unsigned long now = millis();
  
  // Handle web server
  wifiServer.update();
  
  // Read sensors
  if (now - lastSensorRead >= PH_READ_INTERVAL) {
    currentPH = phSensor.read();
    currentTemp = tempSensor.read();
    phState = getPHState(currentPH);
    tempState = getTempState(currentTemp);
    lastSensorRead = now;
  }
  
  // Update auto control
  autoControl.update();
  
  // Update LCD display
  lcdUI.update(
    currentPH, currentTemp, phState, tempState,
    fanControl.getState(),
    phControl.getAcidState(),
    phControl.getBaseState(),
    phControl.getCooldownRemaining(),
    wifiServer.isConnected(),
    wifiServer.getIP()
  );
  
  // Small delay to prevent watchdog issues
  delay(10);
}
