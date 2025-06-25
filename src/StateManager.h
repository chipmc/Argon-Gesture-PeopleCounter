#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "Particle.h"

enum class SystemState {
    INITIALIZING,
    CONNECTING,
    RUNNING,
    ERROR,
    SLEEP
};

enum class ErrorCode {
    NONE,
    SENSOR_INIT_FAILED,
    CLOUD_CONNECTION_FAILED,
    SENSOR_READ_ERROR,
    MEMORY_ERROR
};

class StateManager {
public:
    static StateManager &instance();
    
    SystemState getCurrentState() const { return currentState; }
    ErrorCode getLastError() const { return lastError; }
    
    void setState(SystemState state);
    void setError(ErrorCode error);
    void clearError();
    
    bool isHealthy() const;
    void publishStateToCloud();
    
private:
    StateManager() = default;
    static StateManager* _instance;
    
    SystemState currentState = SystemState::INITIALIZING;
    ErrorCode lastError = ErrorCode::NONE;
    unsigned long stateChangeTime = 0;
};

#endif
