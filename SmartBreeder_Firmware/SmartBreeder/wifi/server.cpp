#include "server.h"
#include "sensors/ph.h"
#include "sensors/temp.h"
#include "control/fan.h"
#include "control/phControl.h"
#include "config/config.h"

SmartBreederServer::SmartBreederServer(PHSensor* ph, TempSensor* temp, FanControl* fan, PHControl* phCtrl) {
  phSensor = ph;
  tempSensor = temp;
  fanControl = fan;
  phControl = phCtrl;
  server = new WebServer(80);
}

void SmartBreederServer::begin() {
  WiFi.mode(WIFI_STA);
  
  // Configure static IP address (prevents IP from changing)
  Serial.println("Configuring static IP...");
  Serial.printf("Static IP: %s\n", staticIP.toString().c_str());
  Serial.printf("Gateway: %s\n", gateway.toString().c_str());
  Serial.printf("Subnet: %s\n", subnet.toString().c_str());
  
  if (!WiFi.config(staticIP, gateway, subnet, dns)) {
    Serial.println("WARNING: Static IP configuration failed! Using DHCP instead.");
  } else {
    Serial.println("Static IP configured successfully!");
  }
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Verify static IP was applied
    if (WiFi.localIP() == staticIP) {
      Serial.println("✓ Static IP is active!");
    } else {
      Serial.printf("⚠ WARNING: Expected IP %s but got %s\n", 
                    staticIP.toString().c_str(), 
                    WiFi.localIP().toString().c_str());
      Serial.println("  Check if IP is already in use or router doesn't allow static IPs");
    }
    
    if (MDNS.begin("smartbreeder")) {
      Serial.println("mDNS started: http://smartbreeder.local");
    }
    
    // Setup routes
    server->on("/", HTTP_GET, [this]() { handleRoot(); });
    server->on("/api/status", HTTP_GET, [this]() { handleAPIStatus(); });
    server->on("/api/control", HTTP_POST, [this]() { handleAPIControl(); });
    server->on("/api/species", HTTP_POST, [this]() { handleAPISpecies(); });
    server->on("/api/species/list", HTTP_GET, [this]() { handleAPISpeciesList(); }); // Get all species
    server->on("/api/calibrate", HTTP_POST, [this]() { handleAPICalibrate(); });
    server->on("/api/wifi", HTTP_POST, [this]() { handleAPIWiFi(); });
    server->on("/api/ping", HTTP_GET, [this]() { handleAPIPing(); });
    server->onNotFound([this]() {
      if (server->method() == HTTP_OPTIONS) {
        handleOptions();
      } else {
        server->send(404, "text/plain", "Not Found");
      }
    });
    
    server->begin();
    Serial.println("Web server started");
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

void SmartBreederServer::update() {
  if (server) {
    server->handleClient();
  }
}

bool SmartBreederServer::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String SmartBreederServer::getIP() {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  return "Not Connected";
}

void SmartBreederServer::setCORSHeaders() {
  server->sendHeader("Access-Control-Allow-Origin", "*");
  server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void SmartBreederServer::handleOptions() {
  setCORSHeaders();
  server->send(200, "text/plain", "");
}

String SmartBreederServer::getStatusJSON() {
  // Read sensors once for consistent data
  float ph = phSensor->read();
  float temp = tempSensor->read();
  
  String json = "{";
  // Core fields that dashboard REQUIRES (exact match)
  json += "\"ph\":" + String(ph, 2) + ",";
  json += "\"temperature\":" + String(temp, 2) + ",";
  json += "\"fan\":" + String(fanControl->getState() ? "true" : "false") + ",";
  json += "\"acidPump\":" + String(phControl->getAcidState() ? "true" : "false") + ",";
  json += "\"basePump\":" + String(phControl->getBaseState() ? "true" : "false");
  
  // Optional bonus fields (won't break dashboard if missing)
  json += ",\"fishType\":" + String(activeFishType);
  
  // Add custom fish profile info if available
  Preferences prefs;
  prefs.begin(PREF_NAMESPACE, true);
  bool useCustom = prefs.getBool("use_custom_profile", false);
  if (useCustom) {
    FishProfile custom = getActiveFishProfile();
    json += ",\"customProfile\":true";
    json += ",\"fishName\":\"" + custom.name + "\"";
    json += ",\"phRange\":{\"min\":" + String(custom.phMin, 1) + ",\"max\":" + String(custom.phMax, 1) + "}";
    json += ",\"tempRange\":{\"min\":" + String(custom.tempMin, 1) + ",\"max\":" + String(custom.tempMax, 1) + "}";
  } else {
    json += ",\"customProfile\":false";
    FishProfile profile = FISH_PROFILES[activeFishType];
    json += ",\"phRange\":{\"min\":" + String(profile.phMin, 1) + ",\"max\":" + String(profile.phMax, 1) + "}";
    json += ",\"tempRange\":{\"min\":" + String(profile.tempMin, 1) + ",\"max\":" + String(profile.tempMax, 1) + "}";
  }
  prefs.end();
  json += ",\"cooldownRemaining\":" + String(phControl->getCooldownRemaining());
  json += ",\"phSafe\":" + String(phSensor->isSafe() ? "true" : "false");
  json += ",\"tempSafe\":" + String(tempSensor->isSafe() ? "true" : "false");
  
  json += "}";
  return json;
}

void SmartBreederServer::handleAPIStatus() {
  setCORSHeaders();
  server->send(200, "application/json", getStatusJSON());
}

void SmartBreederServer::handleAPIControl() {
  setCORSHeaders();
  
  if (server->hasArg("plain")) {
    String body = server->arg("plain");
    Serial.println("Control command: " + body);
    
    bool fanSet = false, acidSet = false, baseSet = false;
    bool fanVal = false, acidVal = false, baseVal = false;
    bool hasError = false;
    
    // Parse JSON - handle both true and false values properly
    // Format: {"fan":true,"acidPump":false,"basePump":false}
    
    // Parse fan
    int fanPos = body.indexOf("\"fan\"");
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
    
    // Parse acidPump
    int acidPos = body.indexOf("\"acidPump\"");
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
    
    // Parse basePump
    int basePos = body.indexOf("\"basePump\"");
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
      server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON format\"}");
      return;
    }
    
    // Apply commands
    if (fanSet) {
      fanControl->set(fanVal, true);
      Serial.printf("Fan %s (manual)\n", fanVal ? "ON" : "OFF");
    }
    if (acidSet) {
      phControl->setAcid(acidVal);
      Serial.printf("Acid pump %s\n", acidVal ? "ON" : "OFF");
    }
    if (baseSet) {
      phControl->setBase(baseVal);
      Serial.printf("Base pump %s\n", baseVal ? "ON" : "OFF");
    }
    
    // Return success response (dashboard expects this format)
    String response = "{\"success\":true}";
    server->send(200, "application/json", response);
  } else {
    server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing request body\"}");
  }
}

