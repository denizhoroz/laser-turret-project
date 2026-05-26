// motor.h — low-level stepper primitives. No position tracking, no clamping;
// the only constraint is the live limit-switch check inside `stepDelta`.
#pragma once

#include "types.h"

void stepOnce(int stepPin, unsigned long pulseUs);

// Execute up to |delta| steps in the direction implied by sign.
//   delta > 0 → DIR HIGH, watches `limHigh`.
//   delta < 0 → DIR LOW,  watches `limLow`.
// Halts the move when the watched limit trips, emits {"event":"limit",...}.
// Returns signed steps actually moved.
long stepDelta(int stepPin, int dirPin, unsigned long pulseUs,
               long delta, Sw* limLow, Sw* limHigh, const char* axisName);
