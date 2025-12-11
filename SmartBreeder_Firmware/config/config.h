#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

// ======================= PIN DEFINITIONS =======================
#define PH_PIN 35           // ADC pin for pH sensor
#define TEMP_PIN 27         // DS18B20 data pin
#define SDA_PIN 21          // I2C SDA for LCD
#define SCL_PIN 22          // I2C SCL for LCD

// Relay pins (Active-Low)
#define REL_FAN 18          // Cooler fan relay
#define REL_ACID_PUMP 23    // Acid pump relay
#define REL_BASE_PUMP 19    // Base pump relay

// LCD Configuration
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// ======================= WIFI CONFIG =======================
const char* WIFI_SSID = "Abu Hosain";
const char* WIFI_PASS = "01731373179";

// ======================= RELAY CONFIG =======================
const bool RELAY_ACTIVE_HIGH = false; // Active-Low relays

// ======================= TIMING CONSTANTS =======================
const unsigned long TEMP_READ_INTERVAL = 2000;    // 2 seconds
const unsigned long PH_READ_INTERVAL = 500;        // 500ms
const unsigned long LCD_UPDATE_INTERVAL = 500;     // 500ms
const unsigned long LCD_PAGE_DURATION = 5000;     // 5 seconds per page

// Safety timings
const unsigned long PUMP_MAX_DURATION = 3000;     // 3 seconds max pump ON
const unsigned long PUMP_COOLDOWN = 5UL * 60UL * 1000UL; // 5 minutes cooldown
const unsigned long FAN_MIN_TOGGLE_INTERVAL = 10000; // 10 seconds between fan toggles

// ======================= SENSOR CONFIG =======================
const int PH_MEDIAN_SAMPLES = 15;
const float PH_MIN_SAFE = 5.5;
const float PH_MAX_SAFE = 9.0;
const float TEMP_MAX_SAFE = 40.0; // Emergency fan ON above this

// ======================= CALIBRATION STORAGE =======================
#define PREF_NAMESPACE "smartbreeder"
#define PREF_PH7_KEY "ph7_voltage"
#define PREF_PH4_KEY "ph4_voltage"
#define PREF_TEMP_OFFSET_KEY "temp_offset"
#define PREF_FISH_TYPE_KEY "fish_type"

// ======================= FISH PROFILES =======================
enum FishType {
  FISH_NONE = 0,
  FISH_GOLD,
  FISH_COMET,
  FISH_ROHU
};

struct FishProfile {
  String name;
  float phMin;
  float phMax;
  float tempMin;
  float tempMax;
};

const FishProfile FISH_PROFILES[] = {
  {"None", 6.5, 7.5, 20.0, 25.0},
  {"Gold Fish", 6.8, 7.5, 24.0, 28.0},
  {"Comet", 6.5, 7.2, 23.0, 27.0},
  {"Rohu", 6.6, 8.0, 24.0, 32.0}
};

// ======================= GLOBAL STATE =======================
extern Preferences preferences;
extern FishType activeFishType;

// Helper functions
int getRelayLevel(bool logicalOn);
void loadCalibration();
void saveCalibration();
void loadFishType();
void saveFishType();

#endif