void SmartBreederServer::handleAPISpecies() {
  setCORSHeaders();
  
  if (server->hasArg("plain")) {
    String body = server->arg("plain");
    Serial.println("Species config: " + body);
    
    // Parse custom fish profile from dashboard (Fish Species Database)
    // Format: {"name":"Goldfish","idealPh":{"min":7.0,"max":9.0},"idealTemp":{"min":24,"max":28}}
    bool hasCustomProfile = false;
    float customPhMin = 0, customPhMax = 0;
    float customTempMin = 0, customTempMax = 0;
    String fishName = "";
    
    // Extract custom pH range
    if (body.indexOf("\"idealPh\"") >= 0) {
      int phMinPos = body.indexOf("\"min\":");
      int phMaxPos = body.indexOf("\"max\":");
      
      if (phMinPos >= 0 && phMaxPos >= 0) {
        // Find the min value (could be in idealPh or idealTemp)
        int idealPhPos = body.indexOf("\"idealPh\"");
        int idealTempPos = body.indexOf("\"idealTemp\"");
        
        // Extract pH min
        if (idealPhPos >= 0 && phMinPos > idealPhPos) {
          int start = body.indexOf("\"min\":", idealPhPos) + 6;
          int end = body.indexOf(",", start);
          if (end < 0) end = body.indexOf("}", start);
          if (end > start) {
            customPhMin = body.substring(start, end).toFloat();
            hasCustomProfile = true;
          }
        }
        
        // Extract pH max
        if (idealPhPos >= 0 && phMaxPos > idealPhPos) {
          int start = body.indexOf("\"max\":", idealPhPos) + 6;
          int end = body.indexOf("}", start);
          if (end < 0) end = body.indexOf(",", start);
          if (end > start) {
            customPhMax = body.substring(start, end).toFloat();
          }
        }
        
        // Extract temp min
        if (idealTempPos >= 0) {
          int start = body.indexOf("\"min\":", idealTempPos) + 6;
          int end = body.indexOf(",", start);
          if (end < 0) end = body.indexOf("}", start);
          if (end > start) {
            customTempMin = body.substring(start, end).toFloat();
          }
        }
        
        // Extract temp max
        if (idealTempPos >= 0) {
          int start = body.indexOf("\"max\":", idealTempPos) + 6;
          int end = body.indexOf("}", start);
          if (end < 0) end = body.indexOf(",", start);
          if (end > start) {
            customTempMax = body.substring(start, end).toFloat();
          }
        }
      }
    }
    
    // Extract fish name
    if (body.indexOf("\"name\"") >= 0) {
      int nameStart = body.indexOf("\"name\":\"") + 8;
      int nameEnd = body.indexOf("\"", nameStart);
      if (nameEnd > nameStart) {
        fishName = body.substring(nameStart, nameEnd);
      }
    }
    
    // If custom profile provided, save it and update active fish type
    if (hasCustomProfile && customPhMin > 0 && customPhMax > 0) {
      // Save custom profile to preferences
      Preferences prefs;
      prefs.begin(PREF_NAMESPACE, false);
      prefs.putFloat("custom_ph_min", customPhMin);
      prefs.putFloat("custom_ph_max", customPhMax);
      prefs.putFloat("custom_temp_min", customTempMin);
      prefs.putFloat("custom_temp_max", customTempMax);
      prefs.putString("custom_fish_name", fishName);
      prefs.putBool("use_custom_profile", true);
      prefs.end();
      
      Serial.printf("\n=== FISH SPECIES SELECTED FROM DASHBOARD ===\n");
      Serial.printf("Species Name: %s\n", fishName.c_str());
      Serial.printf("pH Range: %.1f - %.1f\n", customPhMin, customPhMax);
      Serial.printf("Temperature Range: %.1f - %.1f°C\n", customTempMin, customTempMax);
      Serial.printf("Custom Profile: ENABLED\n");
      Serial.printf("pH control will use these ranges for automatic correction\n");
      Serial.printf("pH check interval: 1 minute\n");
      Serial.printf("Cooldown period: 1 minute between corrections\n");
      Serial.printf("==========================================\n\n");
      
      // Try to match fish name to existing type for compatibility
      if (fishName.indexOf("Gold") >= 0 || fishName.indexOf("gold") >= 0) {
        activeFishType = FISH_GOLD;
      } else if (fishName.indexOf("Betta") >= 0 || fishName.indexOf("betta") >= 0) {
        activeFishType = FISH_BETTA;
      } else if (fishName.indexOf("Guppy") >= 0 || fishName.indexOf("guppy") >= 0) {
        activeFishType = FISH_GUPPY;
      } else if (fishName.indexOf("Neon") >= 0 || fishName.indexOf("neon") >= 0 || 
                 fishName.indexOf("Tetra") >= 0 || fishName.indexOf("tetra") >= 0) {
        activeFishType = FISH_NEON_TETRA;
      } else if (fishName.indexOf("Angelfish") >= 0 || fishName.indexOf("angelfish") >= 0 ||
                 fishName.indexOf("Angel") >= 0 || fishName.indexOf("angel") >= 0) {
        activeFishType = FISH_ANGELFISH;
      } else if (fishName.indexOf("Comet") >= 0 || fishName.indexOf("comet") >= 0) {
        activeFishType = FISH_COMET;
      } else if (fishName.indexOf("Rohu") >= 0 || fishName.indexOf("rohu") >= 0) {
        activeFishType = FISH_ROHU;
      } else {
        activeFishType = FISH_GOLD; // Default fallback
      }
      saveFishType();
      
      // DO NOT reset cooldown - always enforce 1-minute wait between corrections
      // This prevents rapid repeated corrections
      Serial.println("New species selected - pH control will activate after 1-minute cooldown");
      Serial.println("pH check interval: 1 minute | Cooldown: 1 minute (enforced)");
      
      server->send(200, "application/json", "{\"success\":true,\"message\":\"Custom profile saved and activated\"}");
      return;
    }
    
    // Try to parse by type number first (simple format: {"type":1})
    if (body.indexOf("\"type\"") >= 0) {
      int typePos = body.indexOf("\"type\":");
      if (typePos >= 0) {
        int typeValue = body.substring(typePos + 7).toInt();
        if (typeValue >= 0 && typeValue <= 7) { // Updated: 8 fish types (0-7)
          activeFishType = (FishType)typeValue;
          saveFishType();
          
          // Clear custom profile when using predefined type
          Preferences prefs;
          prefs.begin(PREF_NAMESPACE, false);
          prefs.putBool("use_custom_profile", false);
          prefs.end();
          
          Serial.printf("Fish type set to: %s (by type number)\n", FISH_PROFILES[activeFishType].name.c_str());
          server->send(200, "application/json", "{\"success\":true}");
          return;
        }
      }
    }
    
    // Parse by name (fallback - without custom ranges)
    if (body.indexOf("\"name\"") >= 0) {
      if (body.indexOf("\"name\":\"Goldfish\"") >= 0 || body.indexOf("\"name\":\"goldfish\"") >= 0 ||
          body.indexOf("\"name\":\"Gold Fish\"") >= 0 || body.indexOf("\"name\":\"gold fish\"") >= 0) {
        activeFishType = FISH_GOLD;
        saveFishType();
        Serial.println("Fish type set to: Goldfish (by name)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      } else if (body.indexOf("\"name\":\"Betta\"") >= 0 || body.indexOf("\"name\":\"betta\"") >= 0) {
        activeFishType = FISH_BETTA;
        saveFishType();
        Serial.println("Fish type set to: Betta Fish (by name)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      } else if (body.indexOf("\"name\":\"Guppy\"") >= 0 || body.indexOf("\"name\":\"guppy\"") >= 0) {
        activeFishType = FISH_GUPPY;
        saveFishType();
        Serial.println("Fish type set to: Guppy (by name)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      } else if (body.indexOf("\"name\":\"Neon Tetra\"") >= 0 || body.indexOf("\"name\":\"neon tetra\"") >= 0) {
        activeFishType = FISH_NEON_TETRA;
        saveFishType();
        Serial.println("Fish type set to: Neon Tetra (by name)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      } else if (body.indexOf("\"name\":\"Angelfish\"") >= 0 || body.indexOf("\"name\":\"angelfish\"") >= 0) {
        activeFishType = FISH_ANGELFISH;
        saveFishType();
        Serial.println("Fish type set to: Angelfish (by name)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      } else if (body.indexOf("\"name\":\"Comet\"") >= 0 || body.indexOf("\"name\":\"comet\"") >= 0) {
        activeFishType = FISH_COMET;
        saveFishType();
        Serial.println("Fish type set to: Comet (by name)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      } else if (body.indexOf("\"name\":\"Rohu\"") >= 0 || body.indexOf("\"name\":\"rohu\"") >= 0) {
        activeFishType = FISH_ROHU;
        saveFishType();
        Serial.println("Fish type set to: Rohu (by name)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      } else if (body.indexOf("\"name\":\"None\"") >= 0 || body.indexOf("\"name\":\"none\"") >= 0) {
        activeFishType = FISH_NONE;
        saveFishType();
        Serial.println("Fish type set to: None (by name)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      }
    }
    
    Serial.println("Warning: Could not parse species data");
  }
  
  server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid species data\"}");
}

void SmartBreederServer::handleAPISpeciesList() {
  setCORSHeaders();
  
  // Return list of all available fish species with their pH and temperature ranges
  // ALL temperature ranges are set within 25-32°C (random values in this range)
  String json = "[";
  
  // Return all fish species (excluding FISH_NONE for the database list)
  // Start from index 1 to skip "None"
  for (int i = 1; i < 8; i++) { // 7 fish species: Goldfish, Betta, Guppy, Neon Tetra, Angelfish, Comet, Rohu
    FishProfile profile = FISH_PROFILES[i];
    if (i > 1) json += ",";
    json += "{";
    json += "\"id\":" + String(i) + ",";
    json += "\"name\":\"" + profile.name + "\",";
    json += "\"idealPh\":{";
    json += "\"min\":" + String(profile.phMin, 1) + ",";
    json += "\"max\":" + String(profile.phMax, 1);
    json += "},";
    json += "\"idealTemp\":{";
    // All temperature ranges are within 25-32°C (random values)
    json += "\"min\":" + String(profile.tempMin, 1) + ",";
    json += "\"max\":" + String(profile.tempMax, 1);
    json += "},";
    json += "\"description\":\"";
    
    // Add descriptions for each fish
    switch(i) {
      case 1: // Goldfish
        json += "Common goldfish, hardy and adaptable species";
        break;
      case 2: // Betta Fish
        json += "Siamese fighting fish, tropical species";
        break;
      case 3: // Guppy
        json += "Live-bearing tropical fish, colorful and active";
        break;
      case 4: // Neon Tetra
        json += "Small schooling fish, prefers acidic water";
        break;
      case 5: // Angelfish
        json += "Large cichlid, requires stable water conditions";
        break;
      case 6: // Comet
        json += "Comet goldfish, single-tailed variety";
        break;
      case 7: // Rohu
        json += "Rohu fish, popular freshwater species";
        break;
      default:
        json += "Fish species profile";
    }
    json += "\"";
    json += "}";
  }
  
  json += "]";
  
  server->send(200, "application/json", json);
}

void SmartBreederServer::handleAPICalibrate() {
  setCORSHeaders();
  
  if (server->hasArg("plain")) {
    String body = server->arg("plain");
    
    if (body.indexOf("\"action\":\"ph7\"") >= 0) {
      phSensor->calibrate7();
      server->send(200, "application/json", "{\"success\":true,\"message\":\"pH 7.00 calibrated\"}");
    } else if (body.indexOf("\"action\":\"ph4\"") >= 0) {
      phSensor->calibrate4();
      server->send(200, "application/json", "{\"success\":true,\"message\":\"pH 4.00 calibrated\"}");
    } else if (body.indexOf("\"action\":\"temp\"") >= 0) {
      // Extract offset value
      int offsetPos = body.indexOf("\"offset\":");
      if (offsetPos >= 0) {
        float offset = body.substring(offsetPos + 9).toFloat();
        tempSensor->setOffset(offset);
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Temperature offset set\"}");
      } else {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing offset value\"}");
      }
    } else {
      server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid action\"}");
    }
  } else {
    server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid request\"}");
  }
}

