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
| DRV8825 #1 (pitch) | OK | 2026-05-17 | Vref **0.78 V** → Imax ≈ 1.56 A (assuming R100 sense) — empirically tuned to clear limit lever |
| DRV8825 #2 (yaw)   | OK | 2026-05-17 | Vref **0.85 V** → Imax ≈ 1.70 A (at rated) — empirically tuned |
| Stepper NEMA 17 (pitch) | OK | 2026-05-17 | moves on `P` cmd · range **~100°** between limits |
| Stepper NEMA 17 (yaw) | **DIR FLAKY** | 2026-05-17 | works on pitch axis; yaw `Y` still trips SAME LIMIT BOTH DIRS — pin 9 wire intermittent. See Open #2. |
| Red LED (A0) | OK | 2026-05-17 | |
| Yellow LED (A1) | OK | 2026-05-17 | |
| Green LED (A2) | OK | 2026-05-17 | fixed — wire was loose at A2 pin |
| Limit switch LEFT (A12) | OK | 2026-05-17 | NC→GND wiring confirmed |
| Limit switch RIGHT (A13) | OK | 2026-05-17 | |
| Limit switch UP (A14) | OK | 2026-05-17 | |
| Limit switch DOWN (A15) | OK | 2026-05-17 | |
| Laser 5V 650nm | **BROKEN** | 2026-05-17 | dead — see Open #1 (burned resistor suspected) |
| IMX179 camera | NOT INTEGRATED | — | |
| Ground Control System | NOT INTEGRATED | — | |

**Legend:** `OK` working as expected · `BROKEN` confirmed fault · `?` unknown / untested · `NOT INTEGRATED` hardware present but not wired/code'd yet

---

## Mechanical envelope

Direct drive (no gearbox), **full-step mode** (DRV8825 MS1/MS2/MS3 all LOW / no jumpers). Motor 17HS4401 = 1.8°/step = 200 steps/rev.

| Axis | Range | Steps (full-step) | Switches |
| --- | --- | --- | --- |
| Yaw   | **270°** | **150 steps** | LEFT (A12) ↔ RIGHT (A13) |
| Pitch | **~100°** | **~56 steps** | UP (A14) ↔ DOWN (A15) — pitch range approximate, refine when measured |

Resolution: 1 step = 1.8° on the turret. Low resolution — if finer aim needed later, enable microstepping (jumpers + change `*_PULSE_US`).

---

## Software state

| Item | State | Notes |
| --- | --- | --- |
| `arduino_codes/test/general_test/general_test.ino` | COMPILES (untested on board) | fix in Resolved 2026-05-17 |
| `arduino_codes/test/general_test/config.h` | NEW | all pins + tunables centralized |
| `arduino_codes/test/general_test/types.h` | NEW | `Sw`, `SweepResult`, `Outcome` — moved out of `.ino` to defeat Arduino auto-prototype bug |
| Pitch pulse half-period | `2000 µs` | tune in config.h |
| Yaw pulse half-period | `2000 µs` | tune in config.h |
| Microstepping mode | **FULL STEP** | MS1/MS2/MS3 all LOW (no jumpers) on both drivers — 1.8°/step |

---

## Open issues

### #2 — Yaw DIR signal dead (MEGA pin 9 or driver DIR input)
- **Confirmed** 2026-05-17 by hardened sanity check: `RELEASE FAKE: RIGHT re-triggered in sanity window — DIR pin likely not switching`. Motor physically can't reverse.
- Third recurrence of pin-9 fault this session. Diagnosis chain:
  1. First incident: wire popped out → re-seated → worked.
  2. Second: re-seated again after recurrence.
  3. Third: re-seat ineffective. Suggests wire internally broken (cheap jumper failure mode) or MEGA pin 9 / driver DIR input damaged.
- **Fix paths (any one):**
  - **A.** Replace pin-9 jumper with a fresh one. Cheapest. Confirms wire vs. pin/driver.
  - **B.** Move yaw DIR to a different MEGA digital pin (e.g. pin 8). Single `PIN_YAW_DIR` change in `config.h` + move wire. Isolates whether MEGA pin 9 is dead.
  - **C.** Test driver DIR input directly: feed DIR pin a manual 5V/GND, send STEP pulses with `Y`, watch direction. Confirms driver health.
- **Verify fix:** `Y` should print `HIT RIGHT` → `released after N + margin 8` → no `RELEASE FAKE` → `HIT LEFT` → done.

