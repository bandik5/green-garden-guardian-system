
import React from 'react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { SensorData } from '@/types';

interface TemperatureChartProps {
  data: SensorData[];
  threshold: number;
}

const TemperatureChart: React.FC<TemperatureChartProps> = ({ data, threshold }) => {
  // Format timestamp for display
  const formatTime = (timestamp: number) => {
    return new Date(timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  };
  
  // Format data for recharts
  const chartData = data.map(point => ({
    time: formatTime(point.timestamp),
    temp: point.temperature,
    humidity: point.humidity,
    timestamp: point.timestamp  // Keep original timestamp for sorting
  }));
  
  // Sort by timestamp to ensure proper ordering
  chartData.sort((a, b) => a.timestamp - b.timestamp);
  
  return (
    <Card>
      <CardHeader className="pb-2">
        <CardTitle>Temperature History</CardTitle>
      </CardHeader>
      <CardContent className="pt-0">
        <div className="h-[240px] w-full">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart
              data={chartData}
              margin={{ top: 10, right: 10, left: -25, bottom: 0 }}
            >
              <CartesianGrid strokeDasharray="3 3" opacity={0.3} />
              <XAxis 
                dataKey="time" 
                tick={{ fontSize: 10 }}
                interval="preserveStartEnd"
                minTickGap={20}
              />
              <YAxis 
                tick={{ fontSize: 10 }}
                domain={['auto', 'auto']}
              />
              <Tooltip />
              <Line 
                type="monotone" 
                dataKey="temp" 
                stroke="#16a34a" 
                strokeWidth={2} 
                activeDot={{ r: 6 }} 
                dot={false}
                name="Temperature (°C)"
              />
              <Line 
                type="monotone" 
                dataKey="humidity" 
                stroke="#3b82f6" 
                strokeWidth={2}
                activeDot={{ r: 6 }} 
                dot={false}
                name="Humidity (%)"
              />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </CardContent>
    </Card>
  );
};

export default TemperatureChart;
