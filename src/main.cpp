#include "StepperMover.h"
#include "BluetoothComm.h"

#ifdef CHAIR_MASTER
#include <WirelessMonitor.h>
#endif

unsigned long startTime = 0;

void setup() {
  Serial.begin(115200);
  // give host a moment to attach the serial monitor after reset
  delay(500);
  
#ifdef CHAIR_MASTER
  Serial.println("Master Chair ESP is on");
#elif defined(CHAIR_SLAVE)
  Serial.println("Slave Chair ESP is on");
#else
  Serial.println("ESP is on (default mode)");
#endif

  // record startup time
  startTime = millis();

  // Initialize stepper pins and safe states
  Stepper.begin();

#ifdef CHAIR_MASTER
  // Master: Start wireless monitor (WiFi AP, webserver, websocket)
  wm.setup();
  
  // Initialize Bluetooth as master
  btComm.beginMaster("RoboticChairMaster");
  
  Serial.println("Master Chair setup complete - WiFi AP and Bluetooth ready");
#elif defined(CHAIR_SLAVE)
  // Slave: Only start Bluetooth slave mode
  btComm.beginSlave("RoboticChairSlave");
  
  Serial.println("Slave Chair setup complete - Bluetooth slave ready");
#else
  // Default mode (backward compatibility)
  wm.setup();
  Serial.println("Setup complete");
#endif
}

void loop() {
  // Progress the stepper state machine (non-blocking) - both chairs need this
  Stepper.update();

#ifdef CHAIR_MASTER
  // Master: Keep WirelessMonitor services running
  wm.loop();
  
  // Check Bluetooth connection status
  btComm.checkConnection();
  
#elif defined(CHAIR_SLAVE)
  // Slave: Check for incoming Bluetooth commands
  if (btComm.checkForCommand()) {
    String command = btComm.getLastCommand();
    
    // Parse the pre-processed command
    int steps = 0;
    float stepDelay = 50.0;
    btComm.parseCommand(command, steps, stepDelay);
    
    if (steps != 0) {
      // Apply the pre-processed values directly
      Stepper.speed = stepDelay;
      
      Serial.printf("Slave executing: %d steps at %.1f ms/step\n", steps, stepDelay);
      stepperMove(steps);
    }
  }
  
#else
  // Default mode (backward compatibility)
  wm.loop();
#endif

  // small delay to yield CPU
  delay(10);
}


