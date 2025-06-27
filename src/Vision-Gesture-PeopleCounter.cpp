/* 
 * Project Argon-Gesture-PeopleCounter
 * Description: This project implements a people counter using the DFRobot GestureFaceDetection sensor.
 * It detects the number of faces and gestures in front of the sensor and publishes the data to the Particle cloud.
 * The project uses the Particle Device OS APIs and the DFRobot GestureFaceDetection library.
 * Author: Charles McClelland 
 * Date: 5/15/2025
 * License: MIT
 * Repo: https://github.com/chipmc/Argon-Gesture-PeopleCounter
*/

// v1.0 - Initial version - Particle Variable implementation for people count and variable
// v1.1 - Made the code non-blocking 
// v1.2 - Added Particle.publish() to send data to the cloud
// v1.3 - Added serial logging for debugging
// v1.4 - Added "digital twin" settings in the cloud to allow for remote configuration of the sensor
// v1.5 - Implemented some basic archtectureal changes to support the new Particle Device OS APIs and the DFRobot GestureFaceDetection library
// v1.6 - Moved to a state machine approach.  Added support for the AB1805 RTC / Watchdog library to manage the device's sleep and wake cycles
 
// Include Particle Device OS APIs
#include "Particle.h"
#include "device_pinout.h"
#include "Particle_Functions.h"
#include "take_measurements.h"
#include "Cloud.h"
#include "MyPersistentData.h"
#include "LocalTimeRK.h"
#include "AB1805_RK.h"
#include "PublishQueuePosixRK.h"

// Prototype Functions
void publishData();                                 // Publish the data to the cloud
void publishStateTransition(void);                  // Keeps track of state machine changes - for debugging
void userSwitchISR();                               // interrupt service routime for the user switch
void sensorISR();
void countSignalTimerISR();							// Keeps the Blue LED on
void UbidotsHandler(const char *event, const char *data); // Handler for the Ubidots integration response event
void dailyCleanup();								                // Reset each morning
void softDelay(uint32_t t);                        // Soft delay function to allow for non-blocking code    

// Initialize Functions
SystemSleepConfiguration config;                    // Initialize new Sleep 2.0 Api
void outOfMemoryHandler(system_event_t event, int param);
LocalTimeConvert conv;								              // For determining if the park should be opened or closed - need local time
AB1805 ab1805(Wire);                                // Rickkas' RTC / Watchdog library

// System Health Variables
int outOfMemory = -1;                               // From reference code provided in AN0023 (see above)

// State Machine Variables
enum State { INITIALIZATION_STATE, ERROR_STATE, IDLE_STATE, SLEEPING_STATE, CONNECTING_STATE, DISCONNECTING_STATE, REPORTING_STATE, RESP_WAIT_STATE};
char stateNames[8][16] = {"Initialize", "Error", "Idle", "Sleeping", "Connecting", "Disconnecting", "Reporting", "Response Wait"};
State state = INITIALIZATION_STATE;
State oldState = INITIALIZATION_STATE;
 
// Global variables
volatile bool userSwitchDectected = false;		
volatile bool sensorDetect = false;					        // Flag for sensor interrupt
bool dataInFlight = false;                          // Flag for whether we are waiting for a response from the webhook

