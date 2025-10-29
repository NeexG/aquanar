# Smart Breeder Control Panel

A professional web dashboard for monitoring and controlling the Autocratic Fish Breeding Machine (AFBM) using ESP32 microcontroller.

## 🐠 Features

- **Real-time Dashboard**: Live monitoring of pH, temperature, fan, and pump status
- **Device Control**: Manual control of fan and acid/base pumps with emergency stop
- **Fish Species Database**: Pre-configured species with ideal pH and temperature ranges
- **Settings Management**: Wi-Fi configuration and system preferences
- **Responsive Design**: Works on desktop, tablet, and mobile devices
- **Aqua Blue Theme**: Professional aquatic-themed UI

## 🛠️ Tech Stack

- **Frontend**: React + Vite + JavaScript
- **UI Framework**: Ant Design
- **State Management**: Redux Toolkit
- **Styling**: Tailwind CSS + Custom CSS
- **API Communication**: Axios
- **Data Persistence**: Redux Persist

## 📡 ESP32 Integration

The dashboard communicates with ESP32 via REST API endpoints:

- `GET /api/status` - Get sensor data (pH, temperature, device status)
- `POST /api/control` - Send control commands (fan, pumps)
- `POST /api/species` - Configure fish species settings
- `POST /api/wifi` - Configure Wi-Fi settings
- `GET /api/ping` - Test connection

### ESP32 Pin Configuration

gi t- **pH Sensor**: GPIO 34 (Analog A3)
- **Temperature Sensor (DS18B20)**: GPIO 12 (Digital Pin 12)
- **Fan Relay**: GPIO 3 (Digital Pin 3)
- **Acid Pump Relay**: GPIO 6 (Digital Pin 6)
- **Alkaline Pump Relay**: GPIO 7 (Digital Pin 7)
- **LCD I2C**: SDA - A4, SCL - A5

## 🔌 Arduino/ESP32 Firmware Code

The following code should be uploaded to your Arduino/ESP32 device for the fish breeding machine controller:

