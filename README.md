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

- **pH Sensor**: GPIO 34
- **Temperature Sensor (DS18B20)**: GPIO 12
- **Fan Relay**: GPIO 3
- **Acid Pump Relay**: GPIO 6
- **Alkaline Pump Relay**: GPIO 7

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