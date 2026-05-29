// serial_link.h — outbound JSON to Jetson (and bench operator).
// Low-level emit primitives only. Inbound parsing lives in dispatch.h.
#pragma once

#include <ArduinoJson.h>

void sendDoc(JsonDocument &doc);
void sendEventLimit(const char* axis, const char* side);
void sendEventHalted(const char* reason);
void sendStatus(const char* tagKey, const char* tagVal);