// Timing variables
const int wakeBoundary = 1*3600 + 0*60 + 0;         // Sets a reporting frequency of 1 hour 0 minutes 0 seconds
const unsigned long stayAwakeLong = 90000;          // In lowPowerMode, how long to stay awake every hour
const unsigned long webhookWait = 30000;            // How long will we wait for a WebHook response
const unsigned long resetWait = 30000;              // How long will we wait in ERROR_STATE until reset
unsigned long stayAwakeTimeStamp = 0;               // Timestamps for our timing variables..
unsigned long stayAwake;                            // Stores the time we need to wait before napping

 
void setup() {

  char responseTopic[125];
	String deviceID = System.deviceID();              // Multiple devices share the same hook - keeps things straight
	deviceID.toCharArray(responseTopic,125);          // Puts the deviceID into the response topic array
	// Particle.subscribe(responseTopic, UbidotsHandler, MY_DEVICES);      // Subscribe to the integration response event
	System.on(out_of_memory, outOfMemoryHandler);     // Enabling an out of memory handler is a good safety tip. If we run out of memory a System.reset() is done.

  Particle_Functions::instance().setup();    // Initialize the Particle functions

  // Remove from production code
	waitFor(Serial.isConnected, 10000);				    // Wait for serial connection 
  // Remove from production code

  initializePinModes();                         // Initialize the pin modes

  sysStatus.setup();								            // Initialize persistent storage
  sensorConfig.setup();							            // Initialize the sensor configuration
	current.setup();                              // Initialize the current status data

  PublishQueuePosix::instance().setup();          // Initialize the Publish Queue
 
  ab1805.withFOUT(D8).setup();                	// Initialize AB1805 RTC   
  ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS);	// Enable watchdog

  // Particle_Functions::instance().connectToCloud(); // Connect to the Particle cloud

  Cloud::instance().setup();                    // Initialize the cloud functions

  take_measurements::instance().setup();        // Initialize the take_measurements functions

	if (!digitalRead(BUTTON_PIN)) {						// The user will press this button at startup to reset settings
		Log.info("User button pressed at startup - setting defaults");
		state = CONNECTING_STATE;
		sysStatus.initialize();                  	// Make sure the device wakes up and connects - reset to defaults and exit low power mode
	}

	if (Time.day(sysStatus.get_lastConnection() != Time.day())) {
		Log.info("New day, resetting counts");
		dailyCleanup();
	}

	if (!Time.isValid()) {
		Log.info("Time is invalid -  %s so connecting", Time.timeStr().c_str());
		state = CONNECTING_STATE;
	}
	else Log.info("Time is valid - %s", Time.timeStr().c_str());

	// Setup local time and set the publishing schedule
	LocalTime::instance().withConfig(LocalTimePosixTimezone("EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"));			// East coast of the US
	conv.withCurrentTime().convert();  	
  
	attachInterrupt(BUTTON_PIN,userSwitchISR,FALLING);						// We may need to monitor the user switch to change behaviours / modes

	if (state == INITIALIZATION_STATE) state = SLEEPING_STATE;               	// Sleep unless otherwise from above code
  	Log.info("Startup complete with last connect %s", Time.format(sysStatus.get_lastConnection(), "%T").c_str());
  	digitalWrite(BLUE_LED,LOW);                                          	// Signal the end of startup

}
 
