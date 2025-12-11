# 📤 Upload Instructions - Which Code to Upload

## ✅ **Upload the NEW Modular Firmware**

You need to upload the **`SmartBreeder_Firmware`** folder, NOT the old `SmartBreeder.ino` file.

## 📁 Project Structure

```
SmartBreeder_Firmware/          ← Upload THIS entire folder
├── SmartBreeder.ino            ← Main file (Arduino IDE opens this)
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

## 🚀 Step-by-Step Upload Process

### Method 1: Direct Upload (Recommended)

1. **Open Arduino IDE**

2. **File → Open**
   - Navigate to: `SmartBreeder_Firmware/SmartBreeder.ino`
   - Click Open
   - Arduino IDE will automatically load all related files

3. **Install Required Libraries** (if not already installed)
   - Go to: **Sketch → Include Library → Manage Libraries**
   - Search and install:
     - **LiquidCrystal_I2C** by Frank de Brabander
     - **OneWire** by Paul Stoffregen
     - **DallasTemperature** by Miles Burton

4. **Select Board**
   - **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
   - **Tools → CPU Frequency → 240MHz**
   - **Tools → Flash Size → 4MB (32Mb)**
   - **Tools → Partition Scheme → Default 4MB with spiffs**

5. **Select Port**
   - **Tools → Port → COMx** (your ESP32 port)

6. **Configure WiFi** (IMPORTANT!)
   - Open: `SmartBreeder_Firmware/config/config.h`
   - Edit lines 20-21:
     ```cpp
     const char* WIFI_SSID = "YourWiFiName";      // ← Change this
     const char* WIFI_PASS = "YourPassword";       // ← Change this
     ```
   - Save the file

7. **Upload**
   - Click **Upload** button (→) or press **Ctrl+U**
   - Wait for compilation and upload to complete
   - Check Serial Monitor (115200 baud) for IP address

### Method 2: Copy to Arduino Sketchbook

1. **Find Arduino Sketchbook Location**
   - **File → Preferences → Sketchbook location**
   - Usually: `C:\Users\YourName\Documents\Arduino\`

2. **Copy Entire Folder**
   - Copy `SmartBreeder_Firmware` folder
   - Paste into your Arduino sketchbook folder
   - Rename if needed (Arduino doesn't like spaces/special chars)

3. **Open in Arduino IDE**
   - **File → Open**
   - Navigate to: `Arduino/SmartBreeder_Firmware/SmartBreeder.ino`
   - Follow steps 3-7 from Method 1

## ⚙️ Pre-Upload Checklist

- [ ] Arduino IDE installed (1.8.19 or later)
- [ ] ESP32 board support installed
- [ ] Required libraries installed
- [ ] WiFi credentials configured in `config/config.h`
- [ ] ESP32 connected via USB
- [ ] Correct COM port selected
- [ ] Correct board selected (ESP32 Dev Module)

## 🔍 Verify Upload Success

After upload, open **Serial Monitor** (115200 baud):

**Expected Output:**
```
========================================
   Smart Breeder - Starting System
========================================

LCD initialized
pH Sensor initialized
Temperature sensor initialized
Fan control initialized
pH Control initialized
Connecting to WiFi........
WiFi Connected!
IP Address: 192.168.1.105

Web server started
API Endpoints:
  GET  /api/status  - Get sensor data
  POST /api/control - Control relays
  POST /api/species - Configure fish species
  POST /api/wifi    - Configure WiFi
  GET  /api/ping    - Test connection

========================================
   System Ready!
========================================

Dashboard: http://192.168.1.105
Or: http://smartbreeder.local
```

## ❌ Common Issues

### "File not found" errors
- **Problem:** Arduino IDE can't find header files
- **Solution:** Make sure you opened `SmartBreeder.ino` from INSIDE the `SmartBreeder_Firmware` folder
- All `.h` and `.cpp` files must be in their respective subfolders

### Compilation errors
- **Problem:** Missing libraries
- **Solution:** Install all required libraries (see step 3 above)

### Upload fails
- **Problem:** Wrong port or board selected
- **Solution:** Check Tools → Port and Tools → Board

### WiFi connection fails
- **Problem:** Wrong credentials
- **Solution:** Double-check `config/config.h` WiFi settings
- Make sure it's 2.4GHz network (ESP32 doesn't support 5GHz)

## 📝 Important Notes

1. **DO NOT upload the old `SmartBreeder.ino`** (in root folder)
   - That's the old single-file version
   - Use the NEW modular version in `SmartBreeder_Firmware/`

2. **All files must stay together**
   - Don't move individual files
   - Keep the folder structure intact

3. **First upload takes longer**
   - ESP32 needs to partition flash
   - Be patient during first upload

4. **Save your WiFi credentials**
   - They're stored in `config/config.h`
   - You'll need to re-upload if you change WiFi

## 🎯 Quick Reference

**Main File:** `SmartBreeder_Firmware/SmartBreeder.ino`

**Configuration:** `SmartBreeder_Firmware/config/config.h`

**Upload Button:** → (Arrow icon in Arduino IDE)

**Serial Monitor:** Tools → Serial Monitor (115200 baud)

---

**Ready to upload?** Follow Method 1 above and you'll be set! 🚀

