#include <Arduino.h>
#include "aim.h"
#include "config.h"
#include "state.h"
#include "motor.h"

void aim(bool firingOn) {
  if (halted || PITCH_AIM_OFFSET_STEPS == 0) return;
  long delta = firingOn ? PITCH_AIM_OFFSET_STEPS : -PITCH_AIM_OFFSET_STEPS;
  stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
            delta, &swD, &swU, "pitch");
}
