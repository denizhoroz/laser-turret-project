#include <Arduino.h>
#include "state.h"

SystemState   currentState        = STATE_IDLE;
bool          laserFiring         = false;
bool          halted              = false;
unsigned long lastTargetSignalMs  = 0;

Sw swL = { PIN_LIM_LEFT,  false, false, 0, "left"  };
Sw swR = { PIN_LIM_RIGHT, false, false, 0, "right" };
Sw swU = { PIN_LIM_UP,    false, false, 0, "up"    };
Sw swD = { PIN_LIM_DOWN,  false, false, 0, "down"  };

int8_t        lastYawDir       = 0;
int8_t        lastPitchDir     = 0;
unsigned long lastYawStepMs    = 0;
unsigned long lastPitchStepMs  = 0;
