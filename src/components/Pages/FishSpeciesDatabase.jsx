import React, { useEffect, useMemo } from 'react';
import { Table, Button, Space, Tag, message, notification, Card, Row, Col, Statistic, Popconfirm } from 'antd';
import { DatabaseOutlined, CheckCircleOutlined, InfoCircleOutlined, CloseCircleOutlined } from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { selectApp } from '../../store';
import { setSelectedSpecies, sendSpeciesConfig, setFishSpecies } from '../../store/appSlice';
import { apiService } from '../../services/apiService';
import { DEFAULT_FISH_SPECIES } from '../../constants';

// Helper function to normalize fish species data (handle both nested and flat formats)
const normalizeSpeciesData = (species) => {
  // Helper to convert string/boolean to boolean
  const toBoolean = (value) => {
    if (value === true || value === 'true' || value === 1 || value === '1') return true;
    if (value === false || value === 'false' || value === 0 || value === '0') return false;
    return false; // Default to false
  };
  
  // If data comes from API with nested format: idealPh: { min, max }, idealTemp: { min, max }
  if (species.idealPh && typeof species.idealPh === 'object') {
    return {
      ...species,
      idealPhMin: species.idealPh.min,
      idealPhMax: species.idealPh.max,
      idealTempMin: species.idealTemp.min,
      idealTempMax: species.idealTemp.max,
      waterFlow: toBoolean(species.waterFlow),
      rain: toBoolean(species.rain)
    };
  }
  // If data is already in flat format: idealPhMin, idealPhMax, etc.
  // Ensure waterFlow and rain have default values and convert to boolean
  return {
    ...species,
    waterFlow: toBoolean(species.waterFlow),
    rain: toBoolean(species.rain)
  };
};

