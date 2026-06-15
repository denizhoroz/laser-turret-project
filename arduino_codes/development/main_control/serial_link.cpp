#include <Arduino.h>
#include <ArduinoJson.h>
#include "serial_link.h"
#include "state.h"
#include "leds.h"     

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
