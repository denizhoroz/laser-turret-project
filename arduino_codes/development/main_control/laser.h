// laser.h — fire control entrypoint.
// Coordinates PIN_LASER, parallax `aim()` shift, and LED refresh on edges.
// Idempotent: no-op when `on` matches the current `laserFiring` state.
#pragma once

void setFiring(bool on);
