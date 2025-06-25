#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include "Particle.h"

struct GestureData {
    int type = 0;
    int confidence = 0;
    bool detected = false;
    unsigned long timestamp = 0;
};

struct FaceData {
    int count = 0;
    int confidence = 0;
    bool detected = false;
    unsigned long timestamp = 0;
};

struct SensorReading {
    GestureData gesture;
    FaceData face;
    bool isValid = false;
    unsigned long readingTime = 0;
    
    void clear() {
        gesture = {};
        face = {};
        isValid = false;
        readingTime = 0;
    }
    
    String toJSON() const {
        return String::format(
            "{\"gesture\":{\"type\":%d,\"confidence\":%d,\"detected\":%s},"
            "\"face\":{\"count\":%d,\"confidence\":%d,\"detected\":%s},"
            "\"timestamp\":%lu}",
            gesture.type, gesture.confidence, gesture.detected ? "true" : "false",
            face.count, face.confidence, face.detected ? "true" : "false",
            readingTime
        );
    }
};

#endif
