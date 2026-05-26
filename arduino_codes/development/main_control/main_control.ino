// main_control.ino — turret production firmware.
// Layers built so far: LEDs · limit switches · motors · homing · JSON link to Jetson.
//
// LED rules (from spec):
//   Green   — ON whenever system is powered.
//   Yellow  — ON in DETECTED state. OFF in IDLE / SCANNING.
//   Red     — ON while laser is firing. OFF otherwise.
//
// JSON link (line-delimited JSON over USB Serial, both directions):
//   Jetson -> Arduino:
//     {"cmd":"move","yaw":N,"pitch":M}      // signed step deltas
//     {"cmd":"home"}                        // re-run homing routine
//     {"cmd":"stop"}                        // raise halt flag (refuses moves until cleared)
//     {"cmd":"resume"}                      // clear halt flag (after operator review)
//     {"cmd":"state","value":"idle|scanning|detected"}
//     {"cmd":"laser","on":true|false}       // sets flag + red LED (PIN_LASER not driven yet)
//     {"cmd":"status"}                      // request snapshot
//     {"cmd":"ping"}                        // health check
//   Arduino -> Jetson:
//     {"ack":"<cmd>", "ok":bool, ...payload}
//     {"event":"limit","axis":"yaw|pitch","side":"left|right|up|down"}
//     {"event":"homed","yawPos":0,"pitchPos":0}
//     {"event":"halted","reason":"..."}
//     {"telemetry":"pos","yawPos":N,"pitchPos":M}
//
// Manual single-char cmds (for bench testing without Jetson):
//   H - help · ? - status
//
// Pin map: .schematic/components.md   ·   Project state: .schematic/status.md

#include <ArduinoJson.h>
#include "config.h"
#include "types.h"

// =============================================================================
// STATE
// =============================================================================
SystemState currentState = STATE_IDLE;
bool laserFiring = false;
bool homed = false;
bool halted = false;

long yawPos   = 0;  // valid only after homed == true
long pitchPos = 0;

Sw swL = { PIN_LIM_LEFT,  false, false, 0, "left"  };
Sw swR = { PIN_LIM_RIGHT, false, false, 0, "right" };
Sw swU = { PIN_LIM_UP,    false, false, 0, "up"    };
Sw swD = { PIN_LIM_DOWN,  false, false, 0, "down"  };

// Serial line buffer
char  rxBuf[SERIAL_LINE_BUF];
int   rxLen = 0;

// =============================================================================
// SWITCHES — debounced reads (HIGH = triggered per INPUT_PULLUP, NC->GND)
// =============================================================================
void updateSw(Sw &s) {
  bool r = digitalRead(s.pin);
  unsigned long now = millis();
  if (r != s.reading) { s.reading = r; s.t = now; }
  if (now - s.t >= DEBOUNCE_MS && r != s.stable) s.stable = r;
}
void updateAll() { updateSw(swL); updateSw(swR); updateSw(swU); updateSw(swD); }
bool triggered(const Sw &s) { return s.stable == HIGH; }
void seedSwitches() {
  swL.stable = swL.reading = digitalRead(swL.pin);
  swR.stable = swR.reading = digitalRead(swR.pin);
  swU.stable = swU.reading = digitalRead(swU.pin);
  swD.stable = swD.reading = digitalRead(swD.pin);
}

// =============================================================================
// LED LAYER
// =============================================================================
void updateLeds() {
  digitalWrite(PIN_LED_GREEN,  HIGH);
  digitalWrite(PIN_LED_YELLOW, currentState == STATE_DETECTED ? HIGH : LOW);
  digitalWrite(PIN_LED_RED,    laserFiring ? HIGH : LOW);
}

const char* stateName(SystemState s) {
  switch (s) {
    case STATE_IDLE:     return "idle";
    case STATE_SCANNING: return "scanning";
    case STATE_DETECTED: return "detected";
  }
  return "?";
}

// =============================================================================
// OUTBOUND JSON — events, acks, telemetry
// =============================================================================
void sendDoc(JsonDocument &doc) {
  serializeJson(doc, Serial);
  Serial.println();
}

void sendEventLimit(const char* axis, const char* side) {
  JsonDocument d;
  d["event"] = "limit";
  d["axis"]  = axis;
  d["side"]  = side;
  sendDoc(d);
}

