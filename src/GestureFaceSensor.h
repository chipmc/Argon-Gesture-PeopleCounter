#ifndef GESTUREFACESENSOR_H
#define GESTUREFACESENSOR_H

#include "Particle.h"
#include "Arduino.h"
#include "Wire.h"
#include "DFRobot_GestureFaceDetection.h"
#include "MyPersistentData.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * DFRobot_GestureFaceDetection::instance().setup();
 * 
 * From global application loop you must call:
 * DFRobot_GestureFaceDetection::instance().loop();
 */
class GestureFaceSensor {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use GestureFaceDetection::instance() to instantiate the singleton.
     */
    static GestureFaceSensor &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use GestureFaceDetection::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use GestureFaceDetection::instance().loop();
     */
    bool loop();

    /** @brief The number of faces detected 
     * 
     */
    bool getFaceData();             // Get the face detection data

    /** @brief The type of gesture detected 
     * 
     */
    bool getGestureData();          // Get the gesture detection data


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use DFRobot_GestureFaceDetection::instance() to instantiate the singleton.
     */
    GestureFaceSensor();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~GestureFaceSensor();

    /**
     * This class is a singleton and cannot be copied
     */
    GestureFaceSensor(const GestureFaceSensor&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    GestureFaceSensor& operator=(const GestureFaceSensor&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static GestureFaceSensor *_instance;

};
#endif  /* __GESTUREFACESENSOR_H */
