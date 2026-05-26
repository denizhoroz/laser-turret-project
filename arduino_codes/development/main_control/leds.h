// leds.h — green/yellow/red indicator logic.
//   Green   — ON whenever system is powered.
//   Yellow  — ON in DETECTED state OR fresh target signal OR laser firing.
//   Red     — ON while laser is firing.
// `updateLeds()` is the single source of truth — call it after any state /
// laser / target-signal change.
#pragma once

#include "types.h"

void updateLeds();
const char* stateName(SystemState s);
