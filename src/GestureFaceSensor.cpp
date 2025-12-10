// src/GestureFaceSensor.cpp - Updated implementation
#include "GestureFaceSensor.h"

// Buffer for formatted output
static char str[100];

// Define the device I2C address for the GestureFaceDetection sensor
#define DEVICE_ID 0x72

GestureFaceSensor *GestureFaceSensor::_instance;

// [static]
GestureFaceSensor &GestureFaceSensor::instance() {
    if (!_instance) {
        _instance = new GestureFaceSensor();
    }
    return *_instance;
}

GestureFaceSensor::GestureFaceSensor() : _initialized(false), _gfd(nullptr) {
    _lastData.sensorType = "GestureFace";
}

GestureFaceSensor::~GestureFaceSensor() {
    if (_gfd) {
        delete _gfd;
        _gfd = nullptr;
    }
}

bool GestureFaceSensor::setup() {
    Log.info("Initializing GestureFace sensor");
    
    // Create the sensor instance
    _gfd = new DFRobot_GestureFaceDetection_I2C(DEVICE_ID);
    
    if (!_gfd) {
        Log.error("Failed to allocate GestureFace sensor");
        return false;
    }
    
    // Initialize I2C communication
    if (_gfd->begin(&Wire)) {
        _initialized = true;
        Log.info("GestureFace sensor communication established");
    } else {
        Log.error("GestureFace sensor communication failed");
        return false;
    }
    
    // Set the face detection threshold
    if (_gfd->setFaceDetectThres(sensorConfig.get_faceThreshold())) {
        Log.info("Face detection threshold set to %d", sensorConfig.get_faceThreshold());
    } else {
        Log.warn("Failed to set face detection threshold");
    }
    
    // Set the gesture detection threshold
    if (_gfd->setGestureDetectThres(sensorConfig.get_gestureThreshold())) {
        Log.info("Gesture detection threshold set to %d", sensorConfig.get_gestureThreshold());
    } else {
        Log.warn("Failed to set gesture detection threshold");
    }
    
    return _initialized;
}

bool GestureFaceSensor::loop() {
    if (!_initialized || !_gfd) {
        return false;
    }
    
    bool hasNewData = false;
    
    // Check for face data
    if (getFaceData()) {
        hasNewData = true;
    }
    
    // Check for gesture data
    if (getGestureData()) {
        hasNewData = true;
    }
    
    if (hasNewData) {
        _lastData.timestamp = Time.now();
        _lastData.hasNewData = true;
        
        // Update persistent storage with new data
        current.set_faceNumber(_lastData.faceNumber);
        current.set_faceScore(_lastData.faceScore);
        current.set_gestureType(_lastData.gestureType);
        current.set_gestureScore(_lastData.gestureScore);
        current.set_lastCountTime(_lastData.timestamp);
    }
    
    return hasNewData;
}

SensorData GestureFaceSensor::getData() const {
    return _lastData;
}

void GestureFaceSensor::reset() {
    Log.info("Resetting GestureFace sensor");
    _lastData = SensorData();
    _lastData.sensorType = "GestureFace";
}

// Get the face detection data
bool GestureFaceSensor::getFaceData() {
    if (!_gfd) return false;
    
    static uint16_t oldFaceNumber = 0;
    uint16_t faceNumber = _gfd->getFaceNumber();
    uint16_t faceScore = _gfd->getFaceScore();
    
    if (faceNumber != oldFaceNumber) {
        Log.info("Face Number: %d, Old Face Number %d, Face Score: %d", 
                 faceNumber, oldFaceNumber, faceScore);
        oldFaceNumber = faceNumber;
        
        _lastData.faceNumber = faceNumber;
        _lastData.faceScore = faceScore;
        
        if (faceNumber == 0) {
            snprintf(str, sizeof(str), "No face detected");
        } else if (faceNumber > 0) {
            snprintf(str, sizeof(str), "Detected %d faces with confidence of %d%%",
                     faceNumber, faceScore);
        } else {
            snprintf(str, sizeof(str), "Error in face detection");
        }
        
        if (Particle.connected() && sysStatus.get_verboseMode()) {
            Particle.publish("Status", str, PRIVATE);
        }
        Log.info("%s", str);
        return true;
    }
    
    return false;
}

bool GestureFaceSensor::getGestureData() {
    if (!_gfd) return false;
    
    // Gesture types:
    // - 1: LIKE (👍)
    // - 2: OK (👌)
    // - 3: STOP (🤚)
    // - 4: YES (✌️)
    // - 5: SIX (🤙)
    static uint16_t oldGestureType = 0;
    char gestureTypeStr[16];
    uint16_t gestureType = _gfd->getGestureType();
    uint16_t gestureScore = _gfd->getGestureScore();
    
    if (gestureType != oldGestureType) {
        oldGestureType = gestureType;
        
        _lastData.gestureType = gestureType;
        _lastData.gestureScore = gestureScore;
        
        if (gestureType == 0) {
            snprintf(str, sizeof(str), "No gesture detected");
        } else if (gestureType > 0) {
            switch (gestureType) {
                case 1: snprintf(gestureTypeStr, sizeof(gestureTypeStr), "LIKE"); break;
                case 2: snprintf(gestureTypeStr, sizeof(gestureTypeStr), "OK"); break;
                case 3: snprintf(gestureTypeStr, sizeof(gestureTypeStr), "STOP"); break;
                case 4: snprintf(gestureTypeStr, sizeof(gestureTypeStr), "PEACE"); break;
                case 5: snprintf(gestureTypeStr, sizeof(gestureTypeStr), "HANG LOOSE"); break;
                default: snprintf(gestureTypeStr, sizeof(gestureTypeStr), "Unknown"); break;
            }
            snprintf(str, sizeof(str), "Detected %s gesture with confidence of %d%%",
                     gestureTypeStr, gestureScore);
        } else {
            snprintf(str, sizeof(str), "Error in gesture detection");
        }
        
        if (Particle.connected() && sysStatus.get_verboseMode()) {
            Particle.publish("Status", str, PRIVATE);
        }
        Log.info("%s", str);
        return true;
    }
    
    return false;
}