/*
 * ESP32 Hub Firmware for Greenhouse Automation System
 * 
 * This firmware runs on the central ESP32 hub to:
 * - Receive data from ESP32 nodes via ESP-NOW
 * - Send control commands to ESP32 nodes
 * - Connect to WiFi and sync data with Firebase
 * - Provide local display and control via OLED and buttons
 */

#include <Wire.h>
#include <EEPROM.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// ----- CONFIGURATION -----
#define WDT_TIMEOUT 30
#define WIFI_RETRY_LIMIT 20
#define WIFI_RETRY_DELAY 500
#define BUTTON_DEBOUNCE_DELAY 50
#define MAX_GREENHOUSES 6
#define NODE_TIMEOUT 300000
#define FIREBASE_SYNC_INTERVAL 30000
#define DISPLAY_UPDATE_INTERVAL 1000
#define MENU_TIMEOUT 30000

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Button pins
#define BUTTON_UP_PIN 32
#define BUTTON_SELECT_PIN 33
#define BUTTON_DOWN_PIN 25

// WiFi and Firebase credentials - UPDATE THESE
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"

// ----- DATA STRUCTURES -----
enum MenuState {
  OVERVIEW, 
  GREENHOUSE_DETAIL, 
  SCHEDULE_SETTING, 
  MANUAL_CONTROL_ALL
};

struct SensorData {
  uint8_t nodeId;
  float temperature;
  float humidity;
  float pressure;
  uint8_t ventStatus; // 0:closed, 1:opening, 2:open, 3:closing
  uint32_t timestamp;
};

struct ScheduleSettings {
  uint8_t openHour = 8;
  uint8_t openMinute = 0;
  uint8_t closeHour = 18;
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
};

struct ControlMessage {
  uint8_t targetNodeId;
  float tempThreshold;
  float hysteresis;
  bool autoMode;
  char manualCommand;
};

// ----- GLOBAL VARIABLES -----
// Display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

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
GreenhouseData greenhouses[MAX_GREENHOUSES + 1];

// Error handling
char lastErrorMessage[64] = {0};
bool hasError = false;

// Function declarations
void displayError(const char* message);
bool validateSensorData(const SensorData& data);
void feedWatchdog();
void handleWiFiConnection();
void checkNodeStatus();
void initDisplay();
void initButtons();
void initESPNow();
void initFirebase();
void saveSettingsToEEPROM();
void loadSettingsFromEEPROM();
void sendControlToNode(uint8_t nodeId);
void sendControlToAllNodes(char command);
void syncWithFirebase();
void checkButtons();
void resetMenuTimeout();
void checkMenuTimeout();
void updateDisplay();
void displayOverview();
void displayGreenhouseDetail();
void displayScheduleSettings();
void displayControlAll();
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void processButtonUp();
void processButtonDown();
void processButtonSelect();
void processButtonLongSelect();

// ----- SETUP -----
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Greenhouse Hub Starting...");
  
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  Wire.begin();
  initDisplay();
  initButtons();
  
  EEPROM.begin(512);
  loadSettingsFromEEPROM();
  
  initESPNow();
  handleWiFiConnection();
  
  if (WiFi.status() == WL_CONNECTED) {
    initFirebase();
  }
  
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    greenhouses[i].isOnline = false;
    greenhouses[i].lastSeen = 0;
    greenhouses[i].settings.temperatureThreshold = 25.0;
    greenhouses[i].settings.hysteresis = 0.5;
    greenhouses[i].settings.autoMode = true;
    greenhouses[i].sensor.nodeId = i;
    greenhouses[i].sensor.ventStatus = 0;
  }
  
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
  
  feedWatchdog();
  
  if (currentMillis - lastButtonCheck >= BUTTON_DEBOUNCE_DELAY) {
    checkButtons();
    lastButtonCheck = currentMillis;
  }
  
  checkMenuTimeout();
  
  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    updateDisplay();
    lastDisplayUpdate = currentMillis;
  }
  
  if (WiFi.status() == WL_CONNECTED && 
      currentMillis - lastFirebaseSync >= FIREBASE_SYNC_INTERVAL) {
    syncWithFirebase();
    lastFirebaseSync = currentMillis;
  } else if (WiFi.status() != WL_CONNECTED && 
             currentMillis - lastFirebaseSync >= WIFI_RETRY_DELAY * 10) {
    handleWiFiConnection();
  }
  
  checkNodeStatus();
}

// ----- DISPLAY FUNCTIONS -----
void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
}