void sendEventHomed() {
  JsonDocument d;
  d["event"]    = "homed";
  d["yawPos"]   = yawPos;
  d["pitchPos"] = pitchPos;
  sendDoc(d);
}

void sendEventHalted(const char* reason) {
  JsonDocument d;
  d["event"]  = "halted";
  d["reason"] = reason;
  sendDoc(d);
}

void sendStatus(const char* tagKey, const char* tagVal) {
  JsonDocument d;
  d[tagKey]      = tagVal;
  d["state"]     = stateName(currentState);
  d["laser"]     = laserFiring;
  d["homed"]     = homed;
  d["halted"]    = halted;
  d["yawPos"]    = yawPos;
  d["pitchPos"]  = pitchPos;
  d["yawRange"]  = YAW_RANGE_STEPS;
  d["pitchRange"] = PITCH_RANGE_STEPS;
  sendDoc(d);
}

// =============================================================================
// MOTOR PRIMITIVES
// =============================================================================
void stepOnce(int stepPin, unsigned long pulseUs) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(pulseUs);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(pulseUs);
}

// Step `count` times in given direction. Watches `limGuard` only (one-sided
// safety net during normal moves). Returns steps actually executed.
long stepN(int stepPin, int dirPin, bool dirHigh, unsigned long pulseUs,
           long count, Sw* limGuard) {
  digitalWrite(dirPin, dirHigh ? HIGH : LOW);
  delay(DIR_SETUP_MS);
  long done = 0;
  for (long i = 0; i < count; i++) {
    updateAll();
    if (limGuard && triggered(*limGuard)) break;
    stepOnce(stepPin, pulseUs);
    done++;
  }
  return done;
}

// Sweep until `lim` triggers or cap reached. Returns true on hit.
bool sweepToLimit(int stepPin, int dirPin, bool dirHigh, unsigned long pulseUs,
                  Sw* lim, long* stepsOut) {
  digitalWrite(dirPin, dirHigh ? HIGH : LOW);
  delay(DIR_SETUP_MS);
  for (long i = 0; i < MAX_SWEEP_STEPS; i++) {
    updateAll();
    if (triggered(*lim)) { if (stepsOut) *stepsOut = i; return true; }
    stepOnce(stepPin, pulseUs);
  }
  if (stepsOut) *stepsOut = MAX_SWEEP_STEPS;
  return false;
}

// Reverse direction; per-step backoff until `lim` releases continuously for
// RELEASE_CLEAR_MS, then add `margin` more steps clear of the lever.
bool backoffFromLimit(int stepPin, int dirPin, bool reverseDirHigh,
                      unsigned long pulseUs, Sw* lim, int margin) {
  digitalWrite(dirPin, reverseDirHigh ? HIGH : LOW);
  delay(DIR_SETUP_MS);
  long stepsTaken = 0;
  while (stepsTaken < MAX_SWEEP_STEPS) {
    stepOnce(stepPin, pulseUs);
    stepsTaken++;
    updateAll();
    if (triggered(*lim)) continue;
    // Confirm release stays clear.
    unsigned long start = millis();
    bool stayed = true;
    while (millis() - start < RELEASE_CLEAR_MS) {
      updateAll();
      if (triggered(*lim)) { stayed = false; break; }
    }
    if (!stayed) continue;
    // Margin steps with per-step limit watch.
    for (int i = 0; i < margin; i++) {
      stepOnce(stepPin, pulseUs);
      updateAll();
      // No need to halt here — margin is small, axis is wide open.
    }
    // Final sanity (sustained LOW).
    unsigned long sanityStart = millis();
    while (millis() - sanityStart < RELEASE_CLEAR_MS) {
      updateAll();
      if (triggered(*lim)) return false;
    }
    return true;
  }
  return false;
}

