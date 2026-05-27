// scanning.h — autonomous target-search sweep.
//
// Runs while the system is in STATE_SCANNING (set by the Jetson via the
// {"type":"data","key":"system_state","value":"scanning"} message). It is a
// cooperative state machine: scanTick() performs one bounded chunk of motion
// per call so the main loop stays responsive to incoming offset/state messages
// and can break out the instant a target is reacquired.
//
// Sequence on (re)entry to scanning:
//   1. HOMING_DOWN — drive pitch down until the DOWN limit (vertical home).
//   2. LEVELING    — from that reference, drive pitch up PITCH_LEVEL_FROM_DOWN_STEPS
//                    so the barrel sits parallel with the horizon.
//   3. SWEEP_RIGHT — drive yaw right until the RIGHT limit.
//   4. SWEEP_LEFT  — drive yaw left until the LEFT limit, then loop 3<->4.
#pragma once

void scanReset();   // restart the sequence at HOMING_DOWN
void scanTick();    // advance one motion chunk; call each loop while scanning