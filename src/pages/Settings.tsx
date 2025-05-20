
import React from 'react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Separator } from '@/components/ui/separator';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs';

const Settings: React.FC = () => {
  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-2xl font-bold">Settings</h1>
        <p className="text-muted-foreground">
          System-wide configuration for your greenhouse automation
        </p>
      </div>
      
      <Tabs defaultValue="general">
        <TabsList>
          <TabsTrigger value="general">General</TabsTrigger>
          <TabsTrigger value="network">Network</TabsTrigger>
          <TabsTrigger value="firmware">Firmware</TabsTrigger>
        </TabsList>
        
        <TabsContent value="general" className="space-y-4 mt-4">
          <Card>
            <CardHeader>
              <CardTitle>System Configuration</CardTitle>
              <CardDescription>
                Configure system-wide settings for all greenhouses
              </CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="space-y-2">
                <Label htmlFor="system-name">System Name</Label>
                <Input id="system-name" defaultValue="Greenhouse Automation System" />
              </div>
              
              <div className="space-y-2">
                <Label htmlFor="update-interval">Data Update Interval (seconds)</Label>
                <Input id="update-interval" type="number" defaultValue="10" />
              </div>
              
              <div className="space-y-2">
                <Label htmlFor="motor-duration">Motor Operation Duration (seconds)</Label>
                <Input id="motor-duration" type="number" defaultValue="5" />
              </div>
              
              <div className="space-y-2">
                <Label htmlFor="cooldown-period">Cooldown Period (minutes)</Label>
                <Input id="cooldown-period" type="number" defaultValue="2" />
              </div>
              
              <Button className="mt-2">Save Changes</Button>
            </CardContent>
          </Card>
          
          <Card>
            <CardHeader>
              <CardTitle>Default Thresholds</CardTitle>
              <CardDescription>
                Set default temperature thresholds for all greenhouses
              </CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="space-y-2">
                <Label htmlFor="default-threshold">Default Temperature Threshold (°C)</Label>
                <Input id="default-threshold" type="number" defaultValue="25" />
              </div>
              
              <div className="space-y-2">
                <Label htmlFor="default-hysteresis">Default Hysteresis (°C)</Label>
                <Input id="default-hysteresis" type="number" defaultValue="0.5" step="0.1" />
              </div>
              
              <Button className="mt-2">Apply to All</Button>
            </CardContent>
          </Card>
        </TabsContent>
        
        <TabsContent value="network" className="space-y-4 mt-4">
          <Card>
            <CardHeader>
              <CardTitle>Network Configuration</CardTitle>
              <CardDescription>
                Configure WiFi and ESP-NOW network settings
              </CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="space-y-2">
                <Label htmlFor="wifi-ssid">WiFi SSID</Label>
                <Input id="wifi-ssid" defaultValue="MyNetwork" />
              </div>
              
              <div className="space-y-2">
                <Label htmlFor="wifi-password">WiFi Password</Label>
                <Input id="wifi-password" type="password" value="********" />
              </div>
              
              <Separator className="my-4" />
              
              <h3 className="text-sm font-semibold">ESP-NOW Configuration</h3>
              
              <div className="space-y-2">
                <Label htmlFor="espnow-channel">ESP-NOW Channel</Label>
                <Input id="espnow-channel" type="number" defaultValue="1" min="1" max="14" />
              </div>
              
              <Button className="mt-2">Update Network Settings</Button>
            </CardContent>
          </Card>
          
          <Card>
            <CardHeader>
              <CardTitle>Firebase Configuration</CardTitle>
              <CardDescription>
                Configure cloud database settings
              </CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="space-y-2">
                <Label htmlFor="firebase-url">Firebase URL</Label>
                <Input id="firebase-url" defaultValue="https://greenhouse-example.firebaseio.com" />
              </div>
              
              <div className="space-y-2">
                <Label htmlFor="firebase-apikey">API Key</Label>
                <Input id="firebase-apikey" value="•••••••••••••••••••••••••••••" />
              </div>
              
              <Button className="mt-2">Update Firebase Settings</Button>
            </CardContent>
          </Card>
        </TabsContent>
        
        <TabsContent value="firmware" className="space-y-4 mt-4">
          <Card>
            <CardHeader>
              <CardTitle>Firmware Management</CardTitle>
              <CardDescription>
                Update firmware for ESP32 hub and ESP-01 nodes
              </CardDescription>
            </CardHeader>
            <CardContent className="space-y-4">
              <div className="space-y-2">
                <Label className="block mb-2">ESP32 Hub Firmware</Label>
                <div className="flex items-center">
                  <div className="flex-1">
                    <div className="text-sm">Current Version: <span className="font-medium">v1.2.0</span></div>
                    <div className="text-xs text-muted-foreground">Released: May 15, 2025</div>
                  </div>
                  <Button>Update</Button>
                </div>
              </div>
              
              <Separator className="my-4" />
              
              <div className="space-y-2">
                <Label className="block mb-2">ESP-01 Node Firmware</Label>
                <div className="flex items-center">
                  <div className="flex-1">
                    <div className="text-sm">Current Version: <span className="font-medium">v1.1.5</span></div>
                    <div className="text-xs text-muted-foreground">Released: May 10, 2025</div>
                  </div>
                  <Button>Update All Nodes</Button>
                </div>
              </div>
              
              <Separator className="my-4" />
              
              <h3 className="text-sm font-semibold">Upload Custom Firmware</h3>
              
              <div className="space-y-2">
                <Label htmlFor="firmware-file">Firmware File</Label>
                <Input id="firmware-file" type="file" />
              </div>
              
              <Button variant="outline" className="mt-2">Upload Custom Firmware</Button>
            </CardContent>
          </Card>
        </TabsContent>
      </Tabs>
    </div>
  );
};

export default Settings;
