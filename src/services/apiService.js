import axios from 'axios';
import { DEVICE_CONSTANTS } from '../constants';

// Determine if we're in production (Vercel) or development
const isProduction = import.meta.env.PROD || window.location.hostname !== 'localhost';

// Create axios instance with base configuration
// In production (Vercel), use the proxy endpoint to avoid mixed content errors
// In development, connect directly to ESP32
const api = axios.create({
  baseURL: isProduction 
    ? '/api/proxy'  // Use Vercel proxy in production (HTTPS -> HTTP)
    : 'http://192.168.0.111', // Direct connection in development
  timeout: 15000, // Increased timeout for proxy + ESP32
  headers: {
    'Content-Type': 'application/json'
  }
});

// Helper to get the correct endpoint path
// In production (proxy), we use the path without /api prefix (proxy adds it)
// In development (direct), we use the full path with /api prefix
const getEndpoint = (endpoint) => {
  if (isProduction) {
    // Remove /api prefix for proxy (proxy adds it back)
    return endpoint.replace('/api/', '');
  }
  return endpoint;
};

// API service for ESP32 communication
export const apiService = {
  // Get device status and sensor data
  async getDeviceStatus() {
    try {
      const endpoint = getEndpoint(DEVICE_CONSTANTS.API_ENDPOINTS.STATUS);
      const response = await api.get(endpoint);
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
        message: error.response?.data?.message || error.response?.data?.error || 'Failed to connect to device'
      };
    }
  },

  // Send control commands to device
  async sendControlCommand(controlData) {
    try {
      const endpoint = getEndpoint(DEVICE_CONSTANTS.API_ENDPOINTS.CONTROL);
      const response = await api.post(endpoint, controlData);
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
        message: error.response?.data?.message || error.response?.data?.error || 'Failed to send control command'
      };
    }
  },

  // Send fish species configuration
  async sendSpeciesConfig(speciesData) {
    try {
      const endpoint = getEndpoint(DEVICE_CONSTANTS.API_ENDPOINTS.SPECIES);
      const response = await api.post(endpoint, speciesData);
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
        message: error.response?.data?.message || error.response?.data?.error || 'Failed to send species configuration'
      };
    }
  },

  // Send Wi-Fi configuration
  async sendWiFiConfig(wifiData) {
    try {
      const endpoint = getEndpoint(DEVICE_CONSTANTS.API_ENDPOINTS.WIFI);
      const response = await api.post(endpoint, wifiData);
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
        message: error.response?.data?.message || error.response?.data?.error || 'Failed to send Wi-Fi configuration'
      };
    }
  },

  // Update base URL for different ESP32 IP (only works in development)
  updateBaseURL(newIP) {
    if (!isProduction) {
      api.defaults.baseURL = `http://${newIP}`;
    } else {
      console.warn('Cannot update ESP32 IP in production. Set ESP32_IP environment variable in Vercel.');
    }
  },

  // Test connection to ESP32
  async testConnection() {
    try {
      const endpoint = isProduction ? 'ping' : '/api/ping';
      const response = await api.get(endpoint);
      return {
        success: true,
        message: 'Connection successful'
      };
    } catch (error) {
      return {
        success: false,
        message: error.response?.data?.message || error.response?.data?.error || 'Connection failed'
      };
    }
  }
};

export default apiService;
