// commands.h
#pragma once

#include <ArduinoJson.h>

void cmdStop  (JsonDocument &resp);
void cmdResume(JsonDocument &resp);
void cmdMove  (JsonDocument &doc, JsonDocument &resp);
void cmdState (JsonDocument &doc, JsonDocument &resp);
void cmdLaser (JsonDocument &doc, JsonDocument &resp);
void cmdPing  (JsonDocument &resp);
void cmdStatusJson();
