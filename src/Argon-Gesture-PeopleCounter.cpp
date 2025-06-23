/* 
 * Project Argon-Gesture-PeopleCounter
 * Description: This project implements a people counter using the DFRobot GestureFaceDetection sensor.
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
 
// Include Particle Device OS APIs
#include "Particle.h"
#include "Arduino.h"
#include <DFRobot_GestureFaceDetection.h>

// Define the device ID for the GestureFaceDetection sensor
#define DEVICE_ID 0x72

// Turn on logging for debugging
SerialLogHandler logHandler;

// Create an instance of DFRobot_GestureFaceDetection_I2C with the specified device ID
DFRobot_GestureFaceDetection_I2C gfd(DEVICE_ID);

// Define the variables here that will be used to share the data with the cloud
int faceNumber = 0;
int faceScore = 0;
int gestureType = 0;
int gestureScore = 0;

// Global variables for configuration - will add storage later
ulong sensorDebounce = 1000; // Debounce time for the sensor
ulong lastSensorUpdate = 0; // Last time the sensor was updated

// We are going to define some variables so we can expose them with the cloud API
bool getFaceData();             // Get the face detection data
bool getGestureData();          // Get the gesture detection data
void publishData();             // Publish the data to the cloud
 
// Buffer for formatted output
char str[100];
 
void setup() {
  // Wait for the sensor to start.
  delay(5000);
  
  // Initialize I2C communication
  gfd.begin(&Wire);

  // Define the Particle variables and functions
  Particle.variable("faceNumber", faceNumber);
  Particle.variable("gestureType", gestureType);
 
  while (!Particle.connected()) {
    Particle.publish("Status","Waiting for cloud connection",PRIVATE);
    Log.info("Waiting for cloud connection...");
    delay(1000);
  }
  Particle.publish("Status","Cloud connected",PRIVATE);
  Log.info("Cloud connected");

  while (!gfd.begin()) {
    Particle.publish("Status","Communication with device failed, please check connection",PRIVATE);
    delay(1000);
  }
  // Set the face detection threshold. Face scores range from 0 to 100.
  // Faces with scores above this threshold will be detected.
  if (gfd.setFaceDetectThres(60)) {
    Particle.publish("Status","Set the face detection threshold success.", PRIVATE);
  } else {
    Particle.publish("Status","Set the face detection threshold fail.", PRIVATE);
  }
 
  // Set the gesture detection threshold. Gesture scores range from 0 to 100.
  // Gestures with scores above this threshold will be detected.
  if (gfd.setGestureDetectThres(60)) {
    Particle.publish("Status","Set the gesture detection threshold success.", PRIVATE);
  } else {
    Particle.publish("Status","Set the gesture detection threshold fail.", PRIVATE);
  }
 
  // Set the gesture detection range.
  // The range is from 0 to 100; 0 has the smallest detection range, and 100 has the largest.
  if (gfd.setDetectThres(100)) {
    Particle.publish("Status","Set the gesture detection range success.", PRIVATE);
  } else {
    Particle.publish("Status","Set the gesture detection range fail.", PRIVATE);
  }
}
 
void loop() {                                 // The main loops runs forever and checks the sensor every "sensorDebounce" milliseconds
  if (millis() - lastSensorUpdate > sensorDebounce) {
    lastSensorUpdate = millis();
    if (getFaceData() || getGestureData()){  // Get data on gestures detected
      publishData();                          // Publish the data to the cloud if anything changed
    }   
  }
}

// Get the face detection data
bool getFaceData() {
  static int oldFaceNumber = 0;

  faceNumber = gfd.getFaceNumber();
  faceScore = gfd.getFaceScore();

  if (faceNumber != oldFaceNumber) {
    oldFaceNumber = faceNumber;
    if (faceNumber == 0) {
      sprintf(str, "No face detected\n");
    }
    else if (faceNumber > 0) {
      sprintf(str, "Detected %d faces with a confidence of %d%%", faceNumber, faceScore);
    }
    else {
      sprintf(str,"Error in face detection\n");

    }
    if(Particle.connected()) Particle.publish("Status", str, PRIVATE);
    Log.info(str);
    return true;
  }
  else
    return false;
}

bool getGestureData() {
    // Print the gesture detection results
    // - 1: LIKE (👍) - blue
    // - 2: OK (👌) - green
    // - 3: STOP (🤚) - red
    // - 4: YES (✌️) - yellow
    // - 5: SIX (🤙) - purple
  static int oldGestureType = 0;
  char gestureTypeStr[16];

  gestureType = gfd.getGestureType();
  gestureScore = gfd.getGestureScore();

  if (gestureType != oldGestureType) {
    oldGestureType = gestureType;
    if (gestureType == 0) {
      sprintf(str, "No gesture detected\n");
    }
    else if (gestureType > 0) {
      switch (gestureType) {
        case 1: sprintf(gestureTypeStr, "LIKE"); break;
        case 2: sprintf(gestureTypeStr, "OK"); break;
        case 3: sprintf(gestureTypeStr, "STOP"); break;
        case 4: sprintf(gestureTypeStr, "YES"); break;
        case 5: sprintf(gestureTypeStr, "SIX"); break;
        default: sprintf(gestureTypeStr, "Unknown gesture");
      }
      sprintf(str, "Dectected a %s gesture with a confidence of %d%%", gestureTypeStr, gestureScore);
    }
    else {
      sprintf(str, "Error in gesture detection\n");
    }
    if (Particle.connected()) Particle.publish("Status", str, PRIVATE);
    return true;
  }
  else
    return false;
}

// Publish the data to the cloud  
void publishData() {
  char str[100];
  sprintf(str, "{\"gesturetype\": %d, \"gesturescore\": %d, \"facenumber\": %d, \"facescore\": %d}", gestureType, gestureScore, faceNumber, faceScore);
  if (Particle.connected()) Particle.publish("ubidots-data", str, PRIVATE);
}

