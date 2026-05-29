#include <Arduino.h>
#include <ArduinoJson.h>
#include "serial_link.h"
#include "state.h"
#include "switches.h" // triggered()
#include "leds.h"     // stateName

// Window after a step during which the axis is reported as moving. After
// this elapses with no fresh step, telemetry reports yaw_dir/pitch_dir = 0.
static const unsigned long DIR_DECAY_MS = 500;

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

// Periodic telemetry frame consumed by Jetson → ground station UI.
// Shape matches frontend renderTelemetry: limit_switches[4], yaw_dir,
// pitch_dir, laser, status. Direction fields decay to 0 after DIR_DECAY_MS.
void sendTelemetry() {
  unsigned long now = millis();
  JsonDocument d;
  d["type"] = "telemetry";
  JsonObject t = d["data"].to<JsonObject>();
  JsonArray sw = t["limit_switches"].to<JsonArray>();
  sw.add(triggered(swL));
  sw.add(triggered(swR));
  sw.add(triggered(swU));
  sw.add(triggered(swD));
  t["yaw_dir"]   = (now - lastYawStepMs   > DIR_DECAY_MS) ? 0 : (int)lastYawDir;
  t["pitch_dir"] = (now - lastPitchStepMs > DIR_DECAY_MS) ? 0 : (int)lastPitchDir;
  t["laser"]     = laserFiring;
  t["status"]    = stateName(currentState);
  sendDoc(d);
}
