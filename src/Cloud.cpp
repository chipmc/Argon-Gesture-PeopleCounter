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


// Identify the cloud ledger we will be using
Ledger defaultSettings;         // Cloud to device, product defaults
Ledger deviceSettings;          // Cloud to device, device-specific overrides

Cloud *Cloud::_instance;

// [static]
Cloud &Cloud::instance() {
    if (!_instance) {
        _instance = new Cloud();
    }
    return *_instance;
}

Cloud::Cloud() {
    isFirstConfiguration = false;
}

Cloud::~Cloud() {
}

void Cloud::setup() {
    // Initialize ledgers but don't sync yet
    defaultSettings = Particle.ledger("default-settings");
    deviceSettings = Particle.ledger("device-settings");
    
    Log.info("Cloud configuration system initialized");
    
    // Load configuration from persistent memory first
    if (!loadConfigurationFromPersistentMemory()) {
        Log.info("Invalid configuration in persistent memory, will need to connect to cloud");
        // Set flag to indicate cloud connection needed
        sysStatus.set_updatesPending(true);
    } else {
        Log.info("Valid configuration loaded from persistent memory");
        // Always check on first connection after boot to ensure we have latest config
        sysStatus.set_updatesPending(true);
        Log.info("Will check for cloud updates on first connection");
    }
}

void Cloud::loop() {
    // Only check for updates if we're connected
    if (Particle.connected()) {
        
        if (sysStatus.get_disconnectedMode()) {
            // Disconnected mode: Check for config changes when updatesPending is set
            // or on first connection after device startup
            if (sysStatus.get_updatesPending()) {
                Log.info("Disconnected mode: Updates pending, checking configuration");
                if (loadConfigurationFromCloud()) {
                    Log.info("Configuration loaded and saved successfully");
                } else {
                    Log.warn("Failed to load configuration from cloud");
                }
            }
        } else {
            // Connected mode: Check once per hour using real time
            static time_t lastConfigCheck = 0;
            const time_t configCheckInterval = 3600; // 1 hour in seconds
            
            time_t currentTime = Time.now();
            if (currentTime - lastConfigCheck >= configCheckInterval || sysStatus.get_updatesPending()) {
                lastConfigCheck = currentTime;
                Log.info("Connected mode: Checking for configuration updates");
                if (loadConfigurationFromCloud()) {
                    Log.info("Configuration loaded and saved successfully");
                } else {
                    Log.warn("Failed to load configuration from cloud");
                }
            }
        }
    }
}

bool Cloud::loadConfigurationFromPersistentMemory() {
    Log.info("Loading configuration from persistent memory");
    
    // Configuration is already loaded into sysStatus and sensorConfig
    // Just need to validate it
    return validateCurrentConfiguration();
}

