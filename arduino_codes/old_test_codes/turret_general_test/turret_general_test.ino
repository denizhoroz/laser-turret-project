// MOTOR DUAL TEST — sweep mode, directional limits, safe step-back
// Motor 1 (12/11) = PITCH, Motor 2 (10/9) = YAW
// Limits NC->GND, INPUT_PULLUP. HIGH = TRIGGERED (fail-safe on broken wire)

// =============================================================================
// PARAMETER BLOCK — tune everything here
// =============================================================================

// ----- Pin assignments -----
const int PIN_PITCH_STEP   = 12;
const int PIN_PITCH_DIR    = 11;
const int PIN_YAW_STEP     = 10;
const int PIN_YAW_DIR      = 9;
const int PIN_LASER        = 2;

const int PIN_LIMIT_LEFT   = A12;
const int PIN_LIMIT_RIGHT  = A13;
const int PIN_LIMIT_UP     = A14;
const int PIN_LIMIT_DOWN   = A15;

// ----- Step pulse timing (microseconds) -----
const int PULSE_WIDTH_FAST = 3200;   // sweep step pulse — lower = faster sweep
const int PULSE_WIDTH_SLOW = 4000;   // step-back step pulse — slower, more torque

// ----- Driver timing (milliseconds) -----
const unsigned long DIR_SETUP_MS   = 50;   // pause after setting DIR before pulsing
const unsigned long POST_STALL_MS  = 500;  // pause after limit hit before step-back

// ----- Switch debounce (milliseconds) -----
const unsigned long DEBOUNCE_MS         = 50;   // bounce filter window
const unsigned long RELEASE_CLEAR_MS    = 250;  // continuous LOW required to declare released
const unsigned long RELEASE_WAIT_MS     = 500;  // max time waited per step-back chunk
const unsigned long STARTUP_SETTLE_MS   = 100;  // pin settle before seeding debounce state

// ----- Step-back parameters per motor -----
const int STEPBACK_CHUNK_PITCH = 25;    // steps per chunk before re-checking switches
const int STEPBACK_MIN_PITCH   = 100;
const int STEPBACK_MAX_PITCH   = 800;   // safety cap on total step-back distance
const int STEPBACK_CHUNK_YAW   = 50;    // yaw has heavier load — bigger bites
const int STEPBACK_MIN_YAW     = 100;
const int STEPBACK_MAX_YAW     = 2000;  // yaw needs more travel to clear switch

// ----- Laser blink timing (milliseconds) -----
const unsigned long LASER_ON_MS    = 500;
const unsigned long LASER_OFF_MS   = 500;

// ----- Misc -----
const unsigned long SERIAL_BAUD       = 115200;
const unsigned long STARTUP_DELAY_MS  = 500;   // pause after setup before first sweep
const unsigned long HALT_LOOP_MS      = 1000;  // idle delay while halted

// =============================================================================
// END PARAMETER BLOCK — code below
// =============================================================================

// Switch identifiers
enum SwitchId { SW_NONE, SW_UP, SW_DOWN, SW_LEFT, SW_RIGHT };

const char* switchName(SwitchId id) {
  switch (id) {
    case SW_UP:    return "UP";
    case SW_DOWN:  return "DOWN";
    case SW_LEFT:  return "LEFT";
    case SW_RIGHT: return "RIGHT";
    default:       return "NONE";
  }
}

// Debounce state per switch
struct Debounced {
  uint8_t pin;
  bool stable;
  bool reading;
  unsigned long lastChange;
};

void updateSwitch(Debounced &s);  // forward decl for Arduino auto-prototype

Debounced swLeft  = { PIN_LIMIT_LEFT,  false, false, 0 };
Debounced swRight = { PIN_LIMIT_RIGHT, false, false, 0 };
Debounced swUp    = { PIN_LIMIT_UP,    false, false, 0 };
Debounced swDown  = { PIN_LIMIT_DOWN,  false, false, 0 };

// Debounce mode:
//   false (sweep):       HIGH is instant — safety first
//   true  (step-back):   both edges debounced — rides out chatter
bool symmetricDebounce = false;

// Halt flag — once set, all motion stops permanently until reset
bool halted = false;

void updateSwitch(Debounced &s) {
  unsigned long now = millis();
  bool r = digitalRead(s.pin);

  if (!symmetricDebounce && r == HIGH) {
    s.reading = HIGH;
    s.stable = HIGH;
    s.lastChange = now;
    return;
  }

  if (r != s.reading) { s.reading = r; s.lastChange = now; }
  if (now - s.lastChange >= DEBOUNCE_MS && r != s.stable) {
    s.stable = r;
  }
}

