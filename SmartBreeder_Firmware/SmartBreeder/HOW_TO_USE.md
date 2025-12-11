# ✅ How to Upload and Use - FIXED VERSION

## 📁 **Correct Folder Structure**

All files are now in: `SmartBreeder_Firmware/SmartBreeder/`

```
SmartBreeder/
├── SmartBreeder.ino       ← Main file (OPEN THIS)
├── config/
│   ├── config.h
│   └── config.cpp
├── sensors/
│   ├── ph.h / ph.cpp
│   └── temp.h / temp.cpp
├── control/
│   ├── fan.h / fan.cpp
│   ├── phControl.h / phControl.cpp
│   └── autoControl.h / autoControl.cpp
├── ui/
│   └── lcd.h / lcd.cpp
└── wifi/
    └── server.h / server.cpp
```

## 🚀 **Step-by-Step Upload**

### 1. **Open in Arduino IDE**
   - File → Open
   - Navigate to: `SmartBreeder_Firmware/SmartBreeder/SmartBreeder.ino`
   - Click Open

### 2. **Install Libraries** (if not installed)
   - Sketch → Include Library → Manage Libraries
   - Install:
     - **LiquidCrystal_I2C** by Frank de Brabander
     - **OneWire** by Paul Stoffregen  
     - **DallasTemperature** by Miles Burton

### 3. **Select Board**
   - Tools → Board → ESP32 Arduino → **ESP32 Dev Module**
   - Tools → CPU Frequency → **240MHz**
   - Tools → Flash Size → **4MB (32Mb)**
   - Tools → Partition Scheme → **Default 4MB with spiffs**

### 4. **Configure WiFi**
   - Open: `SmartBreeder/config/config.h`
   - Edit lines 24-25:
     ```cpp
     const char* WIFI_SSID = "YourWiFiName";
     const char* WIFI_PASS = "YourPassword";
     ```
   - Save

### 5. **Select Port**
   - Tools → Port → **COMx** (your ESP32 port)

### 6. **Compile & Upload**
   - Click **Verify (✓)** to compile
   - If successful, click **Upload (→)**
   - Wait for upload to complete

### 7. **Check Serial Monitor**
   - Tools → Serial Monitor
   - Set baud rate to **115200**
   - You should see:
     ```
     ========================================
        Smart Breeder - Starting System
     ========================================
     
     WiFi Connected!
     IP Address: 192.168.1.105
     
     Dashboard: http://192.168.1.105
     ```

## 🔧 **After Upload**

### 1. **Get ESP32 IP Address**
   - Check Serial Monitor
   - Note the IP address (e.g., `192.168.1.105`)

### 2. **Update Dashboard**
   - Open: `src/services/apiService.js`
   - Change line 6:
     ```javascript
     baseURL: 'http://192.168.1.105', // ← Your ESP32 IP
     ```

### 3. **Start Dashboard**
   ```bash
   npm run dev
   ```

### 4. **Access Dashboard**
   - Open browser: `http://localhost:5173` (or your Vite port)
   - Dashboard will automatically connect to ESP32

## ✅ **Verification**

**Check these work:**
- [ ] Serial Monitor shows "System Ready"
- [ ] IP address displayed
- [ ] Dashboard connects (no errors in browser console)
- [ ] Sensor readings appear (pH, temperature)
- [ ] Manual controls work (fan, pumps)
- [ ] Species selection works

## 🐛 **If Compilation Fails**

1. **Close and reopen Arduino IDE**
2. **Verify all folders are in `SmartBreeder/` folder**
3. **Check all include paths use `"folder/file.h"` format**
4. **Make sure libraries are installed**

## 📝 **Important Notes**

- **All files must be in `SmartBreeder/` folder**
- **Don't move files individually**
- **WiFi credentials in `config/config.h`**
- **LiquidCrystal I2C warning is safe to ignore**

---

**You're all set!** Upload the code and start monitoring your fish breeding system! 🐟

