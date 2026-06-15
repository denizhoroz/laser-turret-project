// motor.h
#pragma once

#include "types.h"

void stepOnce(int stepPin, unsigned long pulseUs);

// Execute up to delta steps
long stepDelta(int stepPin, int dirPin, unsigned long pulseUs,
               long delta, Sw* limLow, Sw* limHigh, const char* axisName);