void loop() {                                       // The main loops runs forever and checks the sensor every "sensorDebounce" milliseconds
	switch (state) {
		case IDLE_STATE: {													// Unlike most sketches - nodes spend most time in sleep and only transit IDLE once or twice each period
			if (state != oldState) publishStateTransition();
			if (sysStatus.get_lowPowerMode() && (millis() - stayAwakeTimeStamp) > stayAwake) state = SLEEPING_STATE;         // When in low power mode, we can nap between taps
			if (Time.hour() != Time.hour(sysStatus.get_lastReport())) state = REPORTING_STATE;                                  // We want to report on the hour but not after bedtime
		} break;

		case SLEEPING_STATE: {
      bool radioOn = false;                                         // Flag to indicate if the radio is on
      #if Wiring_WiFi
        radioOn = WiFi.ready();                                    // Check if the WiFi is ready
      #elif Wiring_Cellular
        radioOn = Cellular.ready();                                 // Check if the Cellular is ready
      #endif

			if (state != oldState) publishStateTransition();              	          // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
 
			if (Particle.connected() || radioOn) {                // If we are connected to the cloud or the radio is on, we will disconnect
				if (!Particle_Functions::instance().disconnectFromParticle()) {         // Disconnect cleanly from Particle and power down the modem
					state = ERROR_STATE;
					// current.alerts = 15;
					break;
				}
			}
			int wakeInSeconds = constrain(wakeBoundary - Time.now() % wakeBoundary, 1, wakeBoundary) + 1;;	// Figure out how long to sleep 		
			config.mode(SystemSleepMode::ULTRA_LOW_POWER)
				.gpio(BUTTON_PIN,CHANGE)
				.duration(wakeInSeconds * 1000L);
			ab1805.stopWDT();  												   // No watchdogs interrupting our slumber
			SystemSleepResult result = System.sleep(config);              	// Put the device to sleep device continues operations from here
			ab1805.resumeWDT();                                                // Wakey Wakey - WDT can resume
			if (result.wakeupPin() == BUTTON_PIN) {                         // If the user woke the device we need to get up - device was sleeping so we need to reset opening hours
				Log.info("Woke with user button - Resetting hours and going to connect");
				sysStatus.set_lowPowerMode(false);
				// sysStatus.set_closeTime(24);								// Not sure we want to reset the open / close time
				// sysStatus.set_openTime(0);
				userSwitchDectected = false;
				stayAwake = stayAwakeLong;
				stayAwakeTimeStamp = millis();
				state = REPORTING_STATE;
			}
			else {															// In this state the device was awoken for hourly reporting
				softDelay(2000);											// Gives the device a couple seconds to get the battery reading
				Log.info("Time is up at %s with %li free memory", Time.format((Time.now()+wakeInSeconds), "%T").c_str(), System.freeMemory());
				state = IDLE_STATE;
			}
		} break;

		case REPORTING_STATE: {
			if (state != oldState) publishStateTransition();
			sysStatus.set_lastReport(Time.now());                              // We are only going to report once each hour from the IDLE state.  We may or may not connect to Particle
			measure.loop();                                                // Take Measurements here for reporting
			if (Time.hour() == sysStatus.get_openTime()) dailyCleanup();       // Once a day, clean house and publish to Google Sheets
			publishData();                        // Publish hourly but not at opening time as there is nothing to publish
			state = CONNECTING_STATE;                                          // Default behaviour would be to connect and send report to Ubidots

			// Let's see if we need to connect 
			if (Particle.connected()) {                                        // We are already connected go to response wait
				stayAwake = stayAwakeLong;                                      // Keeps device awake after reboot - helps with recovery
				stayAwakeTimeStamp = millis();
				state = RESP_WAIT_STATE;
			}
			// If we are in a low battery state - we are not going to connect unless we are over-riding with user switch (active low)
			else if (sysStatus.get_lowBatteryMode() && digitalRead(BUTTON_PIN)) {
				Log.info("Not connecting - low battery mode");
				state = IDLE_STATE;
			}
			// If we are in low power mode, we may bail if battery is too low and we need to reduce reporting frequency
			else if (sysStatus.get_lowPowerMode() && digitalRead(BUTTON_PIN)) {      // Low power mode and user switch not pressed
				if (current.get_stateOfCharge() > 65) {
					Log.info("Sufficient battery power connecting");
				}
				else if (current.get_stateOfCharge() <= 50 && (Time.hour() % 4)) {   // If the battery level is <50%, only connect every fourth hour
					Log.info("Not connecting - <50%% charge - four hour schedule");
					state = IDLE_STATE;                                            // Will send us to connecting state - and it will send us back here
				}                                                                // Leave this state and go connect - will return only if we are successful in connecting
				else if (current.get_stateOfCharge() <= 65 && (Time.hour() % 2)) {   // If the battery level is 50% -  65%, only connect every other hour
					Log.info("Not connecting - 50-65%% charge - two hour schedule");
					state = IDLE_STATE;                                            // Will send us to connecting state - and it will send us back here
					break;                                                         // Leave this state and go connect - will return only if we are successful in connecting
				}
			}
		} break;

  		case RESP_WAIT_STATE: {
    		static unsigned long webhookTimeStamp = 0;                         // Webhook time stamp
    		if (state != oldState) {
      			webhookTimeStamp = millis();                                     // We are connected and we have published, head to the response wait state
      			dataInFlight = true;                                             // set the data inflight flag
      			publishStateTransition();
    		}

    		if (!dataInFlight)  {                                              // Response received --> back to IDLE state
				stayAwake = stayAwakeLong;
				stayAwakeTimeStamp = millis();
				// sysStatus.set_lowPowerMode(true);
      			state = IDLE_STATE;
    		}
    		else if (millis() - webhookTimeStamp > webhookWait) {              // If it takes too long - will need to reset
				Log.info("Webhook timeout - resetting");
      			state = ERROR_STATE;                                             // Go to the ERROR state to decide our fate
    		}
  		} break;

  		case CONNECTING_STATE:{                                              // Will connect - or not and head back to the Idle state - We are using a 3,5, 7 minute back-off approach as recommended by Particle
			static State retainedOldState;                                     // Keep track for where to go next (depends on whether we were called from Reporting)
			static unsigned long connectionStartTimeStamp;                     // Time in Millis that helps us know how long it took to connect
			char data[64];                                                     // Holder for message strings

			if (state != oldState) {                                           // Non-blocking function - these are first time items
			retainedOldState = oldState;                                     // Keep track for where to go next
			sysStatus.set_lastConnectionDuration(0);                            // Will exit with 0 if we do not connect or are already connected.  If we need to connect, this will record connection time.
			publishStateTransition();
			connectionStartTimeStamp = millis();                             // Have to use millis as the clock may get reset on connect
			Particle.connect();                                              // Tells Particle to connect, now we need to wait
			}

			sysStatus.set_lastConnectionDuration(int((millis() - connectionStartTimeStamp)/1000));

			if (Particle.connected()) {
				sysStatus.set_lastConnection(Time.now());                           // This is the last time we last connected
				stayAwake = stayAwakeLong;                                       // Keeps device awake after reconnection - allows us some time to catch the device before it sleeps
				stayAwakeTimeStamp = millis();                                   // Start the stay awake timer now
				measure.getSignalStrength();                                             // Test signal strength since the cellular modem is on and ready
				snprintf(data, sizeof(data),"Connected in %i secs",sysStatus.get_lastConnectionDuration());  // Make up connection string and publish
				Log.info(data);
				if (sysStatus.get_verboseMode()) Particle.publish("Cellular",data,PRIVATE);
				(retainedOldState == REPORTING_STATE) ? state = RESP_WAIT_STATE : state = IDLE_STATE; // so, if we are connecting to report - next step is response wait - otherwise IDLE
			}
			else if (sysStatus.get_lastConnectionDuration() > 600) { // What happens if we do not connect
				state = ERROR_STATE;                                             // Note - not setting the ERROR timestamp to make this go quickly
			}
			else {} // We go round the main loop again
		} break;

		case ERROR_STATE: {													// Where we go if things are not quite right
			if (state != oldState) {
				publishStateTransition();                // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
				Log.info("Error state - resetting");
			}
			static unsigned long resetTimer = millis();
			if (millis() - resetTimer > resetWait) System.reset();

		} break;
	}

	ab1805.loop();                                  					// Keeps the RTC synchronized with the Boron's clock

	// Housekeeping for each transit of the main loop
	current.loop();
	sysStatus.loop();

	PublishQueuePosix::instance().loop();                                // Check to see if we need to tend to the message queue

	if (outOfMemory >= 0) {                         // In this function we are going to reset the system if there is an out of memory error
		Log.info("Resetting due to low memory");
		state = ERROR_STATE;
  	}

	if (userSwitchDectected) {                      // If the user switch is pressed - we will force a connection
		Log.info("User switch pressed - sending data");
		userSwitchDectected = false;
		state = REPORTING_STATE;
	}
} // End of loop

