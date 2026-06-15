// types.h

#pragma once
#include <Arduino.h>

// High-level system state
//   IDLE     — system on
//   SCANNING — actively sweeping for target
//   DETECTED — target acquired
enum SystemState {
  STATE_IDLE,
  STATE_SCANNING,
  STATE_DETECTED
};

// Debounced limit switch.
struct Sw {
  uint8_t pin;
  bool stable;
  bool reading;
  unsigned long t;
  const char* name;
};
