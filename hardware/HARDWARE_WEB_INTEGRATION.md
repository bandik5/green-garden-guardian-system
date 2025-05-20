
# Integrating Hardware System with GreenControl Web Dashboard

This guide explains how to connect the ESP32/ESP-01 hardware system with the GreenControl web dashboard through Firebase.

## Overview

The greenhouse automation system consists of three main components:
1. ESP-01 nodes in each greenhouse
2. ESP32 central hub
3. Web dashboard (React application)

These components communicate as follows:
- ESP-01 nodes <---> ESP32 hub via ESP-NOW wireless protocol
- ESP32 hub <---> Firebase Realtime Database via WiFi
- Web dashboard <---> Firebase Realtime Database via internet connection

## System Communication Flow

1. **Data Flow**:
   - ESP-01 nodes read sensor data and send to ESP32 hub
   - ESP32 hub uploads data to Firebase
   - Web dashboard reads data from Firebase and displays it

2. **Control Flow**:
   - User adjusts settings or sends commands in web dashboard
   - Changes are written to Firebase
   - ESP32 hub reads changes from Firebase
   - ESP32 hub sends commands to appropriate ESP-01 node
   - ESP-01 node controls greenhouse ventilation

3. **Global Control Flow**:
   - User selects "Open All" or "Close All" in web dashboard
   - Command is written to Firebase system/controlAll path
   - ESP32 hub reads command and sends to all online nodes
   - ESP-01 nodes execute command simultaneously

## Firebase Data Structure

The Firebase Realtime Database serves as the bridge between hardware and web app with this structure:

```
/greenhouses
  /1
    /name: "Greenhouse 1"
    /currentData
      /temperature: 23.5
      /humidity: 65.2
      /pressure: 1013.4
      /ventStatus: "open"
      /timestamp: 1621432567000
    /settings
      /temperatureThreshold: 25
      /hysteresis: 0.5
      /mode: "auto"
      /manualControl: null
  /2
    ... (similar structure for all greenhouses)
/system
  /lastSync: 1621432567000
  /hubStatus: "online"
  /controlAll: ""        # Command to control all greenhouses: "open", "close", or ""
  /lastControlAll
    /action: "open"      # Last global action performed
    /timestamp: 1621432567000
```

## Setting Up the Integration

### Step 1: Deploy the Web Application

1. Deploy the GreenControl web application
2. Configure Firebase credentials in the web application

### Step 2: Configure ESP32 with Same Firebase Project

1. Ensure the ESP32 hub firmware uses the same Firebase project
2. Update Firebase credentials in ESP32 firmware:
   ```cpp
   #define API_KEY "YOUR_FIREBASE_API_KEY"
   #define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"
   ```

### Step 3: Verify Communication

1. Power up the ESP32 hub and verify connection to Firebase
2. Check Firebase console to see if data is being updated
3. Open the web dashboard and verify data is displayed correctly
4. Test control functionality:
   - Change a temperature threshold in the web dashboard
   - Verify the change appears in Firebase
   - Verify the ESP32 receives the changed setting
   - Verify the ESP32 sends updated setting to ESP-01 node

## Modifying Dashboard Settings

The web dashboard allows changing these settings for each greenhouse:

1. **Temperature threshold**: Sets the temperature at which ventilation activates
   - Web input updates `/greenhouses/{id}/settings/temperatureThreshold`
   - ESP32 monitors this value and forwards to ESP-01 node

2. **Hysteresis**: Sets the temperature difference for deactivation
   - Web input updates `/greenhouses/{id}/settings/hysteresis`
   - ESP32 monitors this value and forwards to ESP-01 node

3. **Operation mode**: Toggles between automatic and manual operation
   - Web input updates `/greenhouses/{id}/settings/mode`
   - ESP32 monitors this value and forwards to ESP-01 node

4. **Manual control**: Sends direct open/close commands in manual mode
   - Web input updates `/greenhouses/{id}/settings/manualControl`
   - ESP32 monitors this value, forwards command to ESP-01, then resets to null

5. **Global control**: Controls all greenhouses simultaneously
   - Web input updates `/system/controlAll` with "open" or "close"
   - ESP32 monitors this value, forwards command to all ESP-01 nodes, then resets to empty string
   - ESP32 also updates `/system/lastControlAll` with action and timestamp

## Testing Web Integration

Test the complete system with these steps:

1. **Basic data flow test**:
   - Verify temperature readings appear correctly in dashboard
   - Compare with local display on ESP32 hub