void updateAllSwitches() {
  updateSwitch(swLeft);
  updateSwitch(swRight);
  updateSwitch(swUp);
  updateSwitch(swDown);
}

bool anyLimitTriggered() {
  return swLeft.stable  == HIGH ||
         swRight.stable == HIGH ||
         swUp.stable    == HIGH ||
         swDown.stable  == HIGH;
}

SwitchId firstTriggered() {
  if (swUp.stable    == HIGH) return SW_UP;
  if (swDown.stable  == HIGH) return SW_DOWN;
  if (swLeft.stable  == HIGH) return SW_LEFT;
  if (swRight.stable == HIGH) return SW_RIGHT;
  return SW_NONE;
}

void allStop() {
  digitalWrite(PIN_PITCH_STEP, LOW);
  digitalWrite(PIN_YAW_STEP,   LOW);
  digitalWrite(PIN_LASER,      LOW);
}

void halt(const char* reason) {
  allStop();
  halted = true;
  Serial.print("!! HALTED: ");
  Serial.println(reason);
  Serial.println("!! Reset board to resume.");
}

// Sweep until ANY limit triggers. Uses fast step rate.
unsigned long sweep(int stepPin, int dirPin, bool dirHigh) {
  symmetricDebounce = false;  // instant trigger during sweep
  digitalWrite(dirPin, dirHigh ? HIGH : LOW);
  delay(DIR_SETUP_MS);
  unsigned long steps = 0;
  while (!halted) {
    updateAllSwitches();
    if (anyLimitTriggered()) return steps;
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(PULSE_WIDTH_FAST);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(PULSE_WIDTH_FAST);
    steps++;
  }
  return steps;
}

// Step a fixed count, ignoring limits, at a given pulse width
void stepFixed(int stepPin, int dirPin, bool dirHigh, int count, int pulseUs) {
  digitalWrite(dirPin, dirHigh ? HIGH : LOW);
  delay(DIR_SETUP_MS);
  for (int i = 0; i < count; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(pulseUs);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(pulseUs);
  }
}

// Wait up to `windowMs` for all switches to read continuously LOW for RELEASE_CLEAR_MS.
bool waitForRelease(unsigned long windowMs) {
  unsigned long start = millis();
  unsigned long clearSince = 0;
  bool wasClear = false;

  while (millis() - start < windowMs) {
    updateAllSwitches();
    bool clearNow = !anyLimitTriggered();

    if (clearNow && !wasClear) {
      clearSince = millis();
      wasClear = true;
    } else if (!clearNow) {
      wasClear = false;
    }

    if (wasClear && millis() - clearSince >= RELEASE_CLEAR_MS) {
      return true;
    }
  }
  return false;
}

// Step back until all switches release, or fail.
bool stepBack(int stepPin, int dirPin, bool legDirHigh,
              int chunk, int maxSteps, int minSteps) {
  bool backDir = !legDirHigh;

  Serial.print("  post-stall settle ");
  Serial.print(POST_STALL_MS);
  Serial.println("ms");
  delay(POST_STALL_MS);

  symmetricDebounce = true;  // ride out chatter

  // Mandatory initial step-back — guarantees motion regardless of switch state
  Serial.print("  initial backoff ");
  Serial.print(minSteps);
  Serial.println(" steps");
  stepFixed(stepPin, dirPin, backDir, minSteps, PULSE_WIDTH_SLOW);
  int totalBacked = minSteps;

  // Check release after the mandatory backoff
  if (waitForRelease(RELEASE_WAIT_MS)) {
    Serial.print("  released after ");
    Serial.print(totalBacked);
    Serial.println(" steps");
    symmetricDebounce = false;
    return true;
  }

  Serial.print("  continuing");
  while (totalBacked < maxSteps) {
    stepFixed(stepPin, dirPin, backDir, chunk, PULSE_WIDTH_SLOW);
    totalBacked += chunk;

    if (waitForRelease(RELEASE_WAIT_MS)) {
      Serial.print(" — released after ");
      Serial.print(totalBacked);
      Serial.println(" steps");
      symmetricDebounce = false;
      return true;
    }
    Serial.print(".");
  }

  Serial.println();
  symmetricDebounce = false;
  return false;
}

