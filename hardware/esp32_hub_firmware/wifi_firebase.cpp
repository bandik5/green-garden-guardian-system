
#include "wifi_firebase.h"
#include "config.h"
#include "globals.h"
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// External references
extern GreenhouseData greenhouses[];
extern Adafruit_SSD1306 display;

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