const FishSpeciesDatabase = () => {
  const dispatch = useDispatch();
  const { fishSpecies, selectedSpecies, deviceData, isLoading } = useSelector(selectApp);
  
  // Normalize fish species data to handle both API format and default format
  const normalizedFishSpecies = useMemo(() => {
    return fishSpecies.map(normalizeSpeciesData);
  }, [fishSpecies]);
  
  // Fetch fish species from ESP32 on component mount and when component updates
  useEffect(() => {
    const fetchSpeciesFromDevice = async () => {
      try {
        const result = await apiService.getFishSpeciesList();
        if (result.success && result.data && Array.isArray(result.data)) {
          console.log('Raw species data from ESP32:', JSON.stringify(result.data, null, 2));
          
          // Normalize and merge with defaults to preserve waterFlow and rain
          const normalized = result.data.map((esp32Species) => {
            // Find matching default species by ID or name
            const defaultSpecies = DEFAULT_FISH_SPECIES.find(s => 
              String(s.id) === String(esp32Species.id) || 
              s.name.toLowerCase() === esp32Species.name.toLowerCase()
            );
            
            // Normalize ESP32 data
            let normalized = normalizeSpeciesData(esp32Species);
            
            // Always use defaults for waterFlow and rain to ensure correct values
            // ESP32 provides pH and temp ranges, but we use our defaults for waterFlow/rain
            if (defaultSpecies) {
              // Always use default waterFlow and rain values
              normalized.waterFlow = defaultSpecies.waterFlow;
              normalized.rain = defaultSpecies.rain;
              
              // Preserve other default properties
              normalized.id = defaultSpecies.id;
              normalized.description = defaultSpecies.description || normalized.description;
              
              console.log(`Merged ${normalized.name}: waterFlow=${normalized.waterFlow}, rain=${normalized.rain} (from defaults)`);
            } else {
              console.warn(`No default found for ${esp32Species.name}, using ESP32 values`);
            }
            
            return normalized;
          });
          
          console.log('Final normalized species data:', JSON.stringify(normalized, null, 2));
          // Verify waterFlow and rain are preserved
          normalized.forEach((species, index) => {
            console.log(`Species ${index} (${species.name}): waterFlow=${species.waterFlow} (${typeof species.waterFlow}), rain=${species.rain} (${typeof species.rain})`);
          });
          // Update Redux store with fetched species from device
          dispatch(setFishSpecies(normalized));
        } else {
          console.warn('Invalid response from ESP32, using defaults');
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
        },
        waterFlow: normalizedSpecies.waterFlow !== undefined ? normalizedSpecies.waterFlow : false,
        rain: normalizedSpecies.rain !== undefined ? normalizedSpecies.rain : false
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
        
        // Don't refresh species list after selection - it overwrites correct values
        // The species list doesn't change when selecting a fish, so no need to refresh
        console.log('Species selected successfully, keeping current species list with correct waterFlow/rain values');
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

  const handleUnselectSpecies = async () => {
    try {
      // Send type 0 (FISH_NONE) to ESP32 to unselect fish
      const speciesData = {
        type: 0  // FISH_NONE = 0
      };

      console.log('Unselecting fish species - sending type 0 to ESP32');
      const result = await dispatch(sendSpeciesConfig(speciesData));

      if (result.type.endsWith('/fulfilled')) {
        // Clear selected species in Redux
        dispatch(setSelectedSpecies(null));
        
        message.success('Fish species unselected successfully');

        notification.success({
          message: 'Species Unselected',
          description: 'All relays (except light) have been turned OFF. Select a fish species to activate them.',
          duration: 4,
          placement: 'topRight',
        });
      } else {
        message.error('Failed to unselect species');
        notification.error({
          message: 'Unselect Failed',
          description: 'Failed to unselect fish species',
          duration: 5,
          placement: 'topRight',
        });
      }
    } catch (error) {
      message.error('Error unselecting species');
      notification.error({
        message: 'Unselect Error',
        description: 'Error unselecting fish species',
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
      title: 'Water Flow',
      dataIndex: 'waterFlow',
      key: 'waterFlow',
      render: (_, record) => (
        <Tag color={record.waterFlow ? 'green' : 'default'}>
          {record.waterFlow ? 'Yes' : 'No'}
        </Tag>
      )
    },
    {
      title: 'Rain',
      dataIndex: 'rain',
      key: 'rain',
      render: (_, record) => (
        <Tag color={record.rain ? 'blue' : 'default'}>
          {record.rain ? 'Yes' : 'No'}
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
          {selectedSpecies?.id === record.id ? (
            <>
              <Button
                type="default"
                disabled
                icon={<CheckCircleOutlined />}
              >
                Selected
              </Button>
              <Popconfirm
                title="Unselect Fish Species"
                description="Are you sure you want to unselect this fish species? All relays (except light) will be turned OFF."
                onConfirm={handleUnselectSpecies}
                okText="Yes, Unselect"
                cancelText="Cancel"
                okButtonProps={{ danger: true }}
              >
                <Button
                  type="default"
                  danger
                  icon={<CloseCircleOutlined />}
                  loading={isLoading}
                >
                  Unselect
                </Button>
              </Popconfirm>
            </>
          ) : (
            <Button
              type="primary"
              onClick={() => handleSelectSpecies(record)}
              loading={isLoading}
              icon={<CheckCircleOutlined />}
            >
              Select Species
            </Button>
          )}
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
                  <Popconfirm
                    title="Unselect Fish Species"
                    description="Are you sure you want to unselect this fish species? All relays (except light) will be turned OFF."
                    onConfirm={handleUnselectSpecies}
                    okText="Yes, Unselect"
                    cancelText="Cancel"
                    okButtonProps={{ danger: true }}
                  >
                    <Button
                      type="default"
                      danger
                      icon={<CloseCircleOutlined />}
                      loading={isLoading}
                      size="small"
                    >
                      Unselect
                    </Button>
                  </Popconfirm>
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
              <Row gutter={[16, 16]} style={{ marginTop: '16px' }}>
                <Col xs={24} sm={12} lg={6}>
                  <Statistic
                    title="Water Flow"
                    value={selectedSpecies.waterFlow ? 'ON' : 'OFF'}
                    valueStyle={{ color: selectedSpecies.waterFlow ? '#52c41a' : '#d9d9d9' }}
                  />
                </Col>
                <Col xs={24} sm={12} lg={6}>
                  <Statistic
                    title="Rain"
                    value={selectedSpecies.rain ? 'ON' : 'OFF'}
                    valueStyle={{ color: selectedSpecies.rain ? '#52c41a' : '#d9d9d9' }}
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
