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
#include <esp_task_wdt.h>
#include "config.h"
#include "globals.h"
#include "data_structures.h"
#include "wifi_firebase.h"
#include "display_ui.h"
#include "button_menu.h"
#include "esp_now_comm.h"
#include "settings_eeprom.h"

// ----- CONFIGURATION -----
#define WDT_TIMEOUT 30  // Watchdog timeout in seconds
#define WIFI_RETRY_LIMIT 20
#define WIFI_RETRY_DELAY 500
#define BUTTON_DEBOUNCE_DELAY 50

// ----- GLOBAL VARIABLES -----
// Timing control
unsigned long lastFirebaseSync = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastMenuActivity = 0;
unsigned long selectPressStart = 0;
unsigned long lastWatchdogFeed = 0;

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

// Error handling
char lastErrorMessage[64] = {0};
bool hasError = false;

// Function declarations
void displayError(const char* message);
bool validateSensorData(const SensorData& data);
void feedWatchdog();
void handleWiFiConnection();

// ----- SETUP -----
void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println("ESP32 Greenhouse Hub Starting...");
  
  // Initialize watchdog timer
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  // Initialize I2C and display
  Wire.begin();
  initDisplay();
  
  // Initialize buttons with debouncing
  initButtons();
  
  // Initialize EEPROM and load saved settings
  EEPROM.begin(512);
  loadSettingsFromEEPROM();
  
  // Initialize ESP-NOW
  if (!initESPNow()) {
    displayError("ESP-NOW Init Failed");
    delay(2000);
  }
  
  // Initialize WiFi (non-blocking)
  handleWiFiConnection();
  
  // Initialize Firebase (after WiFi connected)
  if (WiFi.status() == WL_CONNECTED) {
    initFirebase();
  }
  
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
  
  // Feed watchdog
  feedWatchdog();
  
  // Check button inputs with debouncing
  if (currentMillis - lastButtonCheck >= BUTTON_DEBOUNCE_DELAY) {
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
  
  // Sync with Firebase if connected
  if (WiFi.status() == WL_CONNECTED && 
      currentMillis - lastFirebaseSync >= FIREBASE_SYNC_INTERVAL) {
    syncWithFirebase();
    lastFirebaseSync = currentMillis;
  } else if (WiFi.status() != WL_CONNECTED && 
             currentMillis - lastFirebaseSync >= WIFI_RETRY_DELAY * 10) {
    // Try to reconnect WiFi periodically
    handleWiFiConnection();
  }
  
  // Check for offline nodes
  checkNodeStatus();
}

// ----- HELPER FUNCTIONS -----
void displayError(const char* message) {
  strncpy(lastErrorMessage, message, sizeof(lastErrorMessage) - 1);
  hasError = true;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("ERROR:");
  display.println(message);
  display.display();
  
  Serial.print("Error: ");
  Serial.println(message);
}

bool validateSensorData(const SensorData& data) {
  // Validate temperature (-40°C to 80°C)
  if (data.temperature < -40.0 || data.temperature > 80.0) return false;
  
  // Validate humidity (0-100%)
  if (data.humidity < 0.0 || data.humidity > 100.0) return false;
  
  // Validate pressure (800-1200 hPa)
  if (data.pressure < 800.0 || data.pressure > 1200.0) return false;
  
  // Validate timestamp (not in the future)
  if (data.timestamp > millis()) return false;
  
  return true;
}

void feedWatchdog() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastWatchdogFeed >= (WDT_TIMEOUT * 1000) / 2) {
    esp_task_wdt_reset();
    lastWatchdogFeed = currentMillis;
  }
}

void handleWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < WIFI_RETRY_LIMIT) {
    delay(WIFI_RETRY_DELAY);
    retryCount++;
    
    // Update display with connection status
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Connecting WiFi:");
    display.println(WIFI_SSID);
    display.print("Attempt: ");
    display.println(retryCount);
    display.display();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    displayError("WiFi Connection Failed");
  } else {
    Serial.println("WiFi Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
}

void checkNodeStatus() {
  unsigned long currentMillis = millis();
  
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    if (greenhouses[i].isOnline && 
        currentMillis - greenhouses[i].lastSeen > NODE_TIMEOUT) {
      greenhouses[i].isOnline = false;
      char errorMsg[32];
      snprintf(errorMsg, sizeof(errorMsg), "Node %d Offline", i);
      displayError(errorMsg);
    }
  }
}