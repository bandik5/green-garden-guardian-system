
# Greenhouse Automation System - Wiring Instructions

This document provides detailed instructions for wiring the ESP-01 nodes and ESP32 hub for the greenhouse automation system.

## ESP-01 Node Wiring

### Components per Greenhouse Node
- ESP-01 (ESP8266) module
- BME280 temperature/humidity/pressure sensor (I2C)
- 2-channel 5V relay module
- AMS1117 3.3V voltage regulator
- 2x IRLZ44N MOSFETs (for level shifting)
- Various resistors and capacitors
- Terminal blocks for connecting 230V motor wires

### ESP-01 Node Wiring Diagram

```
                           AMS1117
                        +----------+
                        |          |
                   +----+ IN  OUT +----+
                   |    |          |    |
                   |    +----------+    |
                   |         |          |
                   |         |          |
               +---+         |          +---+
               |   |         |          |   |
               |   |         |          |   |
             + |   |         |          |   | +
5V Input ------+   +---------+          +---+---- 3.3V to ESP-01, BME280
               -                                 -
GND ------------+--------------------------+--------- GND
                                           |
                                           |
                  +--------+               |
                  |        |               |
          +-------+ ESP-01 |               |
          |       |        |               |
          |       +--------+               |
          |          | |                   |
          |          | |                   |
          |          | |                   |
          |     SCL  | | SDA               |
          |     (2)  | | (0)               |
          |          | |                   |
          |          | |                   |
          |          | |                   |
          |          | |                   |
          |      +---+ +---+               |
          |      |         |               |
          |      |         |               |
       +--+------+-+     +-+------+        |
       |  BME280   |     | Level  |        |
       +-----+-----+     | Shifter|        |
             |           +----+---+        |
             |                |            |
             |                |            |
             +----------------+------------+
                                |
                                |
                         +------+-------+
                         | Relay Module |
                         +------+-------+
                                |
                                |
                         [230V Motor Connections]
```

### ESP-01 Level Shifting Circuit

Since the ESP-01 operates at 3.3V and most relay modules require 5V signal, a level shifter is needed:

```
ESP-01 GPIO (3.3V) ----+
                        |
                      10kΩ
                        |
                        +------- To Relay Input
                        |
                      IRLZ44N
                        |
GND -------------------+
```

### BME280 Connections to ESP-01

| BME280 Pin | ESP-01 Pin  | Notes                 |
|------------|-------------|------------------------|
| VCC        | 3.3V        | Power supply          |
| GND        | GND         | Ground                |
| SCL        | GPIO2       | I2C Clock             |
| SDA        | GPIO0       | I2C Data              |

### Relay Connections to ESP-01

| Relay Module Pin | Connection            | Notes                   |
|------------------|------------------------|-------------------------|
| VCC              | 5V                    | Relay power (5V)        |
| GND              | GND                   | Ground                  |
| IN1              | Level shifter output 1 | Connected to GPIO0 via level shifter |
| IN2              | Level shifter output 2 | Connected to GPIO2 via level shifter |
| COM1, COM2       | Motor power source    | Common terminal         |
| NO1              | Motor open wire       | Normally open contact   |
| NO2              | Motor close wire      | Normally open contact   |

## ESP32 Hub Wiring

### Components for Central Hub
- ESP32 development board
- SSD1306 0.96" OLED display (I2C)
- 2 push buttons
- Various resistors and capacitors

### ESP32 Hub Wiring Diagram

```
                  +------------+
                  |            |
                  |   ESP32    |
                  |            |
                  +---+----+---+
                      |    |
          +-----------+    +------------+
          |                             |
          |                             |
    +-----+------+                +-----+------+
    |            |                |            |
    | SSD1306    |                | Buttons    |
    | OLED       |                |            |
    +-----+------+                +-----+------+
          |                             |
          |                             |
          +-------------+---------------+
                        |
                        |
                    GND / 3.3V
```

### OLED Display Connections

| OLED Pin | ESP32 Pin | Notes               |
|----------|-----------|---------------------|
| VCC      | 3.3V      | Power supply        |
| GND      | GND       | Ground              |
| SCL      | GPIO22    | I2C Clock (default) |
| SDA      | GPIO21    | I2C Data (default)  |

### Button Connections

| Button   | ESP32 Pin | Notes                |
|----------|-----------|----------------------|
| Up       | GPIO32    | With pullup resistor |
| Select   | GPIO33    | With pullup resistor |

Each button should be connected:
```
3.3V ---- 10kΩ ---- Button ---- ESP32 Pin
                        |
                       GND
```

## Power Supply Considerations

1. ESP-01 Nodes:
   - Input: 5V DC supply (wall adapter or regulated power supply)
   - Voltage regulator (AMS1117-3.3) to provide 3.3V for ESP-01 and BME280
   - Ensure adequate current capacity (min. 500mA)
   - Consider adding capacitors (100μF electrolytic and 0.1μF ceramic) for stability

2. Relay Power:
   - Ensure the 5V supply can handle relay coil current (typically 70mA per relay)
   - Use separate power rails for logic and relay if possible
   - Add flyback diodes if not built into the relay module

3. ESP32 Hub:
   - Most ESP32 dev boards have built-in voltage regulation
   - Power via USB or external 5V supply
   - Will draw approx. 100-150mA with OLED display active

## Safety Considerations

1. **IMPORTANT**: The system interfaces with 230V AC electricity for the motor controls. Ensure:
   - All high-voltage wiring is properly insulated
   - Components are housed in an appropriate enclosure
   - Proper fusing is in place for both low and high voltage sides
   - Consider optocoupler isolation between the ESP circuits and relay drivers

2. Mount ESP-01 nodes in weatherproof enclosures suitable for greenhouse environments:
   - Protection from humidity and water splashes
   - Proper ventilation to prevent condensation
   - UV-resistant materials for longevity

## Notes on ESP-NOW Communication

For optimal ESP-NOW communication:
1. Position the ESP32 hub centrally if possible
2. Consider using ESP-01 modules with external antenna option for better range
3. For distances >50m or obstacles, consider signal repeaters

## Testing Procedure

1. ESP-01 Node:
   - Test power supply voltages (5V and 3.3V)
   - Verify BME280 readings via serial monitor
   - Test relay operation with low-voltage load before connecting motors
   - Validate I2C connection and sensor readings

2. ESP32 Hub:
   - Verify OLED display operation
   - Test button functionality
   - Validate WiFi and Firebase connections
   - Test ESP-NOW communication with a single node before full deployment
