
import React from 'react';
import { useParams, Navigate } from 'react-router-dom';
import { useGreenhouseData } from '@/hooks/useGreenhouseData';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Separator } from '@/components/ui/separator';
import { Alert, AlertDescription } from "@/components/ui/alert";
import { CircleX, ChevronLeft, Thermometer, Cloud, Fan, CircleArrowUp } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Link } from 'react-router-dom';
import TemperatureChart from '@/components/TemperatureChart';
import GreenhouseControls from '@/components/GreenhouseControls';
import { Skeleton } from '@/components/ui/skeleton';

const GreenhouseDetail: React.FC = () => {
  const { id } = useParams<{ id: string }>();
  const { greenhouses, loading, error, updateGreenhouseSettings } = useGreenhouseData();
  
  // Redirect if id is not valid
  if (id === undefined || isNaN(Number(id))) {
    return <Navigate to="/" replace />;
  }
  
  const greenhouseId = Number(id);
  
  if (error) {
    return (
      <div className="max-w-4xl mx-auto">
        <Alert variant="destructive">
          <CircleX className="h-4 w-4" />
          <AlertDescription>{error}</AlertDescription>
        </Alert>
      </div>
    );
  }
  
  // Loading state
  if (loading) {
    return (
      <div className="space-y-6">
        <div className="flex items-center justify-between">
          <div className="flex items-center">
            <Button variant="ghost" size="icon" asChild>
              <Link to="/">
                <ChevronLeft className="h-5 w-5" />
              </Link>
            </Button>
            <Skeleton className="h-8 w-40" />
          </div>
        </div>
        
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          <div className="md:col-span-2 space-y-6">
            <Skeleton className="h-[300px] w-full" />
            <Skeleton className="h-[200px] w-full" />
          </div>
          <div className="md:col-span-1">
            <Skeleton className="h-[400px] w-full" />
          </div>
        </div>
      </div>
    );
  }
  
  // Redirect if greenhouse doesn't exist
  const greenhouse = greenhouses[greenhouseId];
  if (!greenhouse) {
    return <Navigate to="/" replace />;
  }
  
  const { currentData, settings } = greenhouse;
  
  // Format timestamp
  const formatTime = (timestamp: number) => {
    return new Date(timestamp).toLocaleString();
  };
  
  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div className="flex items-center">
          <Button variant="ghost" size="icon" asChild className="mr-2">
            <Link to="/">
              <ChevronLeft className="h-5 w-5" />
            </Link>
          </Button>
          <h1 className="text-2xl font-bold">{greenhouse.name}</h1>
          
          <div className={`ml-4 px-2 py-0.5 text-xs rounded-full ${
            settings.mode === 'auto' ? 'bg-green-100 text-green-800' : 'bg-blue-100 text-blue-800'
          }`}>
            {settings.mode === 'auto' ? 'Auto' : 'Manual'}
          </div>
        </div>
        
        <div className={`px-2 py-1 text-sm rounded-md flex items-center ${
          settings.ventStatus === 'open' ? 'bg-blue-100 text-blue-800' :
          settings.ventStatus === 'closed' ? 'bg-gray-100 text-gray-800' : 
          'bg-amber-100 text-amber-800'
        }`}>
          <Fan className="h-4 w-4 mr-1" />
          Vents: {settings.ventStatus}
        </div>
      </div>
      
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <div className="md:col-span-2 space-y-6">
          {/* Current Readings */}
          <Card>
            <CardHeader>
              <CardTitle>Current Readings</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="grid grid-cols-3 gap-4">
                <div className="space-y-1">
                  <div className="flex items-center text-sm text-muted-foreground">
                    <Thermometer className="h-4 w-4 mr-1" />
                    Temperature
                  </div>
                  <p className="text-2xl font-bold">{currentData.temperature}°C</p>
                  <div className="text-xs text-muted-foreground">
                    Threshold: {settings.temperatureThreshold}°C
                  </div>
                </div>
                
                <div className="space-y-1">
                  <div className="flex items-center text-sm text-muted-foreground">
                    <Cloud className="h-4 w-4 mr-1" />
                    Humidity
                  </div>
                  <p className="text-2xl font-bold">{currentData.humidity}%</p>
                </div>
                
                <div className="space-y-1">
                  <div className="flex items-center text-sm text-muted-foreground">
                    <CircleArrowUp className="h-4 w-4 mr-1" />
                    Pressure
                  </div>
                  <p className="text-2xl font-bold">{currentData.pressure} hPa</p>
                </div>
              </div>
              
              <Separator className="my-4" />
              
              <div className="text-sm text-muted-foreground">
                Last updated: {formatTime(currentData.timestamp)}
              </div>
            </CardContent>
          </Card>
          
          {/* Temperature Chart */}
          <TemperatureChart 
            data={greenhouse.history} 
            threshold={settings.temperatureThreshold} 
          />
        </div>
        
        <div className="md:col-span-1">
          {/* Controls */}
          <GreenhouseControls 
            greenhouse={greenhouse} 
            onSettingsChange={updateGreenhouseSettings} 
          />
        </div>
      </div>
    </div>
  );
};

export default GreenhouseDetail;
