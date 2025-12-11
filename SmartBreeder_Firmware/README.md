# Smart Breeder - Production Ready Firmware

Complete modular ESP32 firmware for automated fish breeding system with calibration, safety features, and web dashboard.

## 📁 Project Structure

```
SmartBreeder_Firmware/
├── config/
│   ├── config.h          # Pin definitions, constants, fish profiles
│   └── config.cpp         # EEPROM/Preferences storage functions
├── sensors/
│   ├── ph.h / ph.cpp      # pH sensor with calibration
│   └── temp.h / temp.cpp # Temperature sensor with offset
├── control/
│   ├── fan.h / fan.cpp           # Fan control with safety
│   ├── phControl.h / phControl.cpp # pH pump control with timers
│   └── autoControl.h / autoControl.cpp # Auto-control logic
├── ui/
│   └── lcd.h / lcd.cpp    # LCD display with cycling pages
├── wifi/
│   └── server.h / server.cpp # Web server and dashboard
└── SmartBreeder.ino       # Main program
```

## 🔌 Hardware Connections

### Pin Mapping
```
ESP32 GPIO    Component
---------     ---------
21            LCD SDA (I2C)
22            LCD SCL (I2C)
27            DS18B20 Data
35            pH Sensor (ADC)
18            Fan Relay (Active-Low)
23            Acid Pump Relay (Active-Low)
19            Base Pump Relay (Active-Low)
```

### Wiring Diagram

```
ESP32 Dev Module
    |
    +-- GPIO 21 (SDA) ----[4.7kΩ]---- LCD I2C SDA
    |                              |
    +-- GPIO 22 (SCL) ----[4.7kΩ]---- LCD I2C SCL
    |
    +-- GPIO 27 -------------------- DS18B20 Data
    |                              |
    |                              +-- 4.7kΩ pull-up to 3.3V
    |
    +-- GPIO 35 (ADC) ------------- pH Sensor Signal
    |
    +-- GPIO 18 ------------------- Fan Relay IN
    +-- GPIO 23 ------------------- Acid Pump Relay IN
    +-- GPIO 19 ------------------- Base Pump Relay IN
```

**Relay Module:**
- Active-Low relays (LOW = ON, HIGH = OFF)
- Common GND with ESP32
- 5V or 3.3V power (check relay module specs)

**DS18B20:**
- VCC → 3.3V
- GND → GND
- Data → GPIO 27 (with 4.7kΩ pull-up to 3.3V)

**pH Sensor:**
- VCC → 3.3V
- GND → GND
- Signal → GPIO 35

**LCD I2C:**
- VCC → 5V (or 3.3V if module supports)
- GND → GND
- SDA → GPIO 21
- SCL → GPIO 22

## 📦 Required Libraries

Install via Arduino Library Manager:

1. **LiquidCrystal_I2C** by Frank de Brabander
2. **OneWire** by Paul Stoffregen
3. **DallasTemperature** by Miles Burton

## 🔧 Compilation Instructions

1. **Install Arduino IDE** (1.8.19 or later) or **PlatformIO**

2. **Install ESP32 Board Support:**
   - Arduino IDE: File → Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install

3. **Select Board:**
   - Tools → Board → ESP32 Arduino → ESP32 Dev Module
   - CPU Frequency: 240MHz
   - Flash Size: 4MB
   - Partition Scheme: Default 4MB with spiffs

4. **Configure WiFi:**
   - Edit `config/config.h`
   - Update `WIFI_SSID` and `WIFI_PASS`

5. **Upload:**
   - Connect ESP32 via USB
   - Select correct COM port
   - Click Upload

## 🎯 Calibration Guide

### pH Sensor Calibration

1. **pH 7.00 Calibration:**
   - Prepare pH 7.00 buffer solution
   - Immerse pH sensor in buffer
   - Wait 30 seconds for stabilization
   - Open web dashboard: `http://ESP32_IP`
   - Click "Calibrate pH 7.00"
   - Or via API: `POST /api/calibrate` with `{"action":"ph7"}`

2. **pH 4.00 Calibration:**
   - Prepare pH 4.00 buffer solution
   - Immerse pH sensor in buffer
   - Wait 30 seconds
   - Click "Calibrate pH 4.00"
   - Or via API: `POST /api/calibrate` with `{"action":"ph4"}`

3. **Calibration Storage:**
   - Values saved to EEPROM automatically
   - Persists across reboots

### Temperature Offset