void updateDisplay() {
  if (hasError) {
    hasError = false;
    return;
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  switch (currentMenu) {
    case OVERVIEW:
      displayOverview();
      break;
    case GREENHOUSE_DETAIL:
      displayGreenhouseDetail();
      break;
    case SCHEDULE_SETTING:
      displayScheduleSettings();
      break;
    case MANUAL_CONTROL_ALL:
      displayControlAll();
      break;
  }
  
  display.display();
}

void displayOverview() {
  display.setCursor(0, 0);
  display.println("Greenhouse Overview");
  display.println("------------------");
  
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    if (i == selectedGreenhouse) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    
    display.print("GH");
    display.print(i);
    display.print(": ");
    
    if (greenhouses[i].isOnline) {
      display.print(greenhouses[i].sensor.temperature, 1);
      display.print("C ");
      
      switch (greenhouses[i].sensor.ventStatus) {
        case 0: display.print("CLS"); break;
        case 1: display.print("OPN"); break;
        case 2: display.print("OPN"); break;
        case 3: display.print("CLS"); break;
      }
    } else {
      display.print("OFFLINE");
    }
    display.println();
  }
}

void displayGreenhouseDetail() {
  display.setCursor(0, 0);
  display.print("Greenhouse ");
  display.println(selectedGreenhouse);
  display.println("------------------");
  
  if (greenhouses[selectedGreenhouse].isOnline) {
    display.print("Temp: ");
    display.print(greenhouses[selectedGreenhouse].sensor.temperature, 1);
    display.println("C");
    
    display.print("Humidity: ");
    display.print(greenhouses[selectedGreenhouse].sensor.humidity, 1);
    display.println("%");
    
    display.print("Pressure: ");
    display.print(greenhouses[selectedGreenhouse].sensor.pressure, 1);
    display.println("hPa");
    
    display.print("Vent: ");
    switch (greenhouses[selectedGreenhouse].sensor.ventStatus) {
      case 0: display.println("CLOSED"); break;
      case 1: display.println("OPENING"); break;
      case 2: display.println("OPEN"); break;
      case 3: display.println("CLOSING"); break;
    }
    
    display.print("Mode: ");
    display.println(greenhouses[selectedGreenhouse].settings.autoMode ? "AUTO" : "MANUAL");
  } else {
    display.println("OFFLINE");
  }
}

void displayScheduleSettings() {
  display.setCursor(0, 0);
  display.println("Schedule Settings");
  display.println("------------------");
  
  display.print(scheduleSelection == 0 ? "> " : "  ");
  display.print("Open: ");
  display.print(greenhouses[selectedGreenhouse].settings.schedule.openHour);
  display.print(":");
  if (greenhouses[selectedGreenhouse].settings.schedule.openMinute < 10) display.print("0");
  display.println(greenhouses[selectedGreenhouse].settings.schedule.openMinute);
  
  display.print(scheduleSelection == 1 ? "> " : "  ");
  display.print("Close: ");
  display.print(greenhouses[selectedGreenhouse].settings.schedule.closeHour);
  display.print(":");
  if (greenhouses[selectedGreenhouse].settings.schedule.closeMinute < 10) display.print("0");
  display.println(greenhouses[selectedGreenhouse].settings.schedule.closeMinute);
  
  display.print(scheduleSelection == 2 ? "> " : "  ");
  display.print("Enabled: ");
  display.println(greenhouses[selectedGreenhouse].settings.schedule.scheduleEnabled ? "YES" : "NO");
}

void displayControlAll() {
  display.setCursor(0, 0);
  display.println("Control All Vents");
  display.println("------------------");
  
  display.print(controlAllSelection == 0 ? "> " : "  ");
  display.println("Open All");
  
  display.print(controlAllSelection == 1 ? "> " : "  ");
  display.println("Close All");
  
  display.print(controlAllSelection == 2 ? "> " : "  ");
  display.println("Auto Mode All");
}

// ----- BUTTON FUNCTIONS -----
void initButtons() {
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
}

void checkButtons() {
  bool upCurrent = !digitalRead(BUTTON_UP_PIN);
  bool selectCurrent = !digitalRead(BUTTON_SELECT_PIN);
  bool downCurrent = !digitalRead(BUTTON_DOWN_PIN);
  
  if (upCurrent && !buttonUpLast) {
    buttonUpPressed = true;
    resetMenuTimeout();
  }
  
  if (selectCurrent && !buttonSelectLast) {
    selectCurrentlyPressed = true;
    selectPressStart = millis();
    resetMenuTimeout();
  }
  
  if (!selectCurrent && buttonSelectLast && selectCurrentlyPressed) {
    if (millis() - selectPressStart > 1000) {
      selectLongPressed = true;
    } else {
      buttonSelectPressed = true;
    }
    selectCurrentlyPressed = false;
  }
  
  if (downCurrent && !buttonDownLast) {
    buttonDownPressed = true;
    resetMenuTimeout();
  }
  
  if (buttonUpPressed) {
    processButtonUp();
    buttonUpPressed = false;
  }
  
  if (buttonSelectPressed) {
    processButtonSelect();
    buttonSelectPressed = false;
  }
  
  if (selectLongPressed) {
    processButtonLongSelect();
    selectLongPressed = false;
  }
  
  if (buttonDownPressed) {
    processButtonDown();
    buttonDownPressed = false;
  }
  
  buttonUpLast = upCurrent;
  buttonSelectLast = selectCurrent;
  buttonDownLast = downCurrent;
}

