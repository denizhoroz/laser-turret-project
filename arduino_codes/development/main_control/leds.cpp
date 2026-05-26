#include <Arduino.h>
#include "leds.h"
#include "config.h"
#include "state.h"

void updateLeds() {
  digitalWrite(PIN_LED_GREEN, HIGH);
  bool targetFresh = (lastTargetSignalMs > 0) &&
                     (millis() - lastTargetSignalMs < TARGET_SIGNAL_STALE_MS);
  bool yellow = (currentState == STATE_DETECTED) || targetFresh || laserFiring;
  digitalWrite(PIN_LED_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(PIN_LED_RED,    laserFiring ? HIGH : LOW);
}

const char* stateName(SystemState s) {
  switch (s) {
    case STATE_IDLE:     return "idle";
    case STATE_SCANNING: return "scanning";
    case STATE_DETECTED: return "detected";
  }
  return "?";
}
