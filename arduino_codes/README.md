# Arduino Firmware

Embedded C++ firmware for the laser turret's motion/laser/safety layer and for the moving target. The turret firmware runs on an **Arduino Mega 2560**; the moving-target sketch runs on an **Arduino UNO R3**.

## 🌟 Highlights

- 🧩 **Modular turret firmware** — structured into motor, laser, LED, scanning, tracking, PID, limit-recovery, and serial modules
- 🎯 **Closed-loop aiming** — per-axis PID converts camera pixel offsets into stepper motion
- 🛡️ **Hardware safety** — `INPUT_PULLUP` limit switches on all four axis ends with automatic recovery
- 🔌 **Line-delimited JSON** over USB serial at **115200 baud**, same wire format the Jetson speaks
- 🎯 **Instrumented moving target** — photoresistor-based laser-hit detection with adaptive noise thresholding

## ℹ️ Overview

```
arduino_codes/
├── development/
│   ├── main_control/          # Turret firmware (Arduino Mega 2560)
│   │   ├── main_control.ino    # setup()/loop() entry point
│   │   ├── config.h            # pins, baud, motor timing, PID gains, scan params
│   │   ├── state.*             # system state machine
│   │   ├── switches.*          # limit-switch debouncing
│   │   ├── leds.*              # status LEDs
│   │   ├── motor.*             # STEP/DIR stepper control
│   │   ├── laser.*            # laser on/off
│   │   ├── tracking.*          # pixel-offset → aim
│   │   ├── PID.*               # per-axis PID
│   │   ├── scanning.*          # autonomous search sweep
│   │   ├── limit_recovery.*    # back off after hitting a limit
│   │   ├── commands.* / dispatch.*  # parse & route incoming JSON
│   │   ├── serial_link.*       # buffered serial line reader
│   │   └── types.h
│   └── target2/target/
│       └── target.ino          # Moving-target firmware (Arduino UNO R3)
└── test/general_test/          # Standalone bring-up / bench test sketch
```

### 🔄 How it works

The main loop does three things each pass:

1. Feed incoming serial characters into the line parser (`serial_link` → `dispatch`).
2. If in `SCANNING` state, run one search-sweep tick; otherwise flush the latest target offset into motor motion.
3. Recover from any tripped limit switch and refresh the status LEDs (~10 Hz).

The Jetson pushes two kinds of messages the firmware acts on:

- `{"type":"data","key":"system_state","value":"scanning|tracking|firing|idle"}` — switches behavior
- `{"type":"data","key":"current_target_offset","value":...}` — the pixel error the PID drives to zero

### 📌 Pin map

Defined in [`development/main_control/config.h`](development/main_control/config.h):

| Function | Pin |
| --- | --- |
| Pitch STEP / DIR | 12 / 11 |
| Yaw STEP / DIR | 10 / 9 |
| Laser | 2 |
| LEDs (Red / Yellow / Green) | A0 / A1 / A2 |
| Limit switches (Left / Right / Up / Down) | A12 / A13 / A14 / A15 |

> ⚠️ These pins match the wiring in the project report's circuit diagram. If your build differs, edit `config.h` before flashing.
## 🎛️ Configuration

Most tuning lives in `config.h`. Notable knobs:

- **Motion:** `PITCH_PULSE_US`, `YAW_PULSE_US`, `MAX_STEP_PER_TICK`, `DEAD_ZONE_PX_*`
- **PID gains:** `PID_KP_YAW`, `PID_KD_YAW`, `PID_KP_PITCH`, `PID_KD_PITCH`
- **Scanning:** `SCAN_CHUNK_STEPS`, `SCAN_PULSE_US`, `MAX_SWEEP_STEPS`
- **Switch debounce:** `DEBOUNCE_MS`, `RELEASE_CLEAR_MS`

The moving-target sketch exposes its own detection tuning at the top of `target.ino` (oversampling, jump threshold, sliding-window hit/release counts).

## 🔗 Related

- [Jetson mission code](../jetson_mission_codes/) — the other end of the serial link
- Project root [README](../README.md)
