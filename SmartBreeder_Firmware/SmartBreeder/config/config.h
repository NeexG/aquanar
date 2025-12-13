#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

// ======================= PIN DEFINITIONS =======================
#define PH_PIN 35           // ADC pin for pH sensor
#define TEMP_PIN 27         // DS18B20 data pin
#define SDA_PIN 21          // I2C SDA for LCD
#define SCL_PIN 22          // I2C SCL for LCD

// Relay pins (Active-Low) - Updated pin mapping
const uint8_t REL_ACID_PUMP   = 16; // Acid pump relay - G16
const uint8_t REL_ALKALI_PUMP = 23; // Alkali pump relay - G23
const uint8_t REL_COOLER_FAN  = 18; // Cooler fan relay - G18
const uint8_t REL_WATER_HEATER= 19; // Water heater relay - G19
const uint8_t REL_AIR_PUMP    = 26; // Air pump relay - G26
const uint8_t REL_WATER_FLOW  = 32; // Water flow pump relay - G32
const uint8_t REL_RAIN_PUMP   = 33; // Rain pump relay - G33
const uint8_t REL_LIGHT_CTRL  = 25; // Light control relay - G25

// Legacy aliases for backward compatibility
#define REL_FAN REL_COOLER_FAN
#define REL_BASE_PUMP REL_ALKALI_PUMP

// LCD Configuration
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// ======================= WIFI CONFIG =======================
const char* WIFI_SSID = "Abu Hosain";
const char* WIFI_PASS = "01731373179";

// Static IP Configuration (set to fixed IP to prevent IP changes)
// IMPORTANT: Make sure this IP is not used by another device on your network
// Change these values to match your network:
// - Static IP: The fixed IP you want for ESP32 (e.g., 192.168.0.111)
// - Gateway: Your router's IP (usually 192.168.0.1 or 192.168.1.1)
// - Subnet: Usually 255.255.255.0
// - DNS: Your router IP or 8.8.8.8 (Google DNS)
IPAddress staticIP(192, 168, 0, 111);      // Fixed IP for ESP32
IPAddress gateway(192, 168, 0, 1);          // Router IP (change if different)
IPAddress subnet(255, 255, 255, 0);       // Subnet mask
IPAddress dns(192, 168, 0, 1);            // DNS server (usually router IP)

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
  FISH_BETTA,
  FISH_GUPPY,
  FISH_NEON_TETRA,
  FISH_ANGELFISH,
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
  {"None", 6.5, 7.5, 26.0, 30.0},           // Temp: 26-30°C (within 25-32)
  {"Goldfish", 6.5, 8.0, 27.0, 31.0},       // Temp: 27-31°C (within 25-32)
  {"Betta Fish", 6.5, 7.5, 26.5, 30.5},      // Temp: 26.5-30.5°C (within 25-32)
  {"Guppy", 7.0, 8.5, 25.5, 29.5},          // Temp: 25.5-29.5°C (within 25-32)
  {"Neon Tetra", 5.0, 7.0, 25.0, 29.0},      // Temp: 25-29°C (within 25-32)
  {"Angelfish", 6.0, 7.5, 28.0, 32.0},       // Temp: 28-32°C (within 25-32)
  {"Comet", 6.5, 7.2, 26.0, 30.0},          // Temp: 26-30°C (within 25-32)
  {"Rohu", 6.6, 8.0, 27.5, 31.5}            // Temp: 27.5-31.5°C (within 25-32)
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
FishProfile getActiveFishProfile(); // Get active fish profile (custom or default)

#endif

