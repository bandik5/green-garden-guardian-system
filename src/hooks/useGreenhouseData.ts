
import { useState, useEffect } from 'react';
import { Greenhouse } from '@/types';
import { toast } from '@/components/ui/use-toast';

// Import mock data (this would be replaced with Firebase in production)
import { mockGreenhouseData } from '@/data/mockData';

export function useGreenhouseData() {
  const [greenhouses, setGreenhouses] = useState<Record<number, Greenhouse>>({});
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  
  // Load initial data
  useEffect(() => {
    try {
      // In a real implementation, this would fetch from Firebase
      const data = mockGreenhouseData;
      setGreenhouses(data);
      setLoading(false);
    } catch (err) {
      setError('Error loading greenhouse data');
      setLoading(false);
      console.error(err);
    }
  }, []);
  
  // Function to update settings for a specific greenhouse
  const updateGreenhouseSettings = (id: number, settings: Partial<Greenhouse['settings']>) => {
    setGreenhouses(prev => {
      if (!prev[id]) return prev;
      
      const updated = {
        ...prev,
        [id]: {
          ...prev[id],
          settings: {
            ...prev[id].settings,
            ...settings
          }
        }
      };
      
      // Show a notification for the change
      const settingName = Object.keys(settings)[0];
      let message = '';
      
      if (settingName === 'temperatureThreshold') {
        message = `Temperature threshold updated to ${settings.temperatureThreshold}°C`;
      } else if (settingName === 'mode') {
        message = `Mode changed to ${settings.mode}`;
      } else if (settingName === 'manualControl') {
        message = `Manual control: ${settings.manualControl}`;
      }
      
      toast({
        title: `Greenhouse ${id} Updated`,
        description: message,
      });
      
      // In a real implementation, this would update Firebase
      console.log(`Updating greenhouse ${id} settings:`, settings);
      
      return updated;
    });
  };
  
  // Function to control all greenhouses at once
  const controlAllGreenhouses = (action: 'open' | 'close') => {
    // Update all greenhouses in state
    setGreenhouses(prev => {
      const updated = { ...prev };
      
      Object.keys(updated).forEach(key => {
        const id = Number(key);
        updated[id] = {
          ...updated[id],
          settings: {
            ...updated[id].settings,
            mode: 'manual',
            manualControl: action,
            ventStatus: action === 'open' ? 'opening' : 'closing'
          }
        };
      });
      
      // In a real implementation, this would update Firebase
      console.log(`Controlling all greenhouses: ${action}`);
      
      return updated;
    });
    
    // In a real implementation, this delay would be handled by the ESP32 hub
    // Here we're simulating the state changes
    setTimeout(() => {
      setGreenhouses(prev => {
        const updated = { ...prev };
        
        Object.keys(updated).forEach(key => {
          const id = Number(key);
          updated[id] = {
            ...updated[id],
            settings: {
              ...updated[id].settings,
              manualControl: null,
              ventStatus: action === 'open' ? 'open' : 'closed'
            }
          };
        });
        
        return updated;
      });
    }, 2000);
  };
  
  return { 
    greenhouses, 
    loading, 
    error, 
    updateGreenhouseSettings,
    controlAllGreenhouses
  };
}
