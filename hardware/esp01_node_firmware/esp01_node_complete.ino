
/*
 * ESP-01 Node Firmware for Greenhouse Automation System - Complete Version
 * 
 * This firmware runs on ESP-01 modules, one in each greenhouse, to:
 * - Read temperature, humidity and pressure from BME280 sensor
 * - Control vent motor via dual-channel relay (open/close)
 * - Communicate with central ESP32 hub via ESP-NOW
 * - Apply local control logic when operating autonomously
 * 
 * Hardware Connections:
 * - GPIO0: SDA for BME280 + Relay Open (via level shifter)
 * - GPIO2: SCL for BME280 + Relay Close (via level shifter)
 * - VCC: 3.3V from AMS1117 regulator
 * - GND: Common ground
 */

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <EEPROM.h>

// =============================================================================
// CONFIGURATION CONSTANTS
// =============================================================================
#define NODE_ID 1                     // Change this for each ESP-01 node (1-6)
#define RELAY_OPEN_PIN 0              // GPIO0 for OPEN relay
#define RELAY_CLOSE_PIN 2             // GPIO2 for CLOSE relay
#define RELAY_ACTIVE_TIME 5000        // 5 seconds activation time for relay
#define MIN_RELAY_WAIT_TIME 120000    // 2 minutes between actions
#define DEFAULT_TEMP_THRESHOLD 25.0   // Default temperature threshold
#define DEFAULT_HYSTERESIS 0.5        // Default hysteresis value
#define SENSOR_READ_INTERVAL 10000    // Read sensor every 10 seconds
#define HEARTBEAT_INTERVAL 30000      // Send heartbeat every 30 seconds
#define MAX_OFFLINE_TIME 300000       // 5 minutes before going autonomous
#define EEPROM_SIZE 512               // EEPROM size for settings storage

// EEPROM addresses for persistent storage
#define EEPROM_ADDR_TEMP_THRESHOLD 0
#define EEPROM_ADDR_HYSTERESIS 4
#define EEPROM_ADDR_AUTO_MODE 8
#define EEPROM_ADDR_INITIALIZED 12

// =============================================================================
// DATA STRUCTURES
// =============================================================================

// Vent control states
enum VentStatus {
  VENT_CLOSED = 0,
  VENT_OPENING = 1,
  VENT_OPEN = 2,
  VENT_CLOSING = 3
};

// Sensor data structure for ESP-NOW communication
struct SensorData {
  uint8_t nodeId;
  float temperature;
  float humidity;
  float pressure;
  uint8_t ventStatus;
  uint32_t timestamp;
  bool autonomous;
} __attribute__((packed));

// Control message structure from hub
struct ControlMessage {
  uint8_t targetNodeId;
  float tempThreshold;
  float hysteresis;
  bool autoMode;
  char manualCommand;
} __attribute__((packed));

// Node settings structure
struct NodeSettings {
  float temperatureThreshold;
  float hysteresis;
  bool autoMode;
  char manualCommand;
  bool settingsChanged;
};

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

// Hardware components
Adafruit_BME280 bme;
bool sensorInitialized = false;

// ESP-NOW communication
uint8_t hubMacAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; // Replace with actual ESP32 MAC

// Timing control
unsigned long lastSensorRead = 0;
unsigned long lastActionTime = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastHubContact = 0;

// Vent control state
VentStatus currentVentStatus = VENT_CLOSED;
bool relayActive = false;
unsigned long relayStartTime = 0;

// Node settings and data
NodeSettings settings = {
  DEFAULT_TEMP_THRESHOLD,
  DEFAULT_HYSTERESIS,
  true,
  0,
  false
};

SensorData sensorData;
ControlMessage incomingControl;

// System state
bool autonomousMode = false;
bool hubOnline = false;

// =============================================================================
// EEPROM FUNCTIONS
// =============================================================================

void saveSettingsToEEPROM() {
  EEPROM.put(EEPROM_ADDR_TEMP_THRESHOLD, settings.temperatureThreshold);
  EEPROM.put(EEPROM_ADDR_HYSTERESIS, settings.hysteresis);
  EEPROM.put(EEPROM_ADDR_AUTO_MODE, settings.autoMode);
  EEPROM.put(EEPROM_ADDR_INITIALIZED, true);
  EEPROM.commit();
  Serial.println("Settings saved to EEPROM");
}