void publishData() {
  char str[100];
  sprintf(str, "{\"gesturetype\": %d, \"gesturescore\": %d, \"facenumber\": %d, \"facescore\": %d}", current.get_gestureType(), current.get_gestureScore(), current.get_faceNumber(), current.get_faceScore());
  if (Particle.connected()) Particle.publish("ubidots-data", str, PRIVATE);
}

void UbidotsHandler(const char *event, const char *data) {            // Looks at the response from Ubidots - Will reset Photon if no successful response
  char responseString[64];
    // Response is only a single number thanks to Template
  if (!strlen(data)) {                                                // No data in response - Error
    snprintf(responseString, sizeof(responseString),"No Data");
  }
  else if (atoi(data) == 200 || atoi(data) == 201) {
    snprintf(responseString, sizeof(responseString),"Response Received");
	dataInFlight = false;											 // We have received a response - so we can send another
    sysStatus.set_lastHookResponse(Time.now());                          // Record the last successful Webhook Response
  }
  else {
    snprintf(responseString, sizeof(responseString), "Unknown response recevied %i",atoi(data));
  }
  if (sysStatus.get_verboseMode() && Particle.connected()) {
    Particle.publish("Ubidots Hook", responseString, PRIVATE);
  }
  Log.info(responseString);
}