// =============================================================================
// HOMING
// Yaw   : sweep LOW (toward LEFT) → swL trips → set yawPos = 0 → backoff +margin.
// Pitch : sweep LOW (toward DOWN) → swD trips → set pitchPos = 0 → backoff +margin.
// After both succeed, position is (YAW_SAFETY_MARGIN_STEPS, PITCH_SAFETY_MARGIN_STEPS).
// Positive deltas thereafter move toward RIGHT (yaw) and UP (pitch).
// =============================================================================
bool home() {
  homed = false;
  Serial.println(F("# homing yaw -> LEFT"));
  long s = 0;
  if (!sweepToLimit(PIN_YAW_STEP, PIN_YAW_DIR, LOW, YAW_PULSE_US, &swL, &s)) {
    sendEventHalted("home_yaw_timeout");
    halted = true;
    return false;
  }
  Serial.print(F("# yaw LEFT hit in ")); Serial.println(s);
  if (!backoffFromLimit(PIN_YAW_STEP, PIN_YAW_DIR, HIGH, YAW_PULSE_US,
                        &swL, YAW_SAFETY_MARGIN_STEPS)) {
    sendEventHalted("home_yaw_release");
    halted = true;
    return false;
  }
  yawPos = YAW_SAFETY_MARGIN_STEPS;

  Serial.println(F("# homing pitch -> DOWN"));
  if (!sweepToLimit(PIN_PITCH_STEP, PIN_PITCH_DIR, LOW, PITCH_PULSE_US, &swD, &s)) {
    sendEventHalted("home_pitch_timeout");
    halted = true;
    return false;
  }
  Serial.print(F("# pitch DOWN hit in ")); Serial.println(s);
  if (!backoffFromLimit(PIN_PITCH_STEP, PIN_PITCH_DIR, HIGH, PITCH_PULSE_US,
                        &swD, PITCH_SAFETY_MARGIN_STEPS)) {
    sendEventHalted("home_pitch_release");
    halted = true;
    return false;
  }
  pitchPos = PITCH_SAFETY_MARGIN_STEPS;

  homed = true;
  halted = false;
  sendEventHomed();
  return true;
}

// =============================================================================
// MOVE — signed delta with software clamp + live limit halt.
// Returns the actual signed delta executed (may be smaller than requested if
// clamped to range or stopped early by limit trigger).
// =============================================================================
long moveAxis(int stepPin, int dirPin, unsigned long pulseUs,
              long delta, long* pos, long rangeMax,
              Sw* limLow, Sw* limHigh,
              const char* axisName) {
  if (delta == 0) return 0;

  long target = *pos + delta;
  if (target < 0)         target = 0;
  if (target > rangeMax)  target = rangeMax;
  long clamped = target - *pos;
  if (clamped == 0) return 0;

  bool dirHigh  = clamped > 0;     // positive = toward RIGHT (yaw) / UP (pitch)
  long want     = clamped > 0 ? clamped : -clamped;
  Sw* watch     = dirHigh ? limHigh : limLow;

  long done = stepN(stepPin, dirPin, dirHigh, pulseUs, want, watch);
  *pos += dirHigh ? done : -done;

  if (done < want) {
    // Limit interrupted us — emit telemetry.
    sendEventLimit(axisName, watch->name);
  }
  return dirHigh ? done : -done;
}

// =============================================================================
// COMMAND HANDLERS
// =============================================================================
void cmdHome(JsonDocument &resp) {
  bool ok = home();
  resp["ok"] = ok;
  if (ok) {
    resp["yawPos"]   = yawPos;
    resp["pitchPos"] = pitchPos;
  }
}

void cmdStop(JsonDocument &resp) {
  halted = true;
  sendEventHalted("stop_cmd");
  resp["ok"] = true;
}

void cmdResume(JsonDocument &resp) {
  halted = false;
  resp["ok"]     = true;
  resp["halted"] = false;
}

void cmdMove(JsonDocument &doc, JsonDocument &resp) {
  if (!homed)  { resp["ok"] = false; resp["err"] = "not_homed";  return; }
  if (halted)  { resp["ok"] = false; resp["err"] = "halted";     return; }

  long yawDelta   = doc["yaw"]   | 0;
  long pitchDelta = doc["pitch"] | 0;

  long yawDone   = moveAxis(PIN_YAW_STEP,   PIN_YAW_DIR,   YAW_PULSE_US,
                            yawDelta,   &yawPos,   YAW_RANGE_STEPS,
                            &swL, &swR, "yaw");
  long pitchDone = moveAxis(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
                            pitchDelta, &pitchPos, PITCH_RANGE_STEPS,
                            &swD, &swU, "pitch");

  resp["ok"]        = true;
  resp["yawDone"]   = yawDone;
  resp["pitchDone"] = pitchDone;
  resp["yawPos"]    = yawPos;
  resp["pitchPos"]  = pitchPos;
  resp["clipped"]   = (yawDone != yawDelta) || (pitchDone != pitchDelta);
}

