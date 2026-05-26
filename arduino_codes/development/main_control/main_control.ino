// main_control.ino — turret production firmware.
// Layers: LEDs · limit switches · stepper motors (delta-only) · JSON link to Jetson.
//
// **Motion model: pure step deltas, reactive limit safety.**
//   No position tracker, no homing, no software range clamp. The only
//   constraint on motion is the physical limit switch — if it trips during a
//   move, the motor halts that axis and emits a `limit` event. Reason for
//   stripping: mechanical stress / load can falsely advance the tracker out
//   of sync with real position, causing the absolute model to refuse moves
//   into space that's actually available.
//
// LED rules:
//   Green   — ON whenever system is powered.
//   Yellow  — ON in DETECTED state OR fresh target signal OR laser firing.
//   Red     — ON while laser is firing.
//
// JSON link (line-delimited JSON over USB Serial, both directions):
//   Jetson -> Arduino (Python production schema):
//     {"type":"data","key":"current_target_offset","value":[x,y]}   // pixel offset
//     {"type":"data","key":"is_firing","value":true|false}          // laser fire flag
//   Bench / manual schema (acks back):
//     {"cmd":"move","yaw":N,"pitch":M}      // signed step deltas
//     {"cmd":"stop"} / {"cmd":"resume"}     // halt / clear halt
//     {"cmd":"state","value":"idle|scanning|detected"}
//     {"cmd":"laser","on":true|false}
//     {"cmd":"status"} / {"cmd":"ping"}
//   Arduino -> Jetson telemetry:
//     {"ack":"<cmd>", ...}                  // only for {"cmd":...} messages
//     {"event":"limit","axis":"yaw|pitch","side":"left|right|up|down"}
//     {"event":"halted","reason":"..."}
//
// Manual single-char bench cmds:  H - help · ? - status
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
bool halted = false;

// Last time a Python "target signal" arrived (offset or is_firing). Drives
// yellow LED via staleness window (TARGET_SIGNAL_STALE_MS).
unsigned long lastTargetSignalMs = 0;

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
  digitalWrite(PIN_LED_GREEN, HIGH);
  bool targetFresh = (lastTargetSignalMs > 0) &&
                     (millis() - lastTargetSignalMs < TARGET_SIGNAL_STALE_MS);
  bool yellow = (currentState == STATE_DETECTED) || targetFresh || laserFiring;
  digitalWrite(PIN_LED_YELLOW, yellow ? HIGH : LOW);
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
// OUTBOUND JSON — events, acks
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

void sendEventHalted(const char* reason) {
  JsonDocument d;
  d["event"]  = "halted";
  d["reason"] = reason;
  sendDoc(d);
}

void sendStatus(const char* tagKey, const char* tagVal) {
  JsonDocument d;
  d[tagKey]   = tagVal;
  d["state"]  = stateName(currentState);
  d["laser"]  = laserFiring;
  d["halted"] = halted;
  sendDoc(d);
}

// =============================================================================
// MOTOR — pure delta steps with one-sided limit-switch halt.
//
// Direction convention (matches motor coil + DIR wiring):
//   yaw   delta > 0 → DIR HIGH → motor toward RIGHT.   Watch swR.
//   yaw   delta < 0 → DIR LOW  → motor toward LEFT.    Watch swL.
//   pitch delta > 0 → DIR HIGH → motor toward UP.      Watch swU.
//   pitch delta < 0 → DIR LOW  → motor toward DOWN.    Watch swD.
// =============================================================================
void stepOnce(int stepPin, unsigned long pulseUs) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(pulseUs);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(pulseUs);
}

// Forward decl — defined below stepDelta.
void aim(bool firingOn);

// Execute up to |delta| steps in given direction. Stops early if limit trips.
// Returns signed steps actually executed.
long stepDelta(int stepPin, int dirPin, unsigned long pulseUs,
               long delta, Sw* limLow, Sw* limHigh, const char* axisName) {
  if (delta == 0) return 0;
  bool dirHigh = delta > 0;
  long want    = dirHigh ? delta : -delta;
  Sw* watch    = dirHigh ? limHigh : limLow;

  digitalWrite(dirPin, dirHigh ? HIGH : LOW);
  delay(DIR_SETUP_MS);

  long done = 0;
  for (long i = 0; i < want; i++) {
    updateAll();
    if (triggered(*watch)) {
      sendEventLimit(axisName, watch->name);
      break;
    }
    stepOnce(stepPin, pulseUs);
    done++;
  }
  return dirHigh ? done : -done;
}

// =============================================================================
// AIMING — parallax compensation
//
// One-shot pitch shift applied on firing edges. UP before firing, DOWN after.
// Magnitude in config.h (PITCH_AIM_OFFSET_STEPS). Idempotent guards prevent
// double-application if firing state is hammered.
// =============================================================================
void aim(bool firingOn) {
  if (halted || PITCH_AIM_OFFSET_STEPS == 0) return;
  long delta = firingOn ? PITCH_AIM_OFFSET_STEPS : -PITCH_AIM_OFFSET_STEPS;
  stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
            delta, &swD, &swU, "pitch");
}

