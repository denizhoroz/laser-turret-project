// config.h — all tunables for general_test.ino
// Edit values here, recompile. Sketch contains zero literal pin/timing constants.
//
// Pin map is authoritative in .schematic/components.md.
// Project state + open issues in .schematic/status.md.

#pragma once
#include <Arduino.h>

// =============================================================================
// PINS (from .schematic/components.md)
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
// LASER
// =============================================================================
constexpr int           LASER_BLINKS = 3;
constexpr unsigned long LASER_MS     = 500;

// =============================================================================
// SWITCHES
// =============================================================================
constexpr unsigned long DEBOUNCE_MS      = 20;
constexpr unsigned long RELEASE_CLEAR_MS = 250;  // continuous LOW required to declare released

// =============================================================================
// MOTORS
//
// Pulse half-period (HIGH and LOW phase each). Larger = slower step rate =
// motor in higher-torque region of its curve. Software-only torque lever.
// Real torque comes from DRV8825 Vref pot + microstep jumpers + supply voltage
// (see .schematic/status.md "Pending").
// =============================================================================
constexpr unsigned long PITCH_PULSE_US = 2000;
constexpr unsigned long YAW_PULSE_US   = 2000;

constexpr unsigned long DIR_SETUP_MS         = 50;     // DRV8825 DIR setup
constexpr long          MAX_SWEEP_STEPS      = 20000;  // safety cap per sweep

// Back-off after limit release. Must stay SMALL relative to axis travel.
// Axis travel in full-step mode: pitch ~56 steps (~100°), yaw 150 steps (270°).
// Old shared value of 100 steps overshot pitch entirely. Per-axis now.
constexpr int           PITCH_SAFETY_MARGIN_STEPS = 3;  // ~5.4° clearance
constexpr int           YAW_SAFETY_MARGIN_STEPS   = 8;  // ~14.4° clearance

constexpr int           RELEASE_CHUNK_STEPS  = 1;      // per-step check during backoff
                                                       // (matches sweep behavior; prevents overshoot on
                                                       // narrow-range axes like pitch ~56 steps)
constexpr unsigned long INTER_PHASE_PAUSE_MS = 500;
