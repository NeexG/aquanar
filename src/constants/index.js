// Device data constants and types
export const DEVICE_CONSTANTS = {
  DEFAULT_UPDATE_INTERVAL: 5000,
  CHART_DATA_LIMIT: 20,
  API_ENDPOINTS: {
    STATUS: '/api/status',
    CONTROL: '/api/control',
    SPECIES: '/api/species',
    WIFI: '/api/wifi'
  }
};

// Default fish species data
export const DEFAULT_FISH_SPECIES = [
  {
    id: '1',
    name: 'Goldfish',
    idealPhMin: 6.5,
    idealPhMax: 8.0,
    idealTempMin: 18,
    idealTempMax: 24,
    description: 'Common goldfish, hardy and adaptable'
  },
  {
    id: '2',
    name: 'Betta Fish',
    idealPhMin: 6.5,
    idealPhMax: 7.5,
    idealTempMin: 24,
    idealTempMax: 28,
    description: 'Siamese fighting fish, tropical species'
  },
  {
    id: '3',
    name: 'Guppy',
    idealPhMin: 7.0,
    idealPhMax: 8.5,
    idealTempMin: 22,
    idealTempMax: 28,
    description: 'Live-bearing tropical fish, colorful and active'
  },
  {
    id: '4',
    name: 'Neon Tetra',
    idealPhMin: 5.0,
    idealPhMax: 7.0,
    idealTempMin: 20,
    idealTempMax: 26,
    description: 'Small schooling fish, prefers acidic water'
  },
  {
    id: '5',
    name: 'Angelfish',
    idealPhMin: 6.0,
    idealPhMax: 7.5,
    idealTempMin: 24,
    idealTempMax: 30,
    description: 'Large cichlid, requires stable water conditions'
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
