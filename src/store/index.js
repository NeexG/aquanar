import { configureStore } from '@reduxjs/toolkit';
import { persistStore, persistReducer } from 'redux-persist';
import storage from 'redux-persist/lib/storage';
import appReducer from './appSlice';

// Persist configuration
const persistConfig = {
  key: 'smart-breeder-storage',
  storage,
  whitelist: ['settings', 'selectedSpecies'] // Only persist these parts of state
};

// Create persisted reducer
const persistedReducer = persistReducer(persistConfig, appReducer);

// Configure store
export const store = configureStore({
  reducer: {
    app: persistedReducer
  },
  middleware: (getDefaultMiddleware) =>
    getDefaultMiddleware({
      serializableCheck: {
        ignoredActions: ['persist/PERSIST', 'persist/REHYDRATE']
      }
    })
});

// Create persistor
export const persistor = persistStore(store);

// Export selectors
export const selectApp = (state) => state.app;
