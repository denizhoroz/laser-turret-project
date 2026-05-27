// limit_recovery.h — autonomous backoff from triggered limit switches.
//
// Called from the main loop every iteration. If any limit is currently
// triggered, steps the corresponding motor opposite by the per-axis safety
// margin to free it. Runs independently of incoming offset data so the head
// can self-recover when a false detection (sky/ground) drives it into a stop.
#pragma once

void recoverFromLimits();