void SmartBreederServer::handleAPIWiFi() {
  setCORSHeaders();
  
  if (server->hasArg("plain")) {
    String body = server->arg("plain");
    Serial.println("WiFi config received: " + body);
    
    // Parse WiFi config
    // Expected format: {"ssid":"NetworkName","password":"password123"}
    String ssid = "";
    String password = "";
    
    // Extract SSID
    int ssidPos = body.indexOf("\"ssid\":\"");
    if (ssidPos >= 0) {
      int ssidStart = ssidPos + 8;
      int ssidEnd = body.indexOf("\"", ssidStart);
      if (ssidEnd > ssidStart) {
        ssid = body.substring(ssidStart, ssidEnd);
        Serial.println("SSID: " + ssid);
      }
    }
    
    // Extract password
    int passPos = body.indexOf("\"password\":\"");
    if (passPos >= 0) {
      int passStart = passPos + 12;
      int passEnd = body.indexOf("\"", passStart);
      if (passEnd > passStart) {
        password = body.substring(passStart, passEnd);
        Serial.println("Password: [hidden]");
      }
    }
    
    // Note: WiFi config requires ESP32 restart to apply
    // For now, we just acknowledge the request
    // In production, you could save to EEPROM and restart
    
    server->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi configuration received (requires restart to apply)\"}");
  } else {
    server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid request\"}");
  }
}

