
# Greenhouse Automation System - Assembly Guide

This document provides step-by-step instructions for assembling and deploying the entire greenhouse automation system, including both the ESP32 hub and ESP-01 nodes.

## System Components Overview

### ESP32 Hub (Central Controller)
- 1× ESP32 development board
- 1× SSD1306 0.96" OLED display
- 2× Push buttons
- 1× Enclosure for hub components
- Mounting hardware and wiring accessories

### Per Greenhouse (ESP-01 Node - Repeat ×6)
- 1× ESP-01 module
- 1× BME280 sensor
- 1× 2-channel 5V relay module
- 1× AMS1117 3.3V voltage regulator
- 2× IRLZ44N MOSFETs (for level shifting)
- 1× Weatherproof enclosure
- Various resistors, capacitors, terminal blocks
- Mounting hardware and wiring accessories

## Tools Required

- Soldering iron and solder
- Wire cutters and strippers
- Small Phillips and flathead screwdrivers
- Multimeter
- USB to TTL adapter (for programming ESP-01)
- ESP-01 programming adapter (recommended)
- Computer with Arduino IDE installed
- Wire labels or marker

## ESP-01 Node Assembly

### Step 1: Prepare the Circuit Board

1. Use a small perfboard or custom PCB to mount components
2. Install the AMS1117 3.3V voltage regulator with input and output capacitors
   - Connect 5V input to the input pin of AMS1117
   - Connect output pin to 3.3V rail
   - Add 10μF capacitor from input to ground
   - Add 22μF capacitor from output to ground

### Step 2: Install ESP-01 Socket and BME280 Sensor

1. Install an 8-pin socket for the ESP-01 module
2. Connect the BME280 sensor:
   - VCC to 3.3V rail
   - GND to ground
   - SCL to ESP-01 GPIO2
   - SDA to ESP-01 GPIO0
   - Add 4.7kΩ pull-up resistors on both SDA and SCL lines

### Step 3: Build Level Shifter Circuit

For each relay control pin:
1. Connect a 10kΩ resistor from ESP-01 GPIO pin to MOSFET gate
2. Connect MOSFET source to ground
3. Connect MOSFET drain to relay control input
4. Add 10kΩ pull-down resistor from gate to ground for stability

### Step 4: Connect Relay Module

1. Connect relay module VCC to 5V rail
2. Connect relay module GND to ground
3. Connect relay module inputs to MOSFET outputs:
   - IN1 (Open relay) to first MOSFET drain
   - IN2 (Close relay) to second MOSFET drain

### Step 5: Prepare Power Connections

1. Install terminal blocks for:
   - 5V power input
   - Ground connection
   - Motor control connections (Open, Close, Common)

2. Ensure proper current capacity for all power connections

### Step 6: Program ESP-01 Module

1. Use USB to TTL adapter with ESP-01 programming adapter
2. Connect ESP-01 to programmer
3. Open Arduino IDE and load `esp01_node_firmware.ino`
4. **IMPORTANT**: Change NODE_ID based on which greenhouse (1-6)
5. Verify and upload the firmware
6. Test basic functionality before installation

### Step 7: Install in Weatherproof Enclosure

1. Mount circuit board in enclosure
2. Drill holes for:
   - Power cable entry
   - Motor control cables
   - Optional ventilation (with protective mesh)
3. Install cable glands for all cable entries
4. Secure all internal wiring with strain relief

## ESP32 Hub Assembly

### Step 1: Connect OLED Display

1. Connect SSD1306 OLED display:
   - VCC to 3.3V
   - GND to ground
   - SCL to GPIO22
   - SDA to GPIO21

### Step 2: Connect Buttons

1. Connect "Up" button:
   - One terminal to GPIO32
   - Other terminal to ground
   - Add 10kΩ pull-up resistor from GPIO32 to 3.3V

2. Connect "Select" button:
   - One terminal to GPIO33
   - Other terminal to ground
   - Add 10kΩ pull-up resistor from GPIO33 to 3.3V

### Step 3: Program ESP32

