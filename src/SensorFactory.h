// src/SensorFactory.h
#ifndef SENSORFACTORY_H
#define SENSORFACTORY_H

#include "ISensor.h"
#include "GestureFaceSensor.h"

// Future sensors can be added here:
// #include "PIRSensor.h"
// #include "UltrasonicSensor.h"

/**
 * @brief Enumeration of available sensor types
 * 
 * Add new sensor types here as they are implemented
 */
enum class SensorType : uint8_t {
    GESTURE_FACE = 0,
    PIR = 1,
    ULTRASONIC = 2,
    // Add more sensor types as needed
};

/**
 * @brief Factory for creating sensor instances
 * 
 * This centralizes sensor creation and makes it easy to switch sensors
 * without modifying the main application code.
 */
class SensorFactory {
public:
    /**
     * @brief Create a sensor based on type
     * @param type The type of sensor to create
     * @return Pointer to ISensor implementation, or nullptr if invalid/not implemented
     */
    static ISensor* createSensor(SensorType type) {
        switch(type) {
            case SensorType::GESTURE_FACE:
                Log.info("Creating GestureFace sensor");
                return &GestureFaceSensor::instance();
            
            // Future sensor implementations:
            // case SensorType::PIR:
            //     Log.info("Creating PIR sensor");
            //     return &PIRSensor::instance();
            //
            // case SensorType::ULTRASONIC:
            //     Log.info("Creating Ultrasonic sensor");
            //     return &UltrasonicSensor::instance();
            
            default:
                Log.error("Unknown or unimplemented sensor type: %d", (int)type);
                return nullptr;
        }
    }
    
    /**
     * @brief Get sensor type name as string
     * @param type The sensor type
     * @return String name of the sensor type
     */
    static String getSensorTypeName(SensorType type) {
        switch(type) {
            case SensorType::GESTURE_FACE: return "GestureFace";
            case SensorType::PIR: return "PIR";
            case SensorType::ULTRASONIC: return "Ultrasonic";
            default: return "Unknown";
        }
    }
};

#endif /* SENSORFACTORY_H */