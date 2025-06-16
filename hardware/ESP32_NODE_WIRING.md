
# ESP32 Node Wiring Instructions

This document provides wiring instructions for the ESP32 nodes in the greenhouse automation system.

## Components per ESP32 Node
- ESP32 development board (with antenna for better range)
- BME280 temperature/humidity/pressure sensor
- 2-channel 5V relay module
- Various resistors and terminal blocks
- Weatherproof enclosure

## GPIO Pin Assignments

| Component | ESP32 Pin | Notes |
|-----------|-----------|-------|
| BME280 SDA | GPIO21 | I2C Data line |
| BME280 SCL | GPIO22 | I2C Clock line |
| Relay Open | GPIO16 | Controls vent opening |
| Relay Close | GPIO17 | Controls vent closing |
| Status LED | GPIO2 | Built-in LED for status indication |

## Wiring Diagram

```
ESP32 Development Board
+------------------+
|                  |
|   ESP32-WROOM    |
|                  |
+--+--+--+--+--+--+
   |  |  |  |  |  |
   |  |  |  |  |  +-- 3.3V ---- BME280 VCC
   |  |  |  |  +----- GND ----- BME280 GND & Relay GND
   |  |  |  +-------- GPIO21 -- BME280 SDA
   |  |  +----------- GPIO22 -- BME280 SCL
   |  +-------------- GPIO16 -- Relay IN1 (Open)
   +----------------- GPIO17 -- Relay IN2 (Close)

Relay Module:
+-------------+
| 2CH RELAY   |
| VCC GND     |
| IN1 IN2     |
| COM NO NC   |
| COM NO NC   |
+-------------+
```

## Detailed Connections

### BME280 Sensor
- **VCC** → ESP32 3.3V
- **GND** → ESP32 GND
- **SDA** → ESP32 GPIO21
- **SCL** → ESP32 GPIO22

### 2-Channel Relay Module
- **VCC** → ESP32 5V (or external 5V supply)
- **GND** → ESP32 GND
- **IN1** → ESP32 GPIO16 (Vent Open Control)
- **IN2** → ESP32 GPIO17 (Vent Close Control)

### Motor Connections (230V AC)
- **Relay 1 COM** → Live (230V)
- **Relay 1 NO** → Motor Open Wire
- **Relay 2 COM** → Live (230V)
- **Relay 2 NO** → Motor Close Wire
- **Motor Common** → Neutral (230V)

## Power Supply
- **Input**: 5V DC (2A minimum recommended)
- **ESP32**: Powered via USB or VIN pin (5V)
- **Relay Module**: 5V from ESP32 or external supply
- **BME280**: 3.3V from ESP32

## Configuration Before Upload

### 1. Set Node ID
In `esp32_node_firmware.ino`, change this line for each node:
```cpp
#define NODE_ID 1  // Change to 1, 2, 3, 4, 5, or 6
```

### 2. Set Hub MAC Address
Get the MAC address from your ESP32 hub and update:
```cpp
uint8_t hubMacAddress[] = {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF}; // CHANGE THIS
```

You can get the hub's MAC address by running this code on the hub:
```cpp
Serial.println(WiFi.macAddress());
```

## Installation Steps

1. **Wire the components** according to the diagram above
2. **Set unique NODE_ID** (1-6) for each node
3. **Update hub MAC address** in the firmware
4. **Upload firmware** to each ESP32 node
5. **Test basic functionality** before motor connection
6. **Install in weatherproof enclosure**
7. **Connect to greenhouse vent motor**
8. **Mount in appropriate location** in greenhouse

## Safety Notes

⚠️ **WARNING**: 230V AC connections should only be made by qualified personnel!

- Use appropriate fuses for motor circuits
- Ensure all high-voltage connections are properly insulated
- Install in weatherproof enclosures
- Test with low voltage before connecting to mains power
- Follow local electrical codes and regulations

## Troubleshooting

### Node Not Communicating
- Check power supply
- Verify hub MAC address is correct
- Ensure NODE_ID is unique (1-6)
- Check ESP-NOW initialization in serial monitor

### Sensor Not Reading
- Verify BME280 connections
- Check I2C address (default 0x76)
- Try different BME280 module if available

### Motor Not Responding
- Check relay clicking sounds
- Verify motor connections
- Test relays with multimeter
- Check 5V supply to relay module

## Advantages of ESP32 vs ESP-01

- **More GPIO pins**: Dedicated pins for each function
- **Better performance**: Faster processor, more memory
- **Built-in antenna**: Better wireless range
- **USB programming**: Easier to program and debug
- **3.3V and 5V outputs**: No need for voltage regulators
- **Better development support**: More examples and libraries
