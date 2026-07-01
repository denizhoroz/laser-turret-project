// config.h — all tunables for general_test.ino

#pragma once
#include <Arduino.h>

// =============================================================================
// PINS
// =============================================================================
constexpr int PIN_PITCH_STEP  = 12;
constexpr int PIN_PITCH_DIR   = 11;
constexpr int PIN_YAW_STEP    = 10;
constexpr int PIN_YAW_DIR     = 9;
constexpr int PIN_LASER       = 2;

constexpr int PIN_LED_RED     = A0;
constexpr int PIN_LED_YELLOW  = A1;
constexpr int PIN_LED_GREEN   = A2;

constexpr int PIN_LIM_LEFT    = A12;
constexpr int PIN_LIM_RIGHT   = A13;
constexpr int PIN_LIM_UP      = A14;
constexpr int PIN_LIM_DOWN    = A15;

// =============================================================================
// SERIAL
// =============================================================================
constexpr unsigned long BAUD = 115200;

// =============================================================================
// LED
// =============================================================================
constexpr unsigned long LED_ON_MS = 1000;

// =============================================================================
// SWITCHES
// =============================================================================
constexpr unsigned long DEBOUNCE_MS      = 20;
constexpr unsigned long RELEASE_CLEAR_MS = 250;  // continuous LOW required to declare released

// =============================================================================
// MOTORS
// =============================================================================
constexpr unsigned long PITCH_PULSE_US = 2000;
constexpr unsigned long YAW_PULSE_US   = 2000;

constexpr unsigned long DIR_SETUP_MS         = 50;     
constexpr long          MAX_SWEEP_STEPS      = 20000;  

constexpr int           PITCH_SAFETY_MARGIN_STEPS = 3;  // ~5.4° clearance
constexpr int           YAW_SAFETY_MARGIN_STEPS   = 8;  // ~14.4° clearance

constexpr int           RELEASE_CHUNK_STEPS  = 1;      // per-step check during backoff
                                                       // (matches sweep behavior; prevents overshoot on
                                                       // narrow-range axes like pitch ~56 steps)
constexpr unsigned long INTER_PHASE_PAUSE_MS = 500;
