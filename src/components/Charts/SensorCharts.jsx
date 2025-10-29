import React from 'react';
import { Card, Row, Col } from 'antd';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { useSelector } from 'react-redux';
import { selectApp } from '../../store';

const SensorCharts = () => {
  const { chartData, deviceData } = useSelector(selectApp);

  // Prepare chart data
  const chartDataFormatted = chartData.map((point, index) => ({
    time: point.time,
    ph: point.ph,
    temperature: point.temperature,
    index: index + 1
  }));

  const CustomTooltip = ({ active, payload, label }) => {
    if (active && payload && payload.length) {
      return (
        <div style={{
          background: 'white',
          border: '1px solid #ccc',
          borderRadius: '6px',
          padding: '10px',
          boxShadow: '0 2px 8px rgba(0,0,0,0.1)'
        }}>
          <p style={{ margin: 0, fontWeight: 'bold' }}>{`Time: ${label}`}</p>
          {payload.map((entry, index) => (
            <p key={index} style={{ margin: '2px 0', color: entry.color }}>
              {`${entry.name}: ${entry.value}${entry.name === 'ph' ? ' pH' : '°C'}`}
            </p>
          ))}
        </div>
      );
    }
    return null;
  };

  return (
    <Row gutter={[16, 16]}>
      {/* pH Chart */}
      <Col xs={24} lg={12}>
        <Card
          title="pH Level Trend"
          style={{ height: '400px' }}
          bodyStyle={{ height: 'calc(100% - 57px)', padding: '16px' }}
        >
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={chartDataFormatted}>
              <CartesianGrid strokeDasharray="3 3" stroke="#f0f0f0" />
              <XAxis 
                dataKey="time" 
                stroke="#666"
                fontSize={12}
                tick={{ fontSize: 10 }}
              />
              <YAxis 
                stroke="#666"
                fontSize={12}
                domain={[0, 14]}
                label={{ value: 'pH', angle: -90, position: 'insideLeft' }}
              />
              <Tooltip content={<CustomTooltip />} />
              <Legend />
              <Line
                type="monotone"
                dataKey="ph"
                stroke="#00bcd4"
                strokeWidth={3}
                dot={{ fill: '#00bcd4', strokeWidth: 2, r: 4 }}
                activeDot={{ r: 6, stroke: '#00bcd4', strokeWidth: 2 }}
                name="pH Level"
              />
            </LineChart>
          </ResponsiveContainer>
        </Card>
      </Col>

      {/* Temperature Chart */}
      <Col xs={24} lg={12}>
        <Card
          title="Temperature Trend"
          style={{ height: '400px' }}
          bodyStyle={{ height: 'calc(100% - 57px)', padding: '16px' }}
        >
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={chartDataFormatted}>
              <CartesianGrid strokeDasharray="3 3" stroke="#f0f0f0" />
              <XAxis 
                dataKey="time" 
                stroke="#666"
                fontSize={12}
                tick={{ fontSize: 10 }}
              />
              <YAxis 
                stroke="#666"
                fontSize={12}
                domain={[0, 40]}
                label={{ value: '°C', angle: -90, position: 'insideLeft' }}
              />
              <Tooltip content={<CustomTooltip />} />
              <Legend />
              <Line
                type="monotone"
                dataKey="temperature"
                stroke="#ff6b6b"
                strokeWidth={3}
                dot={{ fill: '#ff6b6b', strokeWidth: 2, r: 4 }}
                activeDot={{ r: 6, stroke: '#ff6b6b', strokeWidth: 2 }}
                name="Temperature"
              />
            </LineChart>
          </ResponsiveContainer>
        </Card>
      </Col>

      {/* Combined Chart */}
      <Col span={24}>
        <Card
          title="Combined Sensor Readings"
          style={{ height: '400px' }}
          bodyStyle={{ height: 'calc(100% - 57px)', padding: '16px' }}
        >
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={chartDataFormatted}>
              <CartesianGrid strokeDasharray="3 3" stroke="#f0f0f0" />
              <XAxis 
                dataKey="time" 
                stroke="#666"
                fontSize={12}
                tick={{ fontSize: 10 }}
              />
              <YAxis 
                yAxisId="ph"
                stroke="#00bcd4"
                fontSize={12}
                domain={[0, 14]}
                label={{ value: 'pH', angle: -90, position: 'insideLeft' }}
              />
              <YAxis 
                yAxisId="temp"
                orientation="right"
                stroke="#ff6b6b"
                fontSize={12}
                domain={[0, 40]}
                label={{ value: '°C', angle: 90, position: 'insideRight' }}
              />
              <Tooltip content={<CustomTooltip />} />
              <Legend />
              <Line
                yAxisId="ph"
                type="monotone"
                dataKey="ph"
                stroke="#00bcd4"
                strokeWidth={3}
                dot={{ fill: '#00bcd4', strokeWidth: 2, r: 4 }}
                activeDot={{ r: 6, stroke: '#00bcd4', strokeWidth: 2 }}
                name="pH Level"
              />
              <Line
                yAxisId="temp"
                type="monotone"
                dataKey="temperature"
                stroke="#ff6b6b"
                strokeWidth={3}
                dot={{ fill: '#ff6b6b', strokeWidth: 2, r: 4 }}
                activeDot={{ r: 6, stroke: '#ff6b6b', strokeWidth: 2 }}
                name="Temperature"
              />
            </LineChart>
          </ResponsiveContainer>
        </Card>
      </Col>
    </Row>
  );
};

export default SensorCharts;
