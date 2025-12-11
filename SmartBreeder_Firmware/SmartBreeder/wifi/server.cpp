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
    
    if (MDNS.begin("smartbreeder")) {
      Serial.println("mDNS started: http://smartbreeder.local");
    }
    
    // Setup routes
    server->on("/", HTTP_GET, [this]() { handleRoot(); });
    server->on("/api/status", HTTP_GET, [this]() { handleAPIStatus(); });
    server->on("/api/control", HTTP_POST, [this]() { handleAPIControl(); });
    server->on("/api/species", HTTP_POST, [this]() { handleAPISpecies(); });
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
    
    // Apply commands with manual override
    if (fanSet) {
      fanControl->set(fanVal, true);
      Serial.printf("Fan %s (manual)\n", fanVal ? "ON" : "OFF");
    }
    if (acidSet) {
      phControl->setAcid(acidVal, true);
      Serial.printf("Acid pump %s (manual)\n", acidVal ? "ON" : "OFF");
    }
    if (baseSet) {
      phControl->setBase(baseVal, true);
      Serial.printf("Base pump %s (manual)\n", baseVal ? "ON" : "OFF");
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
    
    // Try to parse by type number first (simple format: {"type":1})
    if (body.indexOf("\"type\"") >= 0) {
      int typePos = body.indexOf("\"type\":");
      if (typePos >= 0) {
        int typeValue = body.substring(typePos + 7).toInt();
        if (typeValue >= 0 && typeValue <= 3) {
          activeFishType = (FishType)typeValue;
          saveFishType();
          Serial.printf("Fish type set to: %s (by type number)\n", FISH_PROFILES[activeFishType].name.c_str());
          server->send(200, "application/json", "{\"success\":true}");
          return;
        }
      }
    }
    
    // Parse by name (React dashboard format)
    // Format: {"name":"Goldfish","idealPh":{"min":6.5,"max":8.0},"idealTemp":{"min":18,"max":24}}
    if (body.indexOf("\"name\"") >= 0) {
      // Match fish name to type (case-insensitive matching)
      // Check multiple case variations
      if (body.indexOf("\"name\":\"Goldfish\"") >= 0 || body.indexOf("\"name\":\"goldfish\"") >= 0 ||
          body.indexOf("\"name\":\"Gold Fish\"") >= 0 || body.indexOf("\"name\":\"gold fish\"") >= 0) {
        activeFishType = FISH_GOLD;
        saveFishType();
        Serial.println("Fish type set to: Gold Fish (by name)");
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
      
      // Try matching other common fish names from dashboard
      if (body.indexOf("\"name\":\"Betta\"") >= 0 || body.indexOf("\"name\":\"betta\"") >= 0 ||
          body.indexOf("\"name\":\"Betta Fish\"") >= 0 || body.indexOf("\"name\":\"betta fish\"") >= 0) {
        // Betta not in firmware profiles, but accept it and use closest match (Gold)
        activeFishType = FISH_GOLD;
        saveFishType();
        Serial.println("Fish type set to: Gold Fish (Betta mapped)");
        server->send(200, "application/json", "{\"success\":true}");
        return;
      }
    }
    
    Serial.println("Warning: Could not parse species data");
  }
  
  server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid species data\"}");
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
                <h2>Fish Profile</h2>
                <select id="fishSelect" onchange="setFishType()">
                    <option value="0">None</option>
                    <option value="1">Gold Fish</option>
                    <option value="2">Comet</option>
                    <option value="3">Rohu</option>
                </select>
                <div class="info-text" id="fishInfo"></div>
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

