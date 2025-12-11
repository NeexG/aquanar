/*
  Smart Breeder - Full ESP32 code
  
  ==================================
  
  - Part-1: LCD + WiFi + WebServer basics + IP display + "Smart Breeder" header handling
  - Part-2: DS18B20 temp + pH sensor (median filter + calibration)
  - Part-3: Relay pins + Auto control logic (6s dosing + 5min wait)
  - Part-4: Fish mode selection (Gold, Comet, Rohu) + per-fish conditions
  - Part-5: Web dashboard (live values + relays + fish choose)
  
  প্রতিটি সেকশনে বাংলা কমেন্ট দেওয়া আছে।
  পিন মেপিং (আপনি দিতে চেয়েছিলেন) - নিশ্চিতভাবে ব্যবহার করা হয়েছে।
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>


// ======================= CONFIG =======================
const char* WIFI_SSID = "Abu Hosain";        // <-- এখানে আপনার WiFi নাম
const char* WIFI_PASS = "01731373179";    // <-- এখানে পাসওয়ার্ড

// Relay active logic: যদি HIGH দিলে রিলে 'ON' হয় -> true.
// যদি আপনার রিলে মডিউল ACTIVE LOW হয়, false করে দিন।
const bool RELAY_ACTIVE_HIGH = true;

// LCD address
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Web server
WebServer server(80);

// ======================= PINS ==========================
#define PH_PIN 35           // ADC pin for pH (PO)
#define TEMP_PIN 27         // DS18B20 data pin (G27)
#define SDA_PIN 21
#define SCL_PIN 22

// Relay pins mapping as user provided
const uint8_t REL_ACID_PUMP   = 23; // Acid pump relay - G23
const uint8_t REL_ALKALI_PUMP = 19; // Alkali pump relay - G19
const uint8_t REL_COOLER_FAN  = 18; // Cooler fan relay - G18
const uint8_t REL_WATER_HEATER= 17; // Water heater relay - G17
const uint8_t REL_AIR_PUMP    = 16; // Air pump relay - G16
const uint8_t REL_WATER_FLOW  = 32; // Water flow pump relay - G32
const uint8_t REL_RAIN_PUMP   = 33; // Rain pump relay - G33
const uint8_t REL_LIGHT_CTRL  = 25; // Light control relay - G25

// সকল রিলে array (সহজ হ্যান্ডলিং)
const uint8_t relayPins[] = {
  REL_ACID_PUMP,
  REL_ALKALI_PUMP,
  REL_COOLER_FAN,
  REL_WATER_HEATER,
  REL_AIR_PUMP,
  REL_WATER_FLOW,
  REL_RAIN_PUMP,
  REL_LIGHT_CTRL
};

const uint8_t RELAY_COUNT = sizeof(relayPins)/sizeof(relayPins[0]);

// Relay names for web interface
const char* relayNames[] = {
  "Acid Pump",
  "Alkali Pump",
  "Cooler Fan",
  "Water Heater",
  "Air Pump",
  "Water Flow",
  "Rain Pump",
  "Light Control"
};

// ======================= SENSORS =======================
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// ===== pH calibration vars (user provided/sample) =====
// আপনার ক্যালিব্রেশন অনুযায়ী adjust করুন
float neutralVoltage = 2.50;    // voltage at pH 7 (উদাহরণ)
float slope = 3.0;              // 1V = 3 pH (উদাহরণ)
float phOffset = 0.20;          // পরিমার্জন করার জন্য

// Median filter settings
const int PH_MED_SAMPLES = 15;
float phSamples[PH_MED_SAMPLES];
int phSampleIndex = 0;

// ======================= TIMERS & UI SEQUENCE =======================
// LCD start sequence & cyclical messages:
// - প্রথমে WiFi IP প্রদর্শিত হবে (5 সেকেন্ড)
// - ৫ মিনিট অন্তর: "AUTOMATIC FISH BREADING MACHINE" ৬ সেকেন্ড, তারপর
//   "INVENTOR : MD. NAIM ISLAM" ৬ সেকেন্ড, তারপর আবার fish name ৬ সেকেন্ড,
//   তার পর স্বাভাবিক ডিসপ্লে (temp/pH) চলবে।
unsigned long lastSequenceMillis = 0;
const unsigned long sequenceInterval = 5UL * 60UL * 1000UL; // 5 min
unsigned long seqStepStart = 0;
int seqStep = 0; // 0 = normal display, 1 = title, 2 = inventor, 3 = fish name
const unsigned long seqShowMs = 6000; // 6 seconds show each
bool showedIP = false;
unsigned long ipShowStart = 0;
const unsigned long ipShowDuration = 5000; // 5 seconds

// Dosing timers & wait logic
unsigned long lastDoseMillis = 0;
const unsigned long doseDurationMs = 6000;       // 6 seconds ON
const unsigned long postDoseWaitMs = 5UL*60UL*1000UL; // 5 minutes wait before re-evaluating
bool inDoseWait = false;
unsigned long doseStartMillis = 0;

// Sensor reading intervals
unsigned long lastTempRead = 0;
unsigned long lastPHRead = 0;
const unsigned long tempReadInterval = 2000;  // 2 seconds
const unsigned long phReadInterval = 500;     // 500ms

// ======================= SENSING & STATE =======================
float currentTempC = 0.0;
float currentPH = 7.0;
String tempState = "Normal";   // Cold / Normal / Hot (subject to fish settings)
String phState = "Neutral";    // Acidic / Normal / Alkaline

// Relay logical states
bool relayState[RELAY_COUNT];

// Manual override flags - prevents auto-control from overriding manual commands
bool manualOverride[RELAY_COUNT];
unsigned long manualOverrideTime[RELAY_COUNT];
const unsigned long MANUAL_OVERRIDE_TIMEOUT = 30000; // 30 seconds - manual control stays for 30s

// Fish modes and their condition thresholds
enum FishType { FISH_NONE=0, FISH_GOLD, FISH_COMET, FISH_ROHU };

FishType activeFish = FISH_NONE;

// Structure to hold fish-specific thresholds
struct FishConfig {
  String name;
  float ph_acid_upper;   // upper bound for acid (exclusive)
  float ph_normal_upper; // upper bound for normal (exclusive)
  float temp_cold_upper;
  float temp_normal_upper;
  // default auto-on relays (always ON when this fish active)
  bool autoOnAirPump;
  bool autoOnLight;
  bool autoOnWaterFlow;
  bool autoOnRainPump;
};

FishConfig fishConfigs[4]; // index by enum value

// ======================= HELPERS =======================

// Convert logical ON (true) to physical pin level
int physLevel(bool logicalOn) {
  return (RELAY_ACTIVE_HIGH ? (logicalOn ? HIGH : LOW) : (logicalOn ? LOW : HIGH));
}

void applyRelaysToPins() {
  for (uint8_t i=0; i<RELAY_COUNT; i++) {
    digitalWrite(relayPins[i], physLevel(relayState[i]));
  }
}

void setRelayLogical(uint8_t index, bool on) {
  if (index >= RELAY_COUNT) return;
  relayState[index] = on;
  digitalWrite(relayPins[index], physLevel(on));
}

// Set relay with manual override flag
void setRelayManual(uint8_t index, bool on) {
  if (index >= RELAY_COUNT) return;
  setRelayLogical(index, on);
  manualOverride[index] = true;
  manualOverrideTime[index] = millis();
  Serial.println("Manual override set for relay " + String(index) + " to " + String(on ? "ON" : "OFF"));
}

// Median filter for pH readings
float getMedianPH() {
  // Sort samples and return median
  float sorted[PH_MED_SAMPLES];
  for (int i = 0; i < PH_MED_SAMPLES; i++) {
    sorted[i] = phSamples[i];
  }
  
  // Simple bubble sort
  for (int i = 0; i < PH_MED_SAMPLES - 1; i++) {
    for (int j = 0; j < PH_MED_SAMPLES - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        float temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  
  return sorted[PH_MED_SAMPLES / 2];
}

// Read pH sensor with median filtering
float readPH() {
  int adcValue = analogRead(PH_PIN);
  float voltage = (adcValue / 4095.0) * 3.3; // ESP32 ADC is 12-bit, 3.3V reference
  
  // pH calculation with calibration
  float phValue = 7.0 + ((neutralVoltage - voltage) / slope) + phOffset;
  
  // Add to median filter buffer
  phSamples[phSampleIndex] = phValue;
  phSampleIndex = (phSampleIndex + 1) % PH_MED_SAMPLES;
  
  // Return median after buffer is filled
  if (phSampleIndex == 0) {
    return getMedianPH();
  }
  
  return phValue; // Return current value until buffer fills
}

// Read temperature from DS18B20
float readTemperature() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  if (temp == DEVICE_DISCONNECTED_C) {
    return currentTempC; // Return last known value if sensor error
  }
  return temp;
}

// Initialize fish configurations
void initFishConfigs() {
  // FISH_NONE (index 0)
  fishConfigs[0].name = "None";
  fishConfigs[0].ph_acid_upper = 6.5;
  fishConfigs[0].ph_normal_upper = 7.5;
  fishConfigs[0].temp_cold_upper = 20.0;
  fishConfigs[0].temp_normal_upper = 25.0;
  fishConfigs[0].autoOnAirPump = false;
  fishConfigs[0].autoOnLight = false;
  fishConfigs[0].autoOnWaterFlow = false;
  fishConfigs[0].autoOnRainPump = false;
  
  // FISH_GOLD (index 1)
  fishConfigs[1].name = "Gold Fish";
  fishConfigs[1].ph_acid_upper = 6.5;
  fishConfigs[1].ph_normal_upper = 7.5;
  fishConfigs[1].temp_cold_upper = 18.0;
  fishConfigs[1].temp_normal_upper = 24.0;
  fishConfigs[1].autoOnAirPump = true;
  fishConfigs[1].autoOnLight = true;
  fishConfigs[1].autoOnWaterFlow = true;
  fishConfigs[1].autoOnRainPump = false;
  
  // FISH_COMET (index 2)
  fishConfigs[2].name = "Comet";
  fishConfigs[2].ph_acid_upper = 6.8;
  fishConfigs[2].ph_normal_upper = 7.8;
  fishConfigs[2].temp_cold_upper = 20.0;
  fishConfigs[2].temp_normal_upper = 26.0;
  fishConfigs[2].autoOnAirPump = true;
  fishConfigs[2].autoOnLight = true;
  fishConfigs[2].autoOnWaterFlow = true;
  fishConfigs[2].autoOnRainPump = true;
  
  // FISH_ROHU (index 3)
  fishConfigs[3].name = "Rohu";
  fishConfigs[3].ph_acid_upper = 7.0;
  fishConfigs[3].ph_normal_upper = 8.0;
  fishConfigs[3].temp_cold_upper = 22.0;
  fishConfigs[3].temp_normal_upper = 28.0;
  fishConfigs[3].autoOnAirPump = true;
  fishConfigs[3].autoOnLight = false;
  fishConfigs[3].autoOnWaterFlow = true;
  fishConfigs[3].autoOnRainPump = false;
}

// Determine temperature state based on active fish config
String getTempState(float temp) {
  if (activeFish == FISH_NONE) return "Normal";
  
  FishConfig& cfg = fishConfigs[activeFish];
  if (temp < cfg.temp_cold_upper) {
    return "Cold";
  } else if (temp < cfg.temp_normal_upper) {
    return "Normal";
  } else {
    return "Hot";
  }
}

// Determine pH state based on active fish config
String getPHState(float ph) {
  if (activeFish == FISH_NONE) return "Neutral";
  
  FishConfig& cfg = fishConfigs[activeFish];
  if (ph < cfg.ph_acid_upper) {
    return "Acidic";
  } else if (ph < cfg.ph_normal_upper) {
    return "Neutral";
  } else {
    return "Alkaline";
  }
}

// Check and clear expired manual overrides
void checkManualOverrides() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    if (manualOverride[i]) {
      unsigned long elapsed = now - manualOverrideTime[i];
      if (elapsed >= MANUAL_OVERRIDE_TIMEOUT) {
        manualOverride[i] = false;
        Serial.println("Manual override expired for relay " + String(i));
      }
    }
  }
}

// Auto control logic - adjust relays based on sensor readings
void updateAutoControl() {
  if (activeFish == FISH_NONE) return;
  
  // Check for expired manual overrides
  checkManualOverrides();
  
  FishConfig& cfg = fishConfigs[activeFish];
  
  // Set always-on relays based on fish config (unless manually overridden)
  if (!manualOverride[4]) setRelayLogical(4, cfg.autoOnAirPump);      // REL_AIR_PUMP
  if (!manualOverride[7]) setRelayLogical(7, cfg.autoOnLight);        // REL_LIGHT_CTRL
  if (!manualOverride[5]) setRelayLogical(5, cfg.autoOnWaterFlow);    // REL_WATER_FLOW
  if (!manualOverride[6]) setRelayLogical(6, cfg.autoOnRainPump);     // REL_RAIN_PUMP
  
  // Check if we're in dosing wait period
  if (inDoseWait) {
    unsigned long waitElapsed = millis() - doseStartMillis;
    if (waitElapsed >= postDoseWaitMs) {
      inDoseWait = false;
    } else {
      return; // Don't evaluate during wait period
    }
  }
  
  // Temperature control (only if not manually overridden)
  if (!manualOverride[2] && !manualOverride[3]) {
    if (currentTempC < cfg.temp_cold_upper) {
      // Too cold - turn on heater, turn off cooler
      setRelayLogical(3, true);   // REL_WATER_HEATER
      setRelayLogical(2, false);   // REL_COOLER_FAN
    } else if (currentTempC >= cfg.temp_normal_upper) {
      // Too hot - turn on cooler, turn off heater
      setRelayLogical(3, false);  // REL_WATER_HEATER
      setRelayLogical(2, true);    // REL_COOLER_FAN
    } else {
      // Normal temperature - turn off both
      setRelayLogical(3, false);  // REL_WATER_HEATER
      setRelayLogical(2, false);   // REL_COOLER_FAN
    }
  }
  
  // pH control with dosing logic (only if not manually overridden)
  if (!manualOverride[0] && !manualOverride[1]) {
    if (currentPH < cfg.ph_acid_upper) {
      // Too acidic - need to add alkali
      phState = "Acidic";
      if (!relayState[1]) { // If alkali pump not already on
        setRelayLogical(1, true);  // REL_ALKALI_PUMP
        setRelayLogical(0, false); // REL_ACID_PUMP
        doseStartMillis = millis();
        inDoseWait = true;
      }
    } else if (currentPH >= cfg.ph_normal_upper) {
      // Too alkaline - need to add acid
      phState = "Alkaline";
      if (!relayState[0]) { // If acid pump not already on
        setRelayLogical(0, true);   // REL_ACID_PUMP
        setRelayLogical(1, false);  // REL_ALKALI_PUMP
        doseStartMillis = millis();
        inDoseWait = true;
      }
    } else {
      // Normal pH - turn off both pumps
      phState = "Neutral";
      setRelayLogical(0, false);  // REL_ACID_PUMP
      setRelayLogical(1, false);   // REL_ALKALI_PUMP
    }
    
    // Check dosing duration (6 seconds)
    if (relayState[0] || relayState[1]) { // If any pump is on
      unsigned long doseElapsed = millis() - doseStartMillis;
      if (doseElapsed >= doseDurationMs) {
        // Turn off pumps after 6 seconds
        setRelayLogical(0, false);
        setRelayLogical(1, false);
        // Start wait period
        inDoseWait = true;
        doseStartMillis = millis();
      }
    }
  }
}

// Update LCD display based on current sequence step
void updateLCD() {
  unsigned long now = millis();
  
  // Show IP address first (once)
  if (!showedIP && WiFi.status() == WL_CONNECTED) {
    if (ipShowStart == 0) {
      ipShowStart = now;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Connected");
      lcd.setCursor(0, 1);
      lcd.print("IP: ");
      lcd.print(WiFi.localIP());
    }
    if (now - ipShowStart < ipShowDuration) {
      return; // Keep showing IP
    } else {
      showedIP = true;
      lcd.clear();
    }
  }
  
  // Check if it's time for sequence messages (every 5 minutes)
  if (now - lastSequenceMillis >= sequenceInterval) {
    lastSequenceMillis = now;
    seqStep = 1; // Start sequence
    seqStepStart = now;
  }
  
  // Handle sequence steps
  if (seqStep > 0) {
    unsigned long stepElapsed = now - seqStepStart;
    
    if (stepElapsed >= seqShowMs) {
      seqStep++;
      seqStepStart = now;
      
      if (seqStep > 3) {
        seqStep = 0; // Back to normal display
      }
    }
    
    // Display current sequence step
    lcd.clear();
    switch (seqStep) {
      case 1:
        lcd.setCursor(0, 0);
        lcd.print("AUTOMATIC FISH");
        lcd.setCursor(0, 1);
        lcd.print("BREEDING MACHINE");
        break;
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("INVENTOR:");
        lcd.setCursor(0, 1);
        lcd.print("MD. NAIM ISLAM");
        break;
      case 3:
        lcd.setCursor(0, 0);
        lcd.print("FISH MODE:");
        lcd.setCursor(0, 1);
        lcd.print(fishConfigs[activeFish].name);
        break;
    }
    return;
  }
  
  // Normal display: Temperature and pH
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(currentTempC, 1);
  lcd.print("C ");
  lcd.print(tempState);
  
  lcd.setCursor(0, 1);
  lcd.print("pH: ");
  lcd.print(currentPH, 2);
  lcd.print(" ");
  lcd.print(phState);
}

// ======================= WEB SERVER =======================

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Smart Breeder</title>";
  html += "<style>";
  html += "body { font-family: Arial; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }";
  html += "h1 { color: #2c3e50; }";
  html += ".sensor { background: #ecf0f1; padding: 15px; margin: 10px 0; border-radius: 5px; }";
  html += ".sensor-value { font-size: 24px; font-weight: bold; color: #27ae60; }";
  html += ".relay { background: #fff; border: 2px solid #bdc3c7; padding: 10px; margin: 5px 0; border-radius: 5px; display: flex; justify-content: space-between; align-items: center; }";
  html += ".relay-on { border-color: #27ae60; background: #d5f4e6; }";
  html += ".relay-off { border-color: #e74c3c; background: #fadbd8; }";
  html += "button { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; font-size: 14px; }";
  html += ".btn-on { background: #27ae60; color: white; }";
  html += ".btn-off { background: #e74c3c; color: white; }";
  html += ".btn-select { background: #3498db; color: white; }";
  html += "select { padding: 10px; font-size: 16px; margin: 10px 0; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>🐟 Smart Breeder Dashboard</h1>";
  
  // Sensor readings
  html += "<div class='sensor'>";
  html += "<h2>Sensor Readings</h2>";
  html += "<div>Temperature: <span class='sensor-value'>" + String(currentTempC, 1) + "°C</span> (" + tempState + ")</div>";
  html += "<div>pH Level: <span class='sensor-value'>" + String(currentPH, 2) + "</span> (" + phState + ")</div>";
  html += "</div>";
  
  // Fish selection
  html += "<div class='sensor'>";
  html += "<h2>Fish Mode</h2>";
  html += "<select id='fishSelect' onchange='setFish(this.value)'>";
  html += "<option value='0'" + String(activeFish == FISH_NONE ? " selected" : "") + ">None</option>";
  html += "<option value='1'" + String(activeFish == FISH_GOLD ? " selected" : "") + ">Gold Fish</option>";
  html += "<option value='2'" + String(activeFish == FISH_COMET ? " selected" : "") + ">Comet</option>";
  html += "<option value='3'" + String(activeFish == FISH_ROHU ? " selected" : "") + ">Rohu</option>";
  html += "</select>";
  html += "<div>Current: <strong>" + fishConfigs[activeFish].name + "</strong></div>";
  html += "</div>";
  
  // Relay controls
  html += "<div class='sensor'>";
  html += "<h2>Relay Controls</h2>";
  for (int i = 0; i < RELAY_COUNT; i++) {
    String relayClass = relayState[i] ? "relay-on" : "relay-off";
    String btnClass = relayState[i] ? "btn-off" : "btn-on";
    String btnText = relayState[i] ? "Turn OFF" : "Turn ON";
    int currentState = relayState[i] ? 1 : 0;
    int newState = relayState[i] ? 0 : 1;
    html += "<div class='relay " + relayClass + "'>";
    html += "<span>" + String(relayNames[i]) + "</span>";
    html += "<button class='" + btnClass + "' onclick='toggleRelay(" + String(i) + "," + String(newState) + ")'>" + btnText + "</button>";
    html += "</div>";
  }
  html += "</div>";
  
  html += "<script>";
  html += "function toggleRelay(index, newState) {";
  html += "  fetch('/relay?index=' + index + '&state=' + newState, {method: 'POST'});";
  html += "  setTimeout(() => location.reload(), 500);";
  html += "}";
  html += "function setFish(value) {";
  html += "  fetch('/fish?type=' + value, {method: 'POST'});";
  html += "  setTimeout(() => location.reload(), 500);";
  html += "}";
  html += "setInterval(() => location.reload(), 5000);"; // Auto-refresh every 5 seconds
  html += "</script>";
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleRelay() {
  if (server.hasArg("index") && server.hasArg("state")) {
    int index = server.arg("index").toInt();
    int state = server.arg("state").toInt();
    if (index >= 0 && index < RELAY_COUNT) {
      setRelayLogical(index, state == 1);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

void handleFish() {
  if (server.hasArg("type")) {
    int type = server.arg("type").toInt();
    if (type >= 0 && type <= 3) {
      activeFish = (FishType)type;
      updateAutoControl(); // Update relays based on new fish selection
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

// Helper function to set CORS headers
void setCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// Handle OPTIONS requests for CORS
void handleOptions() {
  setCORSHeaders();
  server.send(200, "text/plain", "");
}

// API: GET /api/status - Returns device sensor data (matches website format)
void handleAPIStatus() {
  setCORSHeaders();
  
  String json = "{";
  json += "\"ph\":" + String(currentPH, 2) + ",";
  json += "\"temperature\":" + String(currentTempC, 2) + ",";
  json += "\"fan\":" + String(relayState[2] ? "true" : "false") + ",";  // REL_COOLER_FAN
  json += "\"acidPump\":" + String(relayState[0] ? "true" : "false") + ",";  // REL_ACID_PUMP
  json += "\"basePump\":" + String(relayState[1] ? "true" : "false");  // REL_ALKALI_PUMP
  json += "}";
  
  server.send(200, "application/json", json);
}

// API: POST /api/control - Control relays (matches website format)
void handleAPIControl() {
  setCORSHeaders();
  
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.println("Received control command: " + body);
    
    // Parse JSON (simple parsing for ESP32)
    // Expected format: {"fan":true,"acidPump":false,"basePump":false}
    bool fanSet = false, acidSet = false, baseSet = false;
    bool fanVal = false, acidVal = false, baseVal = false;
    bool hasError = false;
    
    // Check for fan (case-insensitive)
    int fanPos = body.indexOf("\"fan\"");
    if (fanPos < 0) fanPos = body.indexOf("\"Fan\"");
    if (fanPos >= 0) {
      fanSet = true;
      int truePos = body.indexOf("true", fanPos);
      int falsePos = body.indexOf("false", fanPos);
      if (truePos >= 0 && (falsePos < 0 || truePos < falsePos)) {
        fanVal = true;
      } else if (falsePos >= 0) {
        fanVal = false;
      } else {
        hasError = true;
        Serial.println("Error: Invalid fan value");
      }
    }
    
    // Check for acidPump (case-insensitive)
    int acidPos = body.indexOf("\"acidPump\"");
    if (acidPos < 0) acidPos = body.indexOf("\"acidpump\"");
    if (acidPos < 0) acidPos = body.indexOf("\"AcidPump\"");
    if (acidPos >= 0) {
      acidSet = true;
      int truePos = body.indexOf("true", acidPos);
      int falsePos = body.indexOf("false", acidPos);
      if (truePos >= 0 && (falsePos < 0 || truePos < falsePos)) {
        acidVal = true;
      } else if (falsePos >= 0) {
        acidVal = false;
      } else {
        hasError = true;
        Serial.println("Error: Invalid acidPump value");
      }
    }
    
    // Check for basePump (case-insensitive)
    int basePos = body.indexOf("\"basePump\"");
    if (basePos < 0) basePos = body.indexOf("\"basepump\"");
    if (basePos < 0) basePos = body.indexOf("\"BasePump\"");
    if (basePos >= 0) {
      baseSet = true;
      int truePos = body.indexOf("true", basePos);
      int falsePos = body.indexOf("false", basePos);
      if (truePos >= 0 && (falsePos < 0 || truePos < falsePos)) {
        baseVal = true;
      } else if (falsePos >= 0) {
        baseVal = false;
      } else {
        hasError = true;
        Serial.println("Error: Invalid basePump value");
      }
    }
    
    if (hasError) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON format\"}");
      return;
    }
    
    // Apply control commands with manual override
    if (fanSet) {
      setRelayManual(2, fanVal);  // REL_COOLER_FAN
      Serial.println("Fan manually set to: " + String(fanVal ? "ON" : "OFF"));
    }
    if (acidSet) {
      setRelayManual(0, acidVal);  // REL_ACID_PUMP
      Serial.println("Acid pump manually set to: " + String(acidVal ? "ON" : "OFF"));
    }
    if (baseSet) {
      setRelayManual(1, baseVal);  // REL_ALKALI_PUMP
      Serial.println("Base pump manually set to: " + String(baseVal ? "ON" : "OFF"));
    }
    
    if (!fanSet && !acidSet && !baseSet) {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"No valid control fields found\"}");
      return;
    }
    
    String response = "{\"success\":true,\"message\":\"Control command executed\"}";
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing request body\"}");
  }
}

// API: POST /api/species - Configure fish species
void handleAPISpecies() {
  setCORSHeaders();
  
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Parse species data (simplified - you can enhance this)
    // Expected format: {"name":"Goldfish","idealPhMin":6.5,"idealPhMax":8.0,"idealTempMin":18,"idealTempMax":24}
    
    // For now, just update active fish based on name match
    if (body.indexOf("\"name\":\"Goldfish\"") >= 0 || body.indexOf("\"name\":\"Gold Fish\"") >= 0) {
      activeFish = FISH_GOLD;
    } else if (body.indexOf("\"name\":\"Comet\"") >= 0) {
      activeFish = FISH_COMET;
    } else if (body.indexOf("\"name\":\"Rohu\"") >= 0) {
      activeFish = FISH_ROHU;
    }
    
    updateAutoControl();
    
    String response = "{\"success\":true,\"message\":\"Species configuration updated\"}";
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid request\"}");
  }
}

// API: POST /api/wifi - Configure WiFi (for future use)
void handleAPIWiFi() {
  setCORSHeaders();
  
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    // Parse WiFi config (simplified)
    // Expected format: {"ssid":"NetworkName","password":"password123"}
    // Note: This would require ESP32 restart to apply, so we'll just acknowledge
    
    String response = "{\"success\":true,\"message\":\"WiFi configuration received (requires restart to apply)\"}";
    server.send(200, "application/json", response);
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid request\"}");
  }
}

// API: GET /api/ping - Connection test
void handleAPIPing() {
  setCORSHeaders();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"pong\"}");
}

// Legacy API endpoint (backward compatibility)
void handleAPI() {
  handleAPIStatus();
}

void setupWebServer() {
  // API endpoints (matching website expectations)
  // Note: WebServer handles OPTIONS automatically, but we'll check in handlers
  server.on("/api/status", []() {
    if (server.method() == HTTP_OPTIONS) {
      handleOptions();
    } else {
      handleAPIStatus();
    }
  });
  
  server.on("/api/control", []() {
    if (server.method() == HTTP_OPTIONS) {
      handleOptions();
    } else {
      handleAPIControl();
    }
  });
  
  server.on("/api/species", []() {
    if (server.method() == HTTP_OPTIONS) {
      handleOptions();
    } else {
      handleAPISpecies();
    }
  });
  
  server.on("/api/wifi", []() {
    if (server.method() == HTTP_OPTIONS) {
      handleOptions();
    } else {
      handleAPIWiFi();
    }
  });
  
  server.on("/api/ping", []() {
    if (server.method() == HTTP_OPTIONS) {
      handleOptions();
    } else {
      handleAPIPing();
    }
  });
  
  // Legacy endpoints (for direct browser access)
  server.on("/", handleRoot);
  server.on("/relay", HTTP_POST, handleRelay);
  server.on("/fish", HTTP_POST, handleFish);
  server.on("/api", HTTP_GET, handleAPI);
  
  server.begin();
  Serial.println("Web server started");
  Serial.println("API Endpoints:");
  Serial.println("  GET  /api/status  - Get sensor data");
  Serial.println("  POST /api/control - Control relays");
  Serial.println("  POST /api/species - Configure fish species");
  Serial.println("  POST /api/wifi    - Configure WiFi");
  Serial.println("  GET  /api/ping    - Test connection");
}

// ======================= SETUP & LOOP =======================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Smart Breeder Starting ===");
  
  // Initialize LCD
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Breeder");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  // Initialize relay pins
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    pinMode(relayPins[i], OUTPUT);
    relayState[i] = false;
    manualOverride[i] = false;
    manualOverrideTime[i] = 0;
    digitalWrite(relayPins[i], physLevel(false));
  }
  Serial.println("Relays initialized");
  
  // Initialize temperature sensor
  sensors.begin();
  Serial.println("Temperature sensor initialized");
  
  // Initialize pH sensor
  pinMode(PH_PIN, INPUT);
  // Initialize pH sample array
  for (int i = 0; i < PH_MED_SAMPLES; i++) {
    phSamples[i] = 7.0;
  }
  Serial.println("pH sensor initialized");
  
  // Initialize fish configurations
  initFishConfigs();
  Serial.println("Fish configurations loaded");
  
  // Connect to WiFi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  lcd.setCursor(0, 1);
  lcd.print(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Setup mDNS
    if (MDNS.begin("smartbreeder")) {
      Serial.println("mDNS responder started");
    }
    
    // Setup web server
    setupWebServer();
  } else {
    Serial.println("\nWiFi Connection Failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
  }
  
  Serial.println("=== Setup Complete ===");
}

void loop() {
  server.handleClient();
  
  unsigned long now = millis();
  
  // Read temperature sensor
  if (now - lastTempRead >= tempReadInterval) {
    currentTempC = readTemperature();
    tempState = getTempState(currentTempC);
    lastTempRead = now;
  }
  
  // Read pH sensor
  if (now - lastPHRead >= phReadInterval) {
    currentPH = readPH();
    phState = getPHState(currentPH);
    lastPHRead = now;
  }
  
  // Update auto control logic
  updateAutoControl();
  
  // Update LCD display
  updateLCD();
  
  delay(100); // Small delay to prevent watchdog issues
}

