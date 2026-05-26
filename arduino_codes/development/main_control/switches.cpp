#include <Arduino.h>
#include "switches.h"
#include "config.h"
#include "state.h"

void updateSw(Sw &s) {
  bool r = digitalRead(s.pin);
  unsigned long now = millis();
  if (r != s.reading) { s.reading = r; s.t = now; }
  if (now - s.t >= DEBOUNCE_MS && r != s.stable) s.stable = r;
}

void updateAll() {
  updateSw(swL);
  updateSw(swR);
  updateSw(swU);
  updateSw(swD);
}

bool triggered(const Sw &s) { return s.stable == HIGH; }

void seedSwitches() {
  swL.stable = swL.reading = digitalRead(swL.pin);
  swR.stable = swR.reading = digitalRead(swR.pin);
  swU.stable = swU.reading = digitalRead(swU.pin);
  swD.stable = swD.reading = digitalRead(swD.pin);
}
