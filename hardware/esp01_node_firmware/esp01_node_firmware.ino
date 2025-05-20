
/*
 * ESP-01 Node Firmware for Greenhouse Automation System
 * 
 * This firmware runs on ESP-01 modules, one in each greenhouse, to:
 * - Read temperature, humidity and pressure from BME280 sensor
 * - Control vent motor via dual-channel relay (open/close)
 * - Communicate with central ESP32 hub via ESP-NOW
 * - Apply local control logic when operating autonomously
 */

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// ----- CONFIGURATION -----
#define NODE_ID 1                  // Change this for each ESP-01 node (1-6)
#define RELAY_OPEN_PIN 0           // GPIO0 for OPEN relay
#define RELAY_CLOSE_PIN 2          // GPIO2 for CLOSE relay
#define RELAY_ACTIVE_TIME 5000     // 5 seconds activation time for relay
#define MIN_RELAY_WAIT_TIME 120000 // 2 minutes between actions
#define DEFAULT_TEMP_THRESHOLD 25.0
#define DEFAULT_HYSTERESIS 0.5

// ----- GLOBAL VARIABLES -----
// BME280 sensor
Adafruit_BME280 bme;
bool sensorInitialized = false;

// ESP-NOW communication
uint8_t hubMacAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; // Replace with actual ESP32 MAC

// Timing control
unsigned long lastSensorRead = 0;
unsigned long lastActionTime = 0;
unsigned long sensorReadInterval = 10000;  // Read every 10 seconds

// Vent control state
enum VentStatus {CLOSED, OPENING, OPEN, CLOSING};
VentStatus currentVentStatus = CLOSED;
bool relayActive = false;
unsigned long relayStartTime = 0;

// Control settings (updated from hub or using defaults)
struct Settings {
  float temperatureThreshold = DEFAULT_TEMP_THRESHOLD;
  float hysteresis = DEFAULT_HYSTERESIS;
  bool autoMode = true;
  char manualCommand = 0;  // 0:none, 'O':open, 'C':close, 'S':stop
} settings;

// Sensor data structure
struct SensorData {
  uint8_t nodeId;
  float temperature;
  float humidity;
  float pressure;
  uint8_t ventStatus; // 0:closed, 1:opening, 2:open, 3:closing
  uint32_t timestamp;
} sensorData;

// Structure for incoming control messages from hub
struct ControlMessage {
  uint8_t targetNodeId;
  float tempThreshold;
  float hysteresis;
  bool autoMode;
  char manualCommand;
} incomingControl;

// ----- FUNCTION DECLARATIONS -----
void initBME280();
void readSensorData();
void processVentControl();
void applyManualCommand();
void activateRelay(uint8_t relayPin);
void deactivateRelays();
void sendDataToHub();
void onDataReceived(uint8_t *mac, uint8_t *data, uint8_t len);

