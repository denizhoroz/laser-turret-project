// dispatch.h — inbound serial line buffer + JSON parse + route.
// `feedSerialChar` is called from loop() with each `Serial.read()` byte;
// on newline it assembles + dispatches the line. Handles both Python
// `{"type":"data",...}` and bench `{"cmd":"..."}` schemas, plus single-char
// menu commands (H / ?).
#pragma once

void feedSerialChar(char c);

// Execute the most recent offset received via `current_target_offset`, if any.
// Called from the main loop after the serial buffer has been fully drained, so
// older offsets queued during a long blocking motion are overwritten by newer
// ones and silently discarded.
void flushPendingOffset();
