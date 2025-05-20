
# Firebase Setup for Greenhouse Automation System

This guide provides step-by-step instructions for setting up Firebase Realtime Database to work with your greenhouse automation system.

## Prerequisites

- A Google account
- Web browser access to set up the Firebase project
- The ESP32 hub hardware ready for programming

## Step 1: Create a Firebase Project

1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Click "Add project"
3. Enter a name for your project (e.g., "Greenhouse Automation")
4. Choose whether to enable Google Analytics (optional)
5. Click "Create project"

## Step 2: Create a Realtime Database

1. In your Firebase project console, click "Build" in the left menu
2. Select "Realtime Database"
3. Click "Create database"
4. Choose a location closest to your physical location
5. Start in test mode (we'll secure it later)
6. Click "Enable"

## Step 3: Set Up Database Structure

Copy and paste the following JSON into your database to set up the initial structure:

```json
{
  "greenhouses": {
    "1": {
      "name": "Greenhouse 1",
      "currentData": {
        "temperature": 0,
        "humidity": 0,
        "pressure": 0,
        "ventStatus": "closed",
        "timestamp": 0
      },
      "settings": {
        "temperatureThreshold": 25,
        "hysteresis": 0.5,
        "mode": "auto"
      }
    },
    "2": {
      "name": "Greenhouse 2",
      "currentData": {
        "temperature": 0,
        "humidity": 0,
        "pressure": 0,
        "ventStatus": "closed",
        "timestamp": 0
      },
      "settings": {
        "temperatureThreshold": 25,
        "hysteresis": 0.5,
        "mode": "auto"
      }
    },
    "3": {
      "name": "Greenhouse 3",
      "currentData": {
        "temperature": 0,
        "humidity": 0,
        "pressure": 0,
        "ventStatus": "closed",
        "timestamp": 0
      },
      "settings": {
        "temperatureThreshold": 25,
        "hysteresis": 0.5,
        "mode": "auto"
      }
    },
    "4": {
      "name": "Greenhouse 4",
      "currentData": {
        "temperature": 0,
        "humidity": 0,
        "pressure": 0,
        "ventStatus": "closed",
        "timestamp": 0
      },
      "settings": {
        "temperatureThreshold": 25,
        "hysteresis": 0.5,
        "mode": "auto"
      }
    },
    "5": {
      "name": "Greenhouse 5",
      "currentData": {
        "temperature": 0,
        "humidity": 0,
        "pressure": 0,
        "ventStatus": "closed",
        "timestamp": 0
      },
      "settings": {
        "temperatureThreshold": 25,
        "hysteresis": 0.5,
        "mode": "auto"
      }
    },
    "6": {
      "name": "Greenhouse 6",
      "currentData": {
        "temperature": 0,
        "humidity": 0,
        "pressure": 0,
        "ventStatus": "closed",
        "timestamp": 0
      },
      "settings": {
        "temperatureThreshold": 25,
        "hysteresis": 0.5,
        "mode": "auto"
      }
    }
  },
  "system": {
    "lastSync": 0,
    "hubStatus": "offline",
    "hubIp": "",
    "version": "1.0"
  }
}
```

To do this:
1. In the Realtime Database interface, click on the "+" icon at the root level
2. Paste the JSON structure above
3. Click "Add"

## Step 4: Set Up Authentication

For this project, we'll use anonymous authentication:

1. In your Firebase project console, click "Build" in the left menu
2. Select "Authentication"
3. Click "Get started"
4. Select the "Anonymous" provider
5. Toggle the "Enable" switch
6. Click "Save"

## Step 5: Generate API Keys

1. In your Firebase project settings (gear icon near Project Overview):
2. Go to "Project settings"
3. Scroll down to "Your apps" section
4. Click on the web platform icon `</>`
5. Register your app with a nickname (e.g., "Greenhouse Web App")
6. Copy the Firebase configuration object - you'll need this for both the ESP32 and web application
7. Click "Register app"

## Step 6: Set Up Database Rules

Navigate to "Realtime Database" > "Rules" tab and replace the default with:

```json
{
  "rules": {
    ".read": "auth != null",
    "greenhouses": {
      "$greenhouse_id": {
        "currentData": {
          ".write": "auth != null"
        },
        "settings": {
          ".write": "auth != null"
        },
        "name": {
          ".write": "auth != null"
        }
      }
    },
    "system": {
      ".write": "auth != null"
    }
  }
}
```

Click "Publish" to save the rules.

## Step 7: Configure ESP32 with Firebase Credentials

1. In the ESP32 hub firmware (`esp32_hub_firmware.ino`), update these lines with your Firebase configuration:

```cpp
// ----- WIFI CONFIGURATION -----
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ----- FIREBASE CONFIGURATION -----
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"
```

Replace:
- `YOUR_WIFI_SSID` with your WiFi network name
- `YOUR_WIFI_PASSWORD` with your WiFi password
- `YOUR_FIREBASE_API_KEY` with the API key from your Firebase configuration
- `YOUR_FIREBASE_DATABASE_URL` with the RTDB URL from your Firebase configuration (e.g., "https://greenhouse-automation-xxxxx.firebaseio.com")

## Step 8: Firebase Library for ESP32

The ESP32 firmware requires the Firebase ESP client library. Install it in Arduino IDE:

1. Open Arduino IDE
2. Go to Tools > Manage Libraries
3. Search for "Firebase ESP Client"
4. Install the library by Mobizt

## Step 9: Configure Web Application with Firebase

If you're using the web application:

1. In the React app, update the Firebase configuration in `/src/firebase.ts`
2. Use the same configuration object from step 5

## Connecting ESP32 Hub to Firebase

The ESP32 hub connects to Firebase and performs these operations:

1. **Uploads data**: Sends sensor readings from all greenhouses to Firebase
2. **Downloads settings**: Gets user-defined settings like temperature thresholds
3. **Handles manual commands**: Processes manual open/close commands from the web interface

## Testing the Connection

1. Upload the ESP32 hub firmware with your credentials
2. Monitor the Serial output for connection status
3. Check the Firebase console - you should see data updating
4. Try changing settings in the web app or Firebase console - the hub should respond

## Troubleshooting Firebase Connection

- **Connection issues**: Check WiFi credentials and signal strength
- **Authentication fails**: Verify API key is correct
- **Database writes fail**: Check database rules and path structure
- **Missing data**: Ensure JSON structure matches the code expectations

## Security Considerations

While this setup works for a personal project, for a production environment consider:

1. Implementing more robust authentication
2. Setting up Firebase App Check
3. Using environment variables for sensitive credentials
4. Implementing secure boot on ESP32
5. Setting database size limits to prevent abuse
