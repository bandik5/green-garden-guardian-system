
# MAC Address Setup Guide

This guide explains how to find and configure MAC addresses for ESP-NOW communication between the ESP32 hub and nodes.

## Overview

ESP-NOW requires MAC addresses to establish communication between devices. Each ESP32 has a unique MAC address that needs to be configured in the firmware.

## Step 1: Find MAC Addresses

### Get Hub MAC Address

1. Upload this simple sketch to your ESP32 hub:

```cpp
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.print("Hub MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  delay(1000);
}
```

2. Open Serial Monitor and note the MAC address (format: XX:XX:XX:XX:XX:XX)

### Get Node MAC Addresses (Optional)

Repeat the same process for each ESP32 node if you want to track specific node addresses.

## Step 2: Update Firmware

### Update Hub Firmware

In `esp32_hub_firmware.ino`, you don't need to change anything as the hub receives from all nodes automatically.

### Update Node Firmware

In `esp32_node_firmware.ino`, update the hub MAC address for each node:

```cpp
// Replace XX:XX:XX:XX:XX:XX with your hub's actual MAC address
uint8_t hubMacAddress[] = {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF};
```

**Convert MAC address format:**
- From: `24:6F:28:AB:CD:EF`
- To: `{0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF}`

## Step 3: Set Unique Node IDs

For each ESP32 node, set a unique NODE_ID:

**Node 1:**
```cpp
#define NODE_ID 1
```

**Node 2:**
```cpp
#define NODE_ID 2
```

**...and so on up to Node 6**

## Step 4: Verify Communication

1. Upload the updated firmware to all devices
2. Power on the hub first
3. Power on nodes one by one
4. Check serial monitor on hub for incoming data
5. Check serial monitor on nodes for successful sends

### Expected Serial Output

**Hub should show:**
```
Data received from node 1: Temp=23.5°C, Humidity=65.2%, Pressure=1013.2hPa, Vent=0
Data received from node 2: Temp=24.1°C, Humidity=68.5%, Pressure=1012.8hPa, Vent=2
```

**Nodes should show:**
```
Data sent to hub successfully
Control message received from hub
```

## Troubleshooting Communication Issues

### No Data Received on Hub
1. Verify hub MAC address in node firmware
2. Check that WiFi.mode(WIFI_STA) is set on hub
3. Ensure ESP-NOW is initialized on both devices
4. Check power supply stability

### Nodes Not Receiving Commands
1. Verify ESP-NOW peer is added correctly
2. Check broadcast address in hub firmware
3. Ensure nodes are within range
4. Try reducing ESP_NOW_SEND_INTERVAL for testing

### Range Issues
1. Use ESP32 modules with external antenna
2. Position hub centrally
3. Avoid obstacles between devices
4. Consider signal repeaters for large installations

## Example MAC Address Configuration

If your hub MAC address is `30:AE:A4:15:77:B4`, update each node like this:

```cpp
uint8_t hubMacAddress[] = {0x30, 0xAE, 0xA4, 0x15, 0x77, 0xB4};
```

## Testing Checklist

- [ ] Hub MAC address correctly configured in all nodes
- [ ] Each node has unique NODE_ID (1-6)
- [ ] Hub shows incoming data from all nodes
- [ ] Nodes show successful data transmission
- [ ] Manual commands from hub reach nodes
- [ ] Temperature thresholds can be updated from hub
