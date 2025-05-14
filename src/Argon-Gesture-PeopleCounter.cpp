/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */


 // V1.0 - Initial version - Particle Variable implementation for people count and variable
 
// Include Particle Device OS APIs
#include "Particle.h"
#include "Arduino.h"
#include <DFRobot_GestureFaceDetection.h>

 // Define the device ID for the GestureFaceDetection sensor
 #define DEVICE_ID 0x72
 
 // Create an instance of DFRobot_GestureFaceDetection_I2C with the specified device ID
 DFRobot_GestureFaceDetection_I2C gfd(DEVICE_ID);

 // Define the variables here that will be used to share the data with the cloud
 int faceNumber = 0;
 int faceScore = 0;
 int gestureType = 0;
 int gestureScore = 0;

 // We are going to define some variables so we can expose them with the cloud API
bool getFaceData();             // Get the face detection data
bool getGestureData();       // Get the gesture detection data
 
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
     delay(1000);
   }
   Particle.publish("Status","Cloud connected",PRIVATE);

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
 
 void loop(){

  getFaceData();        // Get data on faces detected
  getGestureData();    // Get data on gestures detected

  delay(1000);          // Rate Limit the updates
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
      sprintf(str, "detect %d face(s) with %d%% confidence\n", faceNumber, faceScore);
    }
    else {
      sprintf(str, "Error in face detection\n");
    }
    Particle.publish("Status", str, PRIVATE);
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
  gestureType = gfd.getGestureType();
  gestureScore = gfd.getGestureScore();
  if (gestureType != oldGestureType) {
    oldGestureType = gestureType;
    if (gestureType == 0) {
      sprintf(str, "No gesture detected\n");
    }
    else if (gestureType > 0) {
      sprintf(str, "detect %d gesture(s) with %d%% confidence\n", gestureType, gestureScore);
    }
    else {
      sprintf(str, "Error in gesture detection\n");
    }
    Particle.publish("Status", str, PRIVATE);
    return true;
  }
  else
    return false;
}