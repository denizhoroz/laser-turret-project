// state.h
#pragma once

#include "types.h"
#include "config.h"

extern SystemState   currentState;
extern bool          laserFiring;
extern bool          halted;
extern unsigned long lastTargetSignalMs;

extern Sw swL, swR, swU, swD;
