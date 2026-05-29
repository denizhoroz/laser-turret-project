#include <Arduino.h>
#include <string.h>
#include "motor.h"
#include "config.h"
#include "state.h"
#include "switches.h"
#include "serial_link.h"   // sendEventLimit

void stepOnce(int stepPin, unsigned long pulseUs) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(pulseUs);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(pulseUs);
}

long stepDelta(int stepPin, int dirPin, unsigned long pulseUs,
               long delta, Sw* limLow, Sw* limHigh, const char* axisName) {
  if (delta == 0) return 0;
  bool dirHigh = delta > 0;
  long want    = dirHigh ? delta : -delta;
  Sw* watch    = dirHigh ? limHigh : limLow;

  digitalWrite(dirPin, dirHigh ? HIGH : LOW);
  delay(DIR_SETUP_MS);

  long done = 0;
  for (long i = 0; i < want; i++) {
    updateAll();
    if (triggered(*watch)) {
      sendEventLimit(axisName, watch->name);
      break;
    }
    stepOnce(stepPin, pulseUs);
    done++;
  }
  long signedDone = dirHigh ? done : -done;

  // Telemetry hook: record direction + timestamp per axis so sendTelemetry()
  // can report a yaw_dir / pitch_dir that decays to 0 when idle.
  if (signedDone != 0) {
    int8_t dir = (signedDone > 0) ? 1 : -1;
    unsigned long now = millis();
    if (strcmp(axisName, "yaw") == 0) {
      lastYawDir    = dir;
      lastYawStepMs = now;
    } else if (strcmp(axisName, "pitch") == 0) {
      lastPitchDir    = dir;
      lastPitchStepMs = now;
    }
  }

  return signedDone;
}