2. **Settings synchronization test**:
   - Change temperature threshold through web dashboard
   - Verify change is reflected on ESP32 hub display
   - Verify ESP-01 behavior changes accordingly

3. **Manual control test**:
   - Switch a greenhouse to manual mode
   - Send open/close commands
   - Verify ventilation motor responds correctly

4. **Global control test**:
   - Use "Open All" or "Close All" button on dashboard
   - Verify all connected greenhouses respond
   - Check ESP32 display shows mode changes
   - Verify motors operate correctly

## Troubleshooting Integration Issues

### No Data in Web Dashboard

**Possible causes and solutions:**
- ESP32 not connected to WiFi: Check WiFi credentials and signal strength
- ESP32 not updating Firebase: Check Firebase credentials and rules
- Firebase permissions issue: Verify database rules allow read/write
- Web app configuration issue: Confirm Firebase config matches in web app

### Settings Not Reaching ESP32

**Possible causes and solutions:**
- ESP32 not reading from Firebase: Check sync interval in ESP32 code
- Path mismatch: Ensure web app and ESP32 use same database paths
- Data format mismatch: Verify data types match between web app and ESP32
- Connection interruptions: Implement better error handling and retries

### Commands Not Reaching ESP-01 Nodes

**Possible causes and solutions:**
- ESP-NOW communication issues: Check range between hub and nodes
- Node ID mismatch: Verify node IDs are consistent across system
- ESP-01 not responding: Check power and connections at node
- Command format issue: Debug command structure between ESP32 and ESP-01

### Global Control Not Working

**Possible causes and solutions:**
- Firebase path mismatch: Verify path is `/system/controlAll`
- Data format issue: Command should be string "open" or "close"
- ESP32 not processing: Check control all handling in ESP32 code
- ESP-NOW broadcast issue: Verify broadcast addressing in ESP32

## Remote Monitoring Best Practices

For effective remote monitoring:

1. **Data frequency considerations**:
   - ESP-01 to ESP32: every 10-30 seconds (balance between responsiveness and power)
   - ESP32 to Firebase: every 30-60 seconds (balance between responsiveness and data usage)
   - Set appropriate `firebaseSyncInterval` in ESP32 firmware

2. **Handling connection interruptions**:
   - ESP32 firmware caches settings in EEPROM for persistence
   - ESP-01 nodes continue autonomous operation if hub communication fails
   - Web app shows "last seen" timestamp for each greenhouse

3. **Security considerations**:
   - Use secure authentication for Firebase
   - Consider implementing credential encryption in ESP32
   - Use secure hosting for web dashboard

## Advanced Integration Features

Consider implementing these advanced features:

1. **Data logging and analysis**:
   - Store historical data in Firebase
   - Add charts and trends to web dashboard
   - Implement data export functionality

2. **Alert system**:
   - Configure temperature thresholds for alerts
   - Send push notifications for critical events
   - Add email alerts for system failures

3. **Multi-user access**:
   - Implement user authentication in web dashboard
   - Create admin and viewer roles
   - Add audit logging for changes

## Using the Three-Button Navigation System

The ESP32 hub features a three-button navigation system:
1. **UP button**: Navigate up or increase values
2. **SELECT button**: Select menu items or confirm changes
3. **DOWN button**: Navigate down or decrease values

These buttons allow for easy navigation through the menu system, with different behaviors depending on the current screen:

1. In the overview screen:
   - UP/DOWN: Navigate between greenhouses
   - SELECT: View detailed information for selected greenhouse

2. In the greenhouse detail screen:
   - UP/DOWN: Navigate between settings
   - SELECT: Edit selected setting
   - When editing:
     - UP: Increase value
     - DOWN: Decrease value
     - SELECT: Confirm change

3. In the settings screen:
   - UP: Go to "Control All" screen
   - DOWN: Return to greenhouse detail
   - SELECT: Return to overview

4. In the "Control All" screen:
   - UP/DOWN: Toggle between "Open All" and "Close All"
   - SELECT: Execute selected command

## Conclusion

When properly integrated, the hardware system and web dashboard provide a complete greenhouse automation solution featuring:

- Real-time monitoring of all greenhouses
- Remote control capabilities from anywhere
- Global control for all greenhouses simultaneously
- Data visualization and analysis
- Fault tolerance and autonomous operation
- Customizable settings for each greenhouse

Regularly monitor system performance and update firmware as needed to maintain optimal operation.
