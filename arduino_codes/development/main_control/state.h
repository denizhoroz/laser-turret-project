// state.h — shared globals for main_control modules.
// One copy lives in state.cpp; every other module sees them via extern.
#pragma once

#include "types.h"
#include "config.h"

extern SystemState   currentState;
extern bool          laserFiring;
extern bool          halted;
extern unsigned long lastTargetSignalMs;   // last Python target signal (offset / is_firing)

extern Sw swL, swR, swU, swD;
