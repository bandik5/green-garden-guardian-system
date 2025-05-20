
export interface SensorData {
  temperature: number;
  humidity: number;
  pressure: number;
  timestamp: number;
}

export interface GreenhouseSettings {
  temperatureThreshold: number;
  hysteresis: number;
  mode: 'auto' | 'manual';
  ventStatus: 'closed' | 'opening' | 'open' | 'closing';
  manualControl?: 'open' | 'close' | 'stop';
}

export interface Greenhouse {
  id: number;
  name: string;
  currentData: SensorData;
  settings: GreenhouseSettings;
  history: SensorData[];
}

export type GreenhouseState = {
  greenhouses: Record<number, Greenhouse>;
  loading: boolean;
  error: string | null;
};
