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
