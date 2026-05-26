// tracking.h — vision-driven tracking entrypoint.
// Converts a pixel offset (target − crosshair, image coords) into step deltas
// per axis and executes them via `stepDelta()`. Sign-flips pitch because image
// y+ = down but pitch + = up.
// Conversion model: integer divisor + deadzone + min-1-step floor (config.h).
#pragma once

void applyOffset(int offsetX, int offsetY);
