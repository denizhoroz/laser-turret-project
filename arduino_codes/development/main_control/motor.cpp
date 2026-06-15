#include <Arduino.h>
#include "motor.h"
#include "config.h"
#include "switches.h"
#include "serial_link.h" 

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
  return dirHigh ? done : -done;
}