// ----- SETUP -----
void setup() {
  // Initialize serial for debugging (can be removed in final deployment)
  Serial.begin(115200);
  Serial.println("ESP-01 Greenhouse Node Starting...");
  Serial.println("Node ID: " + String(NODE_ID));

  // Initialize GPIO pins
  pinMode(RELAY_OPEN_PIN, OUTPUT);
  pinMode(RELAY_CLOSE_PIN, OUTPUT);
  digitalWrite(RELAY_OPEN_PIN, LOW);
  digitalWrite(RELAY_CLOSE_PIN, LOW);

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  Serial.print("Node MAC: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }
  
  // Register callback for incoming messages
  esp_now_register_recv_cb(onDataReceived);
  
  // Add hub as peer
  esp_now_add_peer(hubMacAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Initialize BME280 sensor
  initBME280();
  
  // Initialize node data
  sensorData.nodeId = NODE_ID;
  sensorData.ventStatus = (uint8_t)currentVentStatus;
}

// ----- MAIN LOOP -----
void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensor data at specified interval
  if (currentMillis - lastSensorRead >= sensorReadInterval) {
    readSensorData();
    sendDataToHub();
    lastSensorRead = currentMillis;
    
    // Process vent control logic if in auto mode
    if (settings.autoMode) {
      processVentControl();
    }
  }
  
  // Process manual command if available
  if (!settings.autoMode && settings.manualCommand != 0) {
    applyManualCommand();
  }
  
  // Check if relay needs to be turned off
  if (relayActive && (currentMillis - relayStartTime >= RELAY_ACTIVE_TIME)) {
    deactivateRelays();
    relayActive = false;
    
    // Update vent status after relay action completes
    if (currentVentStatus == OPENING) {
      currentVentStatus = OPEN;
    } else if (currentVentStatus == CLOSING) {
      currentVentStatus = CLOSED;
    }
    sensorData.ventStatus = (uint8_t)currentVentStatus;
    sendDataToHub(); // Send updated status
  }
  
  // Light sleep to save power (but still wake for time checks)
  delay(100);
}

// ----- FUNCTIONS -----
void initBME280() {
  Wire.begin(0, 2);  // Use GPIO0(SDA) and GPIO2(SCL) for I2C
  
  // Try 0x76 and 0x77 addresses
  if (bme.begin(0x76)) {
    sensorInitialized = true;
    Serial.println("BME280 initialized at 0x76");
  } else if (bme.begin(0x77)) {
    sensorInitialized = true;
    Serial.println("BME280 initialized at 0x77");
  } else {
    Serial.println("BME280 initialization failed!");
  }
}

void readSensorData() {
  if (!sensorInitialized) {
    // Try to re-initialize
    initBME280();
    if (!sensorInitialized) {
      return;
    }
  }
  
  sensorData.temperature = bme.readTemperature();
  sensorData.humidity = bme.readHumidity();
  sensorData.pressure = bme.readPressure() / 100.0F;  // hPa
  sensorData.timestamp = millis();
  
  Serial.print("Temperature: "); Serial.print(sensorData.temperature);
  Serial.print(" °C, Humidity: "); Serial.print(sensorData.humidity);
  Serial.print(" %, Pressure: "); Serial.print(sensorData.pressure);
  Serial.println(" hPa");
}

void processVentControl() {
  unsigned long currentMillis = millis();
  
  // Check if minimum wait time has passed since last action
  if (currentMillis - lastActionTime < MIN_RELAY_WAIT_TIME || relayActive) {
    return;
  }
  
  float currentTemp = sensorData.temperature;
  
  // Decide action based on temperature relative to threshold
  if (currentTemp > settings.temperatureThreshold && currentVentStatus == CLOSED) {
    // Too hot, need to open vent
    currentVentStatus = OPENING;
    sensorData.ventStatus = (uint8_t)currentVentStatus;
    activateRelay(RELAY_OPEN_PIN);
    lastActionTime = currentMillis;
    Serial.println("Opening vent - Temperature above threshold");
  } 
  else if (currentTemp < (settings.temperatureThreshold - settings.hysteresis) && currentVentStatus == OPEN) {
    // Cool enough to close vent (with hysteresis)
    currentVentStatus = CLOSING;
    sensorData.ventStatus = (uint8_t)currentVentStatus;
    activateRelay(RELAY_CLOSE_PIN);
    lastActionTime = currentMillis;
    Serial.println("Closing vent - Temperature below threshold minus hysteresis");
  }
}

void applyManualCommand() {
  unsigned long currentMillis = millis();
  
  // Only process if not currently active and wait time passed
  if (relayActive || currentMillis - lastActionTime < MIN_RELAY_WAIT_TIME) {
    return;
  }
  
  switch (settings.manualCommand) {
    case 'O': // Open command
      if (currentVentStatus != OPEN && currentVentStatus != OPENING) {
        currentVentStatus = OPENING;
        sensorData.ventStatus = (uint8_t)currentVentStatus;
        activateRelay(RELAY_OPEN_PIN);
        lastActionTime = currentMillis;
        Serial.println("Manual open command executed");
      }
      break;
      
    case 'C': // Close command
      if (currentVentStatus != CLOSED && currentVentStatus != CLOSING) {
        currentVentStatus = CLOSING;
        sensorData.ventStatus = (uint8_t)currentVentStatus;
        activateRelay(RELAY_CLOSE_PIN);
        lastActionTime = currentMillis;
        Serial.println("Manual close command executed");
      }
      break;
      
    case 'S': // Stop command - just deactivate relays
      deactivateRelays();
      Serial.println("Manual stop command executed");
      break;
  }
  
  // Clear the command after processing
  settings.manualCommand = 0;
}

void activateRelay(uint8_t relayPin) {
  // Safety check - deactivate all relays first
  deactivateRelays();
  
  // Activate requested relay
  digitalWrite(relayPin, HIGH);
  relayActive = true;
  relayStartTime = millis();
  Serial.println("Relay activated: " + String(relayPin));
}

void deactivateRelays() {
  digitalWrite(RELAY_OPEN_PIN, LOW);
  digitalWrite(RELAY_CLOSE_PIN, LOW);
}

void sendDataToHub() {
  esp_now_send(hubMacAddress, (uint8_t*)&sensorData, sizeof(sensorData));
  Serial.println("Data sent to hub");
}

void onDataReceived(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len == sizeof(ControlMessage)) {
    memcpy(&incomingControl, data, sizeof(ControlMessage));
    
    // Only process messages targeted for this node
    if (incomingControl.targetNodeId == NODE_ID || incomingControl.targetNodeId == 0) {
      // Update local settings
      settings.temperatureThreshold = incomingControl.tempThreshold;
      settings.hysteresis = incomingControl.hysteresis;
      settings.autoMode = incomingControl.autoMode;
      settings.manualCommand = incomingControl.manualCommand;
      
      Serial.println("Received control update:");
      Serial.print("Threshold: "); Serial.print(settings.temperatureThreshold);
      Serial.print(", Hysteresis: "); Serial.print(settings.hysteresis);
      Serial.print(", Mode: "); Serial.print(settings.autoMode ? "Auto" : "Manual");
      if (settings.manualCommand) {
        Serial.print(", Command: "); Serial.println(settings.manualCommand);
      } else {
        Serial.println();
      }
    }
  }
}
