// dispatch.h — inbound serial line buffer + JSON parse + route.
// `feedSerialChar` is called from loop() with each `Serial.read()` byte;
// on newline it assembles + dispatches the line. Handles both Python
// `{"type":"data",...}` and bench `{"cmd":"..."}` schemas, plus single-char
// menu commands (H / ?).
#pragma once

void feedSerialChar(char c);
