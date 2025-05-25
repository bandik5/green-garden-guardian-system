
#include "esp_now_comm.h"
#include "config.h"
#include "globals.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// External references
extern GreenhouseData greenhouses[];
extern FirebaseData fbdo;

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
