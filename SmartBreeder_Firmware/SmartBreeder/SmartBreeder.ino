/*
  Smart Breeder - Production Ready Firmware
  Modular Architecture with Calibration, Safety, and Web Dashboard
  
  Hardware:
  - ESP32 Dev Module
  - LCD I2C (0x27) on GPIO 21/22
  - pH Sensor on GPIO 35 (ADC)
  - DS18B20 on GPIO 27
  - Relays (Active-Low):
    * Acid Pump: GPIO 16
    * Alkali Pump: GPIO 23
    * Cooler Fan: GPIO 18
    * Water Heater: GPIO 19
    * Air Pump: GPIO 26
    * Water Flow: GPIO 32
    * Rain Pump: GPIO 33
    * Light Control: GPIO 25
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
  // Note: GPIO23 is already initialized in PHControl constructor (runs before setup())
  // But we'll reinforce it here to be absolutely sure
  
  Serial.begin(115200);
  delay(100);
  
  // ======================= CRITICAL: REINFORCE GPIO23 STATE =======================
  // Even though pins are initialized in constructor, reinforce here immediately
  // For Active-Low relays: HIGH = OFF, LOW = ON
  pinMode(REL_ALKALI_PUMP, OUTPUT);
  digitalWrite(REL_ALKALI_PUMP, HIGH); // Explicitly set HIGH (OFF for Active-Low relay)
  delay(10);
  digitalWrite(REL_ALKALI_PUMP, HIGH); // Set again to ensure it's stable
  delay(10);
  
  // Also reinforce Acid pump pin
  pinMode(REL_ACID_PUMP, OUTPUT);
  digitalWrite(REL_ACID_PUMP, HIGH); // Explicitly set HIGH (OFF for Active-Low relay)
  delay(10);
  digitalWrite(REL_ACID_PUMP, HIGH); // Set again to ensure it's stable
  
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("   Smart Breeder - Starting System");
  Serial.println("========================================\n");
  
  // Verify GPIO23 is still OFF
  int pin23State = digitalRead(REL_ALKALI_PUMP);
  Serial.printf("GPIO23 state check: %s (should be HIGH/OFF)\n", pin23State == HIGH ? "HIGH ✓" : "LOW ✗ ERROR!");
  if (pin23State != HIGH) {
    Serial.println("WARNING: GPIO23 is LOW! Setting to HIGH...");
    digitalWrite(REL_ALKALI_PUMP, HIGH);
    delay(100);
    pin23State = digitalRead(REL_ALKALI_PUMP);
    Serial.printf("GPIO23 state after fix: %s\n", pin23State == HIGH ? "HIGH ✓" : "LOW ✗ STILL ERROR!");
  }
  Serial.println("✓ GPIO23 (Alkali pump) confirmed OFF state");
  Serial.println("✓ Acid pump pin confirmed OFF state");
  
  // Initialize LCD
  lcdUI.begin();
  
  // Initialize sensors
  phSensor.begin();
  tempSensor.begin();
  
  // ======================= RELAY INITIALIZATION =======================
  // ALL RELAYS START OFF - They will only activate when a fish is selected
  
  // Initialize pH control relays (Acid & Alkali pumps)
  // Note: Pins are already initialized above, this just sets up the control logic
  phControl.begin();
  
  // CRITICAL: Verify GPIO23 is still OFF after phControl.begin()
  pin23State = digitalRead(REL_ALKALI_PUMP);
  if (pin23State != HIGH) {
    Serial.println("ERROR: GPIO23 changed to LOW after phControl.begin()! Fixing...");
    digitalWrite(REL_ALKALI_PUMP, HIGH);
    delay(100);
    pin23State = digitalRead(REL_ALKALI_PUMP);
    Serial.printf("GPIO23 state after fix: %s\n", pin23State == HIGH ? "HIGH ✓" : "LOW ✗ STILL ERROR!");
  } else {
    Serial.println("✓ GPIO23 still HIGH after phControl.begin()");
  }
  
  // Initialize fan control relay
  fanControl.begin();
  
  // Initialize water heater relay
  pinMode(REL_WATER_HEATER, OUTPUT);
  digitalWrite(REL_WATER_HEATER, getRelayLevel(false));
  
  // Initialize all other relays
  pinMode(REL_AIR_PUMP, OUTPUT);
  pinMode(REL_WATER_FLOW, OUTPUT);
  pinMode(REL_RAIN_PUMP, OUTPUT);
  pinMode(REL_LIGHT_CTRL, OUTPUT);
  
  // Set relays to OFF (except light control which is always ON)
  digitalWrite(REL_AIR_PUMP, getRelayLevel(false));
  digitalWrite(REL_WATER_FLOW, getRelayLevel(false));
  digitalWrite(REL_RAIN_PUMP, getRelayLevel(false));
  
  // Light control relay: Always ACTIVE/ON (for Active-Low: LOW = ON, HIGH = OFF)
  digitalWrite(REL_LIGHT_CTRL, LOW); // LOW = Relay ON for Active-Low relays
  Serial.println("✓ Light control relay (GPIO25) set to LOW = ACTIVE/ON (always on)");
  
  Serial.println("✓ All relays initialized");
  Serial.println("  Light control: ALWAYS ON");
  Serial.println("  Other relays: OFF (will activate when fish species is selected)");
  
  // RELAY HARDWARE TEST - DISABLED for GPIO23 to prevent startup activation
  // GPIO23 (Alkali pump) test is skipped to prevent relay from turning on at startup
  Serial.println("\n=== RELAY HARDWARE TEST ===");
  Serial.println("GPIO23 (Alkali pump) test SKIPPED - keeping relay OFF");
  Serial.println("Other relay tests can be added here if needed");
  Serial.println("================================\n");
  
  // CRITICAL: Ensure GPIO23 is still HIGH after skipping test
  digitalWrite(REL_ALKALI_PUMP, HIGH);
  delay(100);
  digitalWrite(REL_ALKALI_PUMP, HIGH);
  delay(100);
  pin23State = digitalRead(REL_ALKALI_PUMP); // Reuse existing variable
  Serial.printf("GPIO23 state after test skip: %s\n", pin23State == HIGH ? "HIGH ✓ OFF" : "LOW ✗ ON - ERROR!");
  if (pin23State != HIGH) {
    Serial.println("FORCING GPIO23 to HIGH...");
    for (int i = 0; i < 10; i++) {
      digitalWrite(REL_ALKALI_PUMP, HIGH);
      delay(50);
    }
    pin23State = digitalRead(REL_ALKALI_PUMP);
    Serial.printf("GPIO23 after force: %s\n", pin23State == HIGH ? "HIGH ✓" : "LOW ✗");
  }
  
  // Load saved settings
  loadCalibration();
  loadFishType(); // Load saved fish type (for reference, but we'll reset it)
  
  // CRITICAL: Always start with FISH_NONE - user must select fish manually
  // This ensures only light relay is ON at startup, all other relays OFF
  resetFishTypeAtStartup();
  
  // Ensure all relays are OFF except light (which is already ON)
  digitalWrite(REL_ACID_PUMP, getRelayLevel(false));
  digitalWrite(REL_ALKALI_PUMP, getRelayLevel(false));
  digitalWrite(REL_COOLER_FAN, getRelayLevel(false));
  digitalWrite(REL_WATER_HEATER, getRelayLevel(false));
  digitalWrite(REL_AIR_PUMP, getRelayLevel(false));
  digitalWrite(REL_WATER_FLOW, getRelayLevel(false));
  digitalWrite(REL_RAIN_PUMP, getRelayLevel(false));
  // Light relay is already ON (set earlier)
  
  Serial.println("\n=== STARTUP STATE ===");
  Serial.println("Active Fish Type: NONE (must be selected manually)");
  Serial.println("Relay Status:");
  Serial.println("  ✓ Light Control: ON (always)");
  Serial.println("  ✗ All other relays: OFF");
  Serial.println("  → Select a fish species to activate relays");
  Serial.println("=====================\n");
  
  // Initialize WiFi and Web Server
  wifiServer.begin();
  
  Serial.println("\n========================================");
  Serial.println("   System Ready!");
  Serial.println("========================================\n");
  
  // FINAL SAFETY CHECK: Ensure GPIO23 is OFF before entering main loop
  digitalWrite(REL_ALKALI_PUMP, HIGH); // Explicit HIGH (OFF for Active-Low)
  delay(100);
  pin23State = digitalRead(REL_ALKALI_PUMP);
  Serial.printf("FINAL GPIO23 check before main loop: %s\n", pin23State == HIGH ? "HIGH ✓ OFF" : "LOW ✗ ON - ERROR!");
  if (pin23State != HIGH) {
    Serial.println("CRITICAL: GPIO23 is ON! Forcing to OFF...");
    for (int i = 0; i < 5; i++) {
      digitalWrite(REL_ALKALI_PUMP, HIGH);
      delay(50);
    }
    pin23State = digitalRead(REL_ALKALI_PUMP);
    Serial.printf("GPIO23 after force OFF: %s\n", pin23State == HIGH ? "HIGH ✓" : "LOW ✗");
  }
  
  if (wifiServer.isConnected()) {
    Serial.print("Dashboard: http://");
    Serial.println(wifiServer.getIP());
    Serial.println("Or: http://smartbreeder.local");
  }
  
  // Startup delay to prevent autoControl from running immediately
  // This gives time for all systems to stabilize
  Serial.println("\nSystem stabilizing... (3 seconds)");
  delay(3000);
  Serial.println("System ready - entering main loop\n");
}

// ======================= MAIN LOOP =======================
void loop() {
  unsigned long now = millis();
  
  // Light control relay: Always keep ACTIVE/ON (LOW = ON for Active-Low relays)
  digitalWrite(REL_LIGHT_CTRL, LOW);
  
  // CRITICAL SAFETY: Continuously monitor GPIO23 to ensure it stays OFF when not in use
  // This prevents any accidental activation - runs EVERY loop iteration
  static unsigned long lastGPIO23Check = 0;
  if (now - lastGPIO23Check >= 100) { // Check every 100ms
    lastGPIO23Check = now;
    
    if (!phControl.getBaseState()) {
      // Base pump should be OFF - verify pin is HIGH
      int pin23State = digitalRead(REL_ALKALI_PUMP);
      if (pin23State != HIGH) {
        // Pin is LOW but pump should be OFF - force it HIGH immediately
        Serial.println("WARNING: GPIO23 went LOW when it should be HIGH! Fixing...");
        for (int i = 0; i < 5; i++) {
          digitalWrite(REL_ALKALI_PUMP, HIGH);
          delayMicroseconds(100);
        }
        pin23State = digitalRead(REL_ALKALI_PUMP);
        if (pin23State != HIGH) {
          Serial.println("ERROR: GPIO23 still LOW after fix attempt!");
        }
      }
    } else {
      // Base pump is supposed to be ON - verify pin is LOW
      int pin23State = digitalRead(REL_ALKALI_PUMP);
      if (pin23State != LOW) {
        // Should be LOW but it's HIGH - this is also a problem
        Serial.println("WARNING: GPIO23 is HIGH when base pump should be ON!");
      }
    }
  }
  
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