void loadSettingsFromEEPROM() {
  bool initialized;
  EEPROM.get(EEPROM_ADDR_INITIALIZED, initialized);
  
  if (initialized) {
    EEPROM.get(EEPROM_ADDR_TEMP_THRESHOLD, settings.temperatureThreshold);
    EEPROM.get(EEPROM_ADDR_HYSTERESIS, settings.hysteresis);
    EEPROM.get(EEPROM_ADDR_AUTO_MODE, settings.autoMode);
    Serial.println("Settings loaded from EEPROM");
    Serial.println("Temp threshold: " + String(settings.temperatureThreshold));
    Serial.println("Hysteresis: " + String(settings.hysteresis));
    Serial.println("Auto mode: " + String(settings.autoMode ? "ON" : "OFF"));
  } else {
    Serial.println("EEPROM not initialized, using defaults");
    saveSettingsToEEPROM();
  }
}

// =============================================================================
// BME280 SENSOR FUNCTIONS
// =============================================================================

void initBME280() {
  Serial.println("Initializing BME280...");
  Wire.begin(0, 2);  // SDA=GPIO0, SCL=GPIO2
  
  // Try both common I2C addresses
  if (bme.begin(0x76)) {
    sensorInitialized = true;
    Serial.println("BME280 initialized at address 0x76");
  } else if (bme.begin(0x77)) {
    sensorInitialized = true;
    Serial.println("BME280 initialized at address 0x77");
  } else {
    Serial.println("BME280 initialization failed!");
    sensorInitialized = false;
  }
  
  if (sensorInitialized) {
    // Configure sensor settings for greenhouse monitoring
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1,   // temperature
                    Adafruit_BME280::SAMPLING_X1,   // pressure
                    Adafruit_BME280::SAMPLING_X1,   // humidity
                    Adafruit_BME280::FILTER_OFF);
  }
}

bool readSensorData() {
  if (!sensorInitialized) {
    initBME280();
    if (!sensorInitialized) {
      return false;
    }
  }
  
  // Force a reading
  bme.takeForcedMeasurement();
  
  // Read sensor values
  sensorData.temperature = bme.readTemperature();
  sensorData.humidity = bme.readHumidity();
  sensorData.pressure = bme.readPressure() / 100.0F;  // Convert to hPa
  sensorData.timestamp = millis();
  
  // Validate readings
  if (isnan(sensorData.temperature) || isnan(sensorData.humidity) || isnan(sensorData.pressure)) {
    Serial.println("Invalid sensor readings");
    return false;
  }
  
  Serial.print("T: "); Serial.print(sensorData.temperature, 1);
  Serial.print("°C, H: "); Serial.print(sensorData.humidity, 1);
  Serial.print("%, P: "); Serial.print(sensorData.pressure, 1);
  Serial.println(" hPa");
  
  return true;
}

// =============================================================================
// RELAY CONTROL FUNCTIONS
// =============================================================================

void activateRelay(uint8_t relayPin) {
  // Safety: deactivate all relays first
  deactivateRelays();
  
  // Activate the requested relay
  digitalWrite(relayPin, HIGH);
  relayActive = true;
  relayStartTime = millis();
  
  Serial.print("Relay activated: GPIO");
  Serial.println(relayPin);
}

void deactivateRelays() {
  digitalWrite(RELAY_OPEN_PIN, LOW);
  digitalWrite(RELAY_CLOSE_PIN, LOW);
  Serial.println("All relays deactivated");
}

void updateVentStatus() {
  if (relayActive && (millis() - relayStartTime >= RELAY_ACTIVE_TIME)) {
    deactivateRelays();
    relayActive = false;
    
    // Update vent status after relay action completes
    if (currentVentStatus == VENT_OPENING) {
      currentVentStatus = VENT_OPEN;
      Serial.println("Vent fully opened");
    } else if (currentVentStatus == VENT_CLOSING) {
      currentVentStatus = VENT_CLOSED;
      Serial.println("Vent fully closed");
    }
    
    sensorData.ventStatus = (uint8_t)currentVentStatus;
  }
}

// =============================================================================
// CONTROL LOGIC FUNCTIONS
// =============================================================================

