import axios from 'axios';
import { DEVICE_CONSTANTS } from '../constants';

// Create axios instance with base configuration
const api = axios.create({
  baseURL: 'http://192.168.1.100', // Default ESP32 IP - should be configurable
  timeout: 5000,
  headers: {
    'Content-Type': 'application/json'
  }
});

// API service for ESP32 communication
export const apiService = {
  // Get device status and sensor data
  async getDeviceStatus() {
    try {
      const response = await api.get(DEVICE_CONSTANTS.API_ENDPOINTS.STATUS);
      return {
        success: true,
        data: response.data,
        message: 'Device status retrieved successfully'
      };
    } catch (error) {
      console.error('Error fetching device status:', error);
      return {
        success: false,
        data: null,
        message: error.response?.data?.message || 'Failed to connect to device'
      };
    }
  },

  // Send control commands to device
  async sendControlCommand(controlData) {
    try {
      const response = await api.post(DEVICE_CONSTANTS.API_ENDPOINTS.CONTROL, controlData);
      return {
        success: true,
        data: response.data,
        message: 'Control command sent successfully'
      };
    } catch (error) {
      console.error('Error sending control command:', error);
      return {
        success: false,
        data: null,
        message: error.response?.data?.message || 'Failed to send control command'
      };
    }
  },

  // Send fish species configuration
  async sendSpeciesConfig(speciesData) {
    try {
      const response = await api.post(DEVICE_CONSTANTS.API_ENDPOINTS.SPECIES, speciesData);
      return {
        success: true,
        data: response.data,
        message: 'Species configuration sent successfully'
      };
    } catch (error) {
      console.error('Error sending species config:', error);
      return {
        success: false,
        data: null,
        message: error.response?.data?.message || 'Failed to send species configuration'
      };
    }
  },

  // Send Wi-Fi configuration
  async sendWiFiConfig(wifiData) {
    try {
      const response = await api.post(DEVICE_CONSTANTS.API_ENDPOINTS.WIFI, wifiData);
      return {
        success: true,
        data: response.data,
        message: 'Wi-Fi configuration sent successfully'
      };
    } catch (error) {
      console.error('Error sending Wi-Fi config:', error);
      return {
        success: false,
        data: null,
        message: error.response?.data?.message || 'Failed to send Wi-Fi configuration'
      };
    }
  },

  // Update base URL for different ESP32 IP
  updateBaseURL(newIP) {
    api.defaults.baseURL = `http://${newIP}`;
  },

  // Test connection to ESP32
  async testConnection() {
    try {
      const response = await api.get('/api/ping');
      return {
        success: true,
        message: 'Connection successful'
      };
    } catch (error) {
      return {
        success: false,
        message: 'Connection failed'
      };
    }
  }
};

export default apiService;
