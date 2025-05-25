
/*
 * ESP32 Hub Firmware for Greenhouse Automation System
 * 
 * This firmware runs on the central ESP32 hub to:
 * - Receive data from ESP-01 nodes via ESP-NOW
 * - Send control commands to ESP-01 nodes
 * - Connect to WiFi and sync data with Firebase
 * - Provide local display and control via OLED and buttons
 */

#include <Wire.h>
#include <EEPROM.h>
#include "config.h"
#include "globals.h"
#include "data_structures.h"
#include "wifi_firebase.h"
#include "display_ui.h"
#include "button_menu.h"
#include "esp_now_comm.h"
#include "settings_eeprom.h"

// ----- GLOBAL VARIABLES -----
// Timing control
unsigned long lastFirebaseSync = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastMenuActivity = 0;
unsigned long selectPressStart = 0;

// Button state
bool buttonUpPressed = false;
bool buttonSelectPressed = false;
bool buttonDownPressed = false;
bool buttonUpLast = false;
bool buttonSelectLast = false;
bool buttonDownLast = false;
bool selectLongPressed = false;
bool selectCurrentlyPressed = false;

// Menu system
MenuState currentMenu = OVERVIEW;
uint8_t selectedGreenhouse = 1;
uint8_t settingSelection = 0;
uint8_t scheduleSelection = 0;
uint8_t controlAllSelection = 0;
bool editingValue = false;

// Data structures for nodes
GreenhouseData greenhouses[MAX_GREENHOUSES + 1];  // +1 for 1-based indexing

// ----- SETUP -----
void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println("ESP32 Greenhouse Hub Starting...");
  
  // Initialize I2C and display
  Wire.begin();
  initDisplay();
  
  // Initialize buttons
  initButtons();
  
  // Initialize EEPROM and load saved settings
  EEPROM.begin(512);
  loadSettingsFromEEPROM();
  
  // Initialize ESP-NOW
  initESPNow();
  
  // Initialize WiFi (non-blocking)
  initWiFi();
  
  // Initialize Firebase (after WiFi connected)
  initFirebase();
  
  // Initialize greenhouse data
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    greenhouses[i].isOnline = false;
    greenhouses[i].lastSeen = 0;
    greenhouses[i].settings.temperatureThreshold = 25.0;
    greenhouses[i].settings.hysteresis = 0.5;
    greenhouses[i].settings.autoMode = true;
    greenhouses[i].sensor.nodeId = i;
    greenhouses[i].sensor.ventStatus = 0; // CLOSED
  }
  
  // Reset menu timeout
  resetMenuTimeout();
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Hub initialized!");
  display.println("Waiting for nodes...");
  display.display();
}

// ----- MAIN LOOP -----
void loop() {
  unsigned long currentMillis = millis();
  
  // Check button inputs
  if (currentMillis - lastButtonCheck >= BUTTON_CHECK_INTERVAL) {
    checkButtons();
    lastButtonCheck = currentMillis;
  }
  
  // Check menu timeout
  checkMenuTimeout();
  
  // Update display
  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    updateDisplay();
    lastDisplayUpdate = currentMillis;
  }
  
  // Sync with Firebase
  if (WiFi.status() == WL_CONNECTED && 
      currentMillis - lastFirebaseSync >= FIREBASE_SYNC_INTERVAL) {
    syncWithFirebase();
    lastFirebaseSync = currentMillis;
  }
}