void processVentControl() {
  unsigned long currentMillis = millis();
  
  // Check if minimum wait time has passed and no relay is active
  if (currentMillis - lastActionTime < MIN_RELAY_WAIT_TIME || relayActive) {
    return;
  }
  
  float currentTemp = sensorData.temperature;
  bool actionTaken = false;
  
  // Temperature-based control logic with hysteresis
  if (currentTemp > settings.temperatureThreshold && currentVentStatus == VENT_CLOSED) {
    // Too hot, open vent
    currentVentStatus = VENT_OPENING;
    sensorData.ventStatus = (uint8_t)currentVentStatus;
    activateRelay(RELAY_OPEN_PIN);
    actionTaken = true;
    Serial.println("AUTO: Opening vent - temp above threshold");
  } 
  else if (currentTemp < (settings.temperatureThreshold - settings.hysteresis) && currentVentStatus == VENT_OPEN) {
    // Cool enough to close vent
    currentVentStatus = VENT_CLOSING;
    sensorData.ventStatus = (uint8_t)currentVentStatus;
    activateRelay(RELAY_CLOSE_PIN);
    actionTaken = true;
    Serial.println("AUTO: Closing vent - temp below threshold");
  }
  
  if (actionTaken) {
    lastActionTime = currentMillis;
  }
}

void processManualCommand() {
  unsigned long currentMillis = millis();
  
  if (relayActive || currentMillis - lastActionTime < MIN_RELAY_WAIT_TIME) {
    return;
  }
  
  bool actionTaken = false;
  
  switch (settings.manualCommand) {
    case 'O': // Open command
      if (currentVentStatus != VENT_OPEN && currentVentStatus != VENT_OPENING) {
        currentVentStatus = VENT_OPENING;
        sensorData.ventStatus = (uint8_t)currentVentStatus;
        activateRelay(RELAY_OPEN_PIN);
        actionTaken = true;
        Serial.println("MANUAL: Opening vent");
      }
      break;
      
    case 'C': // Close command
      if (currentVentStatus != VENT_CLOSED && currentVentStatus != VENT_CLOSING) {
        currentVentStatus = VENT_CLOSING;
        sensorData.ventStatus = (uint8_t)currentVentStatus;
        activateRelay(RELAY_CLOSE_PIN);
        actionTaken = true;
        Serial.println("MANUAL: Closing vent");
      }
      break;
      
    case 'S': // Stop command
      deactivateRelays();
      Serial.println("MANUAL: Stop command executed");
      break;
  }
  
  if (actionTaken) {
    lastActionTime = currentMillis;
  }
  
  // Clear the command after processing
  settings.manualCommand = 0;
}

// =============================================================================
// ESP-NOW COMMUNICATION FUNCTIONS
// =============================================================================

void sendDataToHub() {
  sensorData.nodeId = NODE_ID;
  sensorData.autonomous = autonomousMode;
  
  uint8_t result = esp_now_send(hubMacAddress, (uint8_t*)&sensorData, sizeof(sensorData));
  
  if (result == 0) {
    Serial.println("Data sent to hub successfully");
    lastHeartbeat = millis();
  } else {
    Serial.println("Failed to send data to hub");
  }
}

void onDataReceived(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(ControlMessage)) {
    Serial.println("Received invalid message size");
    return;
  }
  
  memcpy(&incomingControl, data, sizeof(ControlMessage));
  
  // Only process messages targeted for this node or broadcast (ID 0)
  if (incomingControl.targetNodeId != NODE_ID && incomingControl.targetNodeId != 0) {
    return;
  }
  
  // Update hub contact timestamp
  lastHubContact = millis();
  hubOnline = true;
  autonomousMode = false;
  
  // Check if settings have changed
  bool settingsUpdated = false;
  
  if (settings.temperatureThreshold != incomingControl.tempThreshold) {
    settings.temperatureThreshold = incomingControl.tempThreshold;
    settingsUpdated = true;
  }
  
  if (settings.hysteresis != incomingControl.hysteresis) {
    settings.hysteresis = incomingControl.hysteresis;
    settingsUpdated = true;
  }
  
  if (settings.autoMode != incomingControl.autoMode) {
    settings.autoMode = incomingControl.autoMode;
    settingsUpdated = true;
  }
  
  if (incomingControl.manualCommand != 0) {
    settings.manualCommand = incomingControl.manualCommand;
  }
  
  // Save settings to EEPROM if changed
  if (settingsUpdated) {
    saveSettingsToEEPROM();
    Serial.println("Settings updated from hub");
  }
  
  Serial.print("Received control: Threshold=");
  Serial.print(settings.temperatureThreshold);
  Serial.print(", Hysteresis=");
  Serial.print(settings.hysteresis);
  Serial.print(", Mode=");
  Serial.print(settings.autoMode ? "Auto" : "Manual");
  if (settings.manualCommand != 0) {
    Serial.print(", Command=");
    Serial.print(settings.manualCommand);
  }
  Serial.println();
}

