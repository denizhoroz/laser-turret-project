// main_control.ino — turret production firmware (thin entry).
//
// Modules in this sketch directory:
//   config.h        — all constants (pins, timings, gains)
//   types.h         — Sw struct, SystemState enum
//   state.h/.cpp    — shared globals (currentState, laserFiring, halted, lastTargetSignalMs, swL/R/U/D)
//   switches.h/.cpp — limit-switch debounce
//   leds.h/.cpp     — green/yellow/red indicator logic
//   motor.h/.cpp    — stepOnce + stepDelta primitive (limit-watching)
//   laser.h/.cpp    — setFiring (PIN_LASER + LED coordinator)
//   tracking.h/.cpp — applyOffset (pixel → step deltas). Parallax handled Python-side.
//   PID.h/.cpp      — per-axis PID controller (offset px → step delta).
//   scanning.h/.cpp — autonomous search sweep while STATE_SCANNING.
//   limit_recovery.h/.cpp — autonomous backoff from triggered limit switches.
//   commands.h/.cpp — bench {"cmd":"..."} handlers
//   serial_link.h/.cpp — outbound JSON (events, status, acks)
//   dispatch.h/.cpp — inbound line buffer + JSON parse + route
//
// Pin map: .schematic/components.md   ·   Project state: .schematic/status.md
//
// Motion model: pure step deltas. No position tracker, no homing, no software
// range clamp — the only motion constraint is the live limit-switch check
// inside stepDelta. Reason: mechanical stress can desync absolute tracker
// from real motor position.
//
// JSON link (line-delimited JSON over USB Serial, both directions):
//   Jetson -> Arduino (Python production schema):
//     {"type":"data","key":"current_target_offset","value":[x,y]}
//     {"type":"data","key":"is_firing","value":true|false}
//   Bench / manual schema (acks back):
//     {"cmd":"move","yaw":N,"pitch":M}
//     {"cmd":"stop"} / {"cmd":"resume"}
//     {"cmd":"state","value":"idle|scanning|detected"}
//     {"cmd":"laser","on":true|false}
//     {"cmd":"status"} / {"cmd":"ping"}
//   Manual single-char bench cmds:  H - help · ? - status

#include "config.h"
#include "state.h"
#include "switches.h"
#include "leds.h"
#include "dispatch.h"
#include "limit_recovery.h"
#include "scanning.h"
#include "PID.h"
#include "serial_link.h"   // sendTelemetry

static const unsigned long TELEMETRY_INTERVAL_MS = 200;

void setup() {
  Serial.begin(BAUD);

  pinMode(PIN_LED_GREEN,  OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED,    OUTPUT);

  pinMode(PIN_PITCH_STEP, OUTPUT);
  pinMode(PIN_PITCH_DIR,  OUTPUT);
  pinMode(PIN_YAW_STEP,   OUTPUT);
  pinMode(PIN_YAW_DIR,    OUTPUT);
  pinMode(PIN_LASER,      OUTPUT);
  digitalWrite(PIN_PITCH_STEP, LOW);
  digitalWrite(PIN_YAW_STEP,   LOW);
  digitalWrite(PIN_LASER,      LOW);

  pinMode(PIN_LIM_LEFT,  INPUT_PULLUP);
  pinMode(PIN_LIM_RIGHT, INPUT_PULLUP);
  pinMode(PIN_LIM_UP,    INPUT_PULLUP);
  pinMode(PIN_LIM_DOWN,  INPUT_PULLUP);

  updateLeds();
  delay(100);
  seedSwitches();
  pidReset();

  Serial.println();
  Serial.println(F("# main_control ready (delta-only mode, no homing)."));
}

void loop() {
  while (Serial.available()) {
    feedSerialChar(Serial.read());
  }

  // While the Jetson reports scanning, run the autonomous search sweep.
  // Otherwise consume the freshest target offset (tracking). The two are
  // mutually exclusive — Python sends no offsets while scanning.
  if (currentState == STATE_SCANNING && !halted) {
    scanTick();
  } else {
    flushPendingOffset();
  }
  recoverFromLimits();

  // Refresh LEDs periodically so yellow drops when the target signal goes
  // stale (no recent offset/fire message from Python).
  static unsigned long lastLedTick = 0;
  unsigned long now = millis();
  if (now - lastLedTick > 100) {
    lastLedTick = now;
    updateLeds();
  }

  // Periodic telemetry to Jetson (forwarded to ground station UI).
  static unsigned long lastTelemTick = 0;
  if (now - lastTelemTick >= TELEMETRY_INTERVAL_MS) {
    lastTelemTick = now;
    sendTelemetry();
  }
}
