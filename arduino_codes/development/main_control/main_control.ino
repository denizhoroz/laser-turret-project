// main_control.ino
//
// Modules in this sketch directory:
//   config.h        
//   types.h         
//   state.h/.cpp    
//   switches.h/.cpp 
//   leds.h/.cpp     
//   motor.h/.cpp    
//   laser.h/.cpp    
//   tracking.h/.cpp 
//   PID.h/.cpp      
//   scanning.h/.cpp 
//   limit_recovery.h/.cpp 
//   commands.h/.cpp 
//   serial_link.h/.cpp 
//   dispatch.h/.cpp 

#include "config.h"
#include "state.h"
#include "switches.h"
#include "leds.h"
#include "dispatch.h"
#include "limit_recovery.h"
#include "scanning.h"
#include "PID.h"

void setup() {
  Serial.begin(BAUD);

  pinMode(PIN_LED_GREEN,  OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED,    OUTPUT);

  pinMode(PIN_PITCH_STEP, OUTPUT);
  pinMode(PIN_PITCH_DIR,  OUTPUT);
  pinMode(PIN_YAW_STEP,   OUTPUT);
  pinMode(PIN_YAW_DIR,    OUTPUT);
  pinMode(PIN_LASER,      OUTPUT);
  digitalWrite(PIN_PITCH_STEP, LOW);
  digitalWrite(PIN_YAW_STEP,   LOW);
  digitalWrite(PIN_LASER,      LOW);

  pinMode(PIN_LIM_LEFT,  INPUT_PULLUP);
  pinMode(PIN_LIM_RIGHT, INPUT_PULLUP);
  pinMode(PIN_LIM_UP,    INPUT_PULLUP);
  pinMode(PIN_LIM_DOWN,  INPUT_PULLUP);

  updateLeds();
  delay(100);
  seedSwitches();
  pidReset();

  Serial.println();
  Serial.println(F("# main_control ready (delta-only mode, no homing)."));
}

void loop() {
  while (Serial.available()) {
    feedSerialChar(Serial.read());
  }

  // Run autonomous search sweep.
  if (currentState == STATE_SCANNING && !halted) {
    scanTick();
  } else {
    flushPendingOffset();
  }
  recoverFromLimits();

  // Refresh LEDs
  static unsigned long lastLedTick = 0;
  unsigned long now = millis();
  if (now - lastLedTick > 100) {
    lastLedTick = now;
    updateLeds();
  }
}
