// commands.h — bench `{"cmd":"..."}` handlers (manual / no Jetson).
// Each writes its result into a passed-in JsonDocument that `dispatch.cpp`
// then ships back. `cmdStatusJson` is a fire-and-forget send (no resp).
#pragma once

#include <ArduinoJson.h>

void cmdStop  (JsonDocument &resp);
void cmdResume(JsonDocument &resp);
void cmdMove  (JsonDocument &doc, JsonDocument &resp);
void cmdState (JsonDocument &doc, JsonDocument &resp);
void cmdLaser (JsonDocument &doc, JsonDocument &resp);
void cmdPing  (JsonDocument &resp);
void cmdStatusJson();
