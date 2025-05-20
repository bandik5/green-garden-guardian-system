
import React from 'react';
import GlanceCard from '@/components/GlanceCard';
import { useGreenhouseData } from '@/hooks/useGreenhouseData';
import { Alert, AlertDescription } from "@/components/ui/alert";
import { CircleX } from 'lucide-react';
import { Skeleton } from '@/components/ui/skeleton';

const Dashboard: React.FC = () => {
  const { greenhouses, loading, error } = useGreenhouseData();
  
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
  
  // Render loading skeletons when data is loading
  if (loading) {
    return (
      <div>
        <h1 className="text-2xl font-bold mb-6">Greenhouse Dashboard</h1>
        
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
          {[...Array(6)].map((_, index) => (
            <div key={index} className="p-4 border rounded-lg">
              <div className="flex justify-between mb-4">
                <Skeleton className="h-6 w-24" />
                <Skeleton className="h-6 w-16" />
              </div>
              <div className="space-y-4">
                <Skeleton className="h-8 w-full" />
                <Skeleton className="h-6 w-3/4" />
                <Skeleton className="h-6 w-1/2" />
              </div>
            </div>
          ))}
        </div>
      </div>
    );
  }
  
  return (
    <div>
      <h1 className="text-2xl font-bold mb-6">Greenhouse Dashboard</h1>
      
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
        {Object.values(greenhouses).map((greenhouse) => (
          <GlanceCard key={greenhouse.id} greenhouse={greenhouse} />
        ))}
      </div>
      
      <div className="mt-8">
        <h2 className="text-xl font-semibold mb-4">System Overview</h2>
        <div className="bg-white p-4 rounded-lg border shadow-sm">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <h3 className="text-sm font-medium text-gray-500 mb-2">System Status</h3>
              <p className="text-green-500 font-medium flex items-center">
                <span className="h-2 w-2 rounded-full bg-green-500 mr-2 animate-pulse-slow"></span>
                Online - All Nodes Connected
              </p>
              <p className="text-sm text-gray-600 mt-1">Last sync: {new Date().toLocaleTimeString()}</p>
            </div>
            
            <div>
              <h3 className="text-sm font-medium text-gray-500 mb-2">Average Temperature</h3>
              <p className="text-lg font-semibold">
                {(Object.values(greenhouses).reduce((sum, gh) => sum + gh.currentData.temperature, 0) / Object.values(greenhouses).length).toFixed(1)}°C
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Dashboard;
