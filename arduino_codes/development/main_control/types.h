// types.h — shared types for main_control.ino
// Must live in a header (not the .ino) to defeat Arduino's auto-prototype
// generator, which inserts function prototypes above any struct defined in
// the .ino body. Including types here puts them above the auto-prototypes.

#pragma once
#include <Arduino.h>

// High-level system state. Drives LED behavior and (later) motor logic.
//   IDLE     — system on, no scan, no target. Green only.
//   SCANNING — actively sweeping for target. Green only.
//   DETECTED — target acquired. Green + yellow.
// Red LED is orthogonal — tracks laser firing flag, not the state.
enum SystemState {
  STATE_IDLE,
  STATE_SCANNING,
  STATE_DETECTED
};

// Debounced limit switch.
// NC->GND wiring with INPUT_PULLUP: stable HIGH = triggered, LOW = open.
struct Sw {
  uint8_t pin;
  bool stable;
  bool reading;
  unsigned long t;
  const char* name;
};
