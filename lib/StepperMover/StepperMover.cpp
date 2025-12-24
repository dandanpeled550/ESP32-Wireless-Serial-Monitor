#include "StepperMover.h"
// Implementation of StepperMover (non-blocking)

StepperMover::StepperMover() {}

// single global instance
StepperMover Stepper;

// private state (kept in the implementation)
static volatile long remainingSteps = 0;   // steps left to move
static volatile bool moving = false;       // movement in progress
static volatile bool direction = false;    // true/false direction value written to DIR pin
static volatile bool pulsing = false;      // currently in pulse high period
static unsigned long lastStepMillis = 0;   // last step completed time
static unsigned long pulseStartMicros = 0; // start time of current pulse (microseconds)


void StepperMover::begin() {
  pinMode(PULSE_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  // safe defaults
  digitalWrite(ENABLE_PIN, HIGH); // disable motor (active LOW)
  digitalWrite(PULSE_PIN, HIGH);
}

void StepperMover::move(int steps) {
  Serial.println("Stepper move Started");
  if (steps == 0) return;
  
  // Reject new moves while already moving
  if (moving) {
    Serial.println("Stepper busy - ignoring move request");
    return;
  }

  // follow legacy direction mapping: original code used `!(steps > 0)`
  direction = !(steps > 0);
  remainingSteps = abs((long)steps);

  // configure pins and start
  digitalWrite(DIR_PIN, direction);
  digitalWrite(ENABLE_PIN, LOW); // enable motor (active LOW)

  // prepare for first step
  lastStepMillis = millis();
  pulsing = false;
  moving = true;

  Serial.printf("Requested move: %ld steps, dir=%d\n", remainingSteps, direction);
}

void StepperMover::update() {
  if (!moving) return;

  // If currently in the pulse window, check if we've passed the high time
  if (pulsing) {
    if ((unsigned long)(micros() - pulseStartMicros) >= (unsigned long)PULSE_HIGH_DURATION_US) {
      digitalWrite(PULSE_PIN, HIGH);
      pulsing = false;

      // one step completed
        //Serial.println("one step completed");
      if (remainingSteps > 0) {
        remainingSteps--;
      }

      if (remainingSteps == 0) {
        // finished
        moving = false;
        digitalWrite(ENABLE_PIN, HIGH); // disable motor
        //Serial.println("Stepper move complete");
      } else {
        // schedule next step based on speed
        lastStepMillis = millis();
      }
    }
    return;
  }

  // Not currently pulsing: check if it's time for next pulse
  unsigned long now = millis();
  if ((unsigned long)(now - lastStepMillis) >= (unsigned long)speed) {
    // start pulse
    digitalWrite(PULSE_PIN, LOW);
    pulseStartMicros = micros();
    pulsing = true;
  }
}

bool StepperMover::isMoving() const { return moving; }