// src/GestureFaceSensor.h - Updated to implement ISensor
#ifndef GESTUREFACESENSOR_H
#define GESTUREFACESENSOR_H

#include "ISensor.h"
#include "DFRobot_GestureFaceDetection.h"
#include "MyPersistentData.h"
#include "Wire.h"

/**
 * @brief Concrete implementation of ISensor for DFRobot gesture/face detection
 * 
 * This class wraps the DFRobot GestureFaceDetection library and provides
 * a standardized interface for the sensor manager.
 */
class GestureFaceSensor : public ISensor {
public:
    /**
     * @brief Gets the singleton instance of this class
     */
    static GestureFaceSensor &instance();
    
    // ISensor interface implementation
    bool setup() override;
    bool loop() override;
    SensorData getData() const override;
    String getSensorType() const override { return "GestureFace"; }
    bool isReady() const override { return _initialized; }
    void reset() override;
    
protected:
    GestureFaceSensor();
    virtual ~GestureFaceSensor();
    GestureFaceSensor(const GestureFaceSensor &) = delete;
    GestureFaceSensor &operator=(const GestureFaceSensor &) = delete;
    
    static GestureFaceSensor *_instance;
    bool _initialized;
    SensorData _lastData;
    DFRobot_GestureFaceDetection_I2C* _gfd;
    
private:
    bool getFaceData();
    bool getGestureData();
};

#endif /* GESTUREFACESENSOR_H */