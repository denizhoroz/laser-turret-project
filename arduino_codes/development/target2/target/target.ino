// ============================================================================
// Adaptive laser-hit detector — RC-car photoresistor array (standalone UNO)
//
// One aggregate analog signal (A2) from many LDRs on a pertinax coating.
// Red LED (A1) is ON while a laser strikes the coating; Green LED (A0) = powered.
//
// DETECTION PRINCIPLE — jump RATE, not level or amplitude.
// Ground-truth captures showed the laser-hit signal is erratic: the reading makes
// large excursions (tens to >100 counts) that may be a dense oscillation OR sparse
// intermittent spikes, with the DC level wandering the whole time. What is ALWAYS
// true and is absent otherwise: many large sample-to-sample JUMPS clustered in
// time. Quiet ambient makes ~0 jumps; a car reposition makes exactly ONE (a step,
// then stable); the laser makes SEVERAL. So we count jumps in a sliding window:
//
//   d          = |reading - prevReading|           per-sample change
//   jump       = d > K_JUMP * quietJitter (floored) a "big" change
//   jumpCount  = jumps within the last WINDOW samples
//   hit  when  jumpCount >= MIN_JUMPS_ON           (hysteresis to release)
//
// This is immune to DC drift and to repositions (1 jump < threshold), works for
// both dense and intermittent laser profiles, and cannot run away: the quiet floor
// updates only on calm (non-jump) samples and is frozen while a hit is latched
// (the failure mode of the level/amplitude versions, where the signal inflated its
// own threshold).
//
// Pin layout (fixed by wiring):
//   A0 -> Green LED (+)   power indicator
//   A1 -> Red   LED (+)   hit indicator
//   A2 -> P-Res signal    aggregate LDR voltage
// ============================================================================

// --- Pins ---
constexpr uint8_t PIN_LED_GREEN = A0;
constexpr uint8_t PIN_LED_RED   = A1;
constexpr uint8_t PIN_SIGNAL    = A2;

// --- Serial (debug / diagnostics only) ---
constexpr unsigned long BAUD = 115200;   // matches main_control project BAUD

// --- Sampling / SNR ---
constexpr uint8_t       OVERSAMPLE         = 16;  // raw reads averaged per sample (~4x noise cut)
constexpr unsigned long SAMPLE_INTERVAL_MS = 10;  // non-blocking loop tick

// --- Jump detection ---
constexpr float   K_JUMP          = 8.0f;   // d > K_JUMP*quietJitter -> a jump
constexpr float   MIN_JUMP_COUNTS = 8.0f;   // absolute floor for the jump threshold (counts)
constexpr float   ALPHA_JITTER    = 0.05f;  // quiet per-sample jitter tracker (calm samples only)

// --- Sliding window / decision ---
constexpr uint8_t WINDOW_SAMPLES = 24;  // ~240 ms @ 10 ms
constexpr uint8_t MIN_JUMPS_ON   = 3;   // jumps in window to assert a hit
constexpr uint8_t MIN_JUMPS_OFF  = 1;   // jumps in window to release (hysteresis)

// --- Timing ---
constexpr unsigned long WARMUP_MS  = 800;    // seed jitter, no detection
constexpr unsigned long MAX_HIT_MS = 20000;  // backstop: re-seed if a hit sticks implausibly long

// --- Debug ---
constexpr bool          DEBUG_PRINT       = true;
constexpr unsigned long DEBUG_INTERVAL_MS = 200;

// ============================================================================
// State
// ============================================================================
enum DetectState : uint8_t { WARMUP, TRACKING };

DetectState state       = WARMUP;
float       prevReading = 0.0f;
float       quietJitter = 1.0f;   // quiet |reading-prevReading|, seeded small
bool        hit         = false;

bool    jumpRing[WINDOW_SAMPLES];  // sliding window of jump flags
uint8_t ringIdx   = 0;
uint8_t jumpCount = 0;             // live sum of jumpRing

unsigned long startMs      = 0;
unsigned long hitStartMs   = 0;
unsigned long lastSampleMs = 0;
unsigned long lastDebugMs  = 0;

// ============================================================================
// Helpers
// ============================================================================
float oversampleRead() {
  long sum = 0;
  for (uint8_t i = 0; i < OVERSAMPLE; ++i) sum += analogRead(PIN_SIGNAL);
  return (float)sum / OVERSAMPLE;
}

void enterWarmup() {
  state   = WARMUP;
  startMs = millis();
  hit     = false;
  digitalWrite(PIN_LED_RED, LOW);
  for (uint8_t i = 0; i < WINDOW_SAMPLES; ++i) jumpRing[i] = false;
  ringIdx   = 0;
  jumpCount = 0;
}

// Push a jump flag into the sliding window, keeping jumpCount in sync.
void pushJump(bool isJump) {
  jumpCount -= jumpRing[ringIdx] ? 1 : 0;
  jumpRing[ringIdx] = isJump;
  jumpCount += isJump ? 1 : 0;
  ringIdx = (ringIdx + 1) % WINDOW_SAMPLES;
}

// ============================================================================
void setup() {
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  // analog input needs no pinMode

  digitalWrite(PIN_LED_GREEN, HIGH);  // powered indicator
  digitalWrite(PIN_LED_RED, LOW);

  Serial.begin(BAUD);

  prevReading = oversampleRead();     // seed so the first delta isn't a false jump
  quietJitter = 1.0f;
  enterWarmup();
}

// ============================================================================
void loop() {
  const unsigned long now = millis();
  if (now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = now;

  const float reading = oversampleRead();
  const float d       = fabs(reading - prevReading);
  prevReading = reading;

  const float jumpTh = max(K_JUMP * quietJitter, MIN_JUMP_COUNTS);
  const bool  isJump = (d > jumpTh);
  pushJump(isJump);

  if (state == WARMUP) {
    if (!isJump) quietJitter += ALPHA_JITTER * (d - quietJitter);  // seed floor
    if (now - startMs >= WARMUP_MS) state = TRACKING;
  } else {  // TRACKING
    if (!hit) {
      // Update the quiet floor ONLY on calm samples so jumps can't inflate it.
      if (!isJump) quietJitter += ALPHA_JITTER * (d - quietJitter);

      if (jumpCount >= MIN_JUMPS_ON) {
        hit        = true;
        hitStartMs = now;
        digitalWrite(PIN_LED_RED, HIGH);
      }
    } else {
      // Hit latched: quietJitter frozen.
      if (jumpCount <= MIN_JUMPS_OFF) {
        hit = false;
        digitalWrite(PIN_LED_RED, LOW);
      } else if (now - hitStartMs > MAX_HIT_MS) {
        enterWarmup();   // stuck implausibly long -> re-seed
      }
    }
  }

  // --- Throttled diagnostics ---
  if (DEBUG_PRINT && (now - lastDebugMs >= DEBUG_INTERVAL_MS)) {
    lastDebugMs = now;
    Serial.print(F("state="));
    Serial.print(state == WARMUP ? F("WUP") : F("TRK"));
    Serial.print(F(" val="));    Serial.print(reading, 1);
    Serial.print(F(" d="));      Serial.print(d, 1);
    Serial.print(F(" jTh="));    Serial.print(jumpTh, 1);
    Serial.print(F(" jmp="));    Serial.print(jumpCount);
    Serial.print(F(" jitter=")); Serial.print(quietJitter, 2);
    Serial.print(F(" hit="));    Serial.println(hit ? 1 : 0);
  }
}
