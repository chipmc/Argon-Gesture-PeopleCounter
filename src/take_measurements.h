/*
 * @file take_measurements.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This function will take measurements at intervals defined int he sleep_helper_config file.  
 * The libraries and functions needed will depend the spectifics of the device and sensors
 * 
 * @version 0.2
 * @date 2022-12-23
 * 
 */

// Particle functions
#ifndef TAKE_MEASUREMENTS_H
#define TAKE_MEASUREMENTS_H

#include "Particle.h"
#include "GestureFaceSensor.h"
// #include "device_pinout.h"

extern char internalTempStr[16];                       // External as this can be called as a Particle variable
extern char signalStr[64];

#define measure take_measurements::instance()

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * take_measurements::instance().setup();
 * 
 * From global application loop you must call:
 * take_measurements::instance().loop();
 */
class take_measurements {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use take_measurements::instance() to instantiate the singleton.
     */
    static take_measurements &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use take_measurements::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use take_measurements::instance().loop();
     */
    bool loop();

    /**
     * @brief tmp36TemperatureC
     * 
     * @details generic code that convers the analog value of the TMP-36 sensors to degrees c - Assume 3.3V Power
     * 
     * @returns Returns the temperature in C as a float value
     * 
     */
    float tmp36TemperatureC (int adcValue);                // Temperature from the tmp36 - inside the enclosure

    /**
     * @brief In this function, we will measure the battery state of charge and the current functional state
     * 
     * @details One factor that is an issue today is the accurace of the state of charge if the device is waking
     * from sleep.  In order to help with this, there is a test for enable sleep and an additional delay.
     * 
     * @return true  - If the battery has a charge over 60%
     * @return false - Less than 60% indicates a low battery condition
     */
    bool batteryState();                                   // Data on state of charge and battery status. Returns true if SOC over 60%

    /**
     * @brief Checks to see if the temperature is in the range to support charging
     * 
     * @details Will enable or disable charging based on the current temperature
     * 
     * @link https://batteryuniversity.com/learn/article/charging_at_high_and_low_temperatures @endlink
     * 
     */
    bool isItSafeToCharge();                               // See if it is safe to charge based on the temperature

    /**
     * @brief Get the Signal Strength values and make up a string for use in the console
     * 
     * @details Provides data on the signal strength and quality
     * 
     */
    void getSignalStrength();


    /**
     * @brief soft delay let's us process Particle functions and service the sensor interrupts while pausing
     * 
     * @details takes a single unsigned long input in millis
     * 
     */
    void softDelay(uint32_t t);                 		// Soft delay is safer than delay

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use take_measurements::instance() to instantiate the singleton.
     */
    take_measurements();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~take_measurements();

    /**
     * This class is a singleton and cannot be copied
     */
    take_measurements(const take_measurements&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    take_measurements& operator=(const take_measurements&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static take_measurements *_instance;

};

#endif  /* __TAKE_MEASUREMENTS_H */