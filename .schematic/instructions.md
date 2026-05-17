# Instructions — Arduino Sketch Authoring

Absolute rules. Apply when creating any new Arduino sketch (`.ino`) in this repo. Derived from patterns proven in `arduino_codes/test/general_test/`.

---

## 1. Always add `config.h` and `types.h` headers

Every sketch directory **must** contain:

- **`config.h`** — every pin, every timing constant, every magic number. Zero literals in the `.ino` body.
- **`types.h`** — every user-defined `struct` or `enum` that appears in a function signature.

Pattern reference: [`arduino_codes/test/general_test/config.h`](../arduino_codes/test/general_test/config.h), [`arduino_codes/test/general_test/types.h`](../arduino_codes/test/general_test/types.h).

**Why types.h:** Arduino's preprocessor auto-generates function prototypes and inserts them *above* any user `struct`/`enum` defined in the `.ino`. A function returning a `.ino`-defined type fails with `'<Type>' does not name a type`. Headers are included before the auto-prototypes, so types in `types.h` resolve.

**Why config.h:** lets you tune (pulse widths, debounce, step counts) without touching logic, and keeps pin assignments in one auditable place.

Boilerplate of each header:
```cpp
#pragma once
#include <Arduino.h>
// ...
```

---

## 2. Pin map is authoritative in `.schematic/components.md`

- Copy pin assignments **exactly** into `config.h`. Do not invent or guess pins.
- If hardware wiring differs from `components.md`, fix `components.md` first (or the wiring). Never silently diverge in code.
- Use `constexpr int PIN_* = ...` (not `#define`) — type-checked, debugger-visible.

---

## 3. Limit switches: INPUT_PULLUP, NC→GND, HIGH = triggered

- Wire normally-closed (NC) terminal to GND. Common terminal to a pin set `INPUT_PULLUP`.
- Logic: switch closed (rest) → pulls pin LOW. Switch triggered/wire broken → pull-up wins → HIGH. **Fail-safe**.
- Debounce every read. Don't trust raw `digitalRead` for control flow.

---

## 4. Any motion that depends on a sensor needs a safety cap

- Motor sweeps "until limit triggers" must have a `MAX_SWEEP_STEPS` (or time) cap.
- On cap exceeded: stop motor, log fault, halt. Reasons it'd trigger: dead switch, broken wire, stalled motor, wrong DIR pin.
- Same rule for any "loop until external condition" pattern.

---

## 5. Driver hardware tuning beats software tuning

- For DRV8825 / stepper drivers: torque comes from **Vref pot** (current limit), then microstep mode, then supply voltage, then step rate.
- Slower pulses in software only put motor in higher-torque region of its curve — they don't increase the current limit.
- Log Vref values for each driver in `.schematic/status.md` after measuring.

---

## 6. Log state to `.schematic/status.md` after every hardware session

- Component table: mark `OK` / `BROKEN` / `?` / `NOT INTEGRATED` with a `Last verified` date.
- Move fixed issues from "Open" → "Resolved" with the date and what was done.
- Add new bugs to "Open" with: symptom, suspects, verification step.

---

## 7. Reference sketch layout (use as template)

```
arduino_codes/<group>/<sketch_name>/
├── <sketch_name>.ino   # logic only, no literals
├── config.h            # constants + pin assignments
└── types.h             # structs + enums (only if any are returned/passed in fn signatures)
```

The directory name must match the `.ino` filename (Arduino build requirement).
