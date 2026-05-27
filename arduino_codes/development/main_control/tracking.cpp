#include <Arduino.h>
#include "tracking.h"
#include "config.h"
#include "state.h"
#include "motor.h"
#include "leds.h"
#include "PID.h"

void applyOffset(int offsetX, int offsetY) {
  if (halted) return;

  // Parallax compensation is applied Python-side inside Tracker.track().
  // PID owns deadzone, integral, derivative, and per-tick saturation.
  long yawDelta   =  pidYaw(offsetX);     // x+ → yaw +
  long pitchDelta = -pidPitch(offsetY);   // y+ (below crosshair) → pitch −

  if (yawDelta != 0) {
    stepDelta(PIN_YAW_STEP, PIN_YAW_DIR, YAW_PULSE_US,
              yawDelta, &swL, &swR, "yaw");
  }
  if (pitchDelta != 0) {
    stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
              pitchDelta, &swD, &swU, "pitch");
  }

  updateLeds();
}
