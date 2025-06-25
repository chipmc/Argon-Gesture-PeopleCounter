/* 
 * Project Argon-Gesture-PeopleCounter
 * Description: This project implements a people counter usvoid loop() {
  // Check system health
  if (!StateManager::instance().isHealthy()) {
    handleSystemError();
    return;
  }
  
  // Sensor reading with configured debounce
  if (millis() - lastSensorUpdate > Config::instance().sensorDebounce) {
    lastSensorUpdate = millis();
    
    if (take_measurements::instance().loop()) {
      // Immediate publish for status updates
      publishData();
    }
  }
  
  // Periodic state publishing
  if (millis() - lastCloudPublish > Config::instance().cloudPublishInterval) {
    lastCloudPublish = millis();
    StateManager::instance().publishStateToCloud();
  }
  
  // Keep cloud connection alive
  Particle.process();
}DFRobot GestureFaceDetection sensor.
 * It detects the number of faces and gestures in front of the sensor and publishes the data to the Particle cloud.
 * The project uses the Particle Device OS APIs and the DFRobot GestureFaceDetection library.
 * Author: Charles McClelland 
 * Date: 5/15/2025
 * License: MIT
 * Repo: https://github.com/chipmc/Argon-Gesture-PeopleCounter
*/

// v1.0 - Initial version - Particle Variable implementation for people count and variable
// v1.1 - Made the code non-blocking 
// v1.2 - Added Particle.publish() to send data to the cloud
// v1.3 - Added serial logging for debugging
// v1.4 - Added "digital twin" settings in the cloud to allow for remote configuration of the sensor
// v1.5 - Implemented some basic archtectureal changes to support the new Particle Device OS APIs and the DFRobot GestureFaceDetection library
 
// Include Particle Device OS APIs
#include "Particle.h"
#include "device_pinout.h"
#include "Particle_Functions.h"
#include "take_measurements.h"
#include "Cloud.h"
#include "Config.h"
#include "StateManager.h"
#include "SensorData.h"

void publishData();             // Publish the data to the cloud
bool initializeSystem();       // Initialize all system components
void handleSystemError();      // Handle system errors

// Remove global variables - use Config class instead
// ulong sensorDebounce = 1000; // Now in Config class
// ulong lastSensorUpdate = 0;

// Local timing variables
unsigned long lastSensorUpdate = 0;
unsigned long lastCloudPublish = 0;
 
void setup() {
  // Development mode serial connection
  if (Config::instance().debugMode) {
    waitFor(Serial.isConnected, 10000);
  }
  
  StateManager::instance().setState(SystemState::INITIALIZING);
  
  if (!initializeSystem()) {
    StateManager::instance().setError(ErrorCode::SENSOR_INIT_FAILED);
    handleSystemError();
    return;
  }
  
  StateManager::instance().setState(SystemState::RUNNING);
  Log.info("System initialized successfully - entering main loop");
}

bool initializeSystem() {
  Log.info("Initializing system components...");
  
  // Load configuration
  Config::instance().loadDefaults();
  
  // Initialize hardware
  initializePinModes();
  
  // Initialize system components
  if (!take_measurements::instance().setup()) {
    Log.error("Failed to initialize sensor");
    return false;
  }
  
  Particle_Functions::instance().setup();
  Cloud::instance().setup();
  
  // Connect to cloud with timeout
  StateManager::instance().setState(SystemState::CONNECTING);
  Particle_Functions::instance().connectToCloud();
  
  unsigned long startTime = millis();
  while (millis() - startTime < 30000 && !Particle.connected()) {
    Particle.process();
    delay(100);
  }
  
  if (!Particle.connected()) {
    Log.warn("Cloud connection failed, continuing in offline mode");
  }
  
  return true;
}

void handleSystemError() {
  Log.error("System error detected - entering error state");
  // Could implement recovery logic here
  while (true) {
    Particle.process();
    delay(1000);
  }
}
 
void loop() {                                 // The main loops runs forever and checks the sensor every "sensorDebounce" milliseconds
  if (millis() - lastSensorUpdate > Config::instance().sensorDebounce) {
    lastSensorUpdate = millis();
    if (take_measurements::instance().loop()) {  // Get data on gestures detected
      publishData();                          // Publish the data to the cloud if anything changed
    }   
  }
}


// Publish the data to the cloud  
void publishData() {
  // char str[100];
  // sprintf(str, "{\"gesturetype\": %d, \"gesturescore\": %d, \"facenumber\": %d, \"facescore\": %d}", gestureType, gestureScore, faceNumber, faceScore);
  // if (Particle.connected()) Particle.publish("ubidots-data", str, PRIVATE);
}

