#include "config.h"

Preferences preferences;
FishType activeFishType = FISH_NONE;

// Convert logical ON to physical pin level
int getRelayLevel(bool logicalOn) {
  return (RELAY_ACTIVE_HIGH ? (logicalOn ? HIGH : LOW) : (logicalOn ? LOW : HIGH));
}

// Load calibration from EEPROM
void loadCalibration() {
  preferences.begin(PREF_NAMESPACE, true); // Read-only mode
  
  float ph7Voltage = preferences.getFloat(PREF_PH7_KEY, 2.50);
  float ph4Voltage = preferences.getFloat(PREF_PH4_KEY, 1.50);
  float tempOffset = preferences.getFloat(PREF_TEMP_OFFSET_KEY, 0.0);
  
  preferences.end();
  
  Serial.println("=== Calibration Loaded ===");
  Serial.printf("pH 7.00 voltage: %.3fV\n", ph7Voltage);
  Serial.printf("pH 4.00 voltage: %.3fV\n", ph4Voltage);
  Serial.printf("Temp offset: %.2f°C\n", tempOffset);
}

// Save calibration to EEPROM
void saveCalibration() {
  preferences.begin(PREF_NAMESPACE, false); // Read-write mode
  
  // Calibration values are set by sensor classes
  // This function is called after calibration
  
  preferences.end();
  Serial.println("Calibration saved to EEPROM");
}

// Save pH calibration values
void savePHCalibration(float ph7Voltage, float ph4Voltage) {
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putFloat(PREF_PH7_KEY, ph7Voltage);
  preferences.putFloat(PREF_PH4_KEY, ph4Voltage);
  preferences.end();
  Serial.printf("pH calibration saved: 7.00=%.3fV, 4.00=%.3fV\n", ph7Voltage, ph4Voltage);
}

// Save temperature offset
void saveTempOffset(float offset) {
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putFloat(PREF_TEMP_OFFSET_KEY, offset);
  preferences.end();
  Serial.printf("Temperature offset saved: %.2f°C\n", offset);
}

// Load fish type
void loadFishType() {
  preferences.begin(PREF_NAMESPACE, true);
  activeFishType = (FishType)preferences.getUChar(PREF_FISH_TYPE_KEY, FISH_NONE);
  preferences.end();
  Serial.printf("Fish type loaded from memory: %s\n", FISH_PROFILES[activeFishType].name.c_str());
}

// Reset fish type to NONE at startup (always start with no fish selected)
void resetFishTypeAtStartup() {
  activeFishType = FISH_NONE;
  Serial.println("Fish type reset to NONE at startup (user must select fish manually)");
}

// Save fish type
void saveFishType() {
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putUChar(PREF_FISH_TYPE_KEY, activeFishType);
  preferences.end();
  Serial.printf("Fish type saved: %s\n", FISH_PROFILES[activeFishType].name.c_str());
  
  // Control air pump based on fish selection
  // Air pump ON when any fish is selected, OFF when no fish selected
  if (activeFishType != FISH_NONE) {
    digitalWrite(REL_AIR_PUMP, getRelayLevel(true)); // ON
    Serial.println("✓ Air pump activated (fish selected)");
  } else {
    digitalWrite(REL_AIR_PUMP, getRelayLevel(false)); // OFF
    Serial.println("✓ Air pump deactivated (no fish selected)");
  }
}

// Get active fish profile (returns custom profile if set, otherwise default profile)
FishProfile getActiveFishProfile() {
  preferences.begin(PREF_NAMESPACE, true);
  bool useCustom = preferences.getBool("use_custom_profile", false);
  
  if (useCustom) {
    // Return custom profile from preferences
    FishProfile custom;
    custom.phMin = preferences.getFloat("custom_ph_min", 7.0f);
    custom.phMax = preferences.getFloat("custom_ph_max", 9.0f);
    custom.tempMin = preferences.getFloat("custom_temp_min", 24.0f);
    custom.tempMax = preferences.getFloat("custom_temp_max", 28.0f);
    custom.name = preferences.getString("custom_fish_name", "Custom");
    preferences.end();
    return custom;
  } else {
    // Return default profile for active fish type
    preferences.end();
    return FISH_PROFILES[activeFishType];
  }
}

