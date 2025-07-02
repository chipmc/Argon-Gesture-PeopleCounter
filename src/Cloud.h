#ifndef __CLOUD_H
#define __CLOUD_H

#include "Particle.h"
#include "device_pinout.h"
#include "MyPersistentData.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * Cloud::instance().setup();
 * 
 * From global application loop you must call:
 * Cloud::instance().loop();
 */
class Cloud {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Cloud::instance() to instantiate the singleton.
     */
    static Cloud &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use Cloud::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use Cloud::instance().loop();
     */
    void loop();

    /**
     * @brief Configure device from cloud defaults using Particle Ledger
     * 
     * Retrieves configuration from the "default-settings" ledger and applies
     * it to the persistent storage structures (sysStatus and sensorConfig)
     * 
     * @return true if configuration was successfully applied
     */
    bool configureFromCloudDefaults();

    /**
     * @brief Manually trigger configuration reload from cloud
     * 
     * This can be called from a Particle function to force reload configuration
     * 
     * @return true if successful
     */
    bool reloadConfiguration();

private:
    /**
     * @brief Apply messaging configuration from ledger data
     * 
     * @param data LedgerData containing messaging config
     * @return true if successful
     */
    bool applyMessagingConfig(const LedgerData& data);

    /**
     * @brief Apply timing configuration from ledger data
     * 
     * @param data LedgerData containing timing config
     * @return true if successful
     */
    bool applyTimingConfig(const LedgerData& data);

    /**
     * @brief Apply power configuration from ledger data
     * 
     * @param data LedgerData containing power config
     * @return true if successful
     */
    bool applyPowerConfig(const LedgerData& data);

    /**
     * @brief Apply sensor configuration from ledger data
     * 
     * @param data LedgerData containing sensor config
     * @return true if successful
     */
    bool applySensorConfig(const LedgerData& data);

    /**
     * @brief Validate configuration value is within acceptable range
     * 
     * @param value Value to validate
     * @param min Minimum acceptable value
     * @param max Maximum acceptable value
     * @param name Parameter name for logging
     * @return true if valid
     */
    template<typename T>
    bool validateRange(T value, T min, T max, const String& name);


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use Cloud::instance() to instantiate the singleton.
     */
    Cloud();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~Cloud();

    /**
     * This class is a singleton and cannot be copied
     */
    Cloud(const Cloud&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Cloud& operator=(const Cloud&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static Cloud *_instance;

};
#endif  /* __CLOUD_H */