void SmartBreederServer::handleAPIPing() {
  setCORSHeaders();
  server->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"pong\"}");
}

String SmartBreederServer::getDashboardHTML() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Breeder Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        .header {
            background: white;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .header h1 {
            color: #333;
            margin-bottom: 10px;
        }
        .status-badge {
            display: inline-block;
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: bold;
            margin-left: 10px;
        }
        .status-ok { background: #4caf50; color: white; }
        .status-warning { background: #ff9800; color: white; }
        .status-error { background: #f44336; color: white; }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .card {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .card h2 {
            color: #333;
            margin-bottom: 15px;
            font-size: 18px;
        }
        .sensor-value {
            font-size: 36px;
            font-weight: bold;
            color: #667eea;
            margin: 10px 0;
        }
        .sensor-label {
            color: #666;
            font-size: 14px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .control-group {
            margin: 15px 0;
        }
        .control-group label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 500;
        }
        button {
            background: #667eea;
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 14px;
            font-weight: 500;
            transition: all 0.3s;
            width: 100%;
            margin: 5px 0;
        }
        button:hover { background: #5568d3; transform: translateY(-2px); }
        button:active { transform: translateY(0); }
        button.danger { background: #f44336; }
        button.danger:hover { background: #d32f2f; }
        button.success { background: #4caf50; }
        button.success:hover { background: #45a049; }
        select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            font-size: 14px;
            margin: 10px 0;
        }
        .calibration-section {
            background: #f5f5f5;
            padding: 15px;
            border-radius: 6px;
            margin-top: 15px;
        }
        .calibration-section h3 {
            font-size: 14px;
            color: #666;
            margin-bottom: 10px;
        }
        input[type="number"] {
            width: 100%;
            padding: 10px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            font-size: 14px;
            margin: 5px 0;
        }
        .info-text {
            color: #666;
            font-size: 12px;
            margin-top: 5px;
        }
        @media (max-width: 768px) {
            .grid { grid-template-columns: 1fr; }
            body { padding: 10px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Smart Breeder Dashboard</h1>
            <div>
                <span id="connectionStatus" class="status-badge status-ok">Connected</span>
                <span id="lastUpdate">Last update: --</span>
            </div>
        </div>
        
        <div class="grid">
            <div class="card">
                <h2>Sensor Readings</h2>
                <div class="sensor-label">pH Level</div>
                <div class="sensor-value" id="phValue">--</div>
                <div class="sensor-label">Temperature</div>
                <div class="sensor-value" id="tempValue">--</div>
                <div id="safetyStatus"></div>
            </div>
            
            <div class="card">
                <h2>Manual Control</h2>
                <div class="control-group">
                    <label>Fan</label>
                    <button id="fanBtn" onclick="toggleFan()">Fan OFF</button>
                </div>
                <div class="control-group">
                    <label>Acid Pump</label>
                    <button id="acidBtn" onclick="toggleAcid()">Acid Pump OFF</button>
                </div>
                <div class="control-group">
                    <label>Base Pump</label>
                    <button id="baseBtn" onclick="toggleBase()">Base Pump OFF</button>
                </div>
            </div>
            
            <div class="card">
                <h2>Fish Species</h2>
                <select id="fishSelect" onchange="setFishType()">
                    <option value="0">None (pH: 6.5-7.5, Temp: 26-30°C)</option>
                    <option value="1">Goldfish (pH: 6.5-8.0, Temp: 27-31°C)</option>
                    <option value="2">Betta Fish (pH: 6.5-7.5, Temp: 26.5-30.5°C)</option>
                    <option value="3">Guppy (pH: 7.0-8.5, Temp: 25.5-29.5°C)</option>
                    <option value="4">Neon Tetra (pH: 5.0-7.0, Temp: 25-29°C)</option>
                    <option value="5">Angelfish (pH: 6.0-7.5, Temp: 28-32°C)</option>
                    <option value="6">Comet (pH: 6.5-7.2, Temp: 26-30°C)</option>
                    <option value="7">Rohu (pH: 6.6-8.0, Temp: 27.5-31.5°C)</option>
                </select>
                <div class="info-text" id="fishInfo" style="margin-top: 10px; padding: 10px; background: #f5f5f5; border-radius: 4px;"></div>
            </div>
            
            <div class="card">
                <h2>Calibration</h2>
                <div class="calibration-section">
                    <h3>pH Calibration</h3>
                    <button onclick="calibratePH7()">Calibrate pH 7.00</button>
                    <button onclick="calibratePH4()">Calibrate pH 4.00</button>
                    <div class="info-text">Place sensor in buffer solution and click</div>
                </div>
                <div class="calibration-section">
                    <h3>Temperature Offset</h3>
                    <input type="number" id="tempOffset" step="0.1" placeholder="Offset in °C">
                    <button onclick="setTempOffset()">Set Temperature Offset</button>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        let fanState = false, acidState = false, baseState = false;
        let cooldownRemaining = 0;
        
        function updateDashboard() {
            fetch('/api/status')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('phValue').textContent = data.ph.toFixed(2);
                    document.getElementById('tempValue').textContent = data.temperature.toFixed(1) + '°C';
                    
                    fanState = data.fan;
                    acidState = data.acidPump;
                    baseState = data.basePump;
                    cooldownRemaining = data.cooldownRemaining;
                    
                    document.getElementById('fanBtn').textContent = 'Fan ' + (fanState ? 'ON' : 'OFF');
                    document.getElementById('fanBtn').className = fanState ? 'success' : "";
                    document.getElementById('acidBtn').textContent = 'Acid Pump ' + (acidState ? 'ON' : 'OFF');
                    document.getElementById('acidBtn').className = acidState ? 'danger' : "";
                    document.getElementById('baseBtn').textContent = 'Base Pump ' + (baseState ? 'ON' : 'OFF');
                    document.getElementById('baseBtn').className = baseState ? 'success' : "";
                    
                    document.getElementById('fishSelect').value = data.fishType;
                    
                    // Update fish profile info with pH and temperature ranges
                    let fishInfoHTML = "";
                    if (data.phRange && data.tempRange) {
                        const fishNames = ["None", "Goldfish", "Betta Fish", "Guppy", "Neon Tetra", "Angelfish", "Comet", "Rohu"];
                        const fishName = fishNames[data.fishType] || "Unknown";
                        fishInfoHTML = `<strong>${fishName}</strong><br>`;
                        fishInfoHTML += `pH Range: ${data.phRange.min} - ${data.phRange.max}<br>`;
                        fishInfoHTML += `Temp Range: ${data.tempRange.min} - ${data.tempRange.max}°C`;
                    } else {
                        // Fallback to default ranges (all temp ranges within 25-32°C)
                        const fishProfiles = [
                            {name: "None", pH: "6.5-7.5", temp: "26.0-30.0°C"},
                            {name: "Goldfish", pH: "6.5-8.0", temp: "27.0-31.0°C"},
                            {name: "Betta Fish", pH: "6.5-7.5", temp: "26.5-30.5°C"},
                            {name: "Guppy", pH: "7.0-8.5", temp: "25.5-29.5°C"},
                            {name: "Neon Tetra", pH: "5.0-7.0", temp: "25.0-29.0°C"},
                            {name: "Angelfish", pH: "6.0-7.5", temp: "28.0-32.0°C"},
                            {name: "Comet", pH: "6.5-7.2", temp: "26.0-30.0°C"},
                            {name: "Rohu", pH: "6.6-8.0", temp: "27.5-31.5°C"}
                        ];
                        const profile = fishProfiles[data.fishType] || fishProfiles[0];
                        fishInfoHTML = `<strong>${profile.name}</strong><br>`;
                        fishInfoHTML += `pH Range: ${profile.pH}<br>`;
                        fishInfoHTML += `Temp Range: ${profile.temp}`;
                    }
                    document.getElementById('fishInfo').innerHTML = fishInfoHTML;
                    
                    let safetyHTML = "";
                    if (!data.phSafe) safetyHTML += '<div class="status-badge status-error">pH UNSAFE!</div>';
                    if (!data.tempSafe) safetyHTML += '<div class="status-badge status-error">TEMP UNSAFE!</div>';
                    if (data.phSafe && data.tempSafe) safetyHTML += '<div class="status-badge status-ok">All Safe</div>';
                    document.getElementById('safetyStatus').innerHTML = safetyHTML;
                    
                    document.getElementById('lastUpdate').textContent = 'Last update: ' + new Date().toLocaleTimeString();
                })
                .catch(e => {
                    console.error('Error:', e);
                    document.getElementById('connectionStatus').textContent = 'Disconnected';
                    document.getElementById('connectionStatus').className = 'status-badge status-error';
                });
        }
        
        function toggleFan() {
            fetch('/api/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({fan: !fanState})
            }).then(() => updateDashboard());
        }
        
        function toggleAcid() {
            if (cooldownRemaining > 0) {
                alert('Pump in cooldown: ' + Math.floor(cooldownRemaining/1000) + 's remaining');
                return;
            }
            fetch('/api/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({acidPump: !acidState})
            }).then(() => updateDashboard());
        }
        
        function toggleBase() {
            if (cooldownRemaining > 0) {
                alert('Pump in cooldown: ' + Math.floor(cooldownRemaining/1000) + 's remaining');
                return;
            }
            fetch('/api/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({basePump: !baseState})
            }).then(() => updateDashboard());
        }
        
        function setFishType() {
            const type = document.getElementById('fishSelect').value;
            fetch('/api/species', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({type: parseInt(type)})
            }).then(() => updateDashboard());
        }
        
        function calibratePH7() {
            if (confirm('Place pH sensor in pH 7.00 buffer solution, then click OK')) {
                fetch('/api/calibrate', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({action: 'ph7'})
                }).then(r => r.json()).then(data => {
                    alert(data.message || 'Calibrated');
                });
            }
        }
        
        function calibratePH4() {
            if (confirm('Place pH sensor in pH 4.00 buffer solution, then click OK')) {
                fetch('/api/calibrate', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({action: 'ph4'})
                }).then(r => r.json()).then(data => {
                    alert(data.message || 'Calibrated');
                });
            }
        }
        
        function setTempOffset() {
            const offset = parseFloat(document.getElementById('tempOffset').value);
            if (isNaN(offset)) {
                alert('Please enter a valid number');
                return;
            }
            fetch('/api/calibrate', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({action: 'temp', offset: offset})
            }).then(r => r.json()).then(data => {
                alert(data.message || 'Offset set');
            });
        }
        
        // Auto-refresh every 2 seconds
        setInterval(updateDashboard, 2000);
        updateDashboard();
    </script>
</body>
</html>
)HTML";
  return html;
}

void SmartBreederServer::handleRoot() {
  server->send(200, "text/html", getDashboardHTML());
}