// =============================================================================
// COMMAND HANDLERS (bench schema)
// =============================================================================
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
  if (halted) { resp["ok"] = false; resp["err"] = "halted"; return; }

  long yawDelta   = doc["yaw"]   | 0;
  long pitchDelta = doc["pitch"] | 0;

  long yawDone   = stepDelta(PIN_YAW_STEP,   PIN_YAW_DIR,   YAW_PULSE_US,
                             yawDelta,   &swL, &swR, "yaw");
  long pitchDone = stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
                             pitchDelta, &swD, &swU, "pitch");

  resp["ok"]        = true;
  resp["yawDone"]   = yawDone;
  resp["pitchDone"] = pitchDone;
  resp["limited"]   = (yawDone != yawDelta) || (pitchDone != pitchDelta);
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
  if (on != laserFiring) {
    if (on) {
      aim(true);                                // shift pitch up first
      laserFiring = true;
      digitalWrite(PIN_LASER, HIGH);
    } else {
      digitalWrite(PIN_LASER, LOW);
      laserFiring = false;
      aim(false);                               // shift back down
    }
    updateLeds();
  }
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

// =============================================================================
// PYTHON DATA-MESSAGE HANDLERS  ({"type":"data","key":"...","value":...})
// Silent — Python does not expect acks on this path.
// =============================================================================
// Convert one axis offset (pixels) to a signed step delta.
//   |px| < DEAD_ZONE_PX           → 0
//   else                          → max(1, |px| / pxPerStep), capped, signed by px.
// Caller flips sign for pitch (image y+ = down, but pitch UP = positive).
static long offsetToSteps(int px, int pxPerStep) {
  int mag = px < 0 ? -px : px;
  if (mag < DEAD_ZONE_PX) return 0;
  long n = mag / pxPerStep;
  if (n < 1) n = 1;                                  // force motion past deadzone
  if (n > MAX_STEP_PER_TICK) n = MAX_STEP_PER_TICK;  // safety cap
  return px > 0 ? n : -n;
}

void handleOffsetData(JsonVariantConst value) {
  lastTargetSignalMs = millis();
  if (halted) return;
  if (!value.is<JsonArrayConst>()) return;
  JsonArrayConst arr = value.as<JsonArrayConst>();
  if (arr.size() < 2) return;

  int ox = arr[0] | 0;
  int oy = arr[1] | 0;

  long yawDelta   =  offsetToSteps(ox, PX_PER_STEP_YAW);   // x+ → yaw +
  long pitchDelta = -offsetToSteps(oy, PX_PER_STEP_PITCH); // y+ (below) → pitch -

  if (yawDelta != 0) {
    stepDelta(PIN_YAW_STEP, PIN_YAW_DIR, YAW_PULSE_US,
              yawDelta, &swL, &swR, "yaw");
  }
  if (pitchDelta != 0) {
    stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US,
              pitchDelta, &swD, &swU, "pitch");
  }

  updateLeds();
}

void handleIsFiringData(bool on) {
  lastTargetSignalMs = millis();
  if (on == laserFiring) return;

  if (on) {
    aim(true);                                  // shift pitch up to compensate parallax
    laserFiring = true;
    digitalWrite(PIN_LASER, HIGH);
  } else {
    digitalWrite(PIN_LASER, LOW);
    laserFiring = false;
    aim(false);                                 // shift back so vision tracking resumes aligned
  }
  updateLeds();
}

void dispatchPythonData(JsonDocument &doc) {
  const char* key = doc["key"] | (const char*)nullptr;
  if (!key) return;
  if      (!strcmp(key, "current_target_offset")) handleOffsetData(doc["value"]);
  else if (!strcmp(key, "is_firing"))             handleIsFiringData(doc["value"] | false);
  // unknown keys silently ignored
}

void dispatchJson(JsonDocument &doc) {
  const char* type = doc["type"] | (const char*)nullptr;
  if (type) {
    if (!strcmp(type, "data")) dispatchPythonData(doc);
    return;
  }

  const char* cmd = doc["cmd"] | (const char*)nullptr;
  if (!cmd) {
    JsonDocument r;
    r["ok"]  = false;
    r["err"] = "missing_type_or_cmd";
    sendDoc(r);
    return;
  }

  if (!strcmp(cmd, "status")) { cmdStatusJson(); return; }

  JsonDocument resp;
  resp["ack"] = cmd;

  if      (!strcmp(cmd, "move"))   cmdMove(doc, resp);
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
  Serial.println(F("# main_control ready (delta-only mode, no homing)."));
  printHelp();
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
      rxLen = 0;
      JsonDocument r;
      r["ok"]  = false;
      r["err"] = "line_too_long";
      sendDoc(r);
    }
  }

  // Refresh LEDs periodically so yellow drops when the target signal goes
  // stale (no recent offset/fire message from Python).
  static unsigned long lastLedTick = 0;
  unsigned long now = millis();
  if (now - lastLedTick > 100) {
    lastLedTick = now;
    updateLeds();
  }
}
