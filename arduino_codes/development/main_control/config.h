// config.h

#pragma once
#include <Arduino.h>

// PINS 
constexpr int PIN_PITCH_STEP  = 12;
constexpr int PIN_PITCH_DIR   = 11;
constexpr int PIN_YAW_STEP    = 10;
constexpr int PIN_YAW_DIR     = 9;
constexpr int PIN_LASER       = 2;

constexpr int PIN_LED_RED     = A0;
constexpr int PIN_LED_YELLOW  = A1;
constexpr int PIN_LED_GREEN   = A2;

constexpr int PIN_LIM_LEFT    = A12;
constexpr int PIN_LIM_RIGHT   = A13;
constexpr int PIN_LIM_UP      = A14;
constexpr int PIN_LIM_DOWN    = A15;

// SERIAL
constexpr unsigned long BAUD = 115200;

// SWITCHES
constexpr unsigned long DEBOUNCE_MS      = 20;
constexpr unsigned long RELEASE_CLEAR_MS = 250; 

// MOTORS
constexpr unsigned long PITCH_PULSE_US = 2000;
constexpr unsigned long YAW_PULSE_US   = 2000;

constexpr unsigned long DIR_SETUP_MS         = 1;      
constexpr long          MAX_SWEEP_STEPS      = 20000;  

constexpr int           PITCH_SAFETY_MARGIN_STEPS = 8;  
constexpr int           YAW_SAFETY_MARGIN_STEPS   = 8;  

constexpr int           RELEASE_CHUNK_STEPS  = 1;      
constexpr unsigned long INTER_PHASE_PAUSE_MS = 500;

constexpr int  SERIAL_LINE_BUF   = 256;

// VISION TRACKING
constexpr int  DEAD_ZONE_PX_YAW    = 4;
constexpr int  DEAD_ZONE_PX_PITCH  = 4;
constexpr long MAX_STEP_PER_TICK   = 20;

// PID gains
constexpr float PID_KP_YAW    = 0.030f;
constexpr float PID_KI_YAW    = 0.000f;
constexpr float PID_KD_YAW    = 0.010f;
constexpr float PID_KP_PITCH  = 0.030f;
constexpr float PID_KI_PITCH  = 0.000f;
constexpr float PID_KD_PITCH  = 0.010f;
constexpr float PID_INTEGRAL_LIMIT = 100.0f;

// Auto-state for yellow LED
constexpr unsigned long TARGET_SIGNAL_STALE_MS = 1500;

// SCANNING
constexpr int  SCAN_CHUNK_STEPS = 4;
constexpr unsigned long SCAN_PULSE_US = 16000;
constexpr long PITCH_LEVEL_FROM_DOWN_STEPS = 20;
