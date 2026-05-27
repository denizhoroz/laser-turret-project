#include <Arduino.h>
#include "scanning.h"
#include "config.h"
#include "state.h"
#include "switches.h"
#include "motor.h"

// stepDelta sign convention (mirrors tracking.cpp / motor.h):
//   pitch delta > 0 → toward swU (up).   delta < 0 → toward swD (down).
//   yaw   delta > 0 → toward swR (right). delta < 0 → toward swL (left).
enum ScanPhase {
  SCAN_HOMING_DOWN,
  SCAN_LEVELING,
  SCAN_SWEEP_RIGHT,
  SCAN_SWEEP_LEFT
};

static ScanPhase phase          = SCAN_HOMING_DOWN;
static long      levelStepsLeft  = 0;

void scanReset() {
  phase         = SCAN_HOMING_DOWN;
  levelStepsLeft = 0;
}

void scanTick() {
  if (halted) return;
  updateAll();

  switch (phase) {
    case SCAN_HOMING_DOWN:
      if (triggered(swD)) {                       // reached vertical home
        levelStepsLeft = PITCH_LEVEL_FROM_DOWN_STEPS;
        phase = SCAN_LEVELING;
        break;
      }
      stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, SCAN_PULSE_US,
                -(long)SCAN_CHUNK_STEPS, &swD, &swU, "pitch");
      break;

    case SCAN_LEVELING:
      if (levelStepsLeft <= 0 || triggered(swU)) {  // level reached (or ceiling)
        phase = SCAN_SWEEP_RIGHT;
        break;
      }
      {
        long chunk = levelStepsLeft < (long)SCAN_CHUNK_STEPS
                       ? levelStepsLeft : (long)SCAN_CHUNK_STEPS;
        long done = stepDelta(PIN_PITCH_STEP, PIN_PITCH_DIR, SCAN_PULSE_US,
                              chunk, &swD, &swU, "pitch");
        levelStepsLeft -= (done < 0 ? -done : done);
      }
      break;

    case SCAN_SWEEP_RIGHT:
      if (triggered(swR)) { phase = SCAN_SWEEP_LEFT; break; }
      stepDelta(PIN_YAW_STEP, PIN_YAW_DIR, SCAN_PULSE_US,
                (long)SCAN_CHUNK_STEPS, &swL, &swR, "yaw");
      break;

    case SCAN_SWEEP_LEFT:
      if (triggered(swL)) { phase = SCAN_SWEEP_RIGHT; break; }
      stepDelta(PIN_YAW_STEP, PIN_YAW_DIR, SCAN_PULSE_US,
                -(long)SCAN_CHUNK_STEPS, &swL, &swR, "yaw");
      break;
  }
}
