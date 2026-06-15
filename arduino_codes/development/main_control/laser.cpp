#include <Arduino.h>
#include "laser.h"
#include "config.h"
#include "state.h"
#include "leds.h"

void setFiring(bool on) {
  if (on == laserFiring) return;
  laserFiring = on;
  digitalWrite(PIN_LASER, laserFiring ? HIGH : LOW);
  updateLeds();
}
