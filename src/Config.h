#ifndef CONFIG_H
#define CONFIG_H

#include "Particle.h"

class Config {
public:
    static Config &instance();
    
    // Configuration parameters
    unsigned long sensorDebounce = 1000;
    unsigned long cloudPublishInterval = 30000;
    bool debugMode = false;
    
    // Sensor thresholds
    int minConfidenceThreshold = 50;
    int maxDetectionRange = 100;
    
    void loadDefaults();
    void saveToEEPROM();
    void loadFromEEPROM();
    
private:
    Config() = default;
    static Config* _instance;
};

#endif
