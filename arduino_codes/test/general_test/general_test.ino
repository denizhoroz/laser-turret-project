// GENERAL TEST — all turret components, serial-menu driven.
// Pin map + tunables: config.h (next to this file).
// Pin map authoritative source: .schematic/components.md.
// Project state + open issues: .schematic/status.md.

#include "config.h"
#include "types.h"

Sw swL = { PIN_LIM_LEFT,  false, false, 0, "LEFT"  };
Sw swR = { PIN_LIM_RIGHT, false, false, 0, "RIGHT" };
Sw swU = { PIN_LIM_UP,    false, false, 0, "UP"    };
Sw swD = { PIN_LIM_DOWN,  false, false, 0, "DOWN"  };

bool halted = false;

// =============================================================================
// HELPERS
// =============================================================================
void allOff() {
  digitalWrite(PIN_LED_RED,    LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_GREEN,  LOW);
  digitalWrite(PIN_LASER,      LOW);
  digitalWrite(PIN_PITCH_STEP, LOW);
  digitalWrite(PIN_YAW_STEP,   LOW);
}

void updateSw(Sw &s) {
  bool r = digitalRead(s.pin);
  unsigned long now = millis();
  if (r != s.reading) { s.reading = r; s.t = now; }
  if (now - s.t >= DEBOUNCE_MS && r != s.stable) {
    s.stable = r;
  }
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

void printSw(const Sw &s) {
  Serial.print('[');
  Serial.print(s.name);
  Serial.print("] ");
  Serial.println(s.stable == HIGH ? "TRIGGERED" : "OPEN");
}

void printHelp() {
  Serial.println();
  Serial.println(F("=== GENERAL TEST MENU ==="));
  Serial.println(F("  H - help"));
  Serial.println(F("  L - LED cycle (Red,Yellow,Green)"));
  Serial.println(F("  Z - Laser blink x3"));
  Serial.println(F("  S - Switch dump (Q to quit)"));
  Serial.println(F("  P - Pitch motor sweep (limit -> release -> opposite -> release)"));
  Serial.println(F("  Y - Yaw motor sweep (same)"));
  Serial.println(F("  A - Run all (LED, Laser, Pitch, Yaw)"));
  Serial.println(F("  X - Emergency stop (requires reset)"));
  Serial.println(F("========================="));
}

// =============================================================================
// COMPONENT TESTS — passive
// =============================================================================
void ledTest() {
  Serial.println(F("[LED] Red"));
  digitalWrite(PIN_LED_RED, HIGH);    delay(LED_ON_MS);
  digitalWrite(PIN_LED_RED, LOW);
  Serial.println(F("[LED] Yellow"));
  digitalWrite(PIN_LED_YELLOW, HIGH); delay(LED_ON_MS);
  digitalWrite(PIN_LED_YELLOW, LOW);
  Serial.println(F("[LED] Green"));
  digitalWrite(PIN_LED_GREEN, HIGH);  delay(LED_ON_MS);
  digitalWrite(PIN_LED_GREEN, LOW);
  Serial.println(F("[LED] done"));
}

void laserTest() {
  Serial.println(F("[LASER] start"));
  for (int i = 0; i < LASER_BLINKS; i++) {
    digitalWrite(PIN_LASER, HIGH); delay(LASER_MS);
    digitalWrite(PIN_LASER, LOW);  delay(LASER_MS);
  }
  Serial.println(F("[LASER] done"));
}

// Switch dump runs until user sends 'Q' (or 'q'). Stray newlines/whitespace
// ignored so Serial Monitor focus changes don't auto-quit.
void switchTest() {
  Serial.println(F("[SW] dump active — press switches; send Q to quit"));
  seedSwitches();
  bool pL = swL.stable, pR = swR.stable, pU = swU.stable, pD = swD.stable;
  for (;;) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'Q' || c == 'q') { Serial.println(F("[SW] quit")); return; }
    }
    updateAll();
    if (swL.stable != pL) { pL = swL.stable; printSw(swL); }
    if (swR.stable != pR) { pR = swR.stable; printSw(swR); }
    if (swU.stable != pU) { pU = swU.stable; printSw(swU); }
    if (swD.stable != pD) { pD = swD.stable; printSw(swD); }
  }
}

// =============================================================================
// MOTOR PRIMITIVES
// =============================================================================
void singleStep(int stepPin, unsigned long pulseUs) {
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(pulseUs);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(pulseUs);
}

