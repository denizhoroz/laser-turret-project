#include <Arduino.h>
#include "laser.h"
#include "config.h"
#include "state.h"
#include "leds.h"

// Parallax is now applied continuously inside tracking.applyOffset(), not on
// firing edges. setFiring is a pure laser-pin + LED coordinator.
void setFiring(bool on) {
  if (on == laserFiring) return;
  laserFiring = on;
  digitalWrite(PIN_LASER, laserFiring ? HIGH : LOW);
  updateLeds();
}