### #1 — Laser not firing
- Symptom: `Z` cmd runs, no beam emitted.
- Diagnosed scope: fault is inside the **laser module circuit itself**, not MEGA pin 2, not the sketch, not wiring path.
- Suspect: current-limit resistor(s) in the laser module **burned out** (over-current or sustained-on damage).
- **Verify suspicion:** measure resistance across the suspect resistor(s); compare to expected value or to an untouched module. Visible discoloration / burn smell on the PCB also a tell.
- **Fix path:** replace burned resistor (if identifiable) OR replace the whole 5V 650 nm laser module. Add an external series resistor or proper driver before re-powering, to prevent repeat.
- Workflow until fixed: skip `Z` in test runs; `A` (run-all) will still attempt it harmlessly.

---

## Resolved issues

*(Fixed problems move here from "Open" with a date. Acts as project memory.)*

### Hardware (2026-05-17) — Motors stalling at limit switches (buzz, no rotation)
- Symptom: both motors hit limit switch then buzzed in place — STEP pulses firing but rotor not rotating. Pitch worse than yaw. Triggered false `SAME LIMIT BOTH DIRS` because lever chatter slipped past new RELEASE FAKE check.
- Cause: switch lever doubles as mechanical end-stop. Reversing off the lever requires extra torque; driver Vref was too low (pitch 0.35 V → 0.7 A vs rated 1.7 A). Motor stalled instead of backing off.
- Fix: raised Vref empirically — **pitch 0.35 → 0.78 V**, **yaw 0.75 → 0.85 V**. Both motors now back off cleanly.
- **Verify fix:** `P` and `Y` both complete full sweep cycle, print `HIT <axis-A>` → release → `HIT <axis-B>` → release → done.
- **Lesson:** lever-as-stop loads the motor at trip point. If software-only torque control (slower pulse) isn't enough, raise Vref before suspecting code.

### Hardware (2026-05-17) — Yaw motor not responding
- Symptom: `Y` cmd printed `[MOTOR YAW] start` then silence. Pitch worked fine. Driver cold, coil wires firm, SLEEP/RESET jumper present.
- Cause: signal wires from MEGA pins **9 (DIR)** and **10 (STEP)** had popped out at the breadboard / driver side.
- Fix: re-seated wires.
- **Lesson:** if motor silent + pitch fine + driver cold → check signal-side wiring before suspecting driver.

### Hardware (2026-05-17) — Green LED not lighting (Open #1)
- Symptom: no light on `L` command when reaching green phase.
- Cause: LED wire at **A2** was loose / not properly seated.
- Fix: re-seated wire.
- **Lesson:** check pin seating first for any single-component dead-channel symptom — cheaper than swap tests.

### Code (2026-05-17) — One-shot post-margin sanity passed false-positive on switch chatter
- Symptom on yaw `Y`: release reported success after 10 steps + 8 margin, but phase 2 immediately tripped same switch. Single-read sanity check happened to catch a momentary LOW reading during switch chatter while motor was still pressed against lever.
- Fix: replaced single-read with **sustained-LOW window** (`RELEASE_CLEAR_MS` continuous) — chatter can't slip through a 250 ms watch. Also watches axisOther + perp throughout.
- Additionally added a **PRE-SWEEP check** in `motorTest()` after `INTER_PHASE_PAUSE_MS`: if the limit from phase 1 has re-engaged during the pause, fault before starting phase 2.
- **Verify fix:** if DIR pin is genuinely intermittent, error now prints `RELEASE FAKE` (mid-test) or `PRE-SWEEP FAULT` (after pause) instead of misleading `SAME LIMIT BOTH DIRS`.

### Code (2026-05-17) — Backoff chunked too coarsely for narrow-range axis (pitch)
- Symptom on pitch `P`: `HIT UP after 28 steps` → `OVERSHOOT during backoff: DOWN tripped`. Backoff was sliding through the entire axis range before UP had a chance to release.
- Cause: `RELEASE_CHUNK_STEPS = 25` in `reverseUntilRelease()`. With pitch range ~56 steps and switch hysteresis ~5–10 steps, three chunks = 75 steps backoff = past DOWN trigger. Switch checks only between chunks, not within.
- Sweep already steps + checks per-step. Backoff didn't. Asymmetric.
- Fix: `RELEASE_CHUNK_STEPS = 1` in `config.h` — per-step check during backoff, matching sweep. Plus diagnostic messages now print step count at fault.
- **Verify fix:** `P` should now print `HIT UP after N` → `released after M steps + margin 3` → `HIT DOWN after K` → `released ... + margin 3` → `done`, where M is small (~5–20).

### Code (2026-05-17) — `SAFETY_MARGIN_STEPS` was overshooting axis range
- Old shared value `100 steps` (~180°) exceeded both axis ranges (pitch 56 steps, yaw 150 steps). After hitting a limit and "releasing", motor was driven through the entire axis and into the opposite limit / mechanical stop.
- Pitch evidence: hit UP after 24 steps, "released" 50 + margin 100 = 150 steps. Range only 56. Motor rammed past DOWN. Next sweep tripped DOWN after just 8 steps (already nearly there).
- Fix: split per-axis in `config.h` — `PITCH_SAFETY_MARGIN_STEPS = 3` (~5.4°), `YAW_SAFETY_MARGIN_STEPS = 8` (~14.4°). Threaded through `motorTest()` and `reverseUntilRelease()`.

### Code (2026-05-17) — `reverseUntilRelease` had no overshoot / DIR-fault detection
- Old version only checked the limit being released. If motor overshot to the opposite end of the same axis, or perpendicular axis tripped, no halt.
- Also: if DIR pin didn't actually reverse (loose pin 9), motor pushed past the lever, switch chattered LOW briefly, release passed falsely — then next sweep tripped the same switch (`SAME LIMIT BOTH DIRS`).
- Fix: `reverseUntilRelease()` now watches (a) opposite-axis limit during chunked backoff and margin steps, (b) perpendicular pair throughout, (c) post-margin sanity check — if releasing limit re-triggers after backoff, fault as "RELEASE FAKE — DIR pin likely not switching".

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

- **DRV8825 sense resistor variant unconfirmed.** Vref → Imax math assumes R100 (0.1 Ω → `Imax = 2 × Vref`). If boards are actually R050 (0.05 Ω), real currents are double the logged values. Read silkscreen on the SMD resistors near motor terminals when convenient. Empirical tune worked either way; this is for record-keeping.
- **Mechanical: limit switch lever doubles as hard stop.** Works with current Vref but fragile — if Vref drifts or motor heats up, stall returns. Long-term fix: reposition switches to trip before mechanical end, giving free retract travel.
- DIR/coil-mapping (which DIR value trips which switch) — now empirically known from working `P` / `Y` runs but not documented. Capture in `.schematic/components.md` later.

---

## Recent changes

- **2026-05-17** Sanity check confirmed yaw DIR is genuinely dead — `RELEASE FAKE` fires correctly. Open #2 updated with 3 fix paths (replace jumper / move pin / test driver). No further software fix possible.
- **2026-05-17** Hardened release sanity → sustained-LOW window. Added PRE-SWEEP check after inter-phase pause. Logged Open #2 (yaw DIR pin 9 intermittent — physical fix pending).
- **2026-05-17** Pitch test confirmed clean on bench after per-step backoff fix.
- **2026-05-17** Backoff per-step (chunk 25 → 1). Pitch no longer overshoots through entire range before UP releases.
- **2026-05-17** Vref raised: pitch 0.35 → 0.78 V, yaw 0.75 → 0.85 V. Motors no longer stall at limit switches. Both axes complete full sweep cycle. Open #2 closed.
- **2026-05-17** Logged Open #2: yaw DIR signal (pin 9) intermittently lost. Needs physical re-seat.
- **2026-05-17** Split `SAFETY_MARGIN_STEPS` per-axis (pitch 3, yaw 8). Added overshoot + DIR-fault detection to `reverseUntilRelease`.
- **2026-05-17** Laser dead — suspected burned resistor in module circuit. Logged as Open #1. MEGA pin 2 + sketch + wiring path ruled out.
- **2026-05-17** Confirmed full-step mode (no microstep jumpers), direct drive. Computed total steps per axis: yaw 150, pitch ~56.
- **2026-05-17** Logged mechanical range: yaw 270°, pitch ~100° between limits. Added Mechanical envelope section.
- **2026-05-17** Yaw motor revived: re-seated loose DIR/STEP wires on MEGA pins 9/10. Moved to Resolved.
- **2026-05-17** Green LED revived: re-seated loose A2 wire. Open #1 closed.
- **2026-05-17** Logged Vref values: pitch 0.35 V (≈0.7 A), yaw 0.75 V (≈1.5 A). Pitch low — flagged in Pending as likely torque culprit.
- **2026-05-17** Fixed compile error (`'Outcome' does not name a type`) by moving structs to `types.h`. Sketch now compiles.
- **2026-05-17** Confirmed working on bench: Red LED, Yellow LED, Laser, all 4 limit switches, both DRV8825s, 12V PSU, MEGA. Updated component table.
- **2026-05-17** Renamed `current-problems.md` → `status.md`, restructured as living context.
- **2026-05-17** Applied fixes from old problems list to `general_test.ino`. Created `config.h`.
- **2026-05-17** First hardware run revealed green LED dead. Logged as Open #1.
- **2026-05-17** Wrote initial `general_test.ino` covering all components via serial menu.
