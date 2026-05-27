#include <Arduino.h>
#include "tracking.h"
#include "config.h"
#include "state.h"
#include "motor.h"
#include "leds.h"

// |px| < deadZone → 0
// else → max(1, |px| / pxPerStep), capped by MAX_STEP_PER_TICK, signed by px.
static long offsetToSteps(int px, int pxPerStep, int deadZone) {
  int mag = px < 0 ? -px : px;
  if (mag < deadZone) return 0;
  long n = mag / pxPerStep;
  if (n < 1) n = 1;
  if (n > MAX_STEP_PER_TICK) n = MAX_STEP_PER_TICK;
  return px > 0 ? n : -n;
}

void applyOffset(int offsetX, int offsetY) {
  if (halted) return;

  // Parallax compensation is now applied Python-side inside Tracker.track().
  // Arduino consumes the biased offset as-is — no further bias here.
  long yawDelta   =  offsetToSteps(offsetX, PX_PER_STEP_YAW,   DEAD_ZONE_PX_YAW);    // x+ → yaw +
  long pitchDelta = -offsetToSteps(offsetY, PX_PER_STEP_PITCH, DEAD_ZONE_PX_PITCH);  // y+ (below) → pitch −

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
