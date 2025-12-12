import axios from 'axios';
import { DEVICE_CONSTANTS } from '../constants';

// Determine if we're in production (Vercel) or development
const isProduction = import.meta.env.PROD || window.location.hostname !== 'localhost';

// Get ESP32 IP from localStorage or use default
// NOTE: This should match the static IP set in ESP32 firmware (config.h)
const getESP32IP = () => {
  if (isProduction) return null; // Use proxy in production
  return localStorage.getItem('esp32_ip') || '192.168.0.111'; // Default matches ESP32 static IP
};

// Create axios instance with base configuration
// In production (Vercel), use the proxy endpoint to avoid mixed content errors
// In development, connect directly to ESP32
const api = axios.create({
  baseURL: isProduction 
    ? '/api/proxy'  // Use Vercel proxy in production (HTTPS -> HTTP)
    : `http://${getESP32IP()}`, // Direct connection in development
  timeout: isProduction ? 20000 : 5000, // Longer timeout for proxy, shorter for localhost
  headers: {
    'Content-Type': 'application/json'
  }
});

// Update base URL when ESP32 IP changes (internal helper)
const refreshBaseURL = () => {
  if (!isProduction) {
    api.defaults.baseURL = `http://${getESP32IP()}`;
  }
};

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
      refreshBaseURL(); // Refresh IP from localStorage
      const endpoint = getEndpoint(DEVICE_CONSTANTS.API_ENDPOINTS.STATUS);
      const response = await api.get(endpoint);
      return {
        success: true,
        data: response.data,
        message: 'Device status retrieved successfully'
      };
    } catch (error) {
      console.error('Error fetching device status:', error);
      
      let errorMessage = 'Failed to connect to device';
      if (error.code === 'ECONNABORTED' || error.message.includes('timeout')) {
        errorMessage = `Connection timeout. ESP32 at ${getESP32IP()} is not responding. Check if device is online.`;
      } else if (error.code === 'ERR_NETWORK' || error.code === 'ECONNREFUSED') {
        errorMessage = `Cannot connect to ESP32 at ${getESP32IP()}. Check IP address and ensure device is online.`;
      } else if (error.response) {
        errorMessage = error.response.data?.message || error.response.data?.error || errorMessage;
      }
      
      return {
        success: false,
        data: null,
        message: errorMessage,
        error: error
      };
    }
  },

  // Send control commands to device
  async sendControlCommand(controlData) {
    try {
      refreshBaseURL(); // Refresh IP from localStorage
      const endpoint = getEndpoint(DEVICE_CONSTANTS.API_ENDPOINTS.CONTROL);
      const response = await api.post(endpoint, controlData);
      return {
        success: true,
        data: response.data,
        message: 'Control command sent successfully'
      };
    } catch (error) {
      console.error('Error sending control command:', error);
      
      let errorMessage = 'Failed to send control command';
      if (error.code === 'ECONNABORTED' || error.message.includes('timeout')) {
        errorMessage = `Connection timeout. ESP32 at ${getESP32IP()} is not responding. Check if device is online.`;
      } else if (error.code === 'ERR_NETWORK' || error.code === 'ECONNREFUSED') {
        errorMessage = `Cannot connect to ESP32 at ${getESP32IP()}. Check IP address and ensure device is online.`;
      } else if (error.response) {
        errorMessage = error.response.data?.message || error.response.data?.error || errorMessage;
      }
      
      return {
        success: false,
        data: null,
        message: errorMessage,
        error: error
      };
    }
  },

  // Send fish species configuration
  async sendSpeciesConfig(speciesData) {
    try {
      refreshBaseURL(); // Refresh IP from localStorage
      const endpoint = getEndpoint(DEVICE_CONSTANTS.API_ENDPOINTS.SPECIES);
      const response = await api.post(endpoint, speciesData);
      return {
        success: true,
        data: response.data,
        message: 'Species configuration sent successfully'
      };
    } catch (error) {
      console.error('Error sending species config:', error);
      
      let errorMessage = 'Failed to send species configuration';
      if (error.code === 'ECONNABORTED' || error.message.includes('timeout')) {
        errorMessage = `Connection timeout. ESP32 at ${getESP32IP()} is not responding. Check if device is online.`;
      } else if (error.code === 'ERR_NETWORK' || error.code === 'ECONNREFUSED') {
        errorMessage = `Cannot connect to ESP32 at ${getESP32IP()}. Check IP address and ensure device is online.`;
      } else if (error.response) {
        errorMessage = error.response.data?.message || error.response.data?.error || errorMessage;
      }
      
      return {
        success: false,
        data: null,
        message: errorMessage,
        error: error
      };
    }
  },

  // Send Wi-Fi configuration
  async sendWiFiConfig(wifiData) {
    try {
      refreshBaseURL(); // Refresh IP from localStorage
      const endpoint = getEndpoint(DEVICE_CONSTANTS.API_ENDPOINTS.WIFI);
      const response = await api.post(endpoint, wifiData);
      return {
        success: true,
        data: response.data,
        message: 'Wi-Fi configuration sent successfully'
      };
    } catch (error) {
      console.error('Error sending Wi-Fi config:', error);
      
      let errorMessage = 'Failed to send Wi-Fi configuration';
      if (error.code === 'ECONNABORTED' || error.message.includes('timeout')) {
        errorMessage = `Connection timeout. ESP32 at ${getESP32IP()} is not responding. Check if device is online.`;
      } else if (error.code === 'ERR_NETWORK' || error.code === 'ECONNREFUSED') {
        errorMessage = `Cannot connect to ESP32 at ${getESP32IP()}. Check IP address and ensure device is online.`;
      } else if (error.response) {
        errorMessage = error.response.data?.message || error.response.data?.error || errorMessage;
      }
      
      return {
        success: false,
        data: null,
        message: errorMessage,
        error: error
      };
    }
  },

  // Update base URL for different ESP32 IP (only works in development)
  updateBaseURL(newIP) {
    if (!isProduction) {
      localStorage.setItem('esp32_ip', newIP);
      api.defaults.baseURL = `http://${newIP}`;
      console.log(`[API] Updated ESP32 IP to: ${newIP}`);
    } else {
      console.warn('Cannot update ESP32 IP in production. Set ESP32_IP environment variable in Vercel.');
    }
  },

  // Get current ESP32 IP
  getESP32IP() {
    return getESP32IP();
  },

  // Test connection to ESP32
  async testConnection() {
    try {
      refreshBaseURL(); // Refresh IP from localStorage
      const endpoint = isProduction ? 'ping' : '/api/ping';
      const response = await api.get(endpoint, { timeout: 3000 }); // Shorter timeout for ping
      return {
        success: true,
        message: `Connection successful to ${getESP32IP()}`
      };
    } catch (error) {
      let errorMessage = 'Connection failed';
      if (error.code === 'ECONNABORTED' || error.message.includes('timeout')) {
        errorMessage = `Timeout: ESP32 at ${getESP32IP()} is not responding`;
      } else if (error.code === 'ERR_NETWORK' || error.code === 'ECONNREFUSED') {
        errorMessage = `Cannot reach ESP32 at ${getESP32IP()}. Check IP address.`;
      } else if (error.response) {
        errorMessage = error.response.data?.message || error.response.data?.error || errorMessage;
      }
      
      return {
        success: false,
        message: errorMessage,
        error: error
      };
    }
  }
};

export default apiService;
