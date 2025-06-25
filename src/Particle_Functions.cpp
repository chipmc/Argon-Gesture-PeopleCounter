#include "Particle.h"
#include "device_pinout.h"
#include "take_measurements.h"
#include "Particle_Functions.h"
#include "config.h"

// Prototypes and System Mode calls
SYSTEM_MODE(SEMI_AUTOMATIC);                        // This will enable user code to start executing automatically.
STARTUP(System.enableFeature(FEATURE_RESET_INFO));

#define SERIAL_LOG_LEVEL 3                            // Set the logging level for the serial log handler
// Temporary - will fix with config file later

#if SERIAL_LOG_LEVEL == 0
  SerialLogHandler logHandler(LOG_LEVEL_NONE);     // Easier to see the program flow
#elif SERIAL_LOG_LEVEL == 1
  SerialLogHandler logHandler(LOG_LEVEL_ERROR);
#elif SERIAL_LOG_LEVEL == 2
  SerialLogHandler logHandler(LOG_LEVEL_WARN);
#elif SERIAL_LOG_LEVEL == 3
SerialLogHandler logHandler(LOG_LEVEL_INFO, { // Logging level for non-application messages
	{ "app.pubq", LOG_LEVEL_ERROR},
	{ "app.seqfile", LOG_LEVEL_ERROR},
  { "comm.dtls", LOG_LEVEL_ERROR},
  { "hal", LOG_LEVEL_ERROR},
  { "comm.protocol", LOG_LEVEL_ERROR},
  { "ncp.rltk.client", LOG_LEVEL_ERROR},
  { "app.device", LOG_LEVEL_ERROR},
  { "system", LOG_LEVEL_ERROR},
  { "app.system.sleep", LOG_LEVEL_ERROR},
  { "app.system.reset", LOG_LEVEL_ERROR},
  { "app.ab1805", LOG_LEVEL_ERROR}
});
#elif SERIAL_LOG_LEVEL == 4
  SerialLogHandler logHandler(LOG_LEVEL_ALL);
#endif


Particle_Functions *Particle_Functions::_instance;

// [static]
Particle_Functions &Particle_Functions::instance() {
  if (!_instance) {
    _instance = new Particle_Functions();
  }
  return *_instance;
}

Particle_Functions::Particle_Functions() {
}

Particle_Functions::~Particle_Functions() {
}

void Particle_Functions::setup() {
  Log.info("Initializing Particle functions and variables");     // Note: Don't have to be connected but these functions need to in first 30 seconds
  // Define the Particle variables and functions
  
}


void Particle_Functions::loop() {
  // Put your code to run during the application thread loop here
}

void Particle_Functions::connectToCloud() {
  Log.info("Connecting to the Particle cloud");
  
  // Connect to the Particle cloud
  if (Particle.connected()) {
    Log.info("Already connected to the Particle cloud");
  } else {
    Particle.connect();
    Log.info("Connecting to the Particle cloud...");
  }
  while (!Particle.connected()) {
    Particle.publish("Status","Waiting for cloud connection",PRIVATE);
    Log.info("Waiting for cloud connection...");
    delay(1000);
  }
  Particle.publish("Status","Cloud connected",PRIVATE);
  Log.info("Cloud connected");
}

// This is the end of the Particle_Functions class