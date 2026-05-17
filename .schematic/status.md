# Project Status

Living context. Updated as work progresses. Snapshot of *what is actually true right now* — not history, not plans. For history see "Recent changes" at bottom.

**Last updated:** 2026-05-17

---

## Component state (hardware)

Source of truth for what works on the bench. Update after every test run.

| Component | State | Last verified | Notes |
| --- | --- | --- | --- |
| Arduino MEGA | OK | 2026-05-17 | runs old sketch fine; new sketch fails compile (see Open #2) |
| Jetson Orin Nano | NOT INTEGRATED | — | not yet connected via USB |
| 12V AC-DC PSU | OK | 2026-05-17 | feeds motor drivers |
| DRV8825 #1 (pitch) | OK | 2026-05-17 | Vref **0.35 V** → Imax ≈ 0.7 A (low — see Pending) |
| DRV8825 #2 (yaw)   | OK | 2026-05-17 | Vref **0.75 V** → Imax ≈ 1.5 A (near rated 1.7 A) |
| Stepper NEMA 17 (pitch) | ? | ? | not tested with new sketch yet |
| Stepper NEMA 17 (yaw) | ? | ? | not tested with new sketch yet |
| Red LED (A0) | OK | 2026-05-17 | |
| Yellow LED (A1) | OK | 2026-05-17 | |
| Green LED (A2) | **BROKEN** | 2026-05-17 | no light on `L` cmd — see Open #1 |
| Limit switch LEFT (A12) | OK | 2026-05-17 | NC→GND wiring confirmed |
| Limit switch RIGHT (A13) | OK | 2026-05-17 | |
| Limit switch UP (A14) | OK | 2026-05-17 | |
| Limit switch DOWN (A15) | OK | 2026-05-17 | |
| Laser 5V 650nm | OK | 2026-05-17 | |
| IMX179 camera | NOT INTEGRATED | — | |
| Ground Control System | NOT INTEGRATED | — | |

**Legend:** `OK` working as expected · `BROKEN` confirmed fault · `?` unknown / untested · `NOT INTEGRATED` hardware present but not wired/code'd yet

---

## Software state

| Item | State | Notes |
| --- | --- | --- |
| `arduino_codes/test/general_test/general_test.ino` | COMPILES (untested on board) | fix in Resolved 2026-05-17 |
| `arduino_codes/test/general_test/config.h` | NEW | all pins + tunables centralized |
| `arduino_codes/test/general_test/types.h` | NEW | `Sw`, `SweepResult`, `Outcome` — moved out of `.ino` to defeat Arduino auto-prototype bug |
| Pitch pulse half-period | `2000 µs` | tune in config.h |
| Yaw pulse half-period | `2000 µs` | tune in config.h |
| Microstepping mode | ? | DRV8825 MS1/MS2/MS3 jumper state unknown |

---

## Open issues

### #1 — Green LED not working
- Wired to **A2** per `.schematic/components.md`. Sketch constant `PIN_LED_GREEN = A2` matches — software ruled out.
- Symptom: no light when `L` command reaches the green phase.
- Investigate: wire continuity, polarity, current-limit resistor, A2 pin itself.
- **Verify fix:** swap LED leads onto A0 (red slot) — if green LED now lights → LED + wiring OK, A2 pin bad. If still dark → LED or its resistor bad.

---

## Resolved issues

*(Fixed problems move here from "Open" with a date. Acts as project memory.)*

### Code (2026-05-17) — Fixed `'Outcome' does not name a type` compile error
- Cause: Arduino's preprocessor auto-generates function prototypes and inserts them at the top of the `.ino`, **above** any user-defined `struct` in the `.ino` body. Functions returning `Outcome` got a prototype before `struct Outcome` was parsed.
- Fix: moved `struct Sw`, `enum SweepResult`, `struct Outcome` into new `types.h`. Header is `#include`d so types resolve before auto-prototypes are inserted.
- **Verify fix:** `arduino-cli compile --fqbn arduino:avr:mega arduino_codes/test/general_test` returns 0.

### Code (2026-05-17) — Externalized constants to `config.h`
- All `PIN_*`, `*_MS`, `*_US`, `*_STEPS` moved out of `.ino`. Tune without touching logic.

### Code (2026-05-17) — Split pitch/yaw pulse, raised for torque
- `PITCH_PULSE_US`, `YAW_PULSE_US` separate in config.h. Doubled from 1000 → 2000 (slower → more torque from motor's torque curve).
- **Caveat:** real torque control is DRV8825 Vref pot, not software. See "Pending" if motor stalls anyway.

### Code (2026-05-17) — Switch dump quit on `Q`
- `switchTest()` now loops until user sends `Q`/`q`. Stray whitespace ignored. `SWITCH_DUMP_MS` removed.

### Code (2026-05-17) — Motor test = limit-driven sweep
- Replaced fixed 200-step move with `sweepUntilLimit()` + `reverseUntilRelease()`.
- Safety: `MAX_SWEEP_STEPS = 20000` cap, perpendicular-switch watch (pitch test halts on LEFT/RIGHT trip and vice versa), same-limit-both-dirs detection (DIR pin fault).
- Back-off: release + `SAFETY_MARGIN_STEPS = 100` both endpoints.

---

## Pending decisions / unknowns

- **Pitch Vref is low (0.35 V → 0.7 A).** Yaw is 2× that (0.75 V → 1.5 A). Pitch stepper rated 1.7 A. Likely cause of original "low torque" symptom. Decide:
  - Raise pitch Vref (recommend ~0.75 V to match yaw) — quick fix, more torque.
  - Or leave low if pitch axis is intentionally lighter (less mass, no overheat margin).
  - **Caveat:** Vref → Imax formula assumes 0.1 Ω sense resistors (`Imax = 2 × Vref`). Some DRV8825 clone boards use different sense Rs. Confirm board variant before trusting numbers.
- Microstepping jumper state (MS1/MS2/MS3) on each driver — unknown.
- Mechanical motor-coil wiring → which DIR value (HIGH/LOW) trips which axis limit? Discovered first time motor test runs.

---

## Recent changes

- **2026-05-17** Logged Vref values: pitch 0.35 V (≈0.7 A), yaw 0.75 V (≈1.5 A). Pitch low — flagged in Pending as likely torque culprit.
- **2026-05-17** Fixed compile error (`'Outcome' does not name a type`) by moving structs to `types.h`. Sketch now compiles.
- **2026-05-17** Confirmed working on bench: Red LED, Yellow LED, Laser, all 4 limit switches, both DRV8825s, 12V PSU, MEGA. Updated component table.
- **2026-05-17** Renamed `current-problems.md` → `status.md`, restructured as living context.
- **2026-05-17** Applied fixes from old problems list to `general_test.ino`. Created `config.h`.
- **2026-05-17** First hardware run revealed green LED dead. Logged as Open #1.
- **2026-05-17** Wrote initial `general_test.ino` covering all components via serial menu.
