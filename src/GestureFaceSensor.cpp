#include "GestureFaceSensor.h"

// Buffer for formatted output
char str[100];

// Define the device IpartiD for the GestureFaceDetection sensor
#define DEVICE_ID 0x72

// Create an instance of DFRobot_GestureFaceDetection_I2C with the specified device ID
DFRobot_GestureFaceDetection_I2C gfd(DEVICE_ID);

GestureFaceSensor *GestureFaceSensor::_instance;

// [static]
GestureFaceSensor &GestureFaceSensor::instance() {
    if (!_instance) {
        _instance = new GestureFaceSensor();
    }
    return *_instance;
}

GestureFaceSensor::GestureFaceSensor() {
}

GestureFaceSensor::~GestureFaceSensor() {
}

void GestureFaceSensor::setup() {

    // Initialize I2C communication
    gfd.begin(&Wire);

    while (!gfd.begin()) {
        Particle.publish("Status","Communication with device failed, please check connection",PRIVATE);
        delay(1000);
    }
    Particle.publish("Status","Communication with device established",PRIVATE);

    // Set the face detection threshold. Face scores range from 0 to 100.
    // Faces with scores above this threshold will be detected.
    if (gfd.setFaceDetectThres(sensorConfig.get_faceThreshold())) {
        Particle.publish("Status","Set the face detection threshold success.", PRIVATE);
    } else {
        Particle.publish("Status","Set the face detection threshold fail.", PRIVATE);
    }
    
    // Set the gesture detection threshold. Gesture scores range from 0 to 100.
    // Gestures with scores above this threshold will be detected.
    if (gfd.setGestureDetectThres(sensorConfig.get_gestureThreshold())) {
        Particle.publish("Status","Set the gesture detection threshold success.", PRIVATE);
    } else {
        Particle.publish("Status","Set the gesture detection threshold fail.", PRIVATE);
    }

}

bool GestureFaceSensor::loop() {
    if (GestureFaceSensor::getFaceData() || GestureFaceSensor::getGestureData()) return true; // If we have new data, return true
    else return false; // If no new data, return false
}


// Get the face detection data
bool GestureFaceSensor::getFaceData() {
  static uint16_t oldFaceNumber = 0;
  uint16_t faceNumber = gfd.getFaceNumber();
  uint16_t faceScore = gfd.getFaceScore();

  if (faceNumber != oldFaceNumber) {
    Log.info("Face Number: %d, Old Face Number %d, Face Score: %d", faceNumber, oldFaceNumber, faceScore);
    oldFaceNumber = faceNumber;
    current.set_faceNumber(faceNumber);
    current.set_faceScore(faceScore);
    if (faceNumber == 0) {
      sprintf(str, "No face detected\n");
    }
    else if (faceNumber > 0) {
      sprintf(str, "Detected %d faces with a confidence of %d %%", faceNumber, faceScore);
    }
    else {
      sprintf(str,"Error in face detection\n");
    }
    if(Particle.connected()) Particle.publish("Status", str, PRIVATE);
    Log.info("%s", str);
    return true;
  }
  else return false;
}

bool GestureFaceSensor::getGestureData() {
    // Print the gesture detection results
    // - 1: LIKE (👍) - blue
    // - 2: OK (👌) - green
    // - 3: STOP (🤚) - red
    // - 4: YES (✌️) - yellow
    // - 5: SIX (🤙) - purple
  static uint16_t oldGestureType = 0;
  char gestureTypeStr[16];
  uint16_t gestureType = gfd.getGestureType();
  uint16_t gestureScore = gfd.getGestureScore();

  if (gestureType != oldGestureType) {
    oldGestureType = gestureType;
    current.set_gestureType(gestureType);
    current.set_gestureScore(gestureScore);
    if (gestureType == 0) {
      sprintf(str, "No gesture detected\n");
    }
    else if (gestureType > 0) {
      switch (gestureType) {
        case 1: sprintf(gestureTypeStr, "LIKE"); break;
        case 2: sprintf(gestureTypeStr, "OK"); break;
        case 3: sprintf(gestureTypeStr, "STOP"); break;
        case 4: sprintf(gestureTypeStr, "PEACE"); break;
        case 5: sprintf(gestureTypeStr, "HANG LOOSE"); break;
        default: sprintf(gestureTypeStr, "Unknown gesture");
      }
      sprintf(str, "Detected a %s gesture with a confidence of %d %%", gestureTypeStr, gestureScore);
    }
    else {
      sprintf(str, "Error in gesture detection\n");
    }
    if (Particle.connected()) Particle.publish("Status", str, PRIVATE);
    Log.info("%s", str);
    return true;
  }
  else return false;
}