1. **Measure Actual Temperature:**
   - Use calibrated thermometer
   - Compare with sensor reading

2. **Set Offset:**
   - Web dashboard: Enter offset in "Temperature Offset" field
   - Click "Set Temperature Offset"
   - Or via API: `POST /api/calibrate` with `{"action":"temp","offset":1.5}`

## 🌐 Web Dashboard Usage

### Access Dashboard

1. **Find ESP32 IP:**
   - Check Serial Monitor (115200 baud)
   - Or check LCD display (WiFi IP page)

2. **Open Browser:**
   - Navigate to: `http://ESP32_IP`
   - Or: `http://smartbreeder.local` (if mDNS works)

### Dashboard Features

**Sensor Readings:**
- Real-time pH and temperature
- Auto-refreshes every 2 seconds
- Safety status indicators

**Manual Control:**
- Fan ON/OFF toggle
- Acid Pump control (with cooldown protection)
- Base Pump control (with cooldown protection)

**Fish Profile Selection:**
- Select from dropdown: None, Gold Fish, Comet, Rohu
- Auto-control adjusts based on selected profile

**Calibration:**
- pH 7.00 calibration button
- pH 4.00 calibration button
- Temperature offset input

### API Endpoints

**GET /api/status**
- Returns JSON with all sensor data and states

**POST /api/control**
- Control relays: `{"fan":true,"acidPump":false,"basePump":false}`

**POST /api/species**
- Set fish type: `{"type":1}` (0=None, 1=Gold, 2=Comet, 3=Rohu)

**POST /api/calibrate**
- Calibrate sensors: `{"action":"ph7"}` or `{"action":"ph4"}` or `{"action":"temp","offset":1.5}`

**GET /api/ping**
- Connection test

## 🐠 Fish Profiles

| Fish Type | pH Range | Temperature Range |
|-----------|----------|-------------------|
| Gold Fish | 6.8 - 7.5 | 24°C - 28°C |
| Comet     | 6.5 - 7.2 | 23°C - 27°C |
| Rohu      | 6.6 - 8.0 | 24°C - 32°C |

Auto-control maintains these ranges when a profile is active.

## 🛡️ Safety Features

1. **Pump Safety:**
   - Maximum ON duration: 3 seconds
   - 5-minute cooldown between doses
   - Emergency stop if pH < 5.5 or pH > 9.0

2. **Temperature Safety:**
   - Emergency fan ON if temp > 40°C
   - Auto-control based on fish profile

3. **Fan Protection:**
   - Minimum 10 seconds between toggles
   - Prevents rapid cycling

4. **Manual Override:**
   - Manual commands protected for 30 seconds
   - Auto-control respects manual settings

## 📱 LCD Display

LCD cycles through 5 pages (5 seconds each):

1. **Readings:** pH and Temperature with states
2. **Fish Type:** Current active profile
3. **Status:** Relay states and pH
4. **Dosing Timer:** Cooldown countdown
5. **WiFi IP:** Network address

**Startup Animation:**
- Shows initialization sequence
- Displays connection status

## 🔍 Troubleshooting

**WiFi Not Connecting:**
- Check SSID and password in `config/config.h`
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- Check Serial Monitor for connection status

**Sensors Not Reading:**
- Verify wiring connections
- Check power supply (3.3V stable)
- For DS18B20: Ensure pull-up resistor (4.7kΩ)
- For pH: Check ADC pin (GPIO 35)

**Relays Not Working:**
- Verify Active-Low setting in `config.h`
- Check relay module power supply
- Test with manual control via dashboard

**Calibration Issues:**
- Use fresh buffer solutions
- Wait 30+ seconds for stabilization
- Check Serial Monitor for calibration values

## 📝 Serial Monitor

Monitor at **115200 baud** for:
- System initialization
- Sensor readings
- Control actions
- Error messages
- WiFi connection status

## 🚀 Production Deployment

1. **Test all features** in development
2. **Calibrate sensors** properly
3. **Set correct fish profile**
4. **Verify WiFi connection**
5. **Monitor for 24 hours** before leaving unattended
6. **Set up alerts** for emergency conditions

## 📄 License

This firmware is provided as-is for educational and commercial use.

## 👨‍💻 Author

MD. NAIM ISLAM
Smart Fish Breeding Automation System

---

**Ready to Flash!** Upload `SmartBreeder.ino` to your ESP32 and start monitoring your fish breeding system.

