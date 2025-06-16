
/*
 * ESP32 Node Firmware for Greenhouse Automation System
 * 
 * This firmware runs on ESP32 nodes (one per greenhouse) to:
 * - Read BME280 temperature/humidity/pressure sensor
 * - Control 2-channel relay module for vent motor
 * - Communicate with ESP32 hub via ESP-NOW
 * - Operate autonomously with temperature-based control
 * - Store settings in EEPROM for power-loss recovery
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>
#include <esp_task_wdt.h>

// ----- NODE CONFIGURATION -----
#define NODE_ID 1  // CHANGE THIS FOR EACH NODE (1-6)
#define WDT_TIMEOUT 30  // Watchdog timeout in seconds

// GPIO Pin Assignments
#define BME_SDA_PIN 21
#define BME_SCL_PIN 22
#define RELAY_OPEN_PIN 16
#define RELAY_CLOSE_PIN 17
#define STATUS_LED_PIN 2

// Timing Configuration
#define SENSOR_READ_INTERVAL 10000    // Read sensors every 10 seconds
#define ESP_NOW_SEND_INTERVAL 30000   // Send data every 30 seconds
#define CONTROL_CHECK_INTERVAL 5000   // Check control logic every 5 seconds
#define MOTOR_OPERATION_TIME 30000    // Motor run time for full open/close
#define MOTOR_COOLDOWN_TIME 60000     // Minimum time between motor operations

// Hub MAC Address - UPDATE THIS TO MATCH YOUR HUB
uint8_t hubMacAddress[] = {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF}; // CHANGE THIS

// ----- DATA STRUCTURES -----
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

struct ControlMessage {
  uint8_t targetNodeId;
  float tempThreshold;
  float hysteresis;
  bool autoMode;
  char manualCommand;
};

// ----- GLOBAL VARIABLES -----
Adafruit_BME280 bme;
SensorData currentSensorData;
GreenhouseSettings settings;

// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastESPNowSend = 0;
unsigned long lastControlCheck = 0;
unsigned long motorStartTime = 0;
unsigned long lastMotorOperation = 0;
unsigned long lastWatchdogFeed = 0;

// Status variables
bool sensorInitialized = false;
bool espNowInitialized = false;
uint8_t currentVentStatus = 0; // 0:closed, 1:opening, 2:open, 3:closing
bool motorRunning = false;
char pendingCommand = 0;

// Error handling
unsigned long lastErrorTime = 0;
char lastError[64] = {0};

// ----- FUNCTION DECLARATIONS -----
void initSensor();
void initESPNow();
void readSensorData();
void sendDataToHub();
void processControlLogic();
void executeMotorControl(char command);
void stopMotor();
void saveSettingsToEEPROM();
void loadSettingsFromEEPROM();
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void feedWatchdog();
void blinkStatusLED(int count);
void logError(const char* message);

// ----- SETUP -----
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Greenhouse Node Starting...");
  Serial.print("Node ID: ");
  Serial.println(NODE_ID);
  
  // Initialize watchdog timer
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  
  // Initialize GPIO pins
  pinMode(RELAY_OPEN_PIN, OUTPUT);
  pinMode(RELAY_CLOSE_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  // Ensure relays are off
  digitalWrite(RELAY_OPEN_PIN, LOW);
  digitalWrite(RELAY_CLOSE_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);
  
  // Initialize EEPROM
  EEPROM.begin(512);
  loadSettingsFromEEPROM();
  
  // Initialize I2C and sensor
  Wire.begin(BME_SDA_PIN, BME_SCL_PIN);
  initSensor();
  
  // Initialize ESP-NOW
  initESPNow();
  
  // Initialize sensor data structure
  currentSensorData.nodeId = NODE_ID;
  currentSensorData.ventStatus = currentVentStatus;
  currentSensorData.timestamp = millis();
  
  // Blink LED to indicate successful initialization
  blinkStatusLED(3);
  
  Serial.println("Node initialization complete");
}

// ----- MAIN LOOP -----
void loop() {
  unsigned long currentMillis = millis();
  
  // Feed watchdog
  feedWatchdog();
  
  // Read sensor data periodically
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    readSensorData();
    lastSensorRead = currentMillis;
  }
  
  // Send data to hub periodically
  if (currentMillis - lastESPNowSend >= ESP_NOW_SEND_INTERVAL) {
    sendDataToHub();
    lastESPNowSend = currentMillis;
  }
  
  // Process control logic
  if (currentMillis - lastControlCheck >= CONTROL_CHECK_INTERVAL) {
    processControlLogic();
    lastControlCheck = currentMillis;
  }
  
  // Handle motor timeout
  if (motorRunning && (currentMillis - motorStartTime >= MOTOR_OPERATION_TIME)) {
    stopMotor();
  }
  
  // Process pending manual commands
  if (pendingCommand != 0 && !motorRunning && 
      (currentMillis - lastMotorOperation >= MOTOR_COOLDOWN_TIME)) {
    executeMotorControl(pendingCommand);
    pendingCommand = 0;
  }
  
  delay(100); // Small delay to prevent excessive CPU usage
}

// ----- SENSOR FUNCTIONS -----
void initSensor() {
  if (bme.begin(0x76)) {
    sensorInitialized = true;
    Serial.println("BME280 sensor initialized successfully");
    
    // Configure sensor settings for optimal greenhouse monitoring
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);
  } else {
    sensorInitialized = false;
    logError("BME280 sensor initialization failed");
    Serial.println("BME280 sensor not found!");
  }
}

void readSensorData() {
  if (!sensorInitialized) {
    initSensor(); // Try to reinitialize
    return;
  }
  
  // Force a reading
  bme.takeForcedMeasurement();
  
  // Read sensor values
  float temp = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F; // Convert to hPa
  
  // Validate readings
  if (!isnan(temp) && !isnan(humidity) && !isnan(pressure)) {
    currentSensorData.temperature = temp;
    currentSensorData.humidity = humidity;
    currentSensorData.pressure = pressure;
    currentSensorData.ventStatus = currentVentStatus;
    currentSensorData.timestamp = millis();
    
    Serial.print("Sensor readings - Temp: ");
    Serial.print(temp);
    Serial.print("°C, Humidity: ");
    Serial.print(humidity);
    Serial.print("%, Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");
  } else {
    logError("Invalid sensor readings");
  }
}

// ----- ESP-NOW COMMUNICATION -----
void initESPNow() {
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    logError("ESP-NOW init failed");
    return;
  }
  
  // Register callbacks
  esp_now_register_recv_cb(onDataReceived);
  esp_now_register_send_cb(onDataSent);
  
  // Add hub as peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, hubMacAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    logError("Failed to add hub peer");
    return;
  }
  
  espNowInitialized = true;
  Serial.println("ESP-NOW initialized successfully");
}

void sendDataToHub() {
  if (!espNowInitialized) {
    return;
  }
  
  esp_err_t result = esp_now_send(hubMacAddress, (uint8_t *)&currentSensorData, sizeof(currentSensorData));
  
  if (result == ESP_OK) {
    Serial.println("Data sent to hub successfully");
    blinkStatusLED(1);
  } else {
    logError("Failed to send data to hub");
  }
}

void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == sizeof(ControlMessage)) {
    ControlMessage* msg = (ControlMessage*)data;
    
    // Check if message is for this node
    if (msg->targetNodeId == NODE_ID) {
      Serial.println("Control message received from hub");
      
      // Update settings
      settings.temperatureThreshold = msg->tempThreshold;
      settings.hysteresis = msg->hysteresis;
      settings.autoMode = msg->autoMode;
      
      // Handle manual command
      if (msg->manualCommand != 0) {
        pendingCommand = msg->manualCommand;
        Serial.print("Manual command received: ");
        Serial.println(msg->manualCommand);
      }
      
      // Save settings to EEPROM
      saveSettingsToEEPROM();
      
      blinkStatusLED(2);
    }
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    logError("ESP-NOW send failed");
  }
}

// ----- CONTROL LOGIC -----
void processControlLogic() {
  if (!sensorInitialized || motorRunning) {
    return;
  }
  
  if (settings.autoMode) {
    float currentTemp = currentSensorData.temperature;
    float threshold = settings.temperatureThreshold;
    float hysteresis = settings.hysteresis;
    
    // Temperature-based control with hysteresis
    if (currentVentStatus == 0 || currentVentStatus == 3) { // Closed or closing
      if (currentTemp > threshold + hysteresis) {
        executeMotorControl('O'); // Open vent
      }
    } else if (currentVentStatus == 1 || currentVentStatus == 2) { // Opening or open
      if (currentTemp < threshold - hysteresis) {
        executeMotorControl('C'); // Close vent
      }
    }
  }
}

void executeMotorControl(char command) {
  unsigned long currentMillis = millis();
  
  // Check cooldown period
  if (currentMillis - lastMotorOperation < MOTOR_COOLDOWN_TIME) {
    Serial.println("Motor cooldown active, command deferred");
    pendingCommand = command;
    return;
  }
  
  Serial.print("Executing motor command: ");
  Serial.println(command);
  
  // Stop any current motor operation
  stopMotor();
  
  switch (command) {
    case 'O': // Open
      if (currentVentStatus != 2) { // Not already open
        digitalWrite(RELAY_OPEN_PIN, HIGH);
        digitalWrite(RELAY_CLOSE_PIN, LOW);
        currentVentStatus = 1; // Opening
        motorRunning = true;
        motorStartTime = currentMillis;
        Serial.println("Opening vent");
      }
      break;
      
    case 'C': // Close
      if (currentVentStatus != 0) { // Not already closed
        digitalWrite(RELAY_OPEN_PIN, LOW);
        digitalWrite(RELAY_CLOSE_PIN, HIGH);
        currentVentStatus = 3; // Closing
        motorRunning = true;
        motorStartTime = currentMillis;
        Serial.println("Closing vent");
      }
      break;
      
    case 'S': // Stop
      stopMotor();
      break;
  }
  
  lastMotorOperation = currentMillis;
}

void stopMotor() {
  digitalWrite(RELAY_OPEN_PIN, LOW);
  digitalWrite(RELAY_CLOSE_PIN, LOW);
  
  if (motorRunning) {
    motorRunning = false;
    
    // Update status based on last operation
    if (currentVentStatus == 1) { // Was opening
      currentVentStatus = 2; // Now open
      Serial.println("Vent opened");
    } else if (currentVentStatus == 3) { // Was closing
      currentVentStatus = 0; // Now closed
      Serial.println("Vent closed");
    }
  }
}

// ----- EEPROM FUNCTIONS -----
void saveSettingsToEEPROM() {
  EEPROM.put(0, settings);
  EEPROM.commit();
  Serial.println("Settings saved to EEPROM");
}

void loadSettingsFromEEPROM() {
  EEPROM.get(0, settings);
  
  // Validate loaded settings and apply defaults if invalid
  if (isnan(settings.temperatureThreshold) || 
      settings.temperatureThreshold < 0 || 
      settings.temperatureThreshold > 50) {
    settings.temperatureThreshold = 25.0;
  }
  
  if (isnan(settings.hysteresis) || 
      settings.hysteresis < 0 || 
      settings.hysteresis > 5) {
    settings.hysteresis = 0.5;
  }
  
  Serial.println("Settings loaded from EEPROM");
  Serial.print("Temperature threshold: ");
  Serial.println(settings.temperatureThreshold);
  Serial.print("Hysteresis: ");
  Serial.println(settings.hysteresis);
  Serial.print("Auto mode: ");
  Serial.println(settings.autoMode ? "ON" : "OFF");
}

// ----- UTILITY FUNCTIONS -----
void feedWatchdog() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastWatchdogFeed >= (WDT_TIMEOUT * 1000) / 2) {
    esp_task_wdt_reset();
    lastWatchdogFeed = currentMillis;
  }
}

void blinkStatusLED(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(100);
  }
}

void logError(const char* message) {
  strncpy(lastError, message, sizeof(lastError) - 1);
  lastErrorTime = millis();
  Serial.print("ERROR: ");
  Serial.println(message);
}
