#include <WirelessMonitor.h>
#include "StepperMover.h"

unsigned long startTime = 0;

void setup() {
  Serial.begin(115200);
  // give host a moment to attach the serial monitor after reset
  delay(500);
  Serial.println("ESP is on");

  // record startup time
  startTime = millis();

  // Initialize stepper pins and safe states
  Stepper.begin();

  // Start wireless monitor (opens AP, webserver, websocket)
  wm.setup();

  Serial.println("Setup complete");
}

void loop() {
  // Progress the stepper state machine (non-blocking)
  Stepper.update();

  // Keep WirelessMonitor services running
  wm.loop();

  // small delay to yield CPU
  delay(10);
}

int stepsSpeed = 20;