// Step in `dir` until one of (axis1, axis2) trips (success) or one of
// (perp1, perp2) trips (mechanical/wiring fault) or MAX_SWEEP_STEPS reached.
Outcome sweepUntilLimit(int stepPin, int dirPin, bool dir, unsigned long pulseUs,
                        Sw* axis1, Sw* axis2, Sw* perp1, Sw* perp2) {
  digitalWrite(dirPin, dir ? HIGH : LOW);
  delay(DIR_SETUP_MS);
  Outcome o = { SR_TIMEOUT, nullptr, 0 };
  for (long i = 0; i < MAX_SWEEP_STEPS; i++) {
    updateAll();
    if (triggered(*axis1)) { o.result = SR_HIT_AXIS; o.hit = axis1; o.steps = i; return o; }
    if (triggered(*axis2)) { o.result = SR_HIT_AXIS; o.hit = axis2; o.steps = i; return o; }
    if (triggered(*perp1)) { o.result = SR_HIT_PERP; o.hit = perp1; o.steps = i; return o; }
    if (triggered(*perp2)) { o.result = SR_HIT_PERP; o.hit = perp2; o.steps = i; return o; }
    singleStep(stepPin, pulseUs);
    o.steps = i + 1;
  }
  return o;
}

// Drive in `reverseDir` in chunks until `lim` releases continuously for
// RELEASE_CLEAR_MS, then back off SAFETY_MARGIN_STEPS more. Capped by
// MAX_SWEEP_STEPS to avoid infinite loops on stuck switches.
bool reverseUntilRelease(int stepPin, int dirPin, bool reverseDir,
                         unsigned long pulseUs, Sw* lim) {
  digitalWrite(dirPin, reverseDir ? HIGH : LOW);
  delay(DIR_SETUP_MS);
  long stepsTaken = 0;
  while (stepsTaken < MAX_SWEEP_STEPS) {
    for (int i = 0; i < RELEASE_CHUNK_STEPS; i++) singleStep(stepPin, pulseUs);
    stepsTaken += RELEASE_CHUNK_STEPS;
    updateAll();
    if (triggered(*lim)) continue;

    // Confirm release stays clear for RELEASE_CLEAR_MS.
    unsigned long start = millis();
    bool stayed = true;
    while (millis() - start < RELEASE_CLEAR_MS) {
      updateAll();
      if (triggered(*lim)) { stayed = false; break; }
    }
    if (!stayed) continue;

    // Confirmed released — add safety margin.
    for (int i = 0; i < SAFETY_MARGIN_STEPS; i++) singleStep(stepPin, pulseUs);
    Serial.print(F("  released after "));
    Serial.print(stepsTaken);
    Serial.print(F(" steps + margin "));
    Serial.println(SAFETY_MARGIN_STEPS);
    return true;
  }
  return false;
}

