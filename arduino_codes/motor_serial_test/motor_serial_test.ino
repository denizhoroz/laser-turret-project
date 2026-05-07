// MOTOR SERIAL TEST
//
// Reads JSON lines from Jetson over USB serial (115200 baud), drives two
// stepper motors based on target offset, and toggles laser by is_firing.
//
// Expected incoming messages (newline-terminated):
//   {"type":"data","key":"current_target_offset","value":[x,y]}
//   {"type":"data","key":"is_firing","value":true}
//
// Requires ArduinoJson (v6+) — install via Library Manager.

#include <ArduinoJson.h>

// ---- Pins ----
const int stepPin1 = 12;   // motor 1 = yaw   (X axis)
const int dirPin1  = 11;
const int stepPin2 = 10;   // motor 2 = pitch (Y axis)
const int dirPin2  = 9;
const int laserPin = 2;

// ---- Motion params ----
const int  PULSE_WIDTH_US     = 1600;  // step pulse half-period; lower = faster
const int  DEAD_ZONE_PX       = 5;     // ignore tiny offsets (anti-jitter)
const int  PIXELS_PER_STEP_X  = 16;    // yaw : bigger = less aggressive
const int  PIXELS_PER_STEP_Y  = 8124;    // pitch: bigger divisor → less travel per px
const int  MAX_STEPS_PER_MSG  = 20;    // cap so loop stays responsive
const bool INVERT_X           = false; // flip if yaw   motor moves wrong way
const bool INVERT_Y           = true;  // flip if pitch motor moves wrong way

// ---- Serial / JSON ----
const unsigned long BAUD = 115200;
const size_t JSON_CAP    = 256;

// ---- State ----
int  offsetX  = 0;
int  offsetY  = 0;
bool isFiring = false;

void sendAck(const char* status, const char* key) {
  StaticJsonDocument<128> out;
  out["type"]   = "ack";
  out["status"] = status;
  if (key) out["key"] = key;
  serializeJson(out, Serial);
  Serial.println();
}

// Pulse one motor `steps` times in given direction.
void stepMotor(int stepPin, int dirPin, int steps, bool forward) {
  digitalWrite(dirPin, forward ? HIGH : LOW);
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(PULSE_WIDTH_US);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(PULSE_WIDTH_US);
  }
}

// Drive motor toward zero error. `err` = pixel offset (target - center).
void driveAxis(int stepPin, int dirPin, int err, int pixelsPerStep, bool invert) {
  if (abs(err) < DEAD_ZONE_PX) return;
  int steps = abs(err) / pixelsPerStep;
  if (steps < 1) steps = 1;
  if (steps > MAX_STEPS_PER_MSG) steps = MAX_STEPS_PER_MSG;
  bool forward = (err > 0);
  if (invert) forward = !forward;
  stepMotor(stepPin, dirPin, steps, forward);
}

void handleOffset(int x, int y) {
  offsetX = x;
  offsetY = y;
  driveAxis(stepPin1, dirPin1, offsetX, PIXELS_PER_STEP_X, INVERT_X);
  driveAxis(stepPin2, dirPin2, offsetY, PIXELS_PER_STEP_Y, INVERT_Y);
}

void handleFiring(bool firing) {
  isFiring = firing;
  digitalWrite(laserPin, isFiring ? HIGH : LOW);
}

void handleLine(const String& line) {
  StaticJsonDocument<JSON_CAP> doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) {
    Serial.print("{\"type\":\"error\",\"msg\":\"json_parse\",\"detail\":\"");
    Serial.print(err.c_str());
    Serial.println("\"}");
    return;
  }

  const char* type = doc["type"] | "";
  if (strcmp(type, "data") != 0) {
    sendAck("ignored", nullptr);
    return;
  }

  const char* key = doc["key"] | "";

  if (strcmp(key, "current_target_offset") == 0) {
    JsonArray v = doc["value"].as<JsonArray>();
    if (v.isNull() || v.size() < 2) {
      sendAck("bad_value", key);
      return;
    }
    handleOffset(v[0].as<int>(), v[1].as<int>());
    sendAck("ok", key);
    return;
  }

  if (strcmp(key, "is_firing") == 0) {
    handleFiring(doc["value"].as<bool>());
    sendAck("ok", key);
    return;
  }

  sendAck("unknown_key", key);
}

void setup() {
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1,  OUTPUT);
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2,  OUTPUT);
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);

  Serial.begin(BAUD);
  while (!Serial) { ; }
  Serial.println("{\"type\":\"hello\",\"node\":\"arduino\"}");
}

void loop() {
  static String buf;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      buf.trim();
      if (buf.length() > 0) handleLine(buf);
      buf = "";
    } else if (c != '\r') {
      buf += c;
      if (buf.length() > JSON_CAP) buf = ""; // overflow guard
    }
  }
}
