/*
 * Cloud Configuration Management
 * 
 * This module handles configuration management using Particle Ledger.
 * 
 * Expected JSON structure in "default-settings" ledger:
 * {
 *     "messaging": {
 *         "serial": true,
 *         "verboseMode": false,
 *         "disconnectedMode": false
 *     },
 *     "timing": {
 *         "timezone": "SGT-8",
 *         "reportingIntervalSec": 3600,
 *         "pollingRateSec": 1,
 *         "openHour": 6,
 *         "closeHour": 22
 *     },
 *     "power": {
 *         "lowPowerMode": false,
 *         "solarPowerMode": true
 *     },
 *     "sensor": {
 *         "facethr": 60,
 *         "gesturethr": 60
 *     }
 * }
 */

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

    Log.info("Cloud configuration system initialized");
    
    // Apply configuration from cloud defaults
    if (!configureFromCloudDefaults()) {
        Log.warn("Failed to load cloud defaults configuration");
    } else {
        Log.info("Cloud defaults applied successfully");
    }
}

void Cloud::loop() {
    // Periodic configuration check (every 5 minutes)
    static unsigned long lastConfigCheck = 0;
    const unsigned long configCheckInterval = 300000; // 5 minutes
    
    if (millis() - lastConfigCheck > configCheckInterval) {
        lastConfigCheck = millis();
        
        if (Particle.connected()) {
            // Check if updates are pending and reload if needed
            if (sysStatus.get_updatesPending()) {
                Log.info("Updates pending detected, reloading configuration");
                reloadConfiguration();
                sysStatus.set_updatesPending(false);
            }
        }
    }
}

bool Cloud::reloadConfiguration() {
    Log.info("Manually reloading configuration from cloud defaults");
    return configureFromCloudDefaults();
}

bool Cloud::configureFromCloudDefaults() {
    LedgerData data = defaultSettings.get();
    if (data.size() == 0) {
        Log.warn("No default configuration found in ledger");
        return false;
    }

    Log.info("Applying configuration from cloud defaults...");
    bool success = true;

    // Apply each configuration section
    success &= applyMessagingConfig(data);
    success &= applyTimingConfig(data);
    success &= applyPowerConfig(data);
    success &= applySensorConfig(data);

    if (success) {
        Log.info("All cloud default configurations applied successfully");
        sysStatus.set_updatesPending(true); // Mark that updates have been applied
    } else {
        Log.warn("Some configurations failed to apply");
    }

    return success;
}

bool Cloud::applyMessagingConfig(const LedgerData& data) {
    if (!data.has("messaging")) {
        Log.info("No messaging configuration found");
        return true; // Not an error, just no config
    }

    auto messaging = data.get("messaging");
    bool success = true;

    // Serial connection setting
    if (messaging.has("serial")) {
        bool serialEnabled = messaging.get("serial").asBool();
        sysStatus.set_serialConnected(serialEnabled);
        Log.info("Serial connection: %s", serialEnabled ? "ENABLED" : "DISABLED");
    }

    // Verbose mode setting
    if (messaging.has("verboseMode")) {
        bool verboseMode = messaging.get("verboseMode").asBool();
        sysStatus.set_verboseMode(verboseMode);
        Log.info("Verbose mode: %s", verboseMode ? "ENABLED" : "DISABLED");
    }

    // Disconnected mode setting
    if (messaging.has("disconnectedMode")) {
        bool disconnectedMode = messaging.get("disconnectedMode").asBool();
        sysStatus.set_disconnectedMode(disconnectedMode);
        Log.info("Disconnected mode: %s", disconnectedMode ? "ENABLED" : "DISABLED");
    }

    return success;
}

