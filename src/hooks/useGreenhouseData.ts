
import { useState, useEffect } from 'react';
import { Greenhouse, GreenhouseState } from '@/types';
import { generateMockGreenhouseData } from '@/data/mockData';

// In a real implementation, this would connect to Firebase
export const useGreenhouseData = () => {
  const [state, setState] = useState<GreenhouseState>({
    greenhouses: {},
    loading: true,
    error: null
  });

  useEffect(() => {
    // Simulate API/Firebase fetch
    const fetchData = async () => {
      try {
        // In real implementation, this would be a Firebase call
        const data = generateMockGreenhouseData();
        
        setState({
          greenhouses: data,
          loading: false,
          error: null
        });
        
        // Set up real-time updates
        const interval = setInterval(() => {
          setState(prevState => {
            const updatedGreenhouses = { ...prevState.greenhouses };
            
            // Update each greenhouse with new sensor data
            Object.keys(updatedGreenhouses).forEach(key => {
              const id = Number(key);
              const greenhouse = updatedGreenhouses[id];
              const baseTemp = greenhouse.currentData.temperature;
              
              // Simulate small changes in temperature
              updatedGreenhouses[id] = {
                ...greenhouse,
                currentData: {
                  temperature: +(baseTemp + (Math.random() * 0.2 - 0.1)).toFixed(1),
                  humidity: +(greenhouse.currentData.humidity + (Math.random() * 1 - 0.5)).toFixed(1),
                  pressure: greenhouse.currentData.pressure,
                  timestamp: Date.now()
                }
              };
              
              // Update vent status based on temperature and mode
              if (greenhouse.settings.mode === 'auto') {
                const threshold = greenhouse.settings.temperatureThreshold;
                const hysteresis = greenhouse.settings.hysteresis;
                
                if (updatedGreenhouses[id].currentData.temperature > threshold && 
                    greenhouse.settings.ventStatus === 'closed') {
                  updatedGreenhouses[id].settings.ventStatus = 'opening';
                  // Simulate the delay for opening - in real app this would be handled by the ESP
                  setTimeout(() => {
                    setState(prevState => {
                      const updated = { ...prevState };
                      if (updated.greenhouses[id]) {
                        updated.greenhouses[id].settings.ventStatus = 'open';
                      }
                      return updated;
                    });
                  }, 5000);
                } else if (updatedGreenhouses[id].currentData.temperature < threshold - hysteresis && 
                           greenhouse.settings.ventStatus === 'open') {
                  updatedGreenhouses[id].settings.ventStatus = 'closing';
                  // Simulate the delay for closing
                  setTimeout(() => {
                    setState(prevState => {
                      const updated = { ...prevState };
                      if (updated.greenhouses[id]) {
                        updated.greenhouses[id].settings.ventStatus = 'closed';
                      }
                      return updated;
                    });
                  }, 5000);
                }
              }
            });
            
            return {
              ...prevState,
              greenhouses: updatedGreenhouses
            };
          });
        }, 10000); // Update every 10 seconds
        
        return () => clearInterval(interval);
      } catch (error) {
        setState({
          greenhouses: {},
          loading: false,
          error: "Failed to load greenhouse data"
        });
      }
    };

    fetchData();
  }, []);

  // Function to update settings for a specific greenhouse
  const updateGreenhouseSettings = (id: number, newSettings: Partial<Greenhouse['settings']>) => {
    setState(prevState => {
      if (!prevState.greenhouses[id]) return prevState;
      
      const updatedGreenhouse = {
        ...prevState.greenhouses[id],
        settings: {
          ...prevState.greenhouses[id].settings,
          ...newSettings
        }
      };
      
      // Handle manual controls
      if (newSettings.manualControl) {
        if (newSettings.manualControl === 'open' && updatedGreenhouse.settings.ventStatus === 'closed') {
          updatedGreenhouse.settings.ventStatus = 'opening';
          setTimeout(() => {
            setState(prevState => {
              const updated = { ...prevState };
              if (updated.greenhouses[id]) {
                updated.greenhouses[id].settings.ventStatus = 'open';
                // Clear the manual control command
                updated.greenhouses[id].settings.manualControl = undefined;
              }
              return updated;
            });
          }, 5000);
        } else if (newSettings.manualControl === 'close' && updatedGreenhouse.settings.ventStatus === 'open') {
          updatedGreenhouse.settings.ventStatus = 'closing';
          setTimeout(() => {
            setState(prevState => {
              const updated = { ...prevState };
              if (updated.greenhouses[id]) {
                updated.greenhouses[id].settings.ventStatus = 'closed';
                // Clear the manual control command
                updated.greenhouses[id].settings.manualControl = undefined;
              }
              return updated;
            });
          }, 5000);
        }
      }
      
      return {
        ...prevState,
        greenhouses: {
          ...prevState.greenhouses,
          [id]: updatedGreenhouse
        }
      };
    });
  };

  return {
    ...state,
    updateGreenhouseSettings
  };
};

export default useGreenhouseData;
