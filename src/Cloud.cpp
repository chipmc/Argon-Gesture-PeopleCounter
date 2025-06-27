#include "Cloud.h"


// Identify the cloud legger we will be using
Ledger deviceSettings;
Ledger defaultSettings;

Cloud *Cloud::_instance;

// [static]
Cloud &Cloud::instance() {
    if (!_instance) {
        _instance = new Cloud();
    }
    return *_instance;
}

Cloud::Cloud() {
}

Cloud::~Cloud() {
}

void Cloud::setup() {
    deviceSettings = Particle.ledger("device-settings");        // Initialize the device settings ledger
    defaultSettings = Particle.ledger("default-settings");      // Initialize the default settings ledger   

  LedgerData data = defaultSettings.get();
  // Read the blueLED key and set the D7 pin accordingly
  if (data.has("GPIO")) {
    if (data.get("GPIO").has("blueLED")) {
      bool blueLED = data.get("GPIO").get("blueLED").asBool();
      digitalWrite(BLUE_LED, blueLED ? HIGH : LOW);
      Log.info("Blue LED setting: %s", blueLED ? "ON" : "OFF");
    } else {
      Log.warn("No blueLED setting found in config");
    }
  } else {
    Log.warn("No blueLED setting found in default settings"); 
  }
  if (data.has("sensor")) {
    if (data.get("sensor").has("facethr")) {
      int faceThreshold = data.get("sensor").get("facethreshold").asInt();
      sensorConfig.set_faceThreshold(faceThreshold); // Set the face detection threshold in the sensor library
      if (faceThreshold >= 0 && faceThreshold <= 100) {
        Log.info("Face detection threshold set to %d", faceThreshold);
        // Set the face detection threshold in the sensor library
        
      } else {
        Log.warn("Invalid face detection threshold: %d", faceThreshold);
      }
    } else {
      Log.warn("No face detection threshold found in config");
    }
  }
}

void Cloud::loop() {
    // Put your code to run during the application thread loop here
}

