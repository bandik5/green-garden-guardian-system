
import React, { useState } from 'react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Label } from '@/components/ui/label';
import { Switch } from '@/components/ui/switch';
import { Slider } from '@/components/ui/slider';
import { ChevronUp, ChevronDown, Power } from 'lucide-react';
import { Greenhouse } from '@/types';

interface GreenhouseControlsProps {
  greenhouse: Greenhouse;
  onSettingsChange: (id: number, settings: Partial<Greenhouse['settings']>) => void;
}

const GreenhouseControls: React.FC<GreenhouseControlsProps> = ({ greenhouse, onSettingsChange }) => {
  const [tempThreshold, setTempThreshold] = useState(greenhouse.settings.temperatureThreshold);
  
  const handleModeToggle = () => {
    onSettingsChange(greenhouse.id, { 
      mode: greenhouse.settings.mode === 'auto' ? 'manual' : 'auto' 
    });
  };
  
  const handleThresholdChange = (newValue: number[]) => {
    setTempThreshold(newValue[0]);
  };
  
  const handleThresholdCommit = () => {
    onSettingsChange(greenhouse.id, { temperatureThreshold: tempThreshold });
  };
  
  const handleManualControl = (action: 'open' | 'close' | 'stop') => {
    onSettingsChange(greenhouse.id, { manualControl: action });
  };
  
  // Determine if manual controls should be disabled
  const isOperating = greenhouse.settings.ventStatus === 'opening' || greenhouse.settings.ventStatus === 'closing';
  const isAutoMode = greenhouse.settings.mode === 'auto';
  
  return (
    <Card>
      <CardHeader>
        <CardTitle>Controls</CardTitle>
      </CardHeader>
      
      <CardContent className="space-y-6">
        {/* Mode Toggle */}
        <div className="flex items-center justify-between">
          <div className="space-y-0.5">
            <Label htmlFor="auto-mode">Automatic Mode</Label>
            <p className="text-sm text-muted-foreground">
              {greenhouse.settings.mode === 'auto' 
                ? 'System controls vents based on temperature' 
                : 'Manual vent control enabled'}
            </p>
          </div>
          <Switch 
            id="auto-mode" 
            checked={greenhouse.settings.mode === 'auto'}
            onCheckedChange={handleModeToggle}
          />
        </div>
        
        {/* Temperature Threshold */}
        <div className="space-y-2">
          <div className="flex items-center justify-between">
            <Label>Temperature Threshold</Label>
            <span className="font-medium text-sm">{tempThreshold}°C</span>
          </div>
          <Slider
            value={[tempThreshold]}
            min={18}
            max={32}
            step={0.5}
            onValueChange={handleThresholdChange}
            onValueCommit={handleThresholdCommit}
            disabled={isOperating}
          />
          <p className="text-xs text-muted-foreground mt-1">
            Vents will open when temperature exceeds this value
          </p>
        </div>
        
        {/* Manual Controls */}
        <div className="space-y-2">
          <Label>Manual Vent Control</Label>
          <div className="flex justify-between gap-2">
            <Button 
              variant="outline" 
              className="flex-1"
              onClick={() => handleManualControl('open')}
              disabled={isAutoMode || isOperating || greenhouse.settings.ventStatus === 'open'}
            >
              <ChevronUp className="mr-1 h-4 w-4" />
              Open
            </Button>
            
            <Button 
              variant="outline" 
              className="flex-1"
              onClick={() => handleManualControl('stop')}
              disabled={isAutoMode || !isOperating}
            >
              <Power className="mr-1 h-4 w-4" />
              Stop
            </Button>
            
            <Button 
              variant="outline" 
              className="flex-1"
              onClick={() => handleManualControl('close')}
              disabled={isAutoMode || isOperating || greenhouse.settings.ventStatus === 'closed'}
            >
              <ChevronDown className="mr-1 h-4 w-4" />
              Close
            </Button>
          </div>
          {isAutoMode && (
            <p className="text-xs text-amber-600">
              Switch to manual mode to control vents
            </p>
          )}
        </div>
      </CardContent>
    </Card>
  );
};

export default GreenhouseControls;