void processButtonUp() {
  switch (currentMenu) {
    case OVERVIEW:
      if (selectedGreenhouse > 1) {
        selectedGreenhouse--;
      }
      break;
    case SCHEDULE_SETTING:
      if (scheduleSelection > 0) {
        scheduleSelection--;
      }
      break;
    case MANUAL_CONTROL_ALL:
      if (controlAllSelection > 0) {
        controlAllSelection--;
      }
      break;
  }
}

void processButtonDown() {
  switch (currentMenu) {
    case OVERVIEW:
      if (selectedGreenhouse < MAX_GREENHOUSES) {
        selectedGreenhouse++;
      }
      break;
    case SCHEDULE_SETTING:
      if (scheduleSelection < 2) {
        scheduleSelection++;
      }
      break;
    case MANUAL_CONTROL_ALL:
      if (controlAllSelection < 2) {
        controlAllSelection++;
      }
      break;
  }
}

void processButtonSelect() {
  switch (currentMenu) {
    case OVERVIEW:
      currentMenu = GREENHOUSE_DETAIL;
      break;
    case GREENHOUSE_DETAIL:
      currentMenu = SCHEDULE_SETTING;
      scheduleSelection = 0;
      break;
    case SCHEDULE_SETTING:
      currentMenu = MANUAL_CONTROL_ALL;
      controlAllSelection = 0;
      break;
    case MANUAL_CONTROL_ALL:
      switch (controlAllSelection) {
        case 0:
          sendControlToAllNodes('O');
          break;
        case 1:
          sendControlToAllNodes('C');
          break;
        case 2:
          for (int i = 1; i <= MAX_GREENHOUSES; i++) {
            greenhouses[i].settings.autoMode = true;
            sendControlToNode(i);
          }
          break;
      }
      currentMenu = OVERVIEW;
      break;
  }
}

void processButtonLongSelect() {
  currentMenu = OVERVIEW;
}

void resetMenuTimeout() {
  lastMenuActivity = millis();
}

void checkMenuTimeout() {
  if (millis() - lastMenuActivity > MENU_TIMEOUT) {
    currentMenu = OVERVIEW;
    resetMenuTimeout();
  }
}

// ----- ESP-NOW COMMUNICATION -----
void initESPNow() {
  WiFi.mode(WIFI_AP_STA);
  
  if (esp_now_init() != ESP_OK) {
    displayError("ESP-NOW Init Failed");
    return;
  }
  
  esp_now_register_recv_cb(onDataReceived);
  esp_now_register_send_cb(onDataSent);
  
  Serial.println("ESP-NOW initialized successfully");
}

void sendControlToNode(uint8_t nodeId) {
  if (!greenhouses[nodeId].isOnline) {
    return;
  }
  
  ControlMessage controlMsg;
  controlMsg.targetNodeId = nodeId;
  controlMsg.tempThreshold = greenhouses[nodeId].settings.temperatureThreshold;
  controlMsg.hysteresis = greenhouses[nodeId].settings.hysteresis;
  controlMsg.autoMode = greenhouses[nodeId].settings.autoMode;
  controlMsg.manualCommand = greenhouses[nodeId].settings.manualCommand;
  
  // Broadcast to all nodes - they will filter by ID
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_is_peer_exist(broadcastAddress) == false) {
    esp_now_add_peer(&peerInfo);
  }
  
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&controlMsg, sizeof(controlMsg));
  
  if (result == ESP_OK) {
    Serial.println("Control message sent to node " + String(nodeId));
  } else {
    Serial.println("Error sending control message");
  }
  
  greenhouses[nodeId].settings.manualCommand = 0;
}

void sendControlToAllNodes(char command) {
  Serial.print("Sending command to all nodes: ");
  Serial.println(command);
  
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    if (!greenhouses[i].isOnline || 
        (millis() - greenhouses[i].lastSeen > 300000)) {
      continue;
    }
    
    greenhouses[i].settings.manualCommand = command;
    
    if (command == 'O' || command == 'C') {
      greenhouses[i].settings.autoMode = false;
    }
    
    sendControlToNode(i);
  }
  
  if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
    FirebaseJson json;
    json.set("action", String(command == 'O' ? "open" : "close"));
    json.set("timestamp", (uint32_t)millis());
    Firebase.RTDB.setJSON(&fbdo, "/system/lastControlAll", &json);
  }
}

