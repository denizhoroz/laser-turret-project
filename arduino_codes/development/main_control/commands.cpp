#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include "commands.h"
#include "config.h"
#include "state.h"
#include "motor.h"
#include "laser.h"
#include "leds.h"
#include "serial_link.h"
#include "types.h"

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
  setFiring(on);
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
