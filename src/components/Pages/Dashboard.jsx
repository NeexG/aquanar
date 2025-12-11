import React, { useEffect } from 'react';
import { Row, Col, Card, Statistic, Progress, Alert, Spin, Divider, Space } from 'antd';
import {
  FireOutlined,
  ExperimentOutlined,
  ThunderboltOutlined,
  DropboxOutlined,
  CheckCircleOutlined,
  ExclamationCircleOutlined,
  BarChartOutlined
} from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { selectApp } from '../../store';
import { fetchDeviceStatus } from '../../store/appSlice';
import SensorCharts from '../Charts/SensorCharts';

const Dashboard = () => {
  const dispatch = useDispatch();
  const { deviceData, deviceStatus, isLoading, error, settings, selectedSpecies } = useSelector(selectApp);

  useEffect(() => {
    // Fetch initial data
    dispatch(fetchDeviceStatus());

    // Set up polling with configurable interval
    const interval = setInterval(() => {
      dispatch(fetchDeviceStatus());
    }, settings.updateInterval || 5000);

    return () => clearInterval(interval);
  }, [dispatch, settings.updateInterval]);

  const getSystemHealthColor = () => {
    if (!deviceData) return 'error';

    const { ph, temperature } = deviceData;

    if (selectedSpecies) {
      const phInRange = ph >= selectedSpecies.idealPhMin && ph <= selectedSpecies.idealPhMax;
      const tempInRange = temperature >= selectedSpecies.idealTempMin && temperature <= selectedSpecies.idealTempMax;

      if (phInRange && tempInRange) return 'success';
      if (phInRange || tempInRange) return 'warning';
    }

    return 'error';
  };

  const getSystemHealthText = () => {
    const health = getSystemHealthColor();
    switch (health) {
      case 'success': return 'System Healthy';
      case 'warning': return 'System Warning';
      default: return 'System Error';
    }
  };

  if (isLoading && !deviceData) {
    return (
      <div style={{ textAlign: 'center', padding: '50px' }}>
        <Spin size="large" />
        <p>Loading device data...</p>
      </div>
    );
  }

  return (
    <div>
      <Row gutter={[16, 16]}>
        {/* pH Level Card */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            hoverable
            style={{
              background: 'linear-gradient(135deg, #00bcd4 0%, #0097a7 100%)',
              color: 'white'
            }}
          >
            <Statistic
              title={
                <span style={{ color: 'white' }}>
                  <ExperimentOutlined style={{ marginRight: '8px' }} />
                  pH Level
                </span>
              }
              value={deviceData?.ph || 0}
              precision={2}
              valueStyle={{ color: 'white', fontSize: '24px' }}
              suffix="pH"
            />
            {deviceData?.ph && (
              <Progress
                percent={Math.min((deviceData.ph / 14) * 100, 100)}
                showInfo={false}
                strokeColor="rgba(255,255,255,0.3)"
                trailColor="rgba(255,255,255,0.1)"
                style={{ marginTop: '10px' }}
              />
            )}
          </Card>
        </Col>

        {/* Temperature Card */}
        <Col xs={24} sm={12} lg={6}>
          <Card
            hoverable
            style={{
              background: 'linear-gradient(135deg, #ff6b6b 0%, #ee5a52 100%)',
              color: 'white'
            }}
          >
            <Statistic
              title={
                <span style={{ color: 'white' }}>
                  <FireOutlined style={{ marginRight: '8px' }} />
                  Temperature
                </span>
              }
              value={deviceData?.temperature || 0}
              precision={1}
              valueStyle={{ color: 'white', fontSize: '24px' }}
              suffix="°C"
            />
            {deviceData?.temperature && (
              <Progress
                percent={Math.min((deviceData.temperature / 40) * 100, 100)}
                showInfo={false}
                strokeColor="rgba(255,255,255,0.3)"
                trailColor="rgba(255,255,255,0.1)"
                style={{ marginTop: '10px' }}
              />
            )}
          </Card>
        </Col>

        {/* Fan Status Card */}
        <Col xs={24} sm={12} lg={6}>
          <Card hoverable>
            <Statistic
              title={
                <span>
                  <ThunderboltOutlined style={{ marginRight: '8px', color: '#00bcd4' }} />
                  Fan Status
                </span>
              }
              value={deviceData?.fan ? 'ON' : 'OFF'}
              valueStyle={{
                color: deviceData?.fan ? '#52c41a' : '#ff4d4f',
                fontSize: '24px'
              }}
            />
            <div style={{ marginTop: '10px' }}>
              {deviceData?.fan ? (
                <CheckCircleOutlined style={{ color: '#52c41a', fontSize: '20px' }} />
              ) : (
                <ExclamationCircleOutlined style={{ color: '#ff4d4f', fontSize: '20px' }} />
              )}
            </div>
          </Card>
        </Col>

        {/* Pumps Status Card */}
        <Col xs={24} sm={12} lg={6}>
          <Card hoverable>
            <Statistic
              title={
                <span>
                  <DropboxOutlined style={{ marginRight: '8px', color: '#00bcd4' }} />
                  Pumps Status
                </span>
              }
              value={`${deviceData?.acidPump ? 'A' : ''}${deviceData?.basePump ? 'B' : ''}` || 'None'}
              valueStyle={{
                color: (deviceData?.acidPump || deviceData?.basePump) ? '#52c41a' : '#d9d9d9',
                fontSize: '24px'
              }}
            />
            <div style={{ marginTop: '10px', fontSize: '12px', color: '#666' }}>
              Acid: {deviceData?.acidPump ? 'ON' : 'OFF'} | Base: {deviceData?.basePump ? 'ON' : 'OFF'}
            </div>
          </Card>
        </Col>
      </Row>

      {/* System Health Alert */}
      <Row style={{ marginTop: '24px' }}>
        <Col span={24}>
          <Alert
            message={getSystemHealthText()}
            type={getSystemHealthColor()}
            showIcon
            style={{ fontSize: '16px' }}
            description={
              deviceData ? (
                <div>
                  <p>Current readings: pH {deviceData.ph?.toFixed(2)}, Temperature {deviceData.temperature?.toFixed(1)}°C</p>
                  <p>Last update: {new Date(deviceStatus.lastUpdate).toLocaleString()}</p>
                </div>
              ) : (
                'No device data available'
              )
            }
          />
        </Col>
      </Row>

      {/* Error Display */}
      {error && (
        <Row style={{ marginTop: '16px' }}>
          <Col span={24}>
            <Alert
              message="Connection Error"
              description={error}
              type="error"
              showIcon
              closable
            />
          </Col>
        </Row>
      )}

      {/* Charts Section */}
      <Divider style={{ marginTop: '32px', marginBottom: '24px' }}>
        <Space>
          <BarChartOutlined style={{ color: '#00bcd4' }} />
          <span style={{ fontSize: '16px', fontWeight: 'bold' }}>Sensor Trends</span>
        </Space>
      </Divider>

      <SensorCharts />
    </div>
  );
};

export default Dashboard;
