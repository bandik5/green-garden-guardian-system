
import React from 'react';
import { Button } from "@/components/ui/button";
import { 
  DropdownMenu, 
  DropdownMenuContent, 
  DropdownMenuItem, 
  DropdownMenuTrigger 
} from "@/components/ui/dropdown-menu";
import { Settings, Menu } from "lucide-react";
import { Link } from 'react-router-dom';

interface HeaderProps {
  onMenuToggle?: () => void;
}

const Header: React.FC<HeaderProps> = ({ onMenuToggle }) => {
  return (
    <header className="bg-white border-b border-gray-200 py-3 px-4 sm:px-6 flex justify-between items-center sticky top-0 z-10">
      <div className="flex items-center">
        <Button 
          variant="ghost" 
          size="icon" 
          onClick={onMenuToggle}
          className="md:hidden mr-2"
        >
          <Menu className="h-5 w-5" />
        </Button>
        <Link to="/" className="flex items-center space-x-2">
          <div className="w-8 h-8 bg-gradient-to-br from-greenhouse to-greenhouse-dark rounded-md flex items-center justify-center">
            <span className="text-white font-bold text-xl">G</span>
          </div>
          <span className="font-bold text-xl hidden sm:inline-block">GreenControl</span>
        </Link>
      </div>

      <div className="flex items-center space-x-2">
        <DropdownMenu>
          <DropdownMenuTrigger asChild>
            <Button variant="ghost" size="icon">
              <Settings className="h-5 w-5" />
            </Button>
          </DropdownMenuTrigger>
          <DropdownMenuContent align="end">
            <DropdownMenuItem asChild>
              <Link to="/settings">System Settings</Link>
            </DropdownMenuItem>
            <DropdownMenuItem asChild>
              <Link to="/profile">Profile</Link>
            </DropdownMenuItem>
            <DropdownMenuItem>
              <span>Sign Out</span>
            </DropdownMenuItem>
          </DropdownMenuContent>
        </DropdownMenu>
      </div>
    </header>
  );
};

export default Header;
