#include <Arduino.h>
#include "PID.h"
#include "config.h"

struct PidState {
  float         integral;
  float         carry;       // fractional step remainder — sub-1.0 outputs accumulate here
                             // until they cross ±1 and commit a step. Without this, low Kp
                             // values would silently round to 0 and the head gets stuck.
  int           prevError;
  unsigned long prevMs;
};

static PidState yaw   = {0.0f, 0.0f, 0, 0};
static PidState pitch = {0.0f, 0.0f, 0, 0};

// dt seed used on the very first call (prevMs == 0). Roughly matches the
// Python detection cadence so the initial integral/derivative aren't garbage.
static constexpr float PID_FIRST_DT_S = 0.05f;
// Upper clamp on dt to absorb long gaps (target loss, mission pause) without
// the integral/derivative spiking.
static constexpr float PID_MAX_DT_S   = 0.5f;

static long pidCompute(int error, float Kp, float Ki, float Kd,
                       int deadZone, PidState& s) {
  int mag = error < 0 ? -error : error;
  if (mag < deadZone) {
    s.integral  = 0.0f;
    s.carry     = 0.0f;
    s.prevError = error;
    s.prevMs    = millis();
    return 0;
  }

  unsigned long now = millis();
  float dt;
  if (s.prevMs == 0) dt = PID_FIRST_DT_S;
  else               dt = (now - s.prevMs) / 1000.0f;
  if (dt <= 0.0f)    dt = 0.001f;
  if (dt > PID_MAX_DT_S) dt = PID_MAX_DT_S;
  s.prevMs = now;

  s.integral += (float)error * dt;
  if (s.integral >  PID_INTEGRAL_LIMIT) s.integral =  PID_INTEGRAL_LIMIT;
  if (s.integral < -PID_INTEGRAL_LIMIT) s.integral = -PID_INTEGRAL_LIMIT;

  float derivative = ((float)(error - s.prevError)) / dt;
  s.prevError = error;

  float output = Kp * (float)error + Ki * s.integral + Kd * derivative;

  // Accumulate fractional output. Small Kp·error values (< 1.0) would otherwise
  // truncate to 0 and the head stalls just outside the deadzone.
  s.carry += output;
  long steps = (long)s.carry;        // truncates toward zero — fractional remainder kept
  s.carry  -= (float)steps;
  if (steps >  MAX_STEP_PER_TICK) steps =  MAX_STEP_PER_TICK;
  if (steps < -MAX_STEP_PER_TICK) steps = -MAX_STEP_PER_TICK;
  return steps;
}

long pidYaw(int errorPx) {
  return pidCompute(errorPx, PID_KP_YAW, PID_KI_YAW, PID_KD_YAW,
                    DEAD_ZONE_PX_YAW, yaw);
}

long pidPitch(int errorPx) {
  return pidCompute(errorPx, PID_KP_PITCH, PID_KI_PITCH, PID_KD_PITCH,
                    DEAD_ZONE_PX_PITCH, pitch);
}

void pidReset() {
  yaw   = {0.0f, 0.0f, 0, 0};
  pitch = {0.0f, 0.0f, 0, 0};
}
