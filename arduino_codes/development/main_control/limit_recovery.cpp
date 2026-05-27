#include <Arduino.h>
#include "limit_recovery.h"
#include "config.h"
#include "state.h"
#include "switches.h"
#include "motor.h"
#include "leds.h"

// Direction convention (mirrors tracking.cpp):
//   yaw delta > 0 → toward swR.   yaw delta < 0 → toward swL.
//   pitch delta > 0 → toward swU. pitch delta < 0 → toward swD.
//
// So to escape a triggered switch, drive delta away from it by the
// per-axis safety margin. stepDelta's internal limit check uses the
// OPPOSITE switch as the watch, which won't fire during a short backoff.
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

  if (triggered(swU)) {
    stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
              -(long)PITCH_SAFETY_MARGIN_STEPS, &swD, &swU, "pitch");
    moved = true;
  } else if (triggered(swD)) {
    stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
              (long)PITCH_SAFETY_MARGIN_STEPS, &swD, &swU, "pitch");
    moved = true;
  }

  if (moved) updateLeds();
}