1. Connect ESP32 to computer via USB
2. Open Arduino IDE and load `esp32_hub_firmware.ino`
3. Update Wi-Fi credentials and Firebase configuration:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   #define API_KEY "YOUR_FIREBASE_API_KEY"
   #define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"
   ```
4. Update the MAC addresses in the code if needed
5. Verify and upload the firmware

### Step 4: Install in Enclosure

1. Mount ESP32 board in enclosure
2. Mount OLED display on enclosure with viewing window
3. Mount buttons on enclosure
4. Provide ventilation holes if necessary
5. Install power cable with proper strain relief

## System Deployment

### Step 1: Position ESP32 Hub

1. Mount hub enclosure in a central location
2. Ensure Wi-Fi coverage from this location
3. Connect to power supply
4. Verify display shows "Starting..."

### Step 2: Install ESP-01 Nodes

For each greenhouse:

1. Mount node enclosure in a protected location
2. Position BME280 sensor for accurate temperature reading:
   - Away from direct sunlight
   - Away from heating elements
   - At plant height for most relevant readings

3. Connect motor control wires to greenhouse ventilation motor:
   - Identify motor Open wire
   - Identify motor Close wire
   - Identify motor Common/Neutral wire
   - Connect to appropriate terminal blocks
   - **IMPORTANT**: Consult motor documentation for correct wiring

4. Connect to power supply
5. Test operation:
   - Verify motor opens and closes when temperature changes
   - Check data is being sent to hub

### Step 3: System Verification

1. On ESP32 hub, verify:
   - All greenhouses appear on overview screen
   - Temperature data is being received
   - Navigate through menus to access each greenhouse

2. Check Firebase integration:
   - Open Firebase console
   - Verify data is updating for each greenhouse

3. Test web application functionality:
   - Monitor readings from all greenhouses
   - Change settings and verify they propagate to the nodes

## Motor Control Wiring

**CAUTION: 230V AC connections should only be handled by qualified individuals**

For each greenhouse ventilation motor:

1. Identify motor wires:
   - Most motors have three wires: Common, Open, and Close
   - Refer to motor documentation for specific wiring

2. Connect through relay module:
   - Common wire connects directly to power supply neutral
   - Open wire connects to NO1 (Normally Open) on relay 1
   - Close wire connects to NO1 (Normally Open) on relay 2
   - Connect COM terminals on both relays to power supply live (230V)

3. Add fuse protection:
   - Install appropriate fuse (typically 1A-5A depending on motor)
   - Fuse should be on the live connection before relay

## Maintenance and Troubleshooting

### Routine Maintenance

1. Periodically check:
   - All enclosures for water ingress or damage
   - Cable connections for security
   - Sensor readings for accuracy

2. Clean sensor openings if dusty/dirty

### Troubleshooting

#### Node Not Communicating with Hub
- Check power supply to node
- Verify node is within range of hub
- Try resetting node by cycling power
- Check ESP-NOW communication (may need to reprogram with debugging enabled)

#### Inaccurate Temperature Readings
- Verify BME280 sensor is properly connected
- Check sensor isn't affected by heat sources or direct sun
- Try recalibrating by comparing with known accurate thermometer

#### Motor Not Responding
- Check relay operation (listen for clicks when activated)
- Verify motor connections
- Check power supply to relay module
- Verify settings in ESP-01 firmware match your motor type

#### Hub Not Connecting to Wi-Fi/Firebase
- Verify Wi-Fi credentials are correct
- Check Wi-Fi signal strength at hub location
- Verify Firebase credentials and database structure

## Final Configuration

After successful installation, you should:

1. Calibrate temperature thresholds:
   - Start with default 25°C threshold
   - Observe plant health and greenhouse conditions
   - Adjust threshold as needed for specific crop requirements

2. Test manual and automatic operation:
   - Use web app to toggle between modes
   - Test manual open/close functionality
   - Verify automatic operation based on temperature

3. Monitor system for at least one full day/night cycle to ensure proper operation under varying conditions

## Safety Reminders

1. All 230V wiring should be properly insulated and secured
2. Use appropriate circuit protection (fuses, circuit breakers)
3. Enclosures should be properly sealed to prevent water ingress
4. Include installation date and contact information inside each enclosure
5. Document final installation with photos for future reference
