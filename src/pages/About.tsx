
import React from 'react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Separator } from '@/components/ui/separator';
import { Fan, Thermometer, Cloud, CircleArrowUp, Settings } from 'lucide-react';

const About: React.FC = () => {
  return (
    <div className="space-y-6 max-w-4xl">
      <div>
        <h1 className="text-2xl font-bold">About GreenControl</h1>
        <p className="text-muted-foreground">
          Automated Multi-Greenhouse Ventilation System with Online Control
        </p>
      </div>
      
      <Card>
        <CardHeader>
          <CardTitle>System Overview</CardTitle>
          <CardDescription>
            How the greenhouse automation system works
          </CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <p>
            This system automates the ventilation control for 6 greenhouses based on temperature readings,
            with online control via this dashboard and local fallback logic. 
          </p>
          
          <Separator className="my-4" />
          
          <h3 className="text-lg font-medium">System Architecture</h3>
          <p className="text-sm">
            The system consists of a central ESP32 hub and 6 ESP-01 nodes (one per greenhouse).
            The ESP32 communicates with the ESP-01 nodes via ESP-NOW protocol and syncs data
            with Firebase for this online dashboard.
          </p>
          
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4 my-6">
            <div className="bg-gray-50 p-4 rounded-lg border">
              <h4 className="font-medium flex items-center mb-2">
                <Thermometer className="h-4 w-4 mr-2" />
                Central ESP32 Hub
              </h4>
              <ul className="list-disc list-inside text-sm space-y-1 text-gray-700">
                <li>Receives data from all ESP-01 nodes</li>
                <li>Displays system state on local OLED screen</li>
                <li>Syncs with Firebase for online control</li>
                <li>Sends control commands to ESP-01 nodes</li>
              </ul>
            </div>
            
            <div className="bg-gray-50 p-4 rounded-lg border">
              <h4 className="font-medium flex items-center mb-2">
                <Fan className="h-4 w-4 mr-2" />
                ESP-01 Node (Per Greenhouse)
              </h4>
              <ul className="list-disc list-inside text-sm space-y-1 text-gray-700">
                <li>Reads BME280 sensor data</li>
                <li>Controls dual-channel relay for vent motor</li>
                <li>Implements local temperature control logic</li>
                <li>Operates independently if hub is offline</li>
              </ul>
            </div>
          </div>
          
          <Separator className="my-4" />
          
          <h3 className="text-lg font-medium">Automation Logic</h3>
          <p className="text-sm">
            The system operates in two modes:
          </p>
          
          <div className="space-y-3 my-4">
            <div className="bg-green-50 p-3 rounded-lg border border-green-200">
              <h4 className="font-medium text-green-800">Automatic Mode</h4>
              <p className="text-sm text-green-700">
                Vents open when temperature exceeds the threshold and close when it falls below
                threshold minus hysteresis. A cooldown period prevents rapid cycling.
              </p>
            </div>
            
            <div className="bg-blue-50 p-3 rounded-lg border border-blue-200">
              <h4 className="font-medium text-blue-800">Manual Mode</h4>
              <p className="text-sm text-blue-700">
                Overrides automatic control, allowing direct control of vent position
                via this dashboard or the local OLED interface.
              </p>
            </div>
          </div>
          
          <Separator className="my-4" />
          
          <h3 className="text-lg font-medium mb-2">System Components</h3>
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
            <Card>
              <CardHeader className="pb-2">
                <CardTitle className="text-base flex items-center">
                  <Thermometer className="h-4 w-4 mr-2" />
                  Sensors
                </CardTitle>
              </CardHeader>
              <CardContent className="pt-1 text-sm">
                <p>BME280 sensors in each greenhouse measuring:</p>
                <ul className="list-disc list-inside mt-2 space-y-1 text-gray-700">
                  <li>Temperature</li>
                  <li>Humidity</li>
                  <li>Barometric Pressure</li>
                </ul>
              </CardContent>
            </Card>
            
            <Card>
              <CardHeader className="pb-2">
                <CardTitle className="text-base flex items-center">
                  <Fan className="h-4 w-4 mr-2" />
                  Actuators
                </CardTitle>
              </CardHeader>
              <CardContent className="pt-1 text-sm">
                <p>Dual-channel 5V relays controlling:</p>
                <ul className="list-disc list-inside mt-2 space-y-1 text-gray-700">
                  <li>230V window-opening motors</li>
                  <li>One direction for opening</li>
                  <li>Reverse direction for closing</li>
                </ul>
              </CardContent>
            </Card>
          </div>
        </CardContent>
      </Card>
    </div>
  );
};

export default About;
