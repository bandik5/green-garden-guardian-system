
#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <stdint.h>

// Menu system states
enum MenuState {
  OVERVIEW, 
  GREENHOUSE_DETAIL, 
  SCHEDULE_SETTING, 
  MANUAL_CONTROL_ALL
};

// Data structures for nodes
struct SensorData {
  uint8_t nodeId;
  float temperature;
  float humidity;
  float pressure;
  uint8_t ventStatus; // 0:closed, 1:opening, 2:open, 3:closing
  uint32_t timestamp;
};

struct ScheduleSettings {
  uint8_t openHour = 8;    // Default open at 8 AM
  uint8_t openMinute = 0;
  uint8_t closeHour = 18;  // Default close at 6 PM
  uint8_t closeMinute = 0;
  bool scheduleEnabled = false;
};

struct GreenhouseSettings {
  float temperatureThreshold = 25.0;
  float hysteresis = 0.5;
  bool autoMode = true;
  char manualCommand = 0;
  ScheduleSettings schedule;
};

struct GreenhouseData {
  SensorData sensor;
  GreenhouseSettings settings;
  bool isOnline;
  unsigned long lastSeen;
};

// Structure for outgoing control messages to nodes
struct ControlMessage {
  uint8_t targetNodeId;
  float tempThreshold;
  float hysteresis;
  bool autoMode;
  char manualCommand;
};

#endif
