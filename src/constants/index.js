// Device data constants and types
export const DEVICE_CONSTANTS = {
  DEFAULT_UPDATE_INTERVAL: 5000,
  CHART_DATA_LIMIT: 20,
  API_ENDPOINTS: {
    STATUS: '/api/status',
    CONTROL: '/api/control',
    SPECIES: '/api/species',
    SPECIES_LIST: '/api/species/list',
    WIFI: '/api/wifi'
  }
};

// Default fish species data
// All temperature ranges are within 25-32°C (random values in this range)
export const DEFAULT_FISH_SPECIES = [
  {
    id: '1',
    name: 'Goldfish',
    idealPhMin: 6.5,
    idealPhMax: 8.0,
    idealTempMin: 27.0,
    idealTempMax: 31.0,
    waterFlow: true,   // Yes
    rain: false,       // No - Test: Water Flow only
    description: 'Common goldfish, hardy and adaptable species - Water Flow: ON, Rain: OFF'
  },
  {
    id: '2',
    name: 'Betta Fish',
    idealPhMin: 6.5,
    idealPhMax: 7.5,
    idealTempMin: 26.5,
    idealTempMax: 30.5,
    waterFlow: false,  // No
    rain: true,        // Yes - Test: Rain only
    description: 'Siamese fighting fish, tropical species - Water Flow: OFF, Rain: ON'
  },
  {
    id: '3',
    name: 'Guppy',
    idealPhMin: 7.0,
    idealPhMax: 8.5,
    idealTempMin: 25.5,
    idealTempMax: 29.5,
    waterFlow: true,   // Yes
    rain: true,        // Yes - Test: Both ON
    description: 'Live-bearing tropical fish, colorful and active - Water Flow: ON, Rain: ON'
  },
  {
    id: '4',
    name: 'Neon Tetra',
    idealPhMin: 5.0,
    idealPhMax: 7.0,
    idealTempMin: 25.0,
    idealTempMax: 29.0,
    waterFlow: false,  // No
    rain: false,       // No - Test: Both OFF
    description: 'Small schooling fish, prefers acidic water - Water Flow: OFF, Rain: OFF'
  },
  {
    id: '5',
    name: 'Angelfish',
    idealPhMin: 6.0,
    idealPhMax: 7.5,
    idealTempMin: 28.0,
    idealTempMax: 32.0,
    waterFlow: true,   // Yes
    rain: true,        // Yes - Test: Both ON (duplicate for testing)
    description: 'Large cichlid, requires stable water conditions - Water Flow: ON, Rain: ON'
  },
  {
    id: '6',
    name: 'Comet',
    idealPhMin: 6.5,
    idealPhMax: 7.2,
    idealTempMin: 26.0,
    idealTempMax: 30.0,
    waterFlow: true,   // Yes
    rain: false,       // No - Test: Water Flow only (duplicate for testing)
    description: 'Comet goldfish, single-tailed variety - Water Flow: ON, Rain: OFF'
  },
  {
    id: '7',
    name: 'Rohu',
    idealPhMin: 6.6,
    idealPhMax: 8.0,
    idealTempMin: 27.5,
    idealTempMax: 31.5,
    waterFlow: false,  // No
    rain: true,        // Yes - Test: Rain only (duplicate for testing)
    description: 'Rohu fish, popular freshwater species - Water Flow: OFF, Rain: ON'
  }
];

// Default settings
export const DEFAULT_SETTINGS = {
  updateInterval: DEVICE_CONSTANTS.DEFAULT_UPDATE_INTERVAL,
  wifi: {
    ssid: '',
    password: ''
  },
  darkMode: false
};
