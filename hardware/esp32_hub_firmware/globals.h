
#ifndef GLOBALS_H
#define GLOBALS_H

#include "data_structures.h"
#include "config.h"

// Timing control
extern unsigned long lastFirebaseSync;
extern unsigned long lastButtonCheck;
extern unsigned long lastDisplayUpdate;
extern unsigned long lastMenuActivity;
extern unsigned long selectPressStart;

// Button state
extern bool buttonUpPressed;
extern bool buttonSelectPressed;
extern bool buttonDownPressed;
extern bool buttonUpLast;
extern bool buttonSelectLast;
extern bool buttonDownLast;
extern bool selectLongPressed;
extern bool selectCurrentlyPressed;

// Menu system
extern MenuState currentMenu;
extern uint8_t selectedGreenhouse;
extern uint8_t settingSelection;
extern uint8_t scheduleSelection;
extern uint8_t controlAllSelection;
extern bool editingValue;

// Greenhouse data
extern GreenhouseData greenhouses[MAX_GREENHOUSES + 1];

// Function declarations from other modules
void saveSettingsToEEPROM();
void loadSettingsFromEEPROM();
void sendControlToNode(uint8_t nodeId);
void sendControlToAllNodes(char command);

#endif