// Motor sweep test: dir HIGH -> axis limit -> reverse + release + margin ->
// dir LOW -> other axis limit -> reverse + release + margin. Halts on
// perpendicular trip, same-limit-both-dirs, timeout, or release failure.
void motorTest(const char* name, int stepPin, int dirPin, unsigned long pulseUs,
               Sw* axis1, Sw* axis2, Sw* perp1, Sw* perp2) {
  if (halted) { Serial.println(F("HALTED — reset board")); return; }
  Serial.print(F("[MOTOR ")); Serial.print(name); Serial.println(F("] start"));

  // Phase 1: sweep dir HIGH
  Serial.println(F("  sweep HIGH"));
  Outcome o1 = sweepUntilLimit(stepPin, dirPin, HIGH, pulseUs, axis1, axis2, perp1, perp2);
  if (o1.result == SR_TIMEOUT) {
    Serial.print(F("  TIMEOUT after ")); Serial.print(o1.steps);
    Serial.println(F(" steps — switch dead, motor stalled (low torque?), or mis-wired"));
    allOff(); halted = true; return;
  }
  if (o1.result == SR_HIT_PERP) {
    Serial.print(F("  PERP HIT: ")); Serial.print(o1.hit->name);
    Serial.println(F(" — wrong-axis switch tripped; check mechanical coupling / wiring"));
    allOff(); halted = true; return;
  }
  Serial.print(F("  HIT ")); Serial.print(o1.hit->name);
  Serial.print(F(" after ")); Serial.print(o1.steps); Serial.println(F(" steps"));

  if (!reverseUntilRelease(stepPin, dirPin, LOW, pulseUs, o1.hit)) {
    Serial.println(F("  RELEASE FAILED on dir A — switch stuck"));
    allOff(); halted = true; return;
  }

  delay(INTER_PHASE_PAUSE_MS);

  // Phase 2: sweep dir LOW
  Serial.println(F("  sweep LOW"));
  Outcome o2 = sweepUntilLimit(stepPin, dirPin, LOW, pulseUs, axis1, axis2, perp1, perp2);
  if (o2.result == SR_TIMEOUT) {
    Serial.print(F("  TIMEOUT after ")); Serial.print(o2.steps);
    Serial.println(F(" steps"));
    allOff(); halted = true; return;
  }
  if (o2.result == SR_HIT_PERP) {
    Serial.print(F("  PERP HIT: ")); Serial.println(o2.hit->name);
    allOff(); halted = true; return;
  }
  if (o2.hit == o1.hit) {
    Serial.print(F("  SAME LIMIT BOTH DIRS: ")); Serial.print(o2.hit->name);
    Serial.println(F(" — DIR pin not flipping or one switch is stuck HIGH"));
    allOff(); halted = true; return;
  }
  Serial.print(F("  HIT ")); Serial.print(o2.hit->name);
  Serial.print(F(" after ")); Serial.print(o2.steps); Serial.println(F(" steps"));

  if (!reverseUntilRelease(stepPin, dirPin, HIGH, pulseUs, o2.hit)) {
    Serial.println(F("  RELEASE FAILED on dir B"));
    allOff(); halted = true; return;
  }

  Serial.print(F("[MOTOR ")); Serial.print(name); Serial.println(F("] done"));
}

void emergencyStop() {
  allOff();
  halted = true;
  Serial.println(F("!! EMERGENCY STOP — reset board to resume"));
}

void runAll() {
  if (halted) { Serial.println(F("HALTED — reset board")); return; }
  Serial.println(F("[ALL] start"));
  // Skip interactive switchTest. Sanity-check no limit currently triggered.
  updateAll();
  if (triggered(swL) || triggered(swR) || triggered(swU) || triggered(swD)) {
    Serial.println(F("[ALL] abort — a limit switch is already triggered. Clear turret first."));
    return;
  }
  ledTest();
  laserTest();
  motorTest("PITCH", PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US, &swU, &swD, &swL, &swR);
  motorTest("YAW",   PIN_YAW_STEP,   PIN_YAW_DIR,   YAW_PULSE_US,   &swL, &swR, &swU, &swD);
  Serial.println(F("[ALL] done"));
}

// =============================================================================
// ARDUINO ENTRY
// =============================================================================
void setup() {
  Serial.begin(BAUD);

  pinMode(PIN_PITCH_STEP, OUTPUT);
  pinMode(PIN_PITCH_DIR,  OUTPUT);
  pinMode(PIN_YAW_STEP,   OUTPUT);
  pinMode(PIN_YAW_DIR,    OUTPUT);
  pinMode(PIN_LASER,      OUTPUT);
  pinMode(PIN_LED_RED,    OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_GREEN,  OUTPUT);

  pinMode(PIN_LIM_LEFT,   INPUT_PULLUP);
  pinMode(PIN_LIM_RIGHT,  INPUT_PULLUP);
  pinMode(PIN_LIM_UP,     INPUT_PULLUP);
  pinMode(PIN_LIM_DOWN,   INPUT_PULLUP);

  allOff();
  delay(100);
  seedSwitches();

  Serial.println();
  Serial.println(F("general_test ready."));
  printHelp();
}

void loop() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == '\n' || c == '\r' || c == ' ') return;
  c = (c >= 'a' && c <= 'z') ? c - 32 : c;

  switch (c) {
    case 'H': printHelp(); break;
    case 'L': ledTest(); break;
    case 'Z': laserTest(); break;
    case 'S': switchTest(); break;
    case 'P': motorTest("PITCH", PIN_PITCH_STEP, PIN_PITCH_DIR, PITCH_PULSE_US, &swU, &swD, &swL, &swR); break;
    case 'Y': motorTest("YAW",   PIN_YAW_STEP,   PIN_YAW_DIR,   YAW_PULSE_US,   &swL, &swR, &swU, &swD); break;
    case 'A': runAll(); break;
    case 'X': emergencyStop(); break;
    default:
      Serial.print(F("? unknown cmd: "));
      Serial.println(c);
      printHelp();
      break;
  }
}
