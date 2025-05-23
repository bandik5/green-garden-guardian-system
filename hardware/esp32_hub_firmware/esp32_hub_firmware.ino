
/*
 * ESP32 Hub Firmware for Greenhouse Automation System
 * 
 * This firmware runs on the central ESP32 hub to:
 * - Receive data from ESP-01 nodes via ESP-NOW
 * - Send control commands to ESP-01 nodes
 * - Connect to WiFi and sync data with Firebase
 * - Provide local display and control via OLED and buttons
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// ----- DISPLAY CONFIGURATION -----
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ----- BUTTON CONFIGURATION -----
#define BUTTON_UP     32
#define BUTTON_SELECT 33
#define BUTTON_DOWN   25
#define DEBOUNCE_TIME 200
#define LONG_PRESS_TIME 2000  // 2 seconds for long press
#define MENU_TIMEOUT 20000    // 20 seconds timeout

// ----- WIFI CONFIGURATION -----
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ----- FIREBASE CONFIGURATION -----
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ----- GLOBAL VARIABLES -----
// Timing control
unsigned long lastFirebaseSync = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastMenuActivity = 0;  // For timeout
unsigned long selectPressStart = 0;  // For long press detection
unsigned long firebaseSyncInterval = 30000;  // 30 seconds
unsigned long buttonCheckInterval = 50;      // 50 ms
unsigned long displayUpdateInterval = 1000;  // 1 second

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
enum MenuState {OVERVIEW, GREENHOUSE_DETAIL, SCHEDULE_SETTING, MANUAL_CONTROL_ALL};
MenuState currentMenu = OVERVIEW;
uint8_t selectedGreenhouse = 1;
uint8_t settingSelection = 0; // 0:threshold, 1:schedule, 2:manual
uint8_t scheduleSelection = 0; // 0:open_time, 1:close_time
uint8_t controlAllSelection = 0; // 0:open all, 1:close all
bool editingValue = false;

// Data structures for nodes
#define MAX_GREENHOUSES 6

struct SensorData {
  uint8_t nodeId;
  float temperature;
  float humidity;
  float pressure;
  uint8_t ventStatus; // 0:closed, 1:opening, 2:open, 3:closing
  uint32_t timestamp;
};

struct ScheduleSettings {
  uint8_t openHour = 8;    // Default open at 8 AM
  uint8_t openMinute = 0;
  uint8_t closeHour = 18;  // Default close at 6 PM
  uint8_t closeMinute = 0;
  bool scheduleEnabled = false;
};

struct GreenhouseSettings {
  float temperatureThreshold = 25.0;
  float hysteresis = 0.5;
  bool autoMode = true;
  char manualCommand = 0;
  ScheduleSettings schedule;
};

struct GreenhouseData {
  SensorData sensor;
  GreenhouseSettings settings;
  bool isOnline;
  unsigned long lastSeen;
} greenhouses[MAX_GREENHOUSES + 1];  // +1 for 1-based indexing

// Structure for outgoing control messages to nodes
struct ControlMessage {
  uint8_t targetNodeId;
  float tempThreshold;
  float hysteresis;
  bool autoMode;
  char manualCommand;
};

// ----- FUNCTION DECLARATIONS -----
void initDisplay();
void initButtons();
void initWiFi();
void initFirebase();
void initESPNow();
void saveSettingsToEEPROM();
void loadSettingsFromEEPROM();
void checkButtons();
void updateDisplay();
void showOverviewScreen();
void showGreenhouseDetailScreen();
void showScheduleSettingScreen();
void showManualControlAllScreen();
void processMenuNavigation();
void checkMenuTimeout();
void resetMenuTimeout();
void syncWithFirebase();
void sendControlToNode(uint8_t nodeId);
void sendControlToAllNodes(char command);
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

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
  if (currentMillis - lastButtonCheck >= buttonCheckInterval) {
    checkButtons();
    lastButtonCheck = currentMillis;
  }
  
  // Check menu timeout
  checkMenuTimeout();
  
  // Update display
  if (currentMillis - lastDisplayUpdate >= displayUpdateInterval) {
    updateDisplay();
    lastDisplayUpdate = currentMillis;
  }
  
  // Sync with Firebase
  if (WiFi.status() == WL_CONNECTED && 
      currentMillis - lastFirebaseSync >= firebaseSyncInterval) {
    syncWithFirebase();
    lastFirebaseSync = currentMillis;
  }
}

// ----- INITIALIZATION FUNCTIONS -----
void initDisplay() {
  // Initialize SSD1306 display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();
}

void initButtons() {
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
}

// ----- WIFI INITIALIZATION -----
void initWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting to WiFi:");
  display.println(ssid);
  display.display();
}

// ----- FIREBASE INITIALIZATION -----
void initFirebase() {
  // Configure Firebase API Key and RTDB URL
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Set up anonymous authentication
  auth.user.email = "";
  auth.user.password = "";
  
  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Firebase initialized");
}

// ----- ESP-NOW INITIALIZATION -----
void initESPNow() {
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(onDataReceived);
  esp_now_register_send_cb(onDataSent);
}

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

void resetMenuTimeout() {
  lastMenuActivity = millis();
}

void checkMenuTimeout() {
  if (currentMenu != OVERVIEW && 
      millis() - lastMenuActivity > MENU_TIMEOUT) {
    currentMenu = OVERVIEW;
    editingValue = false;
    selectLongPressed = false;
    Serial.println("Menu timeout - returning to overview");
  }
}

void checkButtons() {
  // Read current button state
  bool upCurrent = digitalRead(BUTTON_UP) == LOW;
  bool selectCurrent = digitalRead(BUTTON_SELECT) == LOW;
  bool downCurrent = digitalRead(BUTTON_DOWN) == LOW;
  
  // Handle SELECT button long press detection
  if (selectCurrent && !selectCurrentlyPressed) {
    // SELECT just pressed
    selectPressStart = millis();
    selectCurrentlyPressed = true;
  } else if (!selectCurrent && selectCurrentlyPressed) {
    // SELECT just released
    unsigned long pressDuration = millis() - selectPressStart;
    selectCurrentlyPressed = false;
    
    if (pressDuration >= LONG_PRESS_TIME) {
      selectLongPressed = true;
    } else {
      buttonSelectPressed = true;
    }
  }
  
  // Detect button presses (transition from not pressed to pressed)
  if (upCurrent && !buttonUpLast) {
    buttonUpPressed = true;
  }
  
  if (downCurrent && !buttonDownLast) {
    buttonDownPressed = true;
  }
  
  // Save current state for next comparison
  buttonUpLast = upCurrent;
  buttonSelectLast = selectCurrent;
  buttonDownLast = downCurrent;
  
  // Process navigation if button was pressed
  if (buttonUpPressed || buttonSelectPressed || buttonDownPressed || selectLongPressed) {
    resetMenuTimeout();
    processMenuNavigation();
    buttonUpPressed = false;
    buttonSelectPressed = false;
    buttonDownPressed = false;
    selectLongPressed = false;
  }
}

void processMenuNavigation() {
  // Handle menu navigation based on current state
  switch (currentMenu) {
    case OVERVIEW:
      if (buttonUpPressed) {
        selectedGreenhouse = (selectedGreenhouse % MAX_GREENHOUSES) + 1;
      } else if (buttonDownPressed) {
        selectedGreenhouse = (selectedGreenhouse > 1) ? selectedGreenhouse - 1 : MAX_GREENHOUSES;
      } else if (buttonSelectPressed) {
        currentMenu = GREENHOUSE_DETAIL;
        settingSelection = 0;
      } else if (selectLongPressed) {
        currentMenu = MANUAL_CONTROL_ALL;
        controlAllSelection = 0;
      }
      break;
      
    case GREENHOUSE_DETAIL:
      if (editingValue) {
        // Adjusting the selected setting value
        if (buttonUpPressed) {
          // Increase value
          switch (settingSelection) {
            case 0: // Temperature threshold
              greenhouses[selectedGreenhouse].settings.temperatureThreshold += 0.5;
              if (greenhouses[selectedGreenhouse].settings.temperatureThreshold > 40) {
                greenhouses[selectedGreenhouse].settings.temperatureThreshold = 15;
              }
              sendControlToNode(selectedGreenhouse);
              saveSettingsToEEPROM();
              break;
          }
        } else if (buttonDownPressed) {
          // Decrease value
          switch (settingSelection) {
            case 0: // Temperature threshold
              greenhouses[selectedGreenhouse].settings.temperatureThreshold -= 0.5;
              if (greenhouses[selectedGreenhouse].settings.temperatureThreshold < 15) {
                greenhouses[selectedGreenhouse].settings.temperatureThreshold = 40;
              }
              sendControlToNode(selectedGreenhouse);
              saveSettingsToEEPROM();
              break;
          }
        } else if (buttonSelectPressed) {
          editingValue = false; // Stop editing
        }
      } else {
        // Not editing, navigating between settings
        if (buttonUpPressed) {
          settingSelection = (settingSelection + 1) % 3;  // 0:temp, 1:schedule, 2:manual
        } else if (buttonDownPressed) {
          settingSelection = (settingSelection > 0) ? settingSelection - 1 : 2;
        } else if (buttonSelectPressed) {
          if (settingSelection == 0) {
            editingValue = true;  // Edit temperature
          } else if (settingSelection == 1) {
            currentMenu = SCHEDULE_SETTING;  // Go to schedule
            scheduleSelection = 0;
          } else if (settingSelection == 2) {
            // Manual control
            greenhouses[selectedGreenhouse].settings.autoMode = false;
            greenhouses[selectedGreenhouse].settings.manualCommand = 'O';  // Open
            sendControlToNode(selectedGreenhouse);
          }
        } else if (selectLongPressed) {
          currentMenu = MANUAL_CONTROL_ALL;
          controlAllSelection = 0;
        }
      }
      break;
      
    case SCHEDULE_SETTING:
      if (editingValue) {
        // Adjusting schedule values
        if (buttonUpPressed) {
          switch (scheduleSelection) {
            case 0: // Open hour
              greenhouses[selectedGreenhouse].settings.schedule.openHour = 
                (greenhouses[selectedGreenhouse].settings.schedule.openHour + 1) % 24;
              break;
            case 1: // Open minute
              greenhouses[selectedGreenhouse].settings.schedule.openMinute = 
                (greenhouses[selectedGreenhouse].settings.schedule.openMinute + 15) % 60;
              break;
            case 2: // Close hour
              greenhouses[selectedGreenhouse].settings.schedule.closeHour = 
                (greenhouses[selectedGreenhouse].settings.schedule.closeHour + 1) % 24;
              break;
            case 3: // Close minute
              greenhouses[selectedGreenhouse].settings.schedule.closeMinute = 
                (greenhouses[selectedGreenhouse].settings.schedule.closeMinute + 15) % 60;
              break;
          }
          saveSettingsToEEPROM();
        } else if (buttonDownPressed) {
          switch (scheduleSelection) {
            case 0: // Open hour
              greenhouses[selectedGreenhouse].settings.schedule.openHour = 
                (greenhouses[selectedGreenhouse].settings.schedule.openHour == 0) ? 23 : 
                greenhouses[selectedGreenhouse].settings.schedule.openHour - 1;
              break;
            case 1: // Open minute
              greenhouses[selectedGreenhouse].settings.schedule.openMinute = 
                (greenhouses[selectedGreenhouse].settings.schedule.openMinute == 0) ? 45 : 
                greenhouses[selectedGreenhouse].settings.schedule.openMinute - 15;
              break;
            case 2: // Close hour
              greenhouses[selectedGreenhouse].settings.schedule.closeHour = 
                (greenhouses[selectedGreenhouse].settings.schedule.closeHour == 0) ? 23 : 
                greenhouses[selectedGreenhouse].settings.schedule.closeHour - 1;
              break;
            case 3: // Close minute
              greenhouses[selectedGreenhouse].settings.schedule.closeMinute = 
                (greenhouses[selectedGreenhouse].settings.schedule.closeMinute == 0) ? 45 : 
                greenhouses[selectedGreenhouse].settings.schedule.closeMinute - 15;
              break;
          }
          saveSettingsToEEPROM();
        } else if (buttonSelectPressed) {
          editingValue = false;
        }
      } else {
        // Navigate schedule options
        if (buttonUpPressed) {
          scheduleSelection = (scheduleSelection + 1) % 4;
        } else if (buttonDownPressed) {
          scheduleSelection = (scheduleSelection > 0) ? scheduleSelection - 1 : 3;
        } else if (buttonSelectPressed) {
          if (scheduleSelection < 4) {
            editingValue = true;
          } else {
            currentMenu = GREENHOUSE_DETAIL;
          }
        }
      }
      break;
      
    case MANUAL_CONTROL_ALL:
      if (buttonUpPressed) {
        controlAllSelection = (controlAllSelection + 1) % 2;
      } else if (buttonDownPressed) {
        controlAllSelection = (controlAllSelection > 0) ? 0 : 1;
      } else if (buttonSelectPressed) {
        // Execute control all command
        if (controlAllSelection == 0) {
          sendControlToAllNodes('O');  // Open all
        } else {
          sendControlToAllNodes('C');  // Close all
        }
        currentMenu = OVERVIEW;  // Return to overview after command
      }
      break;
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // Check connection status and show indicator
  if (WiFi.status() == WL_CONNECTED) {
    // Small WiFi connected indicator in top right
    display.fillRect(120, 0, 8, 8, SSD1306_WHITE);
    display.setCursor(122, 1);
    display.setTextColor(SSD1306_BLACK);
    display.print("W");
    display.setTextColor(SSD1306_WHITE);
  }
  
  // Show appropriate screen based on current menu
  switch (currentMenu) {
    case OVERVIEW:
      showOverviewScreen();
      break;
    case GREENHOUSE_DETAIL:
      showGreenhouseDetailScreen();
      break;
    case SCHEDULE_SETTING:
      showScheduleSettingScreen();
      break;
    case MANUAL_CONTROL_ALL:
      showManualControlAllScreen();
      break;
  }
  
  display.display();
}

void showOverviewScreen() {
  display.setCursor(0, 0);
  display.println("GREENHOUSE OVERVIEW");
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
  
  int y = 12;
  // Show data for all 6 greenhouses with temperatures prominently
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    // Highlight selected greenhouse with arrow
    if (i == selectedGreenhouse) {
      display.setCursor(0, y);
      display.print(">");
    }
    
    display.setCursor(8, y);
    display.print(i);
    display.print(":");
    
    // Status indicator
    if (!greenhouses[i].isOnline || millis() - greenhouses[i].lastSeen > 300000) {
      display.print(" --.-C OFFLINE");
    } else {
      // Show temperature prominently
      display.print(" ");
      display.print(greenhouses[i].sensor.temperature, 1);
      display.print("C ");
      
      // Compact vent status
      switch (greenhouses[i].sensor.ventStatus) {
        case 0: display.print("CL"); break;  // CLOSED
        case 1: display.print("OP"); break;  // OPENING
        case 2: display.print("OP"); break;  // OPEN
        case 3: display.print("CL"); break;  // CLOSING
      }
    }
    
    y += 8;
  }
  
  display.drawLine(0, 55, 128, 55, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print("UP/DN:Nav SEL:Set LONG:All");
}

void showGreenhouseDetailScreen() {
  GreenhouseData* gh = &greenhouses[selectedGreenhouse];
  
  display.setCursor(0, 0);
  display.print("GREENHOUSE ");
  display.println(selectedGreenhouse);
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
  
  if (!gh->isOnline || millis() - gh->lastSeen > 300000) {
    display.setCursor(0, 20);
    display.println("STATUS: OFFLINE");
  } else {
    display.setCursor(0, 12);
    display.print("Temp: ");
    display.print(gh->sensor.temperature, 1);
    display.print("C (");
    display.print(gh->settings.temperatureThreshold, 1);
    display.println("C)");
    
    // Settings section with visual indicators
    display.drawLine(0, 22, 128, 22, SSD1306_WHITE);
    
    // Temperature threshold setting
    if (settingSelection == 0) {
      display.fillRect(0, 24, 128, 8, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(0, 24);
    display.print("1.Temp Thresh: ");
    display.print(gh->settings.temperatureThreshold, 1);
    display.print("C");
    if (editingValue && settingSelection == 0) display.print(" *");
    display.setTextColor(SSD1306_WHITE);
    
    // Schedule setting
    if (settingSelection == 1) {
      display.fillRect(0, 32, 128, 8, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(0, 32);
    display.print("2.Schedule: ");
    display.printf("%02d:%02d-%02d:%02d", 
      gh->settings.schedule.openHour, gh->settings.schedule.openMinute,
      gh->settings.schedule.closeHour, gh->settings.schedule.closeMinute);
    display.setTextColor(SSD1306_WHITE);
    
    // Manual control
    if (settingSelection == 2) {
      display.fillRect(0, 40, 128, 8, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(0, 40);
    display.print("3.Manual: ");
    display.print(gh->settings.autoMode ? "Auto" : "Manual");
    display.setTextColor(SSD1306_WHITE);
  }
  
  display.drawLine(0, 55, 128, 55, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print("UP/DN:Nav SEL:Edit");
}

void showScheduleSettingScreen() {
  GreenhouseData* gh = &greenhouses[selectedGreenhouse];
  
  display.setCursor(0, 0);
  display.print("SCHEDULE GH");
  display.println(selectedGreenhouse);
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
  
  // Open time settings
  if (scheduleSelection == 0) {
    display.fillRect(0, 16, 60, 8, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  display.setCursor(0, 16);
  display.print("Open Hr: ");
  display.print(gh->settings.schedule.openHour);
  if (editingValue && scheduleSelection == 0) display.print(" *");
  display.setTextColor(SSD1306_WHITE);
  
  if (scheduleSelection == 1) {
    display.fillRect(0, 24, 60, 8, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  display.setCursor(0, 24);
  display.print("Open Min: ");
  display.print(gh->settings.schedule.openMinute);
  if (editingValue && scheduleSelection == 1) display.print(" *");
  display.setTextColor(SSD1306_WHITE);
  
  // Close time settings
  if (scheduleSelection == 2) {
    display.fillRect(0, 32, 60, 8, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  display.setCursor(0, 32);
  display.print("Close Hr: ");
  display.print(gh->settings.schedule.closeHour);
  if (editingValue && scheduleSelection == 2) display.print(" *");
  display.setTextColor(SSD1306_WHITE);
  
  if (scheduleSelection == 3) {
    display.fillRect(0, 40, 60, 8, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  display.setCursor(0, 40);
  display.print("Close Min: ");
  display.print(gh->settings.schedule.closeMinute);
  if (editingValue && scheduleSelection == 3) display.print(" *");
  display.setTextColor(SSD1306_WHITE);
  
  display.drawLine(0, 55, 128, 55, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print("+/-:Adjust SEL:OK");
}

void showManualControlAllScreen() {
  display.setCursor(0, 0);
  display.println("MANUAL CONTROL ALL");
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
  
  display.setCursor(0, 20);
  display.print("Select action:");
  
  // Highlight selected option
  if (controlAllSelection == 0) {
    display.fillRect(0, 30, 128, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  display.setCursor(0, 32);
  display.print("OPEN ALL VENTS");
  display.setTextColor(SSD1306_WHITE);
  
  if (controlAllSelection == 1) {
    display.fillRect(0, 42, 128, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  display.setCursor(0, 44);
  display.print("CLOSE ALL VENTS");
  display.setTextColor(SSD1306_WHITE);
  
  display.drawLine(0, 55, 128, 55, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print("+/-:Nav SEL:Execute");
}

// ----- FIREBASE SYNC FUNCTIONS -----
void syncWithFirebase() {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready");
    return;
  }

  // Upload all greenhouse data to Firebase
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    if (greenhouses[i].isOnline && 
        (millis() - greenhouses[i].lastSeen < 300000)) {
      
      // Create JSON payload to send
      FirebaseJson json;
      json.set("nodeId", greenhouses[i].sensor.nodeId);
      json.set("temperature", greenhouses[i].sensor.temperature);
      json.set("humidity", greenhouses[i].sensor.humidity);
      json.set("pressure", greenhouses[i].sensor.pressure);
      json.set("ventStatus", greenhouses[i].sensor.ventStatus);
      json.set("timestamp", greenhouses[i].sensor.timestamp);
      
      // Path to the sensor data
      String path = "/greenhouses/" + String(i) + "/currentData";
      
      // Update the data using correct API
      if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
        Serial.println("Uploaded data for greenhouse " + String(i));
      } else {
        Serial.println("Failed to upload: " + fbdo.errorReason());
      }
      
      // Now update settings
      FirebaseJson settingsJson;
      settingsJson.set("temperatureThreshold", greenhouses[i].settings.temperatureThreshold);
      settingsJson.set("hysteresis", greenhouses[i].settings.hysteresis);
      settingsJson.set("mode", greenhouses[i].settings.autoMode ? "auto" : "manual");
      
      String ventStatus;
      switch (greenhouses[i].sensor.ventStatus) {
        case 0: ventStatus = "closed"; break;
        case 1: ventStatus = "opening"; break;
        case 2: ventStatus = "open"; break;
        case 3: ventStatus = "closing"; break;
        default: ventStatus = "unknown"; break;
      }
      settingsJson.set("ventStatus", ventStatus);
      
      // Add schedule data
      settingsJson.set("scheduleOpenHour", greenhouses[i].settings.schedule.openHour);
      settingsJson.set("scheduleOpenMinute", greenhouses[i].settings.schedule.openMinute);
      settingsJson.set("scheduleCloseHour", greenhouses[i].settings.schedule.closeHour);
      settingsJson.set("scheduleCloseMinute", greenhouses[i].settings.schedule.closeMinute);
      settingsJson.set("scheduleEnabled", greenhouses[i].settings.schedule.scheduleEnabled);
      
      // Path to settings
      path = "/greenhouses/" + String(i) + "/settings";
      
      // Update settings
      Firebase.RTDB.setJSON(&fbdo, path.c_str(), &settingsJson);
    }
  }
  
  // Download settings and control commands from Firebase
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    String path = "/greenhouses/" + String(i) + "/settings";
    
    if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
      if (fbdo.dataType() == "json") {
        FirebaseJson &json = fbdo.jsonObject();
        FirebaseJsonData result;
        
        // Extract threshold
        json.get(result, "temperatureThreshold");
        if (result.success) {
          float newThreshold = result.to<float>();
          if (newThreshold != greenhouses[i].settings.temperatureThreshold) {
            greenhouses[i].settings.temperatureThreshold = newThreshold;
            sendControlToNode(i);
          }
        }
        
        // Extract hysteresis
        json.get(result, "hysteresis");
        if (result.success) {
          float newHysteresis = result.to<float>();
          if (newHysteresis != greenhouses[i].settings.hysteresis) {
            greenhouses[i].settings.hysteresis = newHysteresis;
            sendControlToNode(i);
          }
        }
        
        // Extract mode
        json.get(result, "mode");
        if (result.success) {
          String mode = result.to<String>();
          bool newAutoMode = (mode == "auto");
          if (newAutoMode != greenhouses[i].settings.autoMode) {
            greenhouses[i].settings.autoMode = newAutoMode;
            sendControlToNode(i);
          }
        }
        
        // Extract manual control command
        json.get(result, "manualControl");
        if (result.success) {
          String command = result.to<String>();
          char manualCmd = 0;
          
          if (command == "open") manualCmd = 'O';
          else if (command == "close") manualCmd = 'C';
          else if (command == "stop") manualCmd = 'S';
          
          if (manualCmd != 0) {
            greenhouses[i].settings.manualCommand = manualCmd;
            sendControlToNode(i);
            
            // Clear the command in Firebase
            FirebaseJson clearJson;
            clearJson.set("manualControl", (const char*)NULL);
            Firebase.RTDB.updateNode(&fbdo, path.c_str(), &clearJson);
          }
        }
        
        // Save updated settings to EEPROM
        saveSettingsToEEPROM();
      }
    }
  }
}

// ----- ESP-NOW FUNCTIONS -----
void sendControlToNode(uint8_t nodeId) {
  // Don't send if node has never been seen
  if (!greenhouses[nodeId].isOnline) {
    return;
  }
  
  // Create control message
  ControlMessage controlMsg;
  controlMsg.targetNodeId = nodeId;
  controlMsg.tempThreshold = greenhouses[nodeId].settings.temperatureThreshold;
  controlMsg.hysteresis = greenhouses[nodeId].settings.hysteresis;
  controlMsg.autoMode = greenhouses[nodeId].settings.autoMode;
  controlMsg.manualCommand = greenhouses[nodeId].settings.manualCommand;
  
  // ESP-NOW broadcast - the node will filter by ID
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  // Add peer
  if (esp_now_is_peer_exist(broadcastAddress) == false) {
    esp_now_add_peer(&peerInfo);
  }
  
  // Send message
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&controlMsg, sizeof(controlMsg));
  
  if (result == ESP_OK) {
    Serial.println("Control message sent to node " + String(nodeId));
  } else {
    Serial.println("Error sending control message");
  }
  
  // Clear manual command after sending
  greenhouses[nodeId].settings.manualCommand = 0;
}

void sendControlToAllNodes(char command) {
  Serial.print("Sending command to all nodes: ");
  Serial.println(command);
  
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    // Skip nodes that are offline
    if (!greenhouses[i].isOnline || 
        (millis() - greenhouses[i].lastSeen > 300000)) {
      continue;
    }
    
    // Set the command for each greenhouse
    greenhouses[i].settings.manualCommand = command;
    
    // Also set to manual mode temporarily if opening/closing all
    if (command == 'O' || command == 'C') {
      greenhouses[i].settings.autoMode = false;
    }
    
    // Send the command to the node
    sendControlToNode(i);
  }
  
  // Update Firebase with new states
  if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
    FirebaseJson json;
    json.set("action", String(command == 'O' ? "open" : "close"));
    json.set("timestamp", (uint32_t)millis());
    Firebase.RTDB.setJSON(&fbdo, "/system/lastControlAll", &json);
  }
}

void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == sizeof(SensorData)) {
    // Copy data
    SensorData* receivedData = (SensorData*)data;
    
    // Validate node ID
    if (receivedData->nodeId < 1 || receivedData->nodeId > MAX_GREENHOUSES) {
      return;
    }
    
    uint8_t nodeId = receivedData->nodeId;
    
    // Update data
    greenhouses[nodeId].sensor = *receivedData;
    greenhouses[nodeId].isOnline = true;
    greenhouses[nodeId].lastSeen = millis();
    
    Serial.print("Data received from node ");
    Serial.print(nodeId);
    Serial.print(": Temp=");
    Serial.print(receivedData->temperature);
    Serial.print("°C, Humidity=");
    Serial.print(receivedData->humidity);
    Serial.print("%, Pressure=");
    Serial.print(receivedData->pressure);
    Serial.print("hPa, Vent=");
    Serial.println(receivedData->ventStatus);
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("ESP-NOW send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}
