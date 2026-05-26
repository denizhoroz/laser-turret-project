#include <Arduino.h>
#include "laser.h"
#include "config.h"
#include "state.h"
#include "aim.h"
#include "leds.h"

void setFiring(bool on) {
  if (on == laserFiring) return;

  if (on) {
    aim(true);                                  // shift pitch up to compensate parallax
    laserFiring = true;
    digitalWrite(PIN_LASER, HIGH);
  } else {
    digitalWrite(PIN_LASER, LOW);
    laserFiring = false;
    aim(false);                                 // shift back so vision tracking resumes aligned
  }

  updateLeds();
}
