# Troubleshooting Connection Issues

## Error: `ERR_CONNECTION_TIMED_OUT`

This error means your browser/React app cannot reach the ESP32 at `http://192.168.0.11`.

### Step 1: Check ESP32 Serial Monitor

1. Open Arduino IDE
2. Connect ESP32 via USB
3. Open Serial Monitor (Tools → Serial Monitor)
4. Set baud rate to **115200**
5. Look for these messages:
   ```
   WiFi Connected!
   IP Address: 192.168.x.x
   Web server started
   ```

**Important:** The IP address shown in Serial Monitor is the **actual IP** of your ESP32. It might NOT be `192.168.0.11`.

### Step 2: Update IP Address in Dashboard

If the Serial Monitor shows a different IP (e.g., `192.168.1.105`):

1. Open `src/services/apiService.js`
2. Change line 6:
   ```javascript
   baseURL: 'http://192.168.1.105', // Use the IP from Serial Monitor
   ```
3. Save and refresh your React app

### Step 3: Verify WiFi Connection

**Check in Serial Monitor:**
- ✅ `WiFi Connected!` → ESP32 is connected
- ❌ `WiFi connection failed!` → Check WiFi credentials

**WiFi Credentials Location:**
- File: `SmartBreeder_Firmware/SmartBreeder/config/config.h`
- Lines:
  ```cpp
  const char* WIFI_SSID = "West zone";      // Your WiFi name
  const char* WIFI_PASS = "sunnyafa@jonny"; // Your WiFi password
  ```

**Update if needed:**
1. Edit `config/config.h`
2. Change `WIFI_SSID` and `WIFI_PASS`
3. Re-upload firmware to ESP32

### Step 4: Test Connection

**Method 1: Browser Test**
1. Open browser
2. Go to: `http://192.168.x.x` (use actual IP from Serial Monitor)
3. You should see the Smart Breeder Dashboard
4. If it works, the IP is correct

**Method 2: Ping Test**
1. Open Command Prompt (Windows) or Terminal (Mac/Linux)
2. Run: `ping 192.168.x.x` (use actual IP)
3. If you get replies, ESP32 is reachable
4. If timeout, ESP32 is not on the network

**Method 3: Serial Monitor API Test**
- Watch Serial Monitor while dashboard tries to connect
- Look for: `GET /api/status` messages
- If you see these, ESP32 is receiving requests

### Step 5: Network Issues

**Common Problems:**

1. **ESP32 and Computer on Different Networks**
   - Both must be on the same WiFi network
   - Check your computer's WiFi network name

2. **Firewall Blocking Connection**
   - Windows Firewall might block ESP32
   - Try disabling firewall temporarily to test

3. **Router Blocking Device Communication**
   - Some routers block device-to-device communication
   - Check router settings for "AP Isolation" or "Client Isolation"
   - Disable if enabled

4. **ESP32 Not Powered**
   - Ensure ESP32 is connected via USB and powered
   - Check USB cable (data cable, not just charging)

5. **Wrong Port/Protocol**
   - ESP32 uses HTTP (port 80), not HTTPS
   - Use `http://` not `https://`
   - Don't add port number (e.g., `:80`)

### Step 6: Dynamic IP Address Solution

ESP32 IP address can change when it reconnects. Solutions:

**Option A: Use mDNS (Recommended)**
- ESP32 is configured with mDNS name: `smartbreeder.local`
- Update `apiService.js`:
  ```javascript
  baseURL: 'http://smartbreeder.local',
  ```
- **Note:** mDNS might not work on all networks/browsers

**Option B: Static IP (Advanced)**
- Configure ESP32 with static IP in `wifi/server.cpp`
- Add after `WiFi.begin()`:
  ```cpp
  IPAddress local_IP(192, 168, 0, 11);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);
  ```

**Option C: IP Configuration in Dashboard**
- Add IP input field in React dashboard
- Use `apiService.updateBaseURL(newIP)` to change IP dynamically

### Step 7: Verify Firmware is Uploaded

1. Check Serial Monitor for startup messages:
   ```
   ========================================
      Smart Breeder - Starting System
   ========================================
   ```
2. If you don't see these messages, firmware is not uploaded
3. Upload firmware: Sketch → Upload

### Quick Checklist

- [ ] ESP32 is powered and connected via USB
- [ ] Serial Monitor shows "WiFi Connected!"
- [ ] IP address in Serial Monitor matches `apiService.js`
- [ ] Computer and ESP32 are on same WiFi network
- [ ] Browser can access `http://[ESP32_IP]`
- [ ] Firewall is not blocking connection
- [ ] Using `http://` not `https://`
- [ ] Firmware is uploaded and running

### Still Not Working?

1. **Reset ESP32:**
   - Press RESET button on ESP32
   - Watch Serial Monitor for new IP address

2. **Check Serial Monitor for Errors:**
   - Look for error messages
   - Sensor initialization errors
   - WiFi connection errors

3. **Test with Simple HTTP Request:**
   ```bash
   curl http://192.168.x.x/api/ping
   ```
   Should return: `{"status":"ok","message":"pong"}`

4. **Check ESP32 Hardware:**
   - Try different USB port
   - Try different USB cable
   - Check if ESP32 LED is blinking (indicates activity)

---

## Need More Help?

1. Share Serial Monitor output
2. Share actual ESP32 IP address
3. Share your network configuration
4. Check if other devices can reach ESP32