void runLeg(const char* name, SwitchId expected,
            int stepPin, int dirPin, bool dirHigh,
            int sbChunk, int sbMax, int sbMin) {
  if (halted) return;

  Serial.print("Sweeping ");
  Serial.print(name);
  Serial.print(" (expecting ");
  Serial.print(switchName(expected));
  Serial.println(" switch) ...");

  unsigned long s = sweep(stepPin, dirPin, dirHigh);
  SwitchId actual = firstTriggered();

  Serial.print("  stopped after ");
  Serial.print(s);
  Serial.print(" steps; triggered: ");
  Serial.println(switchName(actual));

  if (actual == SW_NONE) {
    halt("Sweep returned with no switch triggered (impossible state)");
    return;
  }

  if (actual != expected) {
    Serial.print("  MISMATCH: expected ");
    Serial.print(switchName(expected));
    Serial.print(", got ");
    Serial.println(switchName(actual));
    halt("Direction/switch mismatch — check wiring before continuing");
    return;
  }

  if (!stepBack(stepPin, dirPin, dirHigh, sbChunk, sbMax, sbMin)) {
    halt("Step-back failed — switch did not release within safety cap");
    return;
  }
}

void blinkLaser() {
  if (halted) return;
  Serial.println("Laser ON");
  digitalWrite(PIN_LASER, HIGH);
  delay(LASER_ON_MS);
  digitalWrite(PIN_LASER, LOW);
  Serial.println("Laser OFF");
  delay(LASER_OFF_MS);
}

void setup() {
  Serial.begin(SERIAL_BAUD);

  pinMode(PIN_PITCH_STEP, OUTPUT);
  pinMode(PIN_PITCH_DIR,  OUTPUT);
  pinMode(PIN_YAW_STEP,   OUTPUT);
  pinMode(PIN_YAW_DIR,    OUTPUT);
  pinMode(PIN_LASER,      OUTPUT);
  allStop();

  pinMode(PIN_LIMIT_LEFT,  INPUT_PULLUP);
  pinMode(PIN_LIMIT_RIGHT, INPUT_PULLUP);
  pinMode(PIN_LIMIT_UP,    INPUT_PULLUP);
  pinMode(PIN_LIMIT_DOWN,  INPUT_PULLUP);

  delay(STARTUP_SETTLE_MS);
  swLeft.stable  = swLeft.reading  = digitalRead(PIN_LIMIT_LEFT);
  swRight.stable = swRight.reading = digitalRead(PIN_LIMIT_RIGHT);
  swUp.stable    = swUp.reading    = digitalRead(PIN_LIMIT_UP);
  swDown.stable  = swDown.reading  = digitalRead(PIN_LIMIT_DOWN);

  if (anyLimitTriggered()) {
    Serial.print("Startup: ");
    Serial.print(switchName(firstTriggered()));
    Serial.println(" switch already triggered. Move turret clear before running.");
    halt("Limit triggered at startup");
    return;
  }

  Serial.println("Sweep mode ready.");
  delay(STARTUP_DELAY_MS);
}

void loop() {
  if (halted) {
    delay(HALT_LOOP_MS);
    return;
  }

  // Up -> Left -> Down -> Right, then repeat.
  // Swap HIGH<->LOW in a line if a leg drives the wrong physical direction.
  // Change SW_* arg if the expected switch is wrong for your wiring.
  runLeg("UP",    SW_UP,    PIN_PITCH_STEP, PIN_PITCH_DIR, HIGH,
         STEPBACK_CHUNK_PITCH, STEPBACK_MAX_PITCH, STEPBACK_MIN_PITCH);
  blinkLaser();

  runLeg("LEFT",  SW_LEFT,  PIN_YAW_STEP,   PIN_YAW_DIR,   LOW,
         STEPBACK_CHUNK_YAW, STEPBACK_MAX_YAW, STEPBACK_MIN_YAW);
  blinkLaser();

  runLeg("DOWN",  SW_DOWN,  PIN_PITCH_STEP, PIN_PITCH_DIR, LOW,
         STEPBACK_CHUNK_PITCH, STEPBACK_MAX_PITCH, STEPBACK_MIN_PITCH);
  blinkLaser();

  runLeg("RIGHT", SW_RIGHT, PIN_YAW_STEP,   PIN_YAW_DIR,   HIGH,
         STEPBACK_CHUNK_YAW, STEPBACK_MAX_YAW, STEPBACK_MIN_YAW);
  blinkLaser();
}
