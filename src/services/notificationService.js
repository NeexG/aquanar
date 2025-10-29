import { message, notification } from 'antd';

// Toast notification service
export const notificationService = {
  // Success notification
  success: (content, duration = 3) => {
    message.success({
      content,
      duration,
      style: {
        marginTop: '20px',
      },
    });
  },

  // Error notification
  error: (content, duration = 5) => {
    message.error({
      content,
      duration,
      style: {
        marginTop: '20px',
      },
    });
  },

  // Warning notification
  warning: (content, duration = 4) => {
    message.warning({
      content,
      duration,
      style: {
        marginTop: '20px',
      },
    });
  },

  // Info notification
  info: (content, duration = 3) => {
    message.info({
      content,
      duration,
      style: {
        marginTop: '20px',
      },
    });
  },

  // Loading notification
  loading: (content = 'Loading...') => {
    return message.loading({
      content,
      duration: 0,
      style: {
        marginTop: '20px',
      },
    });
  },

  // Device status notification
  deviceStatus: (connected) => {
    if (connected) {
      notification.success({
        message: 'Device Connected',
        description: 'ESP32 device is now connected and sending data',
        duration: 4,
        placement: 'topRight',
      });
    } else {
      notification.error({
        message: 'Device Disconnected',
        description: 'Lost connection to ESP32 device',
        duration: 0, // Don't auto-close error notifications
        placement: 'topRight',
      });
    }
  },

  // Control action notification
  controlAction: (action, success) => {
    if (success) {
      notification.success({
        message: 'Control Action Executed',
        description: `${action} completed successfully`,
        duration: 3,
        placement: 'topRight',
      });
    } else {
      notification.error({
        message: 'Control Action Failed',
        description: `Failed to execute ${action}`,
        duration: 5,
        placement: 'topRight',
      });
    }
  },

  // Species configuration notification
  speciesConfig: (speciesName, success) => {
    if (success) {
      notification.success({
        message: 'Species Configured',
        description: `${speciesName} settings have been applied to the device`,
        duration: 4,
        placement: 'topRight',
      });
    } else {
      notification.error({
        message: 'Species Configuration Failed',
        description: `Failed to configure ${speciesName} settings`,
        duration: 5,
        placement: 'topRight',
      });
    }
  },

  // Wi-Fi configuration notification
  wifiConfig: (success) => {
    if (success) {
      notification.success({
        message: 'Wi-Fi Configuration Sent',
        description: 'Wi-Fi settings have been sent to ESP32 device',
        duration: 4,
        placement: 'topRight',
      });
    } else {
      notification.error({
        message: 'Wi-Fi Configuration Failed',
        description: 'Failed to send Wi-Fi settings to device',
        duration: 5,
        placement: 'topRight',
      });
    }
  },

  // System health notification
  systemHealth: (status, ph, temperature) => {
    const statusMessages = {
      healthy: {
        type: 'success',
        title: 'System Healthy',
        description: `pH: ${ph?.toFixed(2)}, Temperature: ${temperature?.toFixed(1)}°C - All readings within normal range`
      },
      warning: {
        type: 'warning',
        title: 'System Warning',
        description: `pH: ${ph?.toFixed(2)}, Temperature: ${temperature?.toFixed(1)}°C - Some readings outside ideal range`
      },
      error: {
        type: 'error',
        title: 'System Error',
        description: `pH: ${ph?.toFixed(2)}, Temperature: ${temperature?.toFixed(1)}°C - Critical readings detected`
      }
    };

    const config = statusMessages[status];
    if (config) {
      notification[config.type]({
        message: config.title,
        description: config.description,
        duration: status === 'error' ? 0 : 5,
        placement: 'topRight',
      });
    }
  },

  // Clear all notifications
  clear: () => {
    message.destroy();
    notification.destroy();
  }
};

export default notificationService;
