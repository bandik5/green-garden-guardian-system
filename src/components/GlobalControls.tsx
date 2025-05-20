
import React from 'react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { toast } from '@/components/ui/use-toast';
import { ChevronUp, ChevronDown, AlertTriangle } from 'lucide-react';
import { AlertDialog, AlertDialogAction, AlertDialogCancel, AlertDialogContent, 
  AlertDialogDescription, AlertDialogFooter, AlertDialogHeader, 
  AlertDialogTitle, AlertDialogTrigger } from '@/components/ui/alert-dialog';

interface GlobalControlsProps {
  onControlAll: (action: 'open' | 'close') => void;
  disabled?: boolean;
}

const GlobalControls: React.FC<GlobalControlsProps> = ({ 
  onControlAll, 
  disabled = false 
}) => {
  
  const handleControlAll = (action: 'open' | 'close') => {
    onControlAll(action);
    toast({
      title: `Command sent to all greenhouses`,
      description: `${action === 'open' ? 'Opening' : 'Closing'} all greenhouse vents`,
      duration: 3000,
    });
  };
  
  return (
    <Card>
      <CardHeader>
        <CardTitle>Global Controls</CardTitle>
      </CardHeader>
      
      <CardContent className="space-y-4">
        <p className="text-sm text-muted-foreground">
          Control all greenhouse vents simultaneously
        </p>
        
        <div className="grid grid-cols-2 gap-3">
          <AlertDialog>
            <AlertDialogTrigger asChild>
              <Button 
                variant="default"
                size="lg"
                disabled={disabled}
                className="bg-green-600 hover:bg-green-700"
              >
                <ChevronUp className="mr-1 h-4 w-4" />
                Open All
              </Button>
            </AlertDialogTrigger>
            <AlertDialogContent>
              <AlertDialogHeader>
                <AlertDialogTitle>Open All Greenhouses</AlertDialogTitle>
                <AlertDialogDescription>
                  Are you sure you want to open vents in all greenhouses? 
                  This will override individual settings and switch all greenhouses to manual mode.
                </AlertDialogDescription>
              </AlertDialogHeader>
              <AlertDialogFooter>
                <AlertDialogCancel>Cancel</AlertDialogCancel>
                <AlertDialogAction 
                  onClick={() => handleControlAll('open')}
                  className="bg-green-600 hover:bg-green-700"
                >
                  Open All
                </AlertDialogAction>
              </AlertDialogFooter>
            </AlertDialogContent>
          </AlertDialog>
          
          <AlertDialog>
            <AlertDialogTrigger asChild>
              <Button 
                variant="default"
                size="lg"
                disabled={disabled}
                className="bg-blue-600 hover:bg-blue-700"
              >
                <ChevronDown className="mr-1 h-4 w-4" />
                Close All
              </Button>
            </AlertDialogTrigger>
            <AlertDialogContent>
              <AlertDialogHeader>
                <AlertDialogTitle>Close All Greenhouses</AlertDialogTitle>
                <AlertDialogDescription>
                  Are you sure you want to close vents in all greenhouses? 
                  This will override individual settings and switch all greenhouses to manual mode.
                </AlertDialogDescription>
              </AlertDialogHeader>
              <AlertDialogFooter>
                <AlertDialogCancel>Cancel</AlertDialogCancel>
                <AlertDialogAction 
                  onClick={() => handleControlAll('close')}
                  className="bg-blue-600 hover:bg-blue-700"
                >
                  Close All
                </AlertDialogAction>
              </AlertDialogFooter>
            </AlertDialogContent>
          </AlertDialog>
        </div>
        
        <div className="flex items-center gap-2 text-amber-600 border border-amber-200 bg-amber-50 p-2 rounded-md mt-2">
          <AlertTriangle className="h-4 w-4" />
          <p className="text-xs">Use with caution. This action affects all connected greenhouses.</p>
        </div>
      </CardContent>
    </Card>
  );
};

export default GlobalControls;