bool Cloud::validateCurrentConfiguration() {
    bool isValid = true;
    
    Log.info("Validating current configuration:");
    
    // Validate time settings (most critical)
    int openTime = sysStatus.get_openTime();
    int closeTime = sysStatus.get_closeTime();
    
    Log.info("  openTime: %d, closeTime: %d", openTime, closeTime);
    
    if (openTime < 0 || openTime > 23) {
        Log.warn("Invalid openTime: %d (must be 0-23)", openTime);
        isValid = false;
    }
    
    if (closeTime < 0 || closeTime > 23) {
        Log.warn("Invalid closeTime: %d (must be 0-23)", closeTime);
        isValid = false;
    }
    
    // Validate other critical settings
    int reportingInterval = sysStatus.get_reportingInterval();
    Log.info("  reportingInterval: %d", reportingInterval);
    
    if (reportingInterval < 60 || reportingInterval > 86400) { // 1 minute to 24 hours
        Log.warn("Invalid reportingInterval: %d (must be 60-86400)", reportingInterval);
        isValid = false;
    }
    
    // Polling rate validation - stored in seconds, 0 = interrupt mode
    int pollingRate = sensorConfig.get_pollingRate(); // Already in seconds
    Log.info("  pollingRate: %d seconds", pollingRate);
    
    if (pollingRate < 0 || pollingRate > 3600) { // 0 seconds (interrupt) to 1 hour
        Log.warn("Invalid pollingRate: %d seconds (must be 0-3600, where 0 = interrupt mode)", pollingRate);
        isValid = false;
    }
    
    // Validate sensor thresholds - require meaningful values
    int faceThreshold = sensorConfig.get_faceThreshold();
    int gestureThreshold = sensorConfig.get_gestureThreshold();
    Log.info("  faceThreshold: %d, gestureThreshold: %d", faceThreshold, gestureThreshold);
    
    if (faceThreshold <= 0 || faceThreshold > 100) {
        Log.warn("Invalid faceThreshold: %d (must be 1-100)", faceThreshold);
        isValid = false;
    }
    
    if (gestureThreshold <= 0 || gestureThreshold > 100) {
        Log.warn("Invalid gestureThreshold: %d (must be 1-100)", gestureThreshold);
        isValid = false;
    }
    
    if (!isValid) {
        Log.warn("Configuration validation failed");
    } else {
        Log.info("Configuration validation passed");
    }
    
    return isValid;
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
            Log.warn("Invalid timezone length: %d", timezone.length());
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

    // Polling rate setting - expecting seconds in JSON, storing as seconds
    if (timing.has("pollingRateSec")) {
        int pollingRate = timing.get("pollingRateSec").asInt();
        if (validateRange(pollingRate, 0, 3600, "pollingRateSec")) { // 0 = interrupt, 1-3600 seconds
            sensorConfig.set_pollingRate(pollingRate); // Store directly in seconds
            if (pollingRate == 0) {
                Log.info("Polling rate set to: interrupt mode (0 seconds)");
            } else {
                Log.info("Polling rate set to: %d seconds", pollingRate);
            }
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

// Fix the validateRange function - add template declaration
template<typename T>
bool Cloud::validateRange(T value, T min, T max, const String& name) {
    if (value < min || value > max) {
        Log.warn("Invalid %s value: %d (must be between %d and %d)", name.c_str(), (int)value, (int)min, (int)max);
        return false;
    }
    return true;
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
        if (validateRange(faceThreshold, 1, 100, "facethr")) {
            sensorConfig.set_faceThreshold(faceThreshold);
            Log.info("Face detection threshold set to: %d", faceThreshold);
        } else {
            success = false;
        }
    }

    // Gesture detection threshold
    if (sensor.has("gesturethr")) {
        int gestureThreshold = sensor.get("gesturethr").asInt();
        if (validateRange(gestureThreshold, 1, 100, "gesturethr")) {
            sensorConfig.set_gestureThreshold(gestureThreshold);
            Log.info("Gesture detection threshold set to: %d", gestureThreshold);
        } else {
            success = false;
        }
    }

    return success;
}

bool Cloud::configureFromDeviceSettings() {
    Log.info("Attempting to get data from device settings ledger");
    LedgerData data = deviceSettings.get();
    
    if (data.size() == 0) {
        Log.warn("No device configuration found in ledger");
        return false;
    }

    Log.info("Applying configuration from device-specific settings...");
    bool success = true;

    // Apply each configuration section
    success &= applyMessagingConfig(data);
    success &= applyTimingConfig(data);
    success &= applyPowerConfig(data);
    success &= applySensorConfig(data);

    if (success) {
        Log.info("All device-specific configurations applied successfully");
        
        // Force save the new configuration to persistent memory
        sysStatus.flush(true);
        sensorConfig.flush(true);
        
        Log.info("Configuration saved to persistent memory");
    } else {
        Log.warn("Some configurations failed to apply");
    }

    return success;
}

bool Cloud::configureFromCloudDefaults() {
    Log.info("Attempting to get data from default settings ledger");
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
        
        // DEBUG: Check values before flush
        Log.info("DEBUG: Before flush - faceThreshold: %d, gestureThreshold: %d", 
                 sensorConfig.get_faceThreshold(), sensorConfig.get_gestureThreshold());
        
        // Force save the new configuration to persistent memory
        sysStatus.flush(true);
        sensorConfig.flush(true);  // flush() returns void, not bool
        
        // DEBUG: Check values after flush
        Log.info("DEBUG: After flush - faceThreshold: %d, gestureThreshold: %d", 
                 sensorConfig.get_faceThreshold(), sensorConfig.get_gestureThreshold());
                 
        Log.info("Configuration saved to persistent memory");
    } else {
        Log.warn("Some configurations failed to apply");
    }

    return success;
}

bool Cloud::loadConfigurationFromCloud() {
    if (!Particle.connected()) {
        Log.warn("Not connected to cloud, cannot load configuration");
        return false;
    }
    
    Log.info("Loading configuration from cloud");
    
    // Subscribe to ledger updates if not already done
    if (defaultSettings.isValid()) {
        Log.info("Default settings ledger is valid");
        defaultSettings.onSync(syncCallback);
    } else {
        Log.warn("Default settings ledger is NOT valid");
    }
    
    if (deviceSettings.isValid()) {
        Log.info("Device settings ledger is valid");
        deviceSettings.onSync(syncCallback);
    } else {
        Log.warn("Device settings ledger is NOT valid");
    }
    
    bool success = false;
    
    // Try device-specific settings first
    if (isDeviceConfigured()) {
        Log.info("Device is configured, loading device-specific settings");
        success = configureFromDeviceSettings();
    } else {
        Log.info("Device is NOT configured, will load defaults");
    }
    
    // Fall back to defaults if device settings don't exist or failed
    if (!success) {
        Log.info("Loading default settings from cloud");
        success = configureFromCloudDefaults();
        
        // If defaults loaded successfully, create device settings
        if (success && !isDeviceConfigured()) {
            Log.info("Creating device configuration from defaults");
            writeDeviceConfigurationToCloud();
        }
    }
    
    // Clear the updates pending flag if successful
    if (success) {
        sysStatus.set_updatesPending(false);
        sysStatus.flush(true);  // Save the cleared flag
        Log.info("Configuration updates completed and saved");
    }
    
    return success;
}

bool Cloud::isConnectedMode() {
    // Device is in connected mode if not in disconnected mode
    return !sysStatus.get_disconnectedMode();
}

bool Cloud::reloadConfiguration() {
    Log.info("Manually reloading configuration");
    
    // First try device-specific settings, then fall back to defaults
    if (isDeviceConfigured()) {
        return configureFromDeviceSettings();
    } else {
        return configureFromCloudDefaults();
    }
}

bool Cloud::writeDeviceConfigurationToCloud() {
    if (!Particle.connected()) {
        Log.warn("Not connected to cloud, cannot write device configuration");
        return false;
    }

    String configJSON = createConfigurationJSON();
    if (configJSON.length() == 0) {
        Log.warn("Failed to create configuration JSON");
        return false;
    }

    Log.info("Writing device configuration to cloud: %s", configJSON.c_str());
    
    // Set the device-settings ledger with current configuration
    LedgerData data = LedgerData::fromJSON(configJSON);
    if (data.size() > 0) {  // Fix: Use size() to check if data is valid
        deviceSettings.set(data);
        Log.info("Device configuration written to cloud successfully");
        return true;
    } else {
        Log.warn("Failed to parse configuration JSON");
        return false;
    }
}

String Cloud::createConfigurationJSON() {
    // Create JSON from current device configuration using JSONBufferWriter
    char buffer[2048];  // Adjust size as needed
    JSONBufferWriter writer(buffer, sizeof(buffer));
    
    writer.beginObject();
    
    // Messaging configuration
    writer.name("messaging").beginObject();
    writer.name("serial").value(sysStatus.get_serialConnected());
    writer.name("verboseMode").value(sysStatus.get_verboseMode());
    writer.name("disconnectedMode").value(sysStatus.get_disconnectedMode());
    writer.endObject();
    
    // Timing configuration
    writer.name("timing").beginObject();
    writer.name("timezone").value(sysStatus.get_timeZoneStr());
    writer.name("reportingIntervalSec").value(sysStatus.get_reportingInterval());
    writer.name("pollingRateSec").value(sensorConfig.get_pollingRate()); // Already in seconds
    writer.name("openHour").value(sysStatus.get_openTime());
    writer.name("closeHour").value(sysStatus.get_closeTime());
    writer.endObject();
    
    // Power configuration
    writer.name("power").beginObject();
    writer.name("lowPowerMode").value(sysStatus.get_lowPowerMode());
    writer.name("solarPowerMode").value(sysStatus.get_solarPowerMode());
    writer.endObject();
    
    // Sensor configuration
    writer.name("sensor").beginObject();
    writer.name("facethr").value(sensorConfig.get_faceThreshold());
    writer.name("gesturethr").value(sensorConfig.get_gestureThreshold());
    writer.endObject();
    
    writer.endObject();
    
    if (writer.dataSize() > 0) {  // Check if JSON was created successfully
        return String(buffer);
    } else {
        Log.warn("Failed to create JSON configuration");
        return String("");  // Return empty string on failure
    }
}

void Cloud::syncCallback(Ledger ledger) {
    Log.info("syncCallback called for ledger: %s", ledger.name());
    
    // If device-settings ledger was updated, reload configuration
    if (String(ledger.name()) == "device-settings") {
        Log.info("Device settings updated, triggering configuration reload");
        Cloud::instance().configureFromDeviceSettings();
    }
}

bool Cloud::isDeviceConfigured() {
    LedgerData data = deviceSettings.get();
    return data.size() > 0;
}



