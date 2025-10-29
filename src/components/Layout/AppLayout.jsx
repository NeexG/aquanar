import React, { useState, useEffect } from 'react';
import { Layout, Menu, Typography, Button, Space, Badge, Drawer } from 'antd';
import {
  DashboardOutlined,
  ControlOutlined,
  DatabaseOutlined,
  SettingOutlined,
  WifiOutlined,
  DisconnectOutlined,
  MenuOutlined,
  CloseOutlined
} from '@ant-design/icons';
import { useSelector, useDispatch } from 'react-redux';
import { selectApp } from '../../store';
import { testConnection } from '../../store/appSlice';

const { Header, Sider, Content } = Layout;
const { Title } = Typography;

const AppLayout = ({ children }) => {
  const dispatch = useDispatch();
  const { currentPage, deviceStatus, notifications } = useSelector(selectApp);
  const [collapsed, setCollapsed] = useState(false);
  const [isMobile, setIsMobile] = useState(false);
  const [mobileDrawerVisible, setMobileDrawerVisible] = useState(false);

  // Check if device is mobile
  useEffect(() => {
    const checkIsMobile = () => {
      setIsMobile(window.innerWidth < 768);
      if (window.innerWidth >= 768) {
        setMobileDrawerVisible(false);
      }
    };

    checkIsMobile();
    window.addEventListener('resize', checkIsMobile);
    return () => window.removeEventListener('resize', checkIsMobile);
  }, []);

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
    // Close mobile drawer after selection
    if (isMobile) {
      setMobileDrawerVisible(false);
    }
  };

  const handleReconnect = () => {
    dispatch(testConnection());
  };

  const toggleCollapsed = () => {
    setCollapsed(!collapsed);
  };

  const showMobileDrawer = () => {
    setMobileDrawerVisible(true);
  };

  const hideMobileDrawer = () => {
    setMobileDrawerVisible(false);
  };

  // Sidebar content component
  const SidebarContent = () => (
    <>
      <div style={{ padding: '20px', textAlign: 'center' }}>
        {collapsed ? (
          <div style={{ fontSize: '32px', color: 'white' }}>
            🐠
          </div>
        ) : (
          <>
            <Title level={3} style={{ color: 'white', margin: 0 }}>
              🐠 Smart Breeder
            </Title>
            
          </>
        )}
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
    </>
  );

  return (
    <Layout style={{ minHeight: '100vh' }}>
      {/* Desktop Sidebar */}
      {!isMobile && (
        <Sider
          collapsible
          collapsed={collapsed}
          onCollapse={toggleCollapsed}
          width={250}
          collapsedWidth={80}
          style={{
            background: 'linear-gradient(180deg, #00bcd4 0%, #0097a7 100%)',
            boxShadow: '2px 0 8px rgba(0,0,0,0.1)',
            position: 'fixed',
            left: 0,
            top: 0,
            bottom: 0,
            zIndex: 1000,
            overflow: 'auto'
          }}
          trigger={
            <div style={{
              color: 'white',
              fontSize: '16px',
              padding: '10px',
              textAlign: 'center',
              cursor: 'pointer'
            }}>
              {collapsed ? '→' : '←'}
            </div>
          }
        >
          <SidebarContent />
        </Sider>
      )}

      <Layout style={{ marginLeft: !isMobile ? (collapsed ? 80 : 250) : 0 }}>
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
          <Space>
            {/* Mobile Menu Button */}
            {isMobile && (
              <Button
                type="text"
                icon={<MenuOutlined />}
                onClick={showMobileDrawer}
                style={{ fontSize: '18px' }}
              />
            )}

            <Title level={2} style={{ margin: 0, color: '#00bcd4' }}>
              Smart Breeder Dashboard
            </Title>
          </Space>

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
            margin: isMobile ? '16px' : '24px',
            padding: isMobile ? '16px' : '24px',
            background: '#f5f5f5',
            borderRadius: '8px',
            minHeight: isMobile ? 'calc(100vh - 80px)' : 'calc(100vh - 112px)'
          }}
        >
          {children}
        </Content>
      </Layout>

      {/* Mobile Drawer */}
      <Drawer
        placement="left"
        onClose={hideMobileDrawer}
        open={mobileDrawerVisible}
        width={280}
        styles={{
          body: {
            background: 'linear-gradient(180deg, #00bcd4 0%, #0097a7 100%)',
            padding: 0
          },
          header: {
            background: 'linear-gradient(180deg, #00bcd4 0%, #0097a7 100%)',
            borderBottom: 'none'
          }
        }}
        closeIcon={<CloseOutlined style={{ color: 'white' }} />}
      >
        <SidebarContent />
      </Drawer>
    </Layout>
  );
};

export default AppLayout;