bool Cloud::applyTimingConfig(const LedgerData& data) {
    if (!data.has("timing")) {
        Log.info("No timing configuration found");
        return true;
    }

    auto timing = data.get("timing");
    bool success = true;

    // Timezone setting
    if (timing.has("timezone")) {
        String timezone = timing.get("timezone").asString();
        if (timezone.length() > 0 && timezone.length() < 39) {
            sysStatus.set_timeZoneStr(timezone.c_str());
            Log.info("Timezone set to: %s", timezone.c_str());
        } else {
            Log.warn("Invalid timezone string length: %d", timezone.length());
            success = false;
        }
    }

    // Reporting interval setting
    if (timing.has("reportingIntervalSec")) {
        int reportingInterval = timing.get("reportingIntervalSec").asInt();
        if (validateRange(reportingInterval, 300, 86400, "reportingIntervalSec")) {
            sysStatus.set_reportingInterval(reportingInterval);
            Log.info("Reporting interval set to: %d seconds", reportingInterval);
        } else {
            success = false;
        }
    }

    // Polling rate setting
    if (timing.has("pollingRateSec")) {
        int pollingRate = timing.get("pollingRateSec").asInt();
        if (validateRange(pollingRate, 1, 60, "pollingRateSec")) {
            // Convert seconds to milliseconds for the sensor config
            sensorConfig.set_pollingRate(pollingRate * 1000);
            Log.info("Polling rate set to: %d seconds (%d ms)", pollingRate, pollingRate * 1000);
        } else {
            success = false;
        }
    }

    // Open hour setting
    if (timing.has("openHour")) {
        int openHour = timing.get("openHour").asInt();
        if (validateRange(openHour, 0, 23, "openHour")) {
            sysStatus.set_openTime(openHour);
            Log.info("Open hour set to: %d", openHour);
        } else {
            success = false;
        }
    }

    // Close hour setting
    if (timing.has("closeHour")) {
        int closeHour = timing.get("closeHour").asInt();
        if (validateRange(closeHour, 0, 23, "closeHour")) {
            sysStatus.set_closeTime(closeHour);
            Log.info("Close hour set to: %d", closeHour);
        } else {
            success = false;
        }
    }

    return success;
}

bool Cloud::applyPowerConfig(const LedgerData& data) {
    if (!data.has("power")) {
        Log.info("No power configuration found");
        return true;
    }

    auto power = data.get("power");
    bool success = true;

    // Low power mode setting
    if (power.has("lowPowerMode")) {
        bool lowPowerMode = power.get("lowPowerMode").asBool();
        sysStatus.set_lowPowerMode(lowPowerMode);
        Log.info("Low power mode: %s", lowPowerMode ? "ENABLED" : "DISABLED");
    }

    // Solar power mode setting
    if (power.has("solarPowerMode")) {
        bool solarPowerMode = power.get("solarPowerMode").asBool();
        sysStatus.set_solarPowerMode(solarPowerMode);
        Log.info("Solar power mode: %s", solarPowerMode ? "ENABLED" : "DISABLED");
    }

    return success;
}

bool Cloud::applySensorConfig(const LedgerData& data) {
    if (!data.has("sensor")) {
        Log.info("No sensor configuration found");
        return true;
    }

    auto sensor = data.get("sensor");
    bool success = true;

    // Face detection threshold
    if (sensor.has("facethr")) {
        int faceThreshold = sensor.get("facethr").asInt();
        if (validateRange(faceThreshold, 0, 100, "facethr")) {
            sensorConfig.set_faceThreshold(faceThreshold);
            Log.info("Face detection threshold set to: %d", faceThreshold);
        } else {
            success = false;
        }
    }

    // Gesture detection threshold
    if (sensor.has("gesturethr")) {
        int gestureThreshold = sensor.get("gesturethr").asInt();
        if (validateRange(gestureThreshold, 0, 100, "gesturethr")) {
            sensorConfig.set_gestureThreshold(gestureThreshold);
            Log.info("Gesture detection threshold set to: %d", gestureThreshold);
        } else {
            success = false;
        }
    }

    return success;
}

template<typename T>
bool Cloud::validateRange(T value, T min, T max, const String& name) {
    if (value < min || value > max) {
        Log.warn("Invalid %s value: %d (must be between %d and %d)", 
                name.c_str(), (int)value, (int)min, (int)max);
        return false;
    }
    return true;
}

// Template instantiations for common types
template bool Cloud::validateRange<int>(int value, int min, int max, const String& name);

