import React, { useState } from 'react';
import { Row, Col, Card, Switch, Button, Timeline, Space, message, notification, Divider } from 'antd';
import {
  ThunderboltOutlined,
  DropboxOutlined,
  HistoryOutlined,
  PoweroffOutlined
} from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { selectApp } from '../../store';
import { sendControlCommand } from '../../store/appSlice';

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
      basePump: false
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

  return (
    <div>
      <Row gutter={[16, 16]}>
        {/* Fan Control */}
        <Col xs={24} sm={12} lg={8}>
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
                checked={deviceData?.fan || false}
                onChange={(checked) => handleControlChange('fan', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: deviceData?.fan ? '#52c41a' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: deviceData?.fan ? '#52c41a' : '#ff4d4f' }}>
                  {deviceData?.fan ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Acid Pump Control */}
        <Col xs={24} sm={12} lg={8}>
          <Card
            title={
              <Space>
                <DropboxOutlined style={{ color: '#ff4d4f' }} />
                Acid Pump Control
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={deviceData?.acidPump || false}
                onChange={(checked) => handleControlChange('acidPump', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: deviceData?.acidPump ? '#ff4d4f' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: deviceData?.acidPump ? '#52c41a' : '#ff4d4f' }}>
                  {deviceData?.acidPump ? 'ON' : 'OFF'}
                </span>
              </div>
            </div>
          </Card>
        </Col>

        {/* Base Pump Control */}
        <Col xs={24} sm={12} lg={8}>
          <Card
            title={
              <Space>
                <DropboxOutlined style={{ color: '#1890ff' }} />
                Base Pump Control
              </Space>
            }
            hoverable
          >
            <div style={{ textAlign: 'center' }}>
              <Switch
                checked={deviceData?.basePump || false}
                onChange={(checked) => handleControlChange('basePump', checked)}
                loading={isLoading}
                size="large"
                style={{
                  background: deviceData?.basePump ? '#1890ff' : '#d9d9d9'
                }}
              />
              <div style={{ marginTop: '16px', fontSize: '16px' }}>
                Status: <span style={{ color: deviceData?.basePump ? '#52c41a' : '#ff4d4f' }}>
                  {deviceData?.basePump ? 'ON' : 'OFF'}
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
