import React, { useState, useEffect } from 'react';
import { Card, Form, Input, InputNumber, Switch, Button, Space, message, notification, Divider, Row, Col } from 'antd';
import { SettingOutlined, WifiOutlined, ClockCircleOutlined, BulbOutlined, ApiOutlined } from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { updateSettings, sendWiFiConfig } from '../../store/appSlice';
import { selectApp } from '../../store';
import apiService from '../../services/apiService';

const Settings = () => {
  const dispatch = useDispatch();
  const { settings, isLoading } = useSelector(selectApp);
  const [form] = Form.useForm();
  const [wifiForm] = Form.useForm();
  const [ipForm] = Form.useForm();
  const [currentIP, setCurrentIP] = useState(apiService.getESP32IP() || '192.168.0.111'); // Default matches ESP32 static IP
  const [testingConnection, setTestingConnection] = useState(false);

  useEffect(() => {
    setCurrentIP(apiService.getESP32IP() || '192.168.0.111');
    ipForm.setFieldsValue({ esp32IP: currentIP });
  }, []);

  const handleSettingsSave = (values) => {
    dispatch(updateSettings(values));
    message.success('Settings saved successfully');
  };

  const handleWiFiSave = async (values) => {
    try {
      const result = await dispatch(sendWiFiConfig(values));

      if (result.type.endsWith('/fulfilled')) {
        message.success('Wi-Fi configuration sent to ESP32');

        notification.success({
          message: 'Wi-Fi Configuration Sent',
          description: 'Wi-Fi settings have been sent to ESP32 device',
          duration: 4,
          placement: 'topRight',
        });

        dispatch(updateSettings({ wifi: values }));
      } else {
        // Get error message from the rejected action
        const errorMessage = result.payload || result.error?.message || 'Failed to send Wi-Fi configuration';
        message.error(errorMessage);
        notification.error({
          message: 'Wi-Fi Configuration Failed',
          description: errorMessage,
          duration: 6,
          placement: 'topRight',
        });
      }
    } catch (error) {
      const errorMessage = error.message || 'Error sending Wi-Fi configuration';
      message.error(errorMessage);
      notification.error({
        message: 'Wi-Fi Configuration Error',
        description: errorMessage,
        duration: 6,
        placement: 'topRight',
      });
    }
  };

  const handleResetSettings = () => {
    form.resetFields();
    wifiForm.resetFields();
    message.info('Settings reset to defaults');
  };

  const handleIPUpdate = async (values) => {
    const newIP = values.esp32IP.trim();
    
    // Basic IP validation
    const ipRegex = /^(\d{1,3}\.){3}\d{1,3}$/;
    if (!ipRegex.test(newIP)) {
      message.error('Invalid IP address format');
      return;
    }

    try {
      apiService.updateBaseURL(newIP);
      setCurrentIP(newIP);
      message.success(`ESP32 IP updated to ${newIP}`);
    } catch (error) {
      message.error('Failed to update IP address');
    }
  };

  const handleTestConnection = async () => {
    setTestingConnection(true);
    try {
      const result = await apiService.testConnection();
      if (result.success) {
        message.success(result.message);
        notification.success({
          message: 'Connection Successful',
          description: result.message,
          duration: 3,
        });
      } else {
        message.error(result.message);
        notification.error({
          message: 'Connection Failed',
          description: result.message,
          duration: 5,
        });
      }
    } catch (error) {
      message.error('Error testing connection');
    } finally {
      setTestingConnection(false);
    }
  };

  return (
    <div>
      <Row gutter={[16, 16]}>
        {/* General Settings */}
        <Col xs={24} lg={12}>
          <Card
            title={
              <Space>
                <SettingOutlined style={{ color: '#00bcd4' }} />
                General Settings
              </Space>
            }
          >
            <Form
              form={form}
              layout="vertical"
              initialValues={settings}
              onFinish={handleSettingsSave}
            >
              <Form.Item
                label={
                  <Space>
                    <ClockCircleOutlined />
                    Update Interval (milliseconds)
                  </Space>
                }
                name="updateInterval"
                rules={[
                  { required: true, message: 'Please enter update interval' },
                  { type: 'number', min: 1000, message: 'Minimum interval is 1000ms' }
                ]}
                extra="How often the dashboard fetches new sensor data from ESP32 (1000ms = 1 second)"
              >
                <InputNumber
                  style={{ width: '100%' }}
                  min={1000}
                  max={60000}
                  step={1000}
                  addonAfter="ms"
                />
              </Form.Item>

              <Form.Item
                label={
                  <Space>
                    <BulbOutlined />
                    Dark Mode
                  </Space>
                }
                name="darkMode"
                valuePropName="checked"
              >
                <Switch />
              </Form.Item>

              <Form.Item>
                <Space>
                  <Button type="primary" htmlType="submit" loading={isLoading}>
                    Save Settings
                  </Button>
                  <Button onClick={handleResetSettings}>
                    Reset to Defaults
                  </Button>
                </Space>
              </Form.Item>
            </Form>
          </Card>
        </Col>

        {/* ESP32 Connection Settings */}
        <Col xs={24} lg={12}>
          <Card
            title={
              <Space>
                <ApiOutlined style={{ color: '#00bcd4' }} />
                ESP32 Connection
              </Space>
            }
          >
            <Form
              form={ipForm}
              layout="vertical"
              initialValues={{ esp32IP: currentIP }}
              onFinish={handleIPUpdate}
            >
              <Form.Item
                label="ESP32 IP Address"
                name="esp32IP"
                rules={[
                  { required: true, message: 'Please enter ESP32 IP address' },
                  { pattern: /^(\d{1,3}\.){3}\d{1,3}$/, message: 'Invalid IP address format' }
                ]}
                extra="Enter the IP address of your ESP32 device (check Serial Monitor for IP)"
              >
                <Input placeholder="192.168.0.111" />
              </Form.Item>

              <Form.Item>
                <Space>
                  <Button type="primary" htmlType="submit">
                    Update IP
                  </Button>
                  <Button onClick={handleTestConnection} loading={testingConnection}>
                    Test Connection
                  </Button>
                </Space>
              </Form.Item>
            </Form>
          </Card>
        </Col>

        {/* Wi-Fi Settings */}
        <Col xs={24} lg={12}>
          <Card
            title={
              <Space>
                <WifiOutlined style={{ color: '#00bcd4' }} />
                Wi-Fi Configuration
              </Space>
            }
          >
            <Form
              form={wifiForm}
              layout="vertical"
              initialValues={settings.wifi}
              onFinish={handleWiFiSave}
            >
              <Form.Item
                label="Network Name (SSID)"
                name="ssid"
                rules={[
                  { required: true, message: 'Please enter Wi-Fi SSID' },
                  { min: 1, message: 'SSID cannot be empty' }
                ]}
              >
                <Input placeholder="Enter your Wi-Fi network name" />
              </Form.Item>

              <Form.Item
                label="Password"
                name="password"
                rules={[
                  { required: true, message: 'Please enter Wi-Fi password' },
                  { min: 8, message: 'Password must be at least 8 characters' }
                ]}
              >
                <Input.Password placeholder="Enter your Wi-Fi password" />
              </Form.Item>

              <Form.Item>
                <Button type="primary" htmlType="submit" loading={isLoading} block>
                  Send to ESP32
                </Button>
              </Form.Item>
            </Form>
          </Card>
        </Col>
      </Row>

      {/* Device Information */}
      <Row style={{ marginTop: '24px' }}>
        <Col span={24}>
          <Card title="Device Information">
            <Row gutter={[16, 16]}>
              <Col xs={24} sm={12} md={6}>
                <div style={{ textAlign: 'center' }}>
                  <div style={{ fontSize: '24px', fontWeight: 'bold', color: '#00bcd4' }}>
                    ESP32
                  </div>
                  <div style={{ color: '#666' }}>Microcontroller</div>
                </div>
              </Col>
              <Col xs={24} sm={12} md={6}>
                <div style={{ textAlign: 'center' }}>
                  <div style={{ fontSize: '24px', fontWeight: 'bold', color: '#00bcd4' }}>
                    GPIO 34
                  </div>
                  <div style={{ color: '#666' }}>pH Sensor</div>
                </div>
              </Col>
              <Col xs={24} sm={12} md={6}>
                <div style={{ textAlign: 'center' }}>
                  <div style={{ fontSize: '24px', fontWeight: 'bold', color: '#00bcd4' }}>
                    GPIO 12
                  </div>
                  <div style={{ color: '#666' }}>Temperature Sensor</div>
                </div>
              </Col>
              <Col xs={24} sm={12} md={6}>
                <div style={{ textAlign: 'center' }}>
                  <div style={{ fontSize: '24px', fontWeight: 'bold', color: '#00bcd4' }}>
                    GPIO 3,6,7
                  </div>
                  <div style={{ color: '#666' }}>Relay Controls</div>
                </div>
              </Col>
            </Row>
          </Card>
        </Col>
      </Row>

      {/* API Endpoints */}
      <Row style={{ marginTop: '16px' }}>
        <Col span={24}>
          <Card title="API Endpoints" type="inner">
            <div style={{ fontFamily: 'monospace', fontSize: '14px' }}>
              <div><strong>GET</strong> /api/status - Get device sensor data</div>
              <div><strong>POST</strong> /api/control - Send control commands</div>
              <div><strong>POST</strong> /api/species - Configure fish species settings</div>
              <div><strong>POST</strong> /api/wifi - Configure Wi-Fi settings</div>
              <div><strong>GET</strong> /api/ping - Test connection</div>
            </div>
          </Card>
        </Col>
      </Row>
    </div>
  );
};

export default Settings;
