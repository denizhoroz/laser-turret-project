#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include "dispatch.h"
#include "config.h"
#include "state.h"
#include "tracking.h"
#include "laser.h"
#include "leds.h"
#include "scanning.h"
#include "commands.h"
#include "serial_link.h"

static char rxBuf[SERIAL_LINE_BUF];
static int  rxLen = 0;

// Latest pending offset from Python. New offsets overwrite older ones — by the
// time the main loop calls `flushPendingOffset`, only the freshest survives.
// Prevents stale-offset pile-up when applyOffset() blocks longer than Python's
// send interval.
static int  pendingOx        = 0;
static int  pendingOy        = 0;
static bool hasPendingOffset = false;

// =============================================================================
// MANUAL SINGLE-CHAR MENU (bench testing only)
// =============================================================================
static void printHelp() {
  Serial.println();
  Serial.println(F("=== MAIN CONTROL ==="));
  Serial.println(F("  H - help"));
  Serial.println(F("  ? - status (as JSON)"));
  Serial.println(F("  All control via line-delimited JSON. See main_control.ino header."));
  Serial.println(F("===================="));
}

static void handleMenuChar(char c) {
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
// PYTHON DATA HANDLERS  ({"type":"data","key":"...","value":...})
// Silent — Python does not expect acks on this path.
// =============================================================================
static void handleOffsetData(JsonVariantConst value) {
  lastTargetSignalMs = millis();
  if (!value.is<JsonArrayConst>()) return;
  JsonArrayConst arr = value.as<JsonArrayConst>();
  if (arr.size() < 2) return;
  pendingOx        = arr[0] | 0;
  pendingOy        = arr[1] | 0;
  hasPendingOffset = true;   // newer overwrites older — flushed once per loop tick
}

void flushPendingOffset() {
  if (!hasPendingOffset) return;
  hasPendingOffset = false;
  applyOffset(pendingOx, pendingOy);
}

static void handleIsFiringData(bool on) {
  lastTargetSignalMs = millis();
  setFiring(on);
}

// Jetson mission state. Vocabulary differs from the bench cmd schema:
// idle / scanning / tracking / firing. tracking+firing both mean "target
// acquired" → STATE_DETECTED. Entering scanning restarts the sweep from home.
static void handleSystemStateData(const char* v) {
  if (!v) return;
  SystemState ns;
  if      (!strcmp(v, "idle"))     ns = STATE_IDLE;
  else if (!strcmp(v, "scanning")) ns = STATE_SCANNING;
  else if (!strcmp(v, "tracking")) ns = STATE_DETECTED;
  else if (!strcmp(v, "firing"))   ns = STATE_DETECTED;
  else return;   // unknown state ignored

  if (ns != currentState) {
    if (ns == STATE_SCANNING) scanReset();   // fresh sweep from vertical home
    currentState = ns;
    updateLeds();

    // Emit a state-change event over USB serial so the host can verify the
    // Arduino actually applied the message (debugging stuck-LED / no-scan
    // issues where it's unclear whether the state ever crossed the wire).
    JsonDocument ev;
    ev["event"] = "state";
    ev["value"] = stateName(currentState);
    sendDoc(ev);
  }
}

static void dispatchPythonData(JsonDocument &doc) {
  const char* key = doc["key"] | (const char*)nullptr;
  if (!key) return;
  if      (!strcmp(key, "current_target_offset")) handleOffsetData(doc["value"]);
  else if (!strcmp(key, "is_firing"))             handleIsFiringData(doc["value"] | false);
  else if (!strcmp(key, "system_state"))          handleSystemStateData(doc["value"] | (const char*)nullptr);
  // unknown keys silently ignored
}

// =============================================================================
// TOP-LEVEL DISPATCH
// =============================================================================
static void dispatchJson(JsonDocument &doc) {
  const char* type = doc["type"] | (const char*)nullptr;
  if (type) {
    if (!strcmp(type, "data")) dispatchPythonData(doc);
    return;   // state/mission/frame belong to the web link, not Arduino
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

static void processLine(char* line) {
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

void feedSerialChar(char c) {
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