// =============================================================================
// SYSTEM MONITORING FUNCTIONS
// =============================================================================

void checkHubConnection() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastHubContact > MAX_OFFLINE_TIME) {
    if (!autonomousMode) {
      autonomousMode = true;
      hubOnline = false;
      Serial.println("Hub offline - switching to autonomous mode");
    }
  }
}

void printSystemStatus() {
  Serial.println("=== System Status ===");
  Serial.print("Node ID: "); Serial.println(NODE_ID);
  Serial.print("Mode: "); Serial.println(autonomousMode ? "Autonomous" : "Connected");
  Serial.print("Hub: "); Serial.println(hubOnline ? "Online" : "Offline");
  Serial.print("Vent: ");
  switch (currentVentStatus) {
    case VENT_CLOSED: Serial.println("Closed"); break;
    case VENT_OPENING: Serial.println("Opening"); break;
    case VENT_OPEN: Serial.println("Open"); break;
    case VENT_CLOSING: Serial.println("Closing"); break;
  }
  Serial.print("Control mode: "); Serial.println(settings.autoMode ? "Automatic" : "Manual");
  Serial.print("Temp threshold: "); Serial.println(settings.temperatureThreshold);
  Serial.print("Hysteresis: "); Serial.println(settings.hysteresis);
  Serial.println("====================");
}

// =============================================================================
// SETUP FUNCTION
// =============================================================================

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP-01 Greenhouse Node Starting ===");
  Serial.print("Node ID: "); Serial.println(NODE_ID);
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadSettingsFromEEPROM();
  
  // Initialize GPIO pins for relays
  pinMode(RELAY_OPEN_PIN, OUTPUT);
  pinMode(RELAY_CLOSE_PIN, OUTPUT);
  digitalWrite(RELAY_OPEN_PIN, LOW);
  digitalWrite(RELAY_CLOSE_PIN, LOW);
  Serial.println("GPIO pins initialized");
  
  // Initialize WiFi in station mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("Node MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed!");
    ESP.restart();
  }
  Serial.println("ESP-NOW initialized");
  
  // Register ESP-NOW callback
  esp_now_register_recv_cb(onDataReceived);
  
  // Add hub as peer
  if (esp_now_add_peer(hubMacAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0) != 0) {
    Serial.println("Failed to add hub as peer");
  } else {
    Serial.println("Hub added as ESP-NOW peer");
  }
  
  // Initialize BME280 sensor
  initBME280();
  
  // Initialize sensor data structure
  sensorData.nodeId = NODE_ID;
  sensorData.ventStatus = (uint8_t)currentVentStatus;
  sensorData.autonomous = autonomousMode;
  
  // Record startup time
  lastHubContact = millis();
  lastSensorRead = millis();
  lastHeartbeat = millis();
  
  Serial.println("=== Node initialization complete ===\n");
  printSystemStatus();
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensor data at specified interval
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    if (readSensorData()) {
      sendDataToHub();
    }
    lastSensorRead = currentMillis;
  }
  
  // Send heartbeat if no recent sensor data was sent
  if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    sendDataToHub();
  }
  
  // Check hub connection status
  checkHubConnection();
  
  // Update relay timing and vent status
  updateVentStatus();
  
  // Process control logic based on current mode
  if (settings.autoMode) {
    processVentControl();
  } else {
    if (settings.manualCommand != 0) {
      processManualCommand();
    }
  }
  
  // Save settings if they changed
  if (settings.settingsChanged) {
    saveSettingsToEEPROM();
    settings.settingsChanged = false;
  }
  
  // Print system status every 5 minutes
  static unsigned long lastStatusPrint = 0;
  if (currentMillis - lastStatusPrint >= 300000) {
    printSystemStatus();
    lastStatusPrint = currentMillis;
  }
  
  // Light sleep to save power
  delay(100);
}
