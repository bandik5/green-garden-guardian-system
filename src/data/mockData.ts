
import { Greenhouse } from "@/types";

// Generate a timestamp for the last hour
const getRecentTimestamp = (minutesAgo: number = 0) => {
  return Date.now() - (minutesAgo * 60 * 1000);
};

// Generate random data in a realistic range
const generateMockSensorData = (baseTemp: number, minAgo: number) => ({
  temperature: +(baseTemp + (Math.random() * 2 - 1)).toFixed(1),
  humidity: +(60 + (Math.random() * 20)).toFixed(1),
  pressure: +(1013 + (Math.random() * 10 - 5)).toFixed(1),
  timestamp: getRecentTimestamp(minAgo)
});

// Generate historical data points
const generateHistoricalData = (baseTemp: number, count: number) => {
  return Array.from({ length: count }).map((_, i) => 
    generateMockSensorData(baseTemp, count - i)
  );
};

// Generate mock data for all greenhouses
export const generateMockGreenhouseData = (): Record<number, Greenhouse> => {
  const greenhouses: Record<number, Greenhouse> = {};
  
  for (let i = 1; i <= 6; i++) {
    const baseTemp = 23 + (i % 3);
    
    greenhouses[i] = {
      id: i,
      name: `Greenhouse ${i}`,
      currentData: generateMockSensorData(baseTemp, 0),
      settings: {
        temperatureThreshold: 25,
        hysteresis: 0.5,
        mode: 'auto',
        ventStatus: baseTemp > 25 ? 'open' : 'closed',
      },
      history: generateHistoricalData(baseTemp, 60) // Last hour of data
    };
  }
  
  return greenhouses;
};