void cmdState(JsonDocument &doc, JsonDocument &resp) {
  const char* v = doc["value"] | (const char*)nullptr;
  if (!v) { resp["ok"] = false; resp["err"] = "missing_value"; return; }
  SystemState ns = currentState;
  if      (!strcmp(v, "idle"))     ns = STATE_IDLE;
  else if (!strcmp(v, "scanning")) ns = STATE_SCANNING;
  else if (!strcmp(v, "detected")) ns = STATE_DETECTED;
  else                              { resp["ok"] = false; resp["err"] = "bad_value"; return; }
  currentState = ns;
  updateLeds();
  resp["ok"]    = true;
  resp["state"] = stateName(currentState);
}

void cmdLaser(JsonDocument &doc, JsonDocument &resp) {
  bool on = doc["on"] | false;
  laserFiring = on;
  // PIN_LASER not driven yet — laser hardware dead (status.md Open #1).
  updateLeds();
  resp["ok"]    = true;
  resp["laser"] = laserFiring;
}

void cmdPing(JsonDocument &resp) {
  resp["ok"]   = true;
  resp["pong"] = true;
}

void cmdStatusJson() {
  sendStatus("ack", "status");
}

void dispatchJson(JsonDocument &doc) {
  const char* cmd = doc["cmd"] | (const char*)nullptr;
  if (!cmd) {
    JsonDocument r;
    r["ok"]  = false;
    r["err"] = "missing_cmd";
    sendDoc(r);
    return;
  }

  // status is one-shot, no resp wrapper
  if (!strcmp(cmd, "status")) { cmdStatusJson(); return; }

  JsonDocument resp;
  resp["ack"] = cmd;

  if      (!strcmp(cmd, "move"))   cmdMove(doc, resp);
  else if (!strcmp(cmd, "home"))   cmdHome(resp);
  else if (!strcmp(cmd, "stop"))   cmdStop(resp);
  else if (!strcmp(cmd, "resume")) cmdResume(resp);
  else if (!strcmp(cmd, "state"))  cmdState(doc, resp);
  else if (!strcmp(cmd, "laser"))  cmdLaser(doc, resp);
  else if (!strcmp(cmd, "ping"))   cmdPing(resp);
  else                              { resp["ok"] = false; resp["err"] = "unknown_cmd"; }

  sendDoc(resp);
}

// =============================================================================
// MANUAL SINGLE-CHAR MENU (bench testing only)
// =============================================================================
void printHelp() {
  Serial.println();
  Serial.println(F("=== MAIN CONTROL ==="));
  Serial.println(F("  H - help"));
  Serial.println(F("  ? - status (as JSON)"));
  Serial.println(F("  All control via line-delimited JSON. See header comment."));
  Serial.println(F("===================="));
}

void handleMenuChar(char c) {
  switch (c) {
    case 'H': printHelp(); break;
    case '?': cmdStatusJson(); break;
    default:
      Serial.print(F("? unknown cmd: "));
      Serial.println(c);
      break;
  }
}

// =============================================================================
// LINE PROCESSOR
// =============================================================================
void processLine(char* line) {
  while (*line == ' ' || *line == '\t') line++;
  if (*line == '\0') return;

  if (*line == '{') {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) {
      JsonDocument r;
      r["ok"]  = false;
      r["err"] = "json_parse";
      r["msg"] = err.c_str();
      sendDoc(r);
      return;
    }
    dispatchJson(doc);
  } else {
    char c = (*line >= 'a' && *line <= 'z') ? *line - 32 : *line;
    handleMenuChar(c);
  }
}

// =============================================================================
// ARDUINO ENTRY
// =============================================================================
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

  Serial.println();
  Serial.println(F("# main_control ready."));
  printHelp();

  // Auto-home on boot.
  if (!home()) {
    Serial.println(F("# homing failed at boot — system halted, send {\"cmd\":\"home\"} to retry"));
  }
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (rxLen > 0) {
        rxBuf[rxLen] = '\0';
        processLine(rxBuf);
        rxLen = 0;
      }
    } else if (rxLen < SERIAL_LINE_BUF - 1) {
      rxBuf[rxLen++] = c;
    } else {
      // overflow — drop line
      rxLen = 0;
      JsonDocument r;
      r["ok"]  = false;
      r["err"] = "line_too_long";
      sendDoc(r);
    }
  }
}