```cpp
//SDA - A4 (Red)
//SCL - A5 (Yellow)
// pH Sensor - A3 (Analog pin)
// Temperature Sensor - Digital Pin 12
// Exhaust Fan - Digital Pin 3 (Relay)
// Acid Pump - Digital Pin 6 (Relay)
// Alkaline Pump - Digital Pin 7 (Relay)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // 16x2 LCD

#define PH_SENSOR_PIN A3
#define PH_OFFSET 0.50  // pH ক্যালিব্রেশন মান

#define TEMP_SENSOR_PIN 12
#define FAN_RELAY_PIN 3
#define ACID_PUMP_RELAY 6
#define BASE_PUMP_RELAY 7

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// সময় ট্র্যাকিংয়ের জন্য ভ্যারিয়েবল
unsigned long lastStartupDisplay = 0;
unsigned long lastPHControlTime = 0;
bool firstStartupDone = false;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  sensors.begin();

  // রিলে পিন আউটপুট হিসেবে সেটআপ
  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(ACID_PUMP_RELAY, OUTPUT);
  pinMode(BASE_PUMP_RELAY, OUTPUT);

  // সব রিলে অফ করে শুরু
  digitalWrite(FAN_RELAY_PIN, LOW);
  digitalWrite(ACID_PUMP_RELAY, LOW);
  digitalWrite(BASE_PUMP_RELAY, LOW);

  // প্রথম স্টার্টআপ মেসেজ দেখাও
  showStartupMessage();
  lastStartupDisplay = millis();
}

void loop() {
  unsigned long currentTime = millis();

  // প্রতি ২ মিনিটে একবার স্টার্টআপ মেসেজ দেখাও (১০ সেকেন্ড)
  if ((currentTime - lastStartupDisplay >= 120000) || !firstStartupDone) {
    showStartupMessage();
    lastStartupDisplay = currentTime;
    firstStartupDone = true;
  }

  // পিএইচ ও তাপমাত্রা রিড করা
  float phValue = readPH();
  float temperatureC = readTemperature();

  // ফ্যান ও রিলে নিয়ন্ত্রণ
  controlFan(temperatureC);
  controlPH(phValue);

  // LCD তে মান দেখাও
  displayData(phValue, temperatureC);

  delay(1000); // ১ সেকেন্ড অপেক্ষা
}

// ==== পিএইচ সেন্সর রিডিং ====
float readPH() {
  int buf[10], temp;
  for (int i = 0; i < 10; i++) {
    buf[i] = analogRead(PH_SENSOR_PIN);
    delay(10);
  }

  // ছোট থেকে বড় মান সাজানো (sorting)
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  unsigned long avgValue = 0;
  for (int i = 2; i < 8; i++) avgValue += buf[i];

  float voltage = (float)avgValue * 5.0 / 1024 / 6;
  float ph = 14.0 - (voltage * 14.0 / 6.0);
  ph = ph + PH_OFFSET;

  Serial.print("pH: ");
  Serial.println(ph, 2);
  return ph;
}

// ==== তাপমাত্রা রিডিং ====
float readTemperature() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  Serial.print("Temp: ");
  Serial.print(tempC);
  Serial.println(" C");
  return tempC;
}

// ==== LCD তে মান দেখানো ====
void displayData(float ph, float tempC) {
  lcd.clear();

  // প্রথম লাইনে pH ও অবস্থা
  lcd.setCursor(0, 0);
  lcd.print("pH: ");
  lcd.print(ph, 2);
  lcd.print(" ");

  if (ph < 6.5) {
    lcd.print("Acidic");
  } else if (ph > 7.5) {
    lcd.print("Alkaline");
  } else {
    lcd.print("Normal");
  }

  // দ্বিতীয় লাইনে তাপমাত্রা ও অবস্থা
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(tempC, 1);
  lcd.print("C ");

  if (tempC > 35.0) {
    lcd.print("Hot");
  } else if (tempC < 25.0) {
    lcd.print("Cold");
  } else {
    lcd.print("Norl");
  }
}

// ==== ফ্যান নিয়ন্ত্রণ ====
void controlFan(float tempC) {
  if (tempC > 31.0) {
    digitalWrite(FAN_RELAY_PIN, LOW);
  } else {
    digitalWrite(FAN_RELAY_PIN, HIGH);
  }
}

// ==== পিএইচ কন্ট্রোল ====
void controlPH(float ph) {
  unsigned long currentTime = millis();

  // প্রতি 3 মিনিটে একবার চেক করে (প্রয়োজনে যেকোন সময় পরিবর্তন করা করা যাবে) (1000 = 1 second)
  if (currentTime - lastPHControlTime >= 180000 || !firstStartupDone) {
    
    if (ph < 6.0) {
      // পিএইচ কম → ক্ষারীয় পাম্প চালু ৬০ সেকেন্ড (প্রয়োজনে যেকোন সময় পরিবর্তন করা করা যাবে)
      digitalWrite(BASE_PUMP_RELAY, LOW);
      delay(2500);  // 2.5 সেকেন্ড (প্রয়োজনে যেকোন সময় পরিবর্তন করা করা যাবে)
      digitalWrite(BASE_PUMP_RELAY, HIGH);
    } 
    else if (ph > 8.0) {
      // পিএইচ বেশি → অ্যাসিড পাম্প চালু ৬০ সেকেন্ড (প্রয়োজনে যেকোন সময় পরিবর্তন করা করা যাবে)
      digitalWrite(ACID_PUMP_RELAY,LOW  );
      delay(2500);  // 2.5 সেকেন্ড (প্রয়োজনে যেকোন সময় পরিবর্তন করা করা যাবে)
      digitalWrite(ACID_PUMP_RELAY, HIGH);
    }

    lastPHControlTime = millis(); // সময় আপডেট
  }
}

// ==== স্টার্টআপ মেসেজ ====
void showStartupMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Automatic Fish");

  lcd.setCursor(0, 1);
  lcd.print("Breading Machine");
  delay(5000); // ৫ সেকেন্ড

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Inventor:");
  lcd.setCursor(0, 1);
  lcd.print("MD Naim Islam");
  delay(5000); // ৫ সেকেন্ড

  lcd.clear();
}
```

