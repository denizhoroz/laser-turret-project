// PID.h — per-axis PID controller mapping pixel-offset error → step delta.
// Replaces the old divisor model in tracking.cpp. State is module-static and
// kept separate per axis (yaw / pitch). Tune gains in config.h.
#pragma once

// Zero integrators + prev-error history. Call once from setup().
void pidReset();

// Returns signed step delta for the given pixel error, saturated to
// ±MAX_STEP_PER_TICK. Outputs 0 (and resets the integrator) when |error|
// is inside the per-axis deadzone.
long pidYaw(int errorPx);
long pidPitch(int errorPx);
