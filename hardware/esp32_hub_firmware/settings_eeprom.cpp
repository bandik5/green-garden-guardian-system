
#include "settings_eeprom.h"
#include "config.h"
#include "globals.h"
#include <EEPROM.h>

// External references
extern GreenhouseData greenhouses[];

void saveSettingsToEEPROM() {
  // Save settings to EEPROM for power-loss recovery
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    int addr = (i-1) * sizeof(GreenhouseSettings);
    EEPROM.put(addr, greenhouses[i].settings);
  }
  EEPROM.commit();
}

void loadSettingsFromEEPROM() {
  // Load settings from EEPROM
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    int addr = (i-1) * sizeof(GreenhouseSettings);
    EEPROM.get(addr, greenhouses[i].settings);
    
    // Validate and apply defaults if needed
    if (isnan(greenhouses[i].settings.temperatureThreshold) || 
        greenhouses[i].settings.temperatureThreshold < 0 ||
        greenhouses[i].settings.temperatureThreshold > 50) {
      greenhouses[i].settings.temperatureThreshold = 25.0;
    }
    
    if (isnan(greenhouses[i].settings.hysteresis) ||
        greenhouses[i].settings.hysteresis < 0 ||
        greenhouses[i].settings.hysteresis > 5) {
      greenhouses[i].settings.hysteresis = 0.5;
    }
  }
}