### Required Libraries

Install the following Arduino libraries via the Library Manager:

1. **LiquidCrystal_I2C** by Frank de Brabander
2. **OneWire** by Paul Stoffregen
3. **DallasTemperature** by Miles Burton

### Hardware Setup Notes

- **pH Sensor**: Connected to analog pin A3
- **DS18B20 Temperature Sensor**: Requires 4.7KΩ pull-up resistor, connected to digital pin 12
- **LCD Display**: I2C 16x2 LCD display at address 0x27
- **Relays**: All relays use LOW signal to activate (active LOW)
- **pH Calibration**: Adjust `PH_OFFSET` value (default: 0.50) for accurate readings

### Automatic Control Features

- **Fan Control**: Automatically turns ON when temperature > 31°C
- **pH Control**: Checks every 3 minutes (180 seconds)
  - If pH < 6.0: Activates base pump for 2.5 seconds
  - If pH > 8.0: Activates acid pump for 2.5 seconds
- **Startup Display**: Shows inventor name every 2 minutes

## 🚀 Getting Started

### Prerequisites

- Node.js 16+ 
- npm or yarn
- ESP32 device with sensors and relays

### Installation

1. Clone the repository:
```bash
git clone <repository-url>
cd aquanar
```

2. Install dependencies:
```bash
npm install
```

3. Start the development server:
```bash
npm run dev
```

4. Open your browser and navigate to `http://localhost:3000`

### Configuration

1. **ESP32 IP Address**: Update the base URL in `src/services/apiService.js` to match your ESP32's IP address
2. **Wi-Fi Settings**: Configure your ESP32's Wi-Fi credentials in the Settings page
3. **Fish Species**: Select appropriate species from the database or add custom ones

## 📱 Usage

### Dashboard
- View real-time sensor readings
- Monitor system health status
- Check device connection status

### Device Control
- Toggle fan on/off
- Control acid and base pumps
- Emergency stop all devices
- View control logs

### Fish Species Database
- Browse pre-configured fish species
- Select species for automatic monitoring
- View ideal pH and temperature ranges

### Settings
- Configure Wi-Fi credentials
- Set update intervals
- Toggle dark mode
- View device information

## 🎨 Customization

### Theme Colors
The aqua blue theme can be customized in `src/index.css`:

```css
:root {
  --aqua-primary: #00bcd4;
  --aqua-secondary: #0097a7;
  --aqua-light: #e0f2fe;
  --aqua-dark: #006064;
}
```

### Adding New Fish Species
Add new species in `src/constants/index.js`:

```javascript
{
  id: '6',
  name: 'Your Fish Species',
  idealPhMin: 6.5,
  idealPhMax: 7.5,
  idealTempMin: 22,
  idealTempMax: 26,
  description: 'Description of your fish species'
}
```

## 🔧 Development

### Project Structure

```
src/
├── components/
│   ├── Layout/
│   │   └── AppLayout.jsx
│   └── Pages/
│       ├── Dashboard.jsx
│       ├── DeviceControl.jsx
│       ├── FishSpeciesDatabase.jsx
│       └── Settings.jsx
├── constants/
│   └── index.js
├── services/
│   └── apiService.js
├── store/
│   ├── appSlice.js
│   └── index.js
├── App.jsx
├── main.jsx
└── index.css
```

### Available Scripts

- `npm run dev` - Start development server
- `npm run build` - Build for production
- `npm run preview` - Preview production build
- `npm run lint` - Run ESLint

## 🚨 Troubleshooting

### Connection Issues
- Verify ESP32 IP address in API service
- Check Wi-Fi credentials
- Ensure ESP32 is running and accessible
- Check firewall settings

### Sensor Readings
- Verify sensor connections
- Check GPIO pin assignments
- Ensure sensors are properly calibrated

### UI Issues
- Clear browser cache
- Check console for JavaScript errors
- Verify all dependencies are installed

## 📄 License

This project is licensed under the MIT License.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For support and questions, please open an issue in the repository.