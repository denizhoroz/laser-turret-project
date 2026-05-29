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

// Last commanded direction per axis (-1/0/+1). Set by motor.stepDelta when a
// step actually occurs. Used by serial_link.sendTelemetry; decays to 0 after
// inactivity so the UI shows idle.
extern int8_t        lastYawDir;
extern int8_t        lastPitchDir;
extern unsigned long lastYawStepMs;
extern unsigned long lastPitchStepMs;
