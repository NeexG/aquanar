import { createSlice, createAsyncThunk } from '@reduxjs/toolkit';
import { apiService } from '../services/apiService';
import { DEVICE_CONSTANTS, DEFAULT_FISH_SPECIES, DEFAULT_SETTINGS } from '../constants';

// Async thunks for API calls
export const fetchDeviceStatus = createAsyncThunk(
  'app/fetchDeviceStatus',
  async (_, { rejectWithValue }) => {
    try {
      const result = await apiService.getDeviceStatus();
      if (result.success) {
        return result.data;
      } else {
        return rejectWithValue(result.message);
      }
    } catch (error) {
      return rejectWithValue(error.message);
    }
  }
);

export const sendControlCommand = createAsyncThunk(
  'app/sendControlCommand',
  async (controlData, { rejectWithValue }) => {
    try {
      const result = await apiService.sendControlCommand(controlData);
      if (result.success) {
        return result.data;
      } else {
        return rejectWithValue(result.message);
      }
    } catch (error) {
      return rejectWithValue(error.message);
    }
  }
);

export const sendSpeciesConfig = createAsyncThunk(
  'app/sendSpeciesConfig',
  async (speciesData, { rejectWithValue }) => {
    try {
      const result = await apiService.sendSpeciesConfig(speciesData);
      if (result.success) {
        return result.data;
      } else {
        return rejectWithValue(result.message);
      }
    } catch (error) {
      return rejectWithValue(error.message);
    }
  }
);

export const sendWiFiConfig = createAsyncThunk(
  'app/sendWiFiConfig',
  async (wifiData, { rejectWithValue }) => {
    try {
      const result = await apiService.sendWiFiConfig(wifiData);
      if (result.success) {
        return result.data;
      } else {
        return rejectWithValue(result.message);
      }
    } catch (error) {
      return rejectWithValue(error.message);
    }
  }
);

export const testConnection = createAsyncThunk(
  'app/testConnection',
  async (_, { rejectWithValue }) => {
    try {
      const result = await apiService.testConnection();
      if (result.success) {
        return result.message;
      } else {
        return rejectWithValue(result.message);
      }
    } catch (error) {
      return rejectWithValue(error.message);
    }
  }
);

// Initial state
const initialState = {
  // Device data
  deviceData: null,
  deviceStatus: {
    connected: false,
    lastUpdate: '',
    systemHealth: 'error'
  },
  chartData: [],
  
  // Settings
  settings: DEFAULT_SETTINGS,
  
  // Fish species
  fishSpecies: DEFAULT_FISH_SPECIES,
  selectedSpecies: null,
  
  // UI state
  currentPage: 'dashboard',
  isLoading: false,
  notifications: [],
  
  // API state
  error: null,
  lastAction: null
};

// App slice
const appSlice = createSlice({
  name: 'app',
  initialState,
  reducers: {
    setDeviceData: (state, action) => {
      state.deviceData = action.payload;
      // Add to chart data
      const chartPoint = {
        time: new Date().toLocaleTimeString(),
        ph: action.payload.ph,
        temperature: action.payload.temperature
      };
      state.chartData = [...state.chartData.slice(-(DEVICE_CONSTANTS.CHART_DATA_LIMIT - 1)), chartPoint];
    },
    
    setDeviceStatus: (state, action) => {
      state.deviceStatus = action.payload;
    },
    
    updateSettings: (state, action) => {
      state.settings = { ...state.settings, ...action.payload };
    },
    
    setCurrentPage: (state, action) => {
      state.currentPage = action.payload;
    },
    
    setLoading: (state, action) => {
      state.isLoading = action.payload;
    },
    
    addNotification: (state, action) => {
      state.notifications.push(action.payload);
    },
    
    clearNotifications: (state) => {
      state.notifications = [];
    },
    
    setSelectedSpecies: (state, action) => {
      state.selectedSpecies = action.payload;
    },
    
    setFishSpecies: (state, action) => {
      state.fishSpecies = action.payload;
    },
    
    clearError: (state) => {
      state.error = null;
    },
    
    setLastAction: (state, action) => {
      state.lastAction = action.payload;
    }
  },
  
  extraReducers: (builder) => {
    builder
      // Fetch device status
      .addCase(fetchDeviceStatus.pending, (state) => {
        state.isLoading = true;
        state.error = null;
      })
      .addCase(fetchDeviceStatus.fulfilled, (state, action) => {
        state.isLoading = false;
        state.deviceData = action.payload;
        state.deviceStatus.connected = true;
        state.deviceStatus.lastUpdate = new Date().toISOString();
        
        // Add to chart data
        const chartPoint = {
          time: new Date().toLocaleTimeString(),
          ph: action.payload.ph,
          temperature: action.payload.temperature
        };
        state.chartData = [...state.chartData.slice(-(DEVICE_CONSTANTS.CHART_DATA_LIMIT - 1)), chartPoint];
      })
      .addCase(fetchDeviceStatus.rejected, (state, action) => {
        state.isLoading = false;
        state.error = action.payload;
        state.deviceStatus.connected = false;
      })
      
      // Send control command
      .addCase(sendControlCommand.pending, (state) => {
        state.isLoading = true;
        state.error = null;
      })
      .addCase(sendControlCommand.fulfilled, (state, action) => {
        state.isLoading = false;
        state.lastAction = {
          type: 'control',
          data: action.payload,
          timestamp: new Date().toISOString()
        };
        state.notifications.push(`Control command executed at ${new Date().toLocaleTimeString()}`);
      })
      .addCase(sendControlCommand.rejected, (state, action) => {
        state.isLoading = false;
        state.error = action.payload;
      })
      
      // Send species config
      .addCase(sendSpeciesConfig.pending, (state) => {
        state.isLoading = true;
        state.error = null;
      })
      .addCase(sendSpeciesConfig.fulfilled, (state, action) => {
        state.isLoading = false;
        state.lastAction = {
          type: 'species',
          data: action.payload,
          timestamp: new Date().toISOString()
        };
        state.notifications.push(`Species configuration updated at ${new Date().toLocaleTimeString()}`);
      })
      .addCase(sendSpeciesConfig.rejected, (state, action) => {
        state.isLoading = false;
        state.error = action.payload;
      })
      
      // Send WiFi config
      .addCase(sendWiFiConfig.pending, (state) => {
        state.isLoading = true;
        state.error = null;
      })
      .addCase(sendWiFiConfig.fulfilled, (state, action) => {
        state.isLoading = false;
        state.lastAction = {
          type: 'wifi',
          data: action.payload,
          timestamp: new Date().toISOString()
        };
        state.notifications.push(`WiFi configuration updated at ${new Date().toLocaleTimeString()}`);
      })
      .addCase(sendWiFiConfig.rejected, (state, action) => {
        state.isLoading = false;
        state.error = action.payload;
      })
      
      // Test connection
      .addCase(testConnection.pending, (state) => {
        state.isLoading = true;
        state.error = null;
      })
      .addCase(testConnection.fulfilled, (state) => {
        state.isLoading = false;
        state.deviceStatus.connected = true;
        state.notifications.push('Connection test successful');
      })
      .addCase(testConnection.rejected, (state, action) => {
        state.isLoading = false;
        state.deviceStatus.connected = false;
        state.error = action.payload;
        state.notifications.push('Connection test failed');
      });
  }
});

export const {
  setDeviceData,
  setDeviceStatus,
  updateSettings,
  setCurrentPage,
  setLoading,
  addNotification,
  clearNotifications,
  setSelectedSpecies,
  setFishSpecies,
  clearError,
  setLastAction
} = appSlice.actions;

export default appSlice.reducer;
