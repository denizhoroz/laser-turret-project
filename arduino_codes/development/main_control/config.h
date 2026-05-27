// config.h — all tunables for main_control.ino
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

constexpr unsigned long DIR_SETUP_MS         = 1;      // DRV8825 DIR setup (datasheet ~650 ns; 1 ms is plenty)
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

// Serial line buffer for incoming JSON lines. MEGA has 8KB SRAM — 256 is safe.
constexpr int  SERIAL_LINE_BUF   = 256;

// =============================================================================
// VISION TRACKING (Python -> Arduino)
//
// Python sends pixel-offset of target from crosshair via
//   {"type":"data","key":"current_target_offset","value":[x,y]}
//
// Conversion model: integer divisor + deadzone + min-1-step floor.
//   if |offset| < DEAD_ZONE_PX: zero steps (anti-jitter).
//   else: steps = max(1, |offset| / PX_PER_STEP_*), capped by MAX_STEP_PER_TICK.
//
// This guarantees small offsets still produce motion (vs. a float-gain model
// that rounds sub-1.0 errors to zero and stalls tracking).
//
// Direction signs:
//   image x+ = right of crosshair → yaw should increase (move RIGHT).
//   image y+ = below crosshair    → pitch should DECREASE (UP is +) → inverted in handler.
//
// Tune per-axis: lower divisor = more aggressive (more steps per px).
// =============================================================================
// Per-axis deadzone. Must satisfy DEAD_ZONE_PX_* >= PX_PER_STEP_* / 2 + 1
// to prevent single-step overshoot from causing limit-cycle oscillation.
constexpr int  DEAD_ZONE_PX_YAW    = 8;
constexpr int  DEAD_ZONE_PX_PITCH  = 10;
constexpr int  PX_PER_STEP_YAW     = 12;
constexpr int  PX_PER_STEP_PITCH   = 12;
constexpr long MAX_STEP_PER_TICK   = 20;

// Auto-state for yellow LED. If no offset/fire message arrives for this long,
// yellow turns off (system likely back to scanning/idle).
constexpr unsigned long TARGET_SIGNAL_STALE_MS = 1500;

// =============================================================================
// PARALLAX — handled Python-side.
// `tracker.py` now shifts the virtual aim point inside `Tracker.track()` so
// both the commanded offset and the in_roi check use the same biased target.
// Arduino is a dumb motor controller; it acts on whatever offset Python sends.
// Tune `PARALLAX_BIAS_Y_PX` in `jetson_mission_codes/src/packages/config.py`.
// =============================================================================
