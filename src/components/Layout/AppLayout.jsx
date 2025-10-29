import React from 'react';
import { Layout, Menu, Typography, Button, Space, Badge } from 'antd';
import {
  DashboardOutlined,
  ControlOutlined,
  DatabaseOutlined,
  SettingOutlined,
  WifiOutlined,
  DisconnectOutlined
} from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { selectApp } from '../../store';
import { testConnection } from '../../store/appSlice';

const { Header, Sider, Content } = Layout;
const { Title } = Typography;

const AppLayout = ({ children }) => {
  const dispatch = useDispatch();
  const { currentPage, deviceStatus, notifications } = useSelector(selectApp);

  const menuItems = [
    {
      key: 'dashboard',
      icon: <DashboardOutlined />,
      label: 'Dashboard'
    },
    {
      key: 'device-control',
      icon: <ControlOutlined />,
      label: 'Device Control'
    },
    {
      key: 'species-database',
      icon: <DatabaseOutlined />,
      label: 'Fish Species Database'
    },
    {
      key: 'settings',
      icon: <SettingOutlined />,
      label: 'Settings'
    }
  ];

  const handleMenuClick = ({ key }) => {
    dispatch({ type: 'app/setCurrentPage', payload: key });
  };

  const handleReconnect = () => {
    dispatch(testConnection());
  };

  return (
    <Layout style={{ minHeight: '100vh' }}>
      <Sider
        width={250}
        style={{
          background: 'linear-gradient(180deg, #00bcd4 0%, #0097a7 100%)',
          boxShadow: '2px 0 8px rgba(0,0,0,0.1)'
        }}
      >
        <div style={{ padding: '20px', textAlign: 'center' }}>
          <Title level={3} style={{ color: 'white', margin: 0 }}>
            🐠 Smart Breeder
          </Title>
          <Title level={5} style={{ color: 'rgba(255,255,255,0.8)', margin: '5px 0 0 0' }}>
            Control Panel
          </Title>
        </div>
        
        <Menu
          theme="dark"
          mode="inline"
          selectedKeys={[currentPage]}
          items={menuItems}
          onClick={handleMenuClick}
          style={{
            background: 'transparent',
            border: 'none',
            marginTop: '20px'
          }}
        />
      </Sider>

      <Layout>
        <Header
          style={{
            background: 'white',
            padding: '0 24px',
            boxShadow: '0 2px 8px rgba(0,0,0,0.1)',
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center'
          }}
        >
          <Title level={2} style={{ margin: 0, color: '#00bcd4' }}>
            Smart Breeder Dashboard
          </Title>
          
          <Space size="middle">
            <Badge
              status={deviceStatus.connected ? 'success' : 'error'}
              text={
                <Space>
                  {deviceStatus.connected ? <WifiOutlined /> : <DisconnectOutlined />}
                  {deviceStatus.connected ? 'Connected' : 'Disconnected'}
                </Space>
              }
            />
            
            <Button
              type="primary"
              icon={<WifiOutlined />}
              onClick={handleReconnect}
              style={{
                background: '#00bcd4',
                borderColor: '#00bcd4'
              }}
            >
              Reconnect ESP32
            </Button>
            
            {notifications.length > 0 && (
              <Badge count={notifications.length} size="small">
                <Button type="text">Notifications</Button>
              </Badge>
            )}
          </Space>
        </Header>

        <Content
          style={{
            margin: '24px',
            padding: '24px',
            background: '#f5f5f5',
            borderRadius: '8px',
            minHeight: 'calc(100vh - 112px)'
          }}
        >
          {children}
        </Content>
      </Layout>
    </Layout>
  );
};

export default AppLayout;
