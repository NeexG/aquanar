# ESP32 & Website API Integration Guide

## ✅ Perfect Match - ESP32 Code Now Compatible with Your Website

The ESP32 code has been **fully updated** to match your React website's API expectations.

## 🔌 API Endpoints Mapping

### 1. **GET /api/status** - Device Sensor Data
**Website expects:**
```json
{
  "ph": 7.2,
  "temperature": 24.5,
  "fan": true,
  "acidPump": false,
  "basePump": false
}
```

**ESP32 returns:** ✅ Exact match
- `ph` - Current pH reading (float, 2 decimals)
- `temperature` - Current temperature in Celsius (float, 2 decimals)
- `fan` - Cooler fan relay state (boolean)
- `acidPump` - Acid pump relay state (boolean)
- `basePump` - Alkali/base pump relay state (boolean)

### 2. **POST /api/control** - Control Relays
**Website sends:**
```json
{
  "fan": true,
  "acidPump": false,
  "basePump": false
}
```

**ESP32 handles:** ✅ Exact match
- Accepts JSON body with any combination of `fan`, `acidPump`, `basePump`
- Updates relay states accordingly
- Returns: `{"success":true,"message":"Control command executed"}`

**Relay Mapping:**
- `fan` → REL_COOLER_FAN (GPIO 18)
- `acidPump` → REL_ACID_PUMP (GPIO 23)
- `basePump` → REL_ALKALI_PUMP (GPIO 19)

### 3. **POST /api/species** - Configure Fish Species
**Website sends:**
```json
{
  "name": "Goldfish",
  "idealPhMin": 6.5,
  "idealPhMax": 8.0,
  "idealTempMin": 18,
  "idealTempMax": 24
}
```

**ESP32 handles:** ✅ Compatible
- Parses fish name and updates active fish mode
- Updates auto-control logic based on species
- Returns: `{"success":true,"message":"Species configuration updated"}`

**Supported Fish Names:**
- "Goldfish" or "Gold Fish" → FISH_GOLD
- "Comet" → FISH_COMET
- "Rohu" → FISH_ROHU

### 4. **POST /api/wifi** - WiFi Configuration
**Website sends:**
```json
{
  "ssid": "NetworkName",
  "password": "password123"
}
```

**ESP32 handles:** ✅ Compatible
- Receives WiFi config (requires restart to apply)
- Returns: `{"success":true,"message":"WiFi configuration received (requires restart to apply)"}`

### 5. **GET /api/ping** - Connection Test
**Website expects:** Simple connection test

**ESP32 returns:** ✅ Exact match
```json
{
  "status": "ok",
  "message": "pong"
}
```

## 🌐 CORS Support

All API endpoints include CORS headers:
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, OPTIONS`
- `Access-Control-Allow-Headers: Content-Type`

This allows your React website to make cross-origin requests without issues.

## 📊 Data Flow

### Dashboard Page
1. Website calls `GET /api/status` every 5 seconds (configurable)
2. ESP32 returns current sensor readings and relay states
3. Website displays:
   - pH level with progress bar
   - Temperature with progress bar
   - Fan status (ON/OFF)
   - Pumps status (Acid/Base)

### Device Control Page
1. User toggles switch for fan/acidPump/basePump
2. Website sends `POST /api/control` with updated state
3. ESP32 updates relay immediately
4. Website refreshes status to show new state

### Settings Page
1. User selects fish species
2. Website sends `POST /api/species` with species config
3. ESP32 updates active fish mode and auto-control thresholds
4. Auto-control logic adjusts based on new species

## 🔧 Configuration

### ESP32 IP Address
Update in `src/services/apiService.js`:
```javascript
baseURL: 'http://192.168.1.105', // Change to your ESP32 IP
```

### Update Interval
Configure in website settings (default: 5000ms = 5 seconds)

## ✅ Testing Checklist

- [x] `/api/status` returns correct JSON format
- [x] `/api/control` accepts and processes JSON commands
- [x] `/api/species` updates fish configuration
- [x] `/api/wifi` accepts WiFi config
- [x] `/api/ping` responds to connection test
- [x] CORS headers included on all endpoints
- [x] OPTIONS requests handled for CORS preflight
- [x] JSON parsing handles both true/false values
- [x] Case-insensitive JSON field matching

## 🚀 Ready to Use

Your ESP32 code is now **100% compatible** with your React website. Simply:

1. Upload `SmartBreeder.ino` to your ESP32
2. Update the IP address in `apiService.js` to match your ESP32's IP
3. Connect both devices to the same WiFi network
4. Open your website - it will automatically connect and display sensor data!

## 📝 Notes

- The ESP32 also has a built-in web dashboard at `http://ESP32_IP/` for direct access
- Legacy endpoints (`/relay`, `/fish`, `/api`) are still available for backward compatibility
- All relay states are updated in real-time
- Auto-control logic continues to work independently of website commands

