
import React from 'react';
import { Provider } from 'react-redux';
import { PersistGate } from 'redux-persist/integration/react';
import { ConfigProvider, theme, App as AntdApp } from 'antd';
import { store, persistor } from './store';
import AppLayout from './components/Layout/AppLayout';
import Dashboard from './components/Pages/Dashboard';
import DeviceControl from './components/Pages/DeviceControl';
import FishSpeciesDatabase from './components/Pages/FishSpeciesDatabase';
import Settings from './components/Pages/Settings';
import { useSelector } from 'react-redux';
import { selectApp } from './store';

// Main content component that renders based on current page
const AppContent = () => {
  const { currentPage, settings } = useSelector(selectApp);

  const renderPage = () => {
    switch (currentPage) {
      case 'dashboard':
        return <Dashboard />;
      case 'device-control':
        return <DeviceControl />;
      case 'species-database':
        return <FishSpeciesDatabase />;
      case 'settings':
        return <Settings />;
      default:
        return <Dashboard />;
    }
  };

  return (
    <AppLayout>
      {renderPage()}
    </AppLayout>
  );
};

function App() {
  return (
    <Provider store={store}>
      <PersistGate loading={null} persistor={persistor}>
        <ConfigProvider
          theme={{
            token: {
              colorPrimary: '#00bcd4',
              colorSuccess: '#52c41a',
              colorWarning: '#faad14',
              colorError: '#ff4d4f',
              borderRadius: 6,
            },
            algorithm: theme.defaultAlgorithm,
          }}
        >
          <AntdApp>
            <AppContent />
          </AntdApp>
        </ConfigProvider>
      </PersistGate>
    </Provider>
  );
}

export default App;
