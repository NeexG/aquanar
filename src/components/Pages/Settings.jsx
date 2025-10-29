import React, { useState } from 'react';
import { Card, Form, Input, InputNumber, Switch, Button, Space, message, notification, Divider, Row, Col } from 'antd';
import { SettingOutlined, WifiOutlined, ClockCircleOutlined, BulbOutlined } from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { updateSettings, sendWiFiConfig } from '../../store/appSlice';
import { selectApp } from '../../store';

const Settings = () => {
  const dispatch = useDispatch();
  const { settings, isLoading } = useSelector(selectApp);
  const [form] = Form.useForm();
  const [wifiForm] = Form.useForm();

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
        message.error('Failed to send Wi-Fi configuration');
        notification.error({
          message: 'Wi-Fi Configuration Failed',
          description: 'Failed to send Wi-Fi settings to device',
          duration: 5,
          placement: 'topRight',
        });
      }
    } catch (error) {
      message.error('Error sending Wi-Fi configuration');
      notification.error({
        message: 'Wi-Fi Configuration Error',
        description: 'Error sending Wi-Fi settings to device',
        duration: 5,
        placement: 'topRight',
      });
    }
  };

  const handleResetSettings = () => {
    form.resetFields();
    wifiForm.resetFields();
    message.info('Settings reset to defaults');
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