void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == sizeof(SensorData)) {
    SensorData* receivedData = (SensorData*)data;
    
    if (receivedData->nodeId < 1 || receivedData->nodeId > MAX_GREENHOUSES) {
      return;
    }
    
    uint8_t nodeId = receivedData->nodeId;
    
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

// ----- WIFI AND FIREBASE FUNCTIONS -----
void handleWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < WIFI_RETRY_LIMIT) {
    delay(WIFI_RETRY_DELAY);
    retryCount++;
    
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

void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  auth.user.email = "";
  auth.user.password = "";
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Firebase initialized");
}

void syncWithFirebase() {
  if (!Firebase.ready()) {
    Serial.println("Firebase not ready");
    return;
  }

  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    if (greenhouses[i].isOnline && 
        (millis() - greenhouses[i].lastSeen < 300000)) {
      
      FirebaseJson json;
      json.set("nodeId", greenhouses[i].sensor.nodeId);
      json.set("temperature", greenhouses[i].sensor.temperature);
      json.set("humidity", greenhouses[i].sensor.humidity);
      json.set("pressure", greenhouses[i].sensor.pressure);
      json.set("ventStatus", greenhouses[i].sensor.ventStatus);
      json.set("timestamp", greenhouses[i].sensor.timestamp);
      
      String path = "/greenhouses/" + String(i) + "/currentData";
      
      if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
        Serial.println("Uploaded data for greenhouse " + String(i));
      } else {
        Serial.println("Failed to upload: " + fbdo.errorReason());
      }
      
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
      
      settingsJson.set("scheduleOpenHour", greenhouses[i].settings.schedule.openHour);
      settingsJson.set("scheduleOpenMinute", greenhouses[i].settings.schedule.openMinute);
      settingsJson.set("scheduleCloseHour", greenhouses[i].settings.schedule.closeHour);
      settingsJson.set("scheduleCloseMinute", greenhouses[i].settings.schedule.closeMinute);
      settingsJson.set("scheduleEnabled", greenhouses[i].settings.schedule.scheduleEnabled);
      
      path = "/greenhouses/" + String(i) + "/settings";
      Firebase.RTDB.setJSON(&fbdo, path.c_str(), &settingsJson);
    }
  }
  
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    String path = "/greenhouses/" + String(i) + "/settings";
    
    if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
      if (fbdo.dataType() == "json") {
        FirebaseJson &json = fbdo.jsonObject();
        FirebaseJsonData result;
        
        json.get(result, "temperatureThreshold");
        if (result.success) {
          float newThreshold = result.to<float>();
          if (newThreshold != greenhouses[i].settings.temperatureThreshold) {
            greenhouses[i].settings.temperatureThreshold = newThreshold;
            sendControlToNode(i);
          }
        }
        
        json.get(result, "hysteresis");
        if (result.success) {
          float newHysteresis = result.to<float>();
          if (newHysteresis != greenhouses[i].settings.hysteresis) {
            greenhouses[i].settings.hysteresis = newHysteresis;
            sendControlToNode(i);
          }
        }
        
        json.get(result, "mode");
        if (result.success) {
          String mode = result.to<String>();
          bool newAutoMode = (mode == "auto");
          if (newAutoMode != greenhouses[i].settings.autoMode) {
            greenhouses[i].settings.autoMode = newAutoMode;
            sendControlToNode(i);
          }
        }
        
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
            
            FirebaseJson clearJson;
            clearJson.set("manualControl", (const char*)NULL);
            Firebase.RTDB.updateNode(&fbdo, path.c_str(), &clearJson);
          }
        }
        
        saveSettingsToEEPROM();
      }
    }
  }
}

// ----- EEPROM FUNCTIONS -----
void saveSettingsToEEPROM() {
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    int addr = (i-1) * sizeof(GreenhouseSettings);
    EEPROM.put(addr, greenhouses[i].settings);
  }
  EEPROM.commit();
}

void loadSettingsFromEEPROM() {
  for (int i = 1; i <= MAX_GREENHOUSES; i++) {
    int addr = (i-1) * sizeof(GreenhouseSettings);
    EEPROM.get(addr, greenhouses[i].settings);
    
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
  if (data.temperature < -40.0 || data.temperature > 80.0) return false;
  if (data.humidity < 0.0 || data.humidity > 100.0) return false;
  if (data.pressure < 800.0 || data.pressure > 1200.0) return false;
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
