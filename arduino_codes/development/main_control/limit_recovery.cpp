#include <Arduino.h>
#include "limit_recovery.h"
#include "config.h"
#include "state.h"
#include "switches.h"
#include "motor.h"
#include "leds.h"

void recoverFromLimits() {
  if (halted) return;

  updateAll();

  bool moved = false;

  if (triggered(swR)) {
    stepDelta(PIN_YAW_STEP, PIN_YAW_DIR, YAW_PULSE_US,
              -(long)YAW_SAFETY_MARGIN_STEPS, &swL, &swR, "yaw");
    moved = true;
  } else if (triggered(swL)) {
    stepDelta(PIN_YAW_STEP, PIN_YAW_DIR, YAW_PULSE_US,
              (long)YAW_SAFETY_MARGIN_STEPS, &swL, &swR, "yaw");
    moved = true;
  }

  // Down-limit recovery intentionally disabled
  if (triggered(swU)) {
    stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
              -(long)PITCH_SAFETY_MARGIN_STEPS, &swD, &swU, "pitch");
    moved = true;
  }

  if (moved) updateLeds();
}
