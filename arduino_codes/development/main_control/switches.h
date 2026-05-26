// switches.h — limit-switch debouncing. NC->GND with INPUT_PULLUP.
// HIGH = triggered, LOW = open. Fail-safe on broken wire.
#pragma once

#include "types.h"

void updateSw(Sw &s);            // one switch — call from update loops
void updateAll();                // all four (swL/R/U/D)
bool triggered(const Sw &s);     // current debounced state
void seedSwitches();             // seed debounce state from raw reads (call after pinMode)
