import React, { useState, useEffect, useMemo } from 'react';
import { Row, Col, Card, Switch, Button, Timeline, Space, message, notification, Divider, Alert } from 'antd';
import {
  ThunderboltOutlined,
  DropboxOutlined,
  HistoryOutlined,
  PoweroffOutlined,
  FireOutlined,
  CloudOutlined,
  BulbOutlined,
  ReloadOutlined
} from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { selectApp } from '../../store';
import { sendControlCommand, fetchDeviceStatus } from '../../store/appSlice';

const DeviceControl = () => {
  const dispatch = useDispatch();
  const { deviceData, isLoading, notifications } = useSelector(selectApp);
  const [controlLogs, setControlLogs] = useState([]);

  const handleControlChange = async (controlType, value) => {
    const controlData = { [controlType]: value };

    try {
      const result = await dispatch(sendControlCommand(controlData));

      if (result.type.endsWith('/fulfilled')) {
        const logEntry = {
          id: Date.now(),
          action: `${controlType} turned ${value ? 'ON' : 'OFF'}`,
          timestamp: new Date().toLocaleTimeString(),
          success: true
        };

        setControlLogs(prev => [logEntry, ...prev.slice(0, 9)]); // Keep last 10 logs
        message.success(`${controlType} ${value ? 'activated' : 'deactivated'} successfully`);

        // Show notification
        notification.success({
          message: 'Control Action Executed',
          description: `${controlType} ${value ? 'activated' : 'deactivated'} successfully`,
          duration: 3,
          placement: 'topRight',
        });
        
        // Immediately refresh device status to get updated relay states
        // Use a small delay to ensure ESP32 has processed the command
        setTimeout(async () => {
          const statusResult = await dispatch(fetchDeviceStatus());
          console.log('Status after control:', statusResult.payload);
          console.log(`${controlType} should be ${value}, actual:`, statusResult.payload?.[controlType]);
        }, 300);
      } else {
        message.error(`Failed to control ${controlType}`);
        notification.error({
          message: 'Control Action Failed',
          description: `Failed to execute ${controlType}`,
          duration: 5,
          placement: 'topRight',
        });
      }
    } catch (error) {
      message.error(`Error controlling ${controlType}`);
      notification.error({
        message: 'Control Action Error',
        description: `Error controlling ${controlType}`,
        duration: 5,
        placement: 'topRight',
      });
    }
  };

  const handleEmergencyStop = async () => {
    const controlData = {
      fan: false,
      acidPump: false,
      basePump: false,
      waterHeater: false,
      airPump: false,
      waterFlow: false,
      rainPump: false,
      lightControl: false
    };

    try {
      const result = await dispatch(sendControlCommand(controlData));

      if (result.type.endsWith('/fulfilled')) {
        const logEntry = {
          id: Date.now(),
          action: 'Emergency Stop - All devices turned OFF',
          timestamp: new Date().toLocaleTimeString(),
          success: true
        };

        setControlLogs(prev => [logEntry, ...prev.slice(0, 9)]);
        message.success('Emergency stop executed');

        notification.success({
          message: 'Emergency Stop Executed',
          description: 'All devices have been turned off',
          duration: 4,
          placement: 'topRight',
        });
      } else {
        message.error('Emergency stop failed');
        notification.error({
          message: 'Emergency Stop Failed',
          description: 'Failed to execute emergency stop',
          duration: 5,
          placement: 'topRight',
        });
      }
    } catch (error) {
      message.error('Emergency stop error');
      notification.error({
        message: 'Emergency Stop Error',
        description: 'Error executing emergency stop',
        duration: 5,
        placement: 'topRight',
      });
    }
  };

  // Get selected species for context
  const { selectedSpecies } = useSelector(selectApp);

  // Helper function to convert to boolean - handles all possible formats
  const toBool = (value) => {
    // Handle boolean
    if (value === true) return true;
    if (value === false) return false;
    
    // Handle string "true"/"false"
    if (typeof value === 'string') {
      const lower = value.toLowerCase().trim();
      if (lower === 'true' || lower === '1' || lower === 'on' || lower === 'yes') return true;
      if (lower === 'false' || lower === '0' || lower === 'off' || lower === 'no' || lower === '') return false;
    }
    
    // Handle numbers
    if (typeof value === 'number') {
      if (value === 1) return true;
      if (value === 0) return false;
    }
    
    // Default to false for null, undefined, or unknown values
    return false;
  };

  // Convert all relay states to proper booleans using useMemo
  const relayStates = useMemo(() => {
    if (!deviceData) {
      return {
        fan: false,
        acidPump: false,
        basePump: false,
        waterHeater: false,
        airPump: false,
        waterFlow: false,
        rainPump: false,
        lightControl: false
      };
    }
    
    // Get relay states - handle missing fields (undefined) by defaulting to false
    const states = {
      fan: toBool(deviceData.fan),
      acidPump: toBool(deviceData.acidPump),
      basePump: toBool(deviceData.basePump),
      waterHeater: toBool(deviceData.waterHeater),
      airPump: toBool(deviceData.airPump),
      waterFlow: toBool(deviceData.waterFlow),
      rainPump: toBool(deviceData.rainPump),
      lightControl: toBool(deviceData.lightControl)
    };
    
    // Warn if fields are missing (ESP32 firmware might not be updated)
    const missingFields = [];
    if (deviceData.waterHeater === undefined) missingFields.push('waterHeater');
    if (deviceData.airPump === undefined) missingFields.push('airPump');
    if (deviceData.waterFlow === undefined) missingFields.push('waterFlow');
    if (deviceData.rainPump === undefined) missingFields.push('rainPump');
    if (deviceData.lightControl === undefined) missingFields.push('lightControl');
    
    if (missingFields.length > 0) {
      console.warn('⚠️ Missing relay fields in ESP32 response:', missingFields);
      console.warn('⚠️ Please recompile and upload the ESP32 firmware with the latest server.cpp');
    }
    
    return states;
  }, [deviceData]);

  // Calculate active/inactive relay counts using useMemo
  const activeRelays = useMemo(() => {
    const count = Object.values(relayStates).filter(Boolean).length;
    return count;
  }, [relayStates]);

  // Debug: Log relay states
  useEffect(() => {
    if (deviceData) {
      console.log('=== RELAY STATUS DEBUG ===');
      console.log('Device Data (raw):', deviceData);
      console.log('Device Data (JSON):', JSON.stringify(deviceData, null, 2));
      console.log('Relay States (converted):', relayStates);
      console.log('Active Count:', activeRelays);
      console.log('Individual Raw Values:', {
        fan: deviceData.fan, 'fan type': typeof deviceData.fan,
        acidPump: deviceData.acidPump, 'acidPump type': typeof deviceData.acidPump,
        basePump: deviceData.basePump, 'basePump type': typeof deviceData.basePump,
        waterHeater: deviceData.waterHeater, 'waterHeater type': typeof deviceData.waterHeater,
        airPump: deviceData.airPump, 'airPump type': typeof deviceData.airPump,
        waterFlow: deviceData.waterFlow, 'waterFlow type': typeof deviceData.waterFlow,
        rainPump: deviceData.rainPump, 'rainPump type': typeof deviceData.rainPump,
        lightControl: deviceData.lightControl, 'lightControl type': typeof deviceData.lightControl
      });
      console.log('Individual Converted States:', {
        fan: relayStates.fan,
        acidPump: relayStates.acidPump,
        basePump: relayStates.basePump,
        waterHeater: relayStates.waterHeater,
        airPump: relayStates.airPump,
        waterFlow: relayStates.waterFlow,
        rainPump: relayStates.rainPump,
        lightControl: relayStates.lightControl
      });
      console.log('Filtered Active (before count):', Object.values(relayStates).filter(Boolean));
      console.log('Active Count:', activeRelays);
      console.log('========================');
    }
  }, [deviceData, relayStates, activeRelays]);

  const totalRelays = 8;
  const inactiveRelays = totalRelays - activeRelays;

  // Check if ESP32 firmware is missing relay fields
  const hasMissingFields = deviceData && (
    deviceData.waterHeater === undefined ||
    deviceData.airPump === undefined ||
    deviceData.waterFlow === undefined ||
    deviceData.rainPump === undefined ||
    deviceData.lightControl === undefined
  );

  return (
    <div>
      {/* Firmware Update Warning */}
      {hasMissingFields && (
        <Alert
          message="ESP32 Firmware Update Required"
          description="The ESP32 firmware is missing relay status fields (waterHeater, airPump, waterFlow, rainPump, lightControl). Please recompile and upload the latest firmware to see accurate relay status. Some relays may be active on hardware but showing as inactive here."
          type="warning"
          showIcon
          closable
          style={{ marginBottom: '24px' }}
        />
      )}

      {/* Relay Status Summary */}
      <Row gutter={[16, 16]} style={{ marginBottom: '24px' }}>
        <Col span={24}>
          <Card
            title={
              <Space>
                <ThunderboltOutlined style={{ color: '#00bcd4' }} />
                Relay Status Summary
                {selectedSpecies && (
                  <span style={{ fontSize: '14px', color: '#666', marginLeft: '16px' }}>
                    (Fish Selected: {selectedSpecies.name})
                  </span>
                )}
              </Space>
            }
            style={{ background: 'linear-gradient(135deg, #f0f9ff 0%, #e0f2fe 100%)' }}
          >
            <Row gutter={[16, 16]}>
              <Col xs={24} sm={8}>
                <div style={{ textAlign: 'center', padding: '16px' }}>
                  <div style={{ fontSize: '32px', fontWeight: 'bold', color: '#52c41a' }}>
                    {activeRelays}
                  </div>
                  <div style={{ fontSize: '16px', color: '#666', marginTop: '8px' }}>
                    Active Relays
                  </div>
                </div>
              </Col>
              <Col xs={24} sm={8}>
                <div style={{ textAlign: 'center', padding: '16px' }}>
                  <div style={{ fontSize: '32px', fontWeight: 'bold', color: '#ff4d4f' }}>
                    {inactiveRelays}
                  </div>
                  <div style={{ fontSize: '16px', color: '#666', marginTop: '8px' }}>
                    Inactive Relays
                  </div>
                </div>
              </Col>
              <Col xs={24} sm={8}>
                <div style={{ textAlign: 'center', padding: '16px' }}>
                  <div style={{ fontSize: '32px', fontWeight: 'bold', color: '#1890ff' }}>
                    {totalRelays}
                  </div>
                  <div style={{ fontSize: '16px', color: '#666', marginTop: '8px' }}>
                    Total Relays
                  </div>
                </div>
              </Col>
            </Row>
            <Divider />
            <Row gutter={[8, 8]}>
              <Col xs={12} sm={6}>
                <div style={{ padding: '8px', background: relayStates.fan ? '#f6ffed' : '#fff1f0', borderRadius: '4px', textAlign: 'center' }}>
                  <div style={{ fontWeight: 'bold', color: relayStates.fan ? '#52c41a' : '#ff4d4f' }}>
                    Fan: {relayStates.fan ? 'ACTIVE' : 'INACTIVE'}
                  </div>
                </div>
              </Col>
              <Col xs={12} sm={6}>
                <div style={{ padding: '8px', background: relayStates.acidPump ? '#f6ffed' : '#fff1f0', borderRadius: '4px', textAlign: 'center' }}>
                  <div style={{ fontWeight: 'bold', color: relayStates.acidPump ? '#52c41a' : '#ff4d4f' }}>
                    Acid: {relayStates.acidPump ? 'ACTIVE' : 'INACTIVE'}
                  </div>
                </div>
              </Col>
              <Col xs={12} sm={6}>
                <div style={{ padding: '8px', background: relayStates.basePump ? '#f6ffed' : '#fff1f0', borderRadius: '4px', textAlign: 'center' }}>
                  <div style={{ fontWeight: 'bold', color: relayStates.basePump ? '#52c41a' : '#ff4d4f' }}>
                    Base: {relayStates.basePump ? 'ACTIVE' : 'INACTIVE'}
                  </div>
                </div>
              </Col>
              <Col xs={12} sm={6}>
                <div style={{ padding: '8px', background: relayStates.waterHeater ? '#f6ffed' : '#fff1f0', borderRadius: '4px', textAlign: 'center' }}>
                  <div style={{ fontWeight: 'bold', color: relayStates.waterHeater ? '#52c41a' : '#ff4d4f' }}>
                    Heater: {relayStates.waterHeater ? 'ACTIVE' : 'INACTIVE'}
                  </div>
                </div>
              </Col>
              <Col xs={12} sm={6}>
                <div style={{ padding: '8px', background: relayStates.airPump ? '#f6ffed' : '#fff1f0', borderRadius: '4px', textAlign: 'center' }}>
                  <div style={{ fontWeight: 'bold', color: relayStates.airPump ? '#52c41a' : '#ff4d4f' }}>
                    Air: {relayStates.airPump ? 'ACTIVE' : 'INACTIVE'}
                  </div>
                </div>
              </Col>
              <Col xs={12} sm={6}>
                <div style={{ padding: '8px', background: relayStates.waterFlow ? '#f6ffed' : '#fff1f0', borderRadius: '4px', textAlign: 'center' }}>
                  <div style={{ fontWeight: 'bold', color: relayStates.waterFlow ? '#52c41a' : '#ff4d4f' }}>
                    Flow: {relayStates.waterFlow ? 'ACTIVE' : 'INACTIVE'}
                  </div>
                </div>
              </Col>
              <Col xs={12} sm={6}>
                <div style={{ padding: '8px', background: relayStates.rainPump ? '#f6ffed' : '#fff1f0', borderRadius: '4px', textAlign: 'center' }}>
                  <div style={{ fontWeight: 'bold', color: relayStates.rainPump ? '#52c41a' : '#ff4d4f' }}>
                    Rain: {relayStates.rainPump ? 'ACTIVE' : 'INACTIVE'}
                  </div>
                </div>
              </Col>
              <Col xs={12} sm={6}>
                <div style={{ padding: '8px', background: relayStates.lightControl ? '#f6ffed' : '#fff1f0', borderRadius: '4px', textAlign: 'center' }}>
                  <div style={{ fontWeight: 'bold', color: relayStates.lightControl ? '#52c41a' : '#ff4d4f' }}>
                    Light: {relayStates.lightControl ? 'ACTIVE' : 'INACTIVE'}
                  </div>
                </div>
              </Col>
            </Row>
          </Card>
        </Col>
      </Row>

      <Row gutter={[16, 16]}>
        {/* Fan Control */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            title={
              <Space>
                <ThunderboltOutlined style={{ color: '#00bcd4' }} />
                Fan Control
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={relayStates.fan}
                onChange={(checked) => handleControlChange('fan', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: relayStates.fan ? '#52c41a' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: relayStates.fan ? '#52c41a' : '#ff4d4f' }}>
                  {relayStates.fan ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Acid Pump Control */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            title={
              <Space>
                <DropboxOutlined style={{ color: '#ff4d4f' }} />
                Acid Pump
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={relayStates.acidPump}
                onChange={(checked) => handleControlChange('acidPump', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: relayStates.acidPump ? '#ff4d4f' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: relayStates.acidPump ? '#52c41a' : '#ff4d4f' }}>
                  {relayStates.acidPump ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Base Pump Control */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            title={
              <Space>
                <DropboxOutlined style={{ color: '#1890ff' }} />
                Base Pump
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={relayStates.basePump}
                onChange={(checked) => handleControlChange('basePump', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: relayStates.basePump ? '#1890ff' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: relayStates.basePump ? '#52c41a' : '#ff4d4f' }}>
                  {relayStates.basePump ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Water Heater Control */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            title={
              <Space>
                <FireOutlined style={{ color: '#ff6b6b' }} />
                Water Heater
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={relayStates.waterHeater}
                onChange={(checked) => handleControlChange('waterHeater', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: relayStates.waterHeater ? '#ff6b6b' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: relayStates.waterHeater ? '#52c41a' : '#ff4d4f' }}>
                  {relayStates.waterHeater ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Air Pump Control */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            title={
              <Space>
                <CloudOutlined style={{ color: '#52c41a' }} />
                Air Pump
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={relayStates.airPump}
                onChange={(checked) => handleControlChange('airPump', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: relayStates.airPump ? '#52c41a' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: relayStates.airPump ? '#52c41a' : '#ff4d4f' }}>
                  {relayStates.airPump ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Water Flow Control */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            title={
              <Space>
                <ReloadOutlined style={{ color: '#1890ff' }} />
                Water Flow
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={relayStates.waterFlow}
                onChange={(checked) => handleControlChange('waterFlow', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: relayStates.waterFlow ? '#1890ff' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: relayStates.waterFlow ? '#52c41a' : '#ff4d4f' }}>
                  {relayStates.waterFlow ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Rain Pump Control */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            title={
              <Space>
                <CloudOutlined style={{ color: '#722ed1' }} />
                Rain Pump
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={relayStates.rainPump}
                onChange={(checked) => handleControlChange('rainPump', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: relayStates.rainPump ? '#722ed1' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: relayStates.rainPump ? '#52c41a' : '#ff4d4f' }}>
                  {relayStates.rainPump ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Light Control */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            title={
              <Space>
                <BulbOutlined style={{ color: '#faad14' }} />
                Light Control
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={relayStates.lightControl}
                onChange={(checked) => handleControlChange('lightControl', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: relayStates.lightControl ? '#faad14' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: relayStates.lightControl ? '#52c41a' : '#ff4d4f' }}>
                  {relayStates.lightControl ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>
      </Row>

      {/* Emergency Stop */}
      <Row style={{ marginTop: '24px' }}>
        <Col span={24}>
          <Card
            title={
              <Space>
                <PoweroffOutlined style={{ color: '#ff4d4f' }} />
                Emergency Controls
              </Space>
            }
            style={{ border: '2px solid #ff4d4f' }}
          >
            <div style={{ textAlign: 'center' }}>
              <Button
                type="primary"
                danger
                size="large"
                icon={<PoweroffOutlined />}
                onClick={handleEmergencyStop}
                loading={isLoading}
                style={{
                  height: '50px',
                  fontSize: '16px',
                  padding: '0 30px'
                }}
              >
                EMERGENCY STOP
              </Button>
              <p style={{ marginTop: '10px', color: '#666' }}>
                This will immediately turn off all devices
              </p>
            </div>
          </Card>
        </Col>
      </Row>

      {/* Control Logs */}
      <Row style={{ marginTop: '24px' }}>
        <Col span={24}>
          <Card
            title={
              <Space>
                <HistoryOutlined style={{ color: '#00bcd4' }} />
                Control Logs
              </Space>
            }
          >
            {controlLogs.length > 0 ? (
              <Timeline
                items={controlLogs.map(log => ({
                  color: log.success ? 'green' : 'red',
                  children: (
                    <div>
                      <strong>{log.action}</strong>
                      <div style={{ color: '#666', fontSize: '12px' }}>
                        {log.timestamp}
                      </div>
                    </div>
                  )
                }))}
              />
            ) : (
              <div style={{ textAlign: 'center', color: '#666', padding: '20px' }}>
                No control actions yet
              </div>
            )}
          </Card>
        </Col>
      </Row>

      {/* System Notifications */}
      {notifications.length > 0 && (
        <Row style={{ marginTop: '16px' }}>
          <Col span={24}>
            <Card title="System Notifications">
              <Timeline
                items={notifications.slice(-5).map((notification, index) => ({
                  color: 'blue',
                  children: (
                    <div>
                      {notification}
                    </div>
                  )
                }))}
              />
            </Card>
          </Col>
        </Row>
      )}
    </div>
  );
};

export default DeviceControl;
