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
  // Pin configuration based on build environment
#ifdef CHAIR_MASTER
  static const uint8_t PULSE_PIN = MASTER_PULSE_PIN;
  static const uint8_t ENABLE_PIN = MASTER_ENABLE_PIN;
  static const uint8_t DIR_PIN = MASTER_DIR_PIN;
#elif defined(CHAIR_SLAVE)
  static const uint8_t PULSE_PIN = SLAVE_PULSE_PIN;
  static const uint8_t ENABLE_PIN = SLAVE_ENABLE_PIN;
  static const uint8_t DIR_PIN = SLAVE_DIR_PIN;
#else
  // Default to master pins for backward compatibility
  static const uint8_t PULSE_PIN = 32;
  static const uint8_t ENABLE_PIN = 12;
  static const uint8_t DIR_PIN = 26;
#endif
  static const uint8_t PULSE_HIGH_DURATION_US = 5;
};

// Global instance to be used by the rest of the code
extern StepperMover Stepper;

// Compatibility wrapper so existing code that calls `stepperMove()` continues
inline void stepperMove(int steps) { Stepper.move(steps); }

#endif // STEPPER_MOVER_H