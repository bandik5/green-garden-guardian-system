
import React from 'react';
import { Card, CardContent } from '@/components/ui/card';
import { Thermometer, Cloud, Fan } from 'lucide-react';
import { Link } from 'react-router-dom';
import { cn } from '@/lib/utils';
import { Greenhouse } from '@/types';

interface GlanceCardProps {
  greenhouse: Greenhouse;
  className?: string;
}

const GlanceCard: React.FC<GlanceCardProps> = ({ greenhouse, className }) => {
  const { currentData, settings } = greenhouse;
  
  // Determine status color
  const getStatusColor = () => {
    const temp = currentData.temperature;
    const threshold = settings.temperatureThreshold;
    
    if (temp > threshold + 1) return 'text-red-500';
    if (temp > threshold) return 'text-orange-400';
    if (temp < threshold - settings.hysteresis - 1) return 'text-blue-500';
    if (temp < threshold - settings.hysteresis) return 'text-blue-400';
    return 'text-green-500';
  };
  
  // Determine vent status icon and animation
  const getVentStatus = () => {
    switch (settings.ventStatus) {
      case 'open':
        return <Fan className="h-5 w-5 text-blue-500" />;
      case 'opening':
        return <Fan className="h-5 w-5 text-blue-500 animate-spin" style={{ animationDuration: '3s' }} />;
      case 'closing':
        return <Fan className="h-5 w-5 text-gray-500 animate-spin" style={{ animationDuration: '3s' }} />;
      default:
        return <Fan className="h-5 w-5 text-gray-500" />;
    }
  };
  
  return (
    <Link to={`/greenhouse/${greenhouse.id}`}>
      <Card className={cn("hover:shadow-md transition-shadow cursor-pointer h-full", className)}>
        <CardContent className="p-4">
          <div className="flex items-center justify-between mb-2">
            <h3 className="font-medium text-gray-800">{greenhouse.name}</h3>
            <div className="flex items-center">
              <span className={cn("flex items-center text-sm", settings.mode === 'auto' ? 'text-green-500' : 'text-blue-500')}>
                {settings.mode === 'auto' ? 'Auto' : 'Manual'}
              </span>
            </div>
          </div>
          
          <div className="flex flex-col space-y-3">
            <div className="flex items-center">
              <Thermometer className={cn("h-5 w-5 mr-2", getStatusColor())} />
              <span className="font-semibold text-lg">{currentData.temperature}°C</span>
              <span className="text-xs text-gray-500 ml-1">/ {settings.temperatureThreshold}°C</span>
            </div>
            
            <div className="flex items-center">
              <Cloud className="h-5 w-5 mr-2 text-blue-400" />
              <span>{currentData.humidity}%</span>
            </div>
            
            <div className="flex items-center justify-between">
              <span className="text-sm text-gray-500">Vents: {settings.ventStatus}</span>
              {getVentStatus()}
            </div>
          </div>
        </CardContent>
      </Card>
    </Link>
  );
};

export default GlanceCard;
