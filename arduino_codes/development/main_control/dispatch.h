// dispatch.h 
#pragma once

void feedSerialChar(char c);

// Execute the most recent offset received via `current_target_offset`
void flushPendingOffset();
