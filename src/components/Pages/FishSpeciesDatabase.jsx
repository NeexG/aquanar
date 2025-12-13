import React, { useEffect, useMemo } from 'react';
import { Table, Button, Space, Tag, message, notification, Card, Row, Col, Statistic } from 'antd';
import { DatabaseOutlined, CheckCircleOutlined, InfoCircleOutlined } from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { selectApp } from '../../store';
import { setSelectedSpecies, sendSpeciesConfig, setFishSpecies } from '../../store/appSlice';
import { apiService } from '../../services/apiService';

// Helper function to normalize fish species data (handle both nested and flat formats)
const normalizeSpeciesData = (species) => {
  // If data comes from API with nested format: idealPh: { min, max }, idealTemp: { min, max }
  if (species.idealPh && typeof species.idealPh === 'object') {
    return {
      ...species,
      idealPhMin: species.idealPh.min,
      idealPhMax: species.idealPh.max,
      idealTempMin: species.idealTemp.min,
      idealTempMax: species.idealTemp.max
    };
  }
  // If data is already in flat format: idealPhMin, idealPhMax, etc.
  return species;
};

const FishSpeciesDatabase = () => {
  const dispatch = useDispatch();
  const { fishSpecies, selectedSpecies, deviceData, isLoading } = useSelector(selectApp);
  
  // Normalize fish species data to handle both API format and default format
  const normalizedFishSpecies = useMemo(() => {
    return fishSpecies.map(normalizeSpeciesData);
  }, [fishSpecies]);
  
  // Fetch fish species from ESP32 on component mount
  useEffect(() => {
    const fetchSpeciesFromDevice = async () => {
      try {
        const result = await apiService.getFishSpeciesList();
        if (result.success && result.data && Array.isArray(result.data)) {
          // Normalize and update the species list
          const normalized = result.data.map(normalizeSpeciesData);
          // Update Redux store with fetched species from device
          dispatch(setFishSpecies(normalized));
          console.log('Fetched fish species from device:', normalized);
        }
      } catch (error) {
        console.warn('Could not fetch species from device, using defaults:', error);
        // Keep using default species from constants
      }
    };
    
    fetchSpeciesFromDevice();
  }, [dispatch]);

  const handleSelectSpecies = async (species) => {
    // Normalize species data before using
    const normalizedSpecies = normalizeSpeciesData(species);
    
    // Update selected species in Redux immediately for UI feedback
    dispatch(setSelectedSpecies(normalizedSpecies));

    try {
      // Send to ESP32 in the format it expects
      // This will trigger pH and temperature control based on the selected species
      const speciesData = {
        name: normalizedSpecies.name,
        idealPh: {
          min: normalizedSpecies.idealPhMin,
          max: normalizedSpecies.idealPhMax
        },
        idealTemp: {
          min: normalizedSpecies.idealTempMin,
          max: normalizedSpecies.idealTempMax
        }
      };

      console.log('Sending species config to ESP32:', speciesData);
      const result = await dispatch(sendSpeciesConfig(speciesData));

      if (result.type.endsWith('/fulfilled')) {
        message.success(`Species "${species.name}" selected and configured successfully`);

        notification.success({
          message: 'Species Configured',
          description: `${species.name} settings have been applied to the device`,
          duration: 4,
          placement: 'topRight',
        });
      } else {
        message.error('Failed to configure species settings');
        notification.error({
          message: 'Species Configuration Failed',
          description: `Failed to configure ${species.name} settings`,
          duration: 5,
          placement: 'topRight',
        });
      }
    } catch (error) {
      message.error('Error configuring species settings');
      notification.error({
        message: 'Species Configuration Error',
        description: `Error configuring ${species.name} settings`,
        duration: 5,
        placement: 'topRight',
      });
    }
  };

  const columns = [
    {
      title: 'Species Name',
      dataIndex: 'name',
      key: 'name',
      render: (text, record) => (
        <Space>
          <strong>{text}</strong>
          {selectedSpecies?.id === record.id && (
            <Tag color="green" icon={<CheckCircleOutlined />}>
              Selected
            </Tag>
          )}
        </Space>
      )
    },
    {
      title: 'Ideal pH Range',
      dataIndex: 'idealPh',
      key: 'idealPh',
      render: (_, record) => (
        <Tag color="blue">
          {record.idealPhMin} - {record.idealPhMax}
        </Tag>
      )
    },
    {
      title: 'Ideal Temperature Range',
      dataIndex: 'idealTemp',
      key: 'idealTemp',
      render: (_, record) => (
        <Tag color="orange">
          {record.idealTempMin}°C - {record.idealTempMax}°C
        </Tag>
      )
    },
    {
      title: 'Description',
      dataIndex: 'description',
      key: 'description',
      ellipsis: true
    },
    {
      title: 'Actions',
      key: 'actions',
      render: (_, record) => (
        <Space>
          <Button
            type={selectedSpecies?.id === record.id ? 'default' : 'primary'}
            onClick={() => handleSelectSpecies(record)}
            loading={isLoading}
            icon={<CheckCircleOutlined />}
          >
            {selectedSpecies?.id === record.id ? 'Selected' : 'Select Species'}
          </Button>
        </Space>
      )
    }
  ];

  const getCurrentStatus = () => {
    if (!deviceData || !selectedSpecies) {
      return { phStatus: 'unknown', tempStatus: 'unknown' };
    }

    const { ph, temperature } = deviceData;
    const phInRange = ph >= selectedSpecies.idealPhMin && ph <= selectedSpecies.idealPhMax;
    const tempInRange = temperature >= selectedSpecies.idealTempMin && temperature <= selectedSpecies.idealTempMax;

    return {
      phStatus: phInRange ? 'good' : 'warning',
      tempStatus: tempInRange ? 'good' : 'warning'
    };
  };

  const status = getCurrentStatus();

  return (
    <div>
      {/* Selected Species Status */}
      {selectedSpecies && (
        <Row gutter={[16, 16]} style={{ marginBottom: '24px' }}>
          <Col span={24}>
            <Card
              title={
                <Space>
                  <InfoCircleOutlined style={{ color: '#00bcd4' }} />
                  Current Species: {selectedSpecies.name}
                </Space>
              }
              style={{ background: 'linear-gradient(135deg, #f0f9ff 0%, #e0f2fe 100%)' }}
            >
              <Row gutter={[16, 16]}>
                <Col xs={24} sm={12} lg={6}>
                  <Statistic
                    title="Ideal pH Range"
                    value={`${selectedSpecies.idealPhMin} - ${selectedSpecies.idealPhMax}`}
                    suffix="pH"
                    valueStyle={{ color: status.phStatus === 'good' ? '#52c41a' : '#ff4d4f' }}
                  />
                </Col>
                <Col xs={24} sm={12} lg={6}>
                  <Statistic
                    title="Ideal Temperature Range"
                    value={`${selectedSpecies.idealTempMin} - ${selectedSpecies.idealTempMax}`}
                    suffix="°C"
                    valueStyle={{ color: status.tempStatus === 'good' ? '#52c41a' : '#ff4d4f' }}
                  />
                </Col>
                <Col xs={24} sm={12} lg={6}>
                  <Statistic
                    title="Current pH"
                    value={deviceData?.ph?.toFixed(2) || 'N/A'}
                    suffix="pH"
                    valueStyle={{ color: status.phStatus === 'good' ? '#52c41a' : '#ff4d4f' }}
                  />
                </Col>
                <Col xs={24} sm={12} lg={6}>
                  <Statistic
                    title="Current Temperature"
                    value={deviceData?.temperature?.toFixed(1) || 'N/A'}
                    suffix="°C"
                    valueStyle={{ color: status.tempStatus === 'good' ? '#52c41a' : '#ff4d4f' }}
                  />
                </Col>
              </Row>
              <div style={{ marginTop: '16px', padding: '12px', background: 'white', borderRadius: '6px' }}>
                <strong>Description:</strong> {selectedSpecies.description}
                {status.phAction && (
                  <div style={{ marginTop: '8px', padding: '8px', background: '#fff3cd', borderRadius: '4px', color: '#856404' }}>
                    <InfoCircleOutlined style={{ marginRight: '4px' }} />
                    <strong>Auto Control:</strong> {status.phAction}. pH checks every 1 minute.
                  </div>
                )}
              </div>
            </Card>
          </Col>
        </Row>
      )}

      {/* Species Table */}
      <Card
        title={
          <Space>
            <DatabaseOutlined style={{ color: '#00bcd4' }} />
            Fish Species Database
          </Space>
        }
      >
        <Table
          columns={columns}
          dataSource={normalizedFishSpecies}
          rowKey="id"
          pagination={{
            pageSize: 10,
            showSizeChanger: true,
            showQuickJumper: true,
            showTotal: (total, range) => `${range[0]}-${range[1]} of ${total} species`
          }}
          scroll={{ x: 800 }}
        />
      </Card>

      {/* Instructions */}
      <Card
        title="Instructions"
        style={{ marginTop: '24px' }}
        type="inner"
      >
        <div style={{ lineHeight: '1.6' }}>
          <p><strong>How to use:</strong></p>
          <ol>
            <li>Select a fish species from the table above</li>
            <li>The system will automatically configure the ideal pH and temperature ranges</li>
            <li>Monitor the dashboard to see if current readings are within the ideal ranges</li>
            <li>Use device controls to manually adjust pH and temperature if needed</li>
          </ol>
          <p style={{ color: '#666', marginTop: '16px' }}>
            <InfoCircleOutlined style={{ marginRight: '8px' }} />
            The selected species settings will be sent to your ESP32 device for automatic monitoring.
          </p>
        </div>
      </Card>
    </div>
  );
};

export default FishSpeciesDatabase;