/**
 * @brief Publishes a state transition to the Log Handler and to the Particle monitoring system.
 *
 * @details A good debugging tool.
 */
void publishStateTransition(void)
{
	char stateTransitionString[256];
	if (state == IDLE_STATE) {
		if (!Time.isValid()) snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s with invalid time", stateNames[oldState],stateNames[state]);
		else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	}
	else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	oldState = state;
	Log.info(stateTransitionString);
}

// Here are the various hardware and timer interrupt service routines
void outOfMemoryHandler(system_event_t event, int param) {
    outOfMemory = param;
}

void userSwitchISR() {
  	userSwitchDectected = true;                                          	// The the flag for the user switch interrupt
}

void sensorISR() {
  static bool frontTireFlag = false;
  if (frontTireFlag || sysStatus.get_sensorType() == 1) {               	// Counts the rear tire for pressure sensors and once for PIR
    sensorDetect = true;                                              		// sets the sensor flag for the main loop
    frontTireFlag = false;
  }
  else frontTireFlag = true;
}

void countSignalTimerISR() {
  digitalWrite(BLUE_LED,LOW);
}

bool isParkOpen() {
	if (Time.hour() < sysStatus.get_openTime() || Time.hour() > sysStatus.get_closeTime()) return false;
	else return true;
}

/**
 * @brief Cleanup function that is run at the beginning of the day.
 *
 * @details May or may not be in connected state.  Syncs time with remote service and sets low power mode.
 * Called from Reporting State ONLY. Cleans house at the beginning of a new day.
 */
void dailyCleanup() {
  if (Particle.connected()) Particle.publish("Daily Cleanup","Running", PRIVATE);   // Make sure this is being run
  Log.info("Running Daily Cleanup");
  sysStatus.set_verboseMode(false);                                       // Saves bandwidth - keep extra chatter off
  if (sysStatus.get_solarPowerMode() || current.get_stateOfCharge() <= 65) {     // If Solar or if the battery is being discharged
    // setLowPowerMode("1");
  }
  current.resetEverything();                                                   // If so, we need to Zero the counts for the new day
}

/**
 * @brief soft delay let's us process Particle functions and service the sensor interrupts while pausing
 * 
 * @details takes a single unsigned long input in millis
 * 
 */
inline void softDelay(uint32_t t) {
  for (uint32_t ms = millis(); millis() - ms < t; Particle.process());  //  safer than a delay()
}
