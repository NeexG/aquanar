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
    description: 'Common goldfish, hardy and adaptable species'
  },
  {
    id: '2',
    name: 'Betta Fish',
    idealPhMin: 6.5,
    idealPhMax: 7.5,
    idealTempMin: 26.5,
    idealTempMax: 30.5,
    description: 'Siamese fighting fish, tropical species'
  },
  {
    id: '3',
    name: 'Guppy',
    idealPhMin: 7.0,
    idealPhMax: 8.5,
    idealTempMin: 25.5,
    idealTempMax: 29.5,
    description: 'Live-bearing tropical fish, colorful and active'
  },
  {
    id: '4',
    name: 'Neon Tetra',
    idealPhMin: 5.0,
    idealPhMax: 7.0,
    idealTempMin: 25.0,
    idealTempMax: 29.0,
    description: 'Small schooling fish, prefers acidic water'
  },
  {
    id: '5',
    name: 'Angelfish',
    idealPhMin: 6.0,
    idealPhMax: 7.5,
    idealTempMin: 28.0,
    idealTempMax: 32.0,
    description: 'Large cichlid, requires stable water conditions'
  },
  {
    id: '6',
    name: 'Comet',
    idealPhMin: 6.5,
    idealPhMax: 7.2,
    idealTempMin: 26.0,
    idealTempMax: 30.0,
    description: 'Comet goldfish, single-tailed variety'
  },
  {
    id: '7',
    name: 'Rohu',
    idealPhMin: 6.6,
    idealPhMax: 8.0,
    idealTempMin: 27.5,
    idealTempMax: 31.5,
    description: 'Rohu fish, popular freshwater species'
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
