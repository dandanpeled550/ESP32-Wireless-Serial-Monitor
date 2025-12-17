#ifndef STEPPER_MOVER_H
#define STEPPER_MOVER_H

#include <Arduino.h>

class StepperMover {
 public:
  StepperMover();
  // Initialize pins and set safe initial states
  void begin();
  // Request a (non-blocking) move of the stepper by the given number of steps
  // Positive/negative indicate direction. This returns immediately; call
  // `update()` frequently from your main loop to perform the motion.
  void move(int steps);

  // Must be called frequently from the main loop to step the motor
  void update();

  // Return true while a move is in progress
  bool isMoving() const;

  // Speed parameter in milliseconds (modifiable)
  float speed = 100.0f;

 private:
  // Internal configuration constants (kept private)
  static const uint8_t PULSE_PIN = 32;   // D3
  static const uint8_t ENABLE_PIN = 12;  // D7 (active LOW)
  static const uint8_t DIR_PIN = 26;     // D26
  static const uint8_t PULSE_HIGH_DURATION_US = 5;
};

// Global instance to be used by the rest of the code
extern StepperMover Stepper;

// Compatibility wrapper so existing code that calls `stepperMove()` continues
inline void stepperMove(int steps) { Stepper.move(steps); }

#endif // STEPPER_MOVER_H