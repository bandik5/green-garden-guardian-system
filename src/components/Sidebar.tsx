
import React from 'react';
import { NavLink } from 'react-router-dom';
import { cn } from '@/lib/utils';
import { Home, Thermometer, Settings, Info, Fan } from 'lucide-react';

interface SidebarProps {
  isOpen: boolean;
  onClose?: () => void;
}

const Sidebar: React.FC<SidebarProps> = ({ isOpen, onClose }) => {
  const navItems = [
    { name: 'Dashboard', path: '/', icon: <Home size={20} /> },
    { name: 'Greenhouse 1', path: '/greenhouse/1', icon: <Fan size={20} /> },
    { name: 'Greenhouse 2', path: '/greenhouse/2', icon: <Fan size={20} /> },
    { name: 'Greenhouse 3', path: '/greenhouse/3', icon: <Fan size={20} /> },
    { name: 'Greenhouse 4', path: '/greenhouse/4', icon: <Fan size={20} /> },
    { name: 'Greenhouse 5', path: '/greenhouse/5', icon: <Fan size={20} /> },
    { name: 'Greenhouse 6', path: '/greenhouse/6', icon: <Fan size={20} /> },
    { name: 'Settings', path: '/settings', icon: <Settings size={20} /> },
    { name: 'About', path: '/about', icon: <Info size={20} /> },
  ];

  // Handle sidebar click on mobile (auto close)
  const handleNavClick = () => {
    if (window.innerWidth < 768 && onClose) {
      onClose();
    }
  };

  return (
    <>
      {/* Mobile overlay */}
      {isOpen && (
        <div 
          className="fixed inset-0 bg-black/20 z-20 md:hidden"
          onClick={onClose}
        />
      )}
      
      {/* Sidebar */}
      <aside 
        className={cn(
          "fixed top-0 left-0 z-30 h-full w-64 bg-white border-r border-gray-200 pt-16 transition-transform duration-300 md:translate-x-0 md:pt-16 md:z-0",
          isOpen ? "translate-x-0" : "-translate-x-full"
        )}
      >
        <div className="px-3 py-4">
          <nav className="space-y-1">
            {navItems.map((item) => (
              <NavLink
                key={item.path}
                to={item.path}
                onClick={handleNavClick}
                className={({ isActive }) => cn(
                  "flex items-center px-3 py-2 rounded-md text-sm font-medium transition-colors",
                  isActive
                    ? "bg-greenhouse-light text-greenhouse-dark"
                    : "text-gray-700 hover:bg-gray-100"
                )}
                end={item.path === '/'}
              >
                <span className="mr-3">{item.icon}</span>
                <span>{item.name}</span>
              </NavLink>
            ))}
          </nav>
        </div>
      </aside>
    </>
  );
};

export default Sidebar;
