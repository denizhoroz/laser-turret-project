// types.h — shared types for general_test.ino
// Must live in a header (not the .ino) to defeat Arduino's auto-prototype
// generator, which inserts function prototypes above any struct defined in
// the .ino body. Including types here puts them above the auto-prototypes.

#pragma once
#include <Arduino.h>

// Debounced limit switch.
// NC->GND wiring with INPUT_PULLUP: stable HIGH = triggered, LOW = open.
struct Sw {
  uint8_t pin;
  bool stable;
  bool reading;
  unsigned long t;
  const char* name;
};

// Result of a limit-driven sweep.
enum SweepResult {
  SR_HIT_AXIS,  // expected axis limit tripped
  SR_HIT_PERP,  // perpendicular-axis limit tripped (fault)
  SR_TIMEOUT    // MAX_SWEEP_STEPS reached with nothing tripped (fault)
};

struct Outcome {
  SweepResult result;
  Sw* hit;
  long steps;
};
