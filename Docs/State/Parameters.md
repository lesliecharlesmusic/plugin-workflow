# Parameters — [PLUGIN NAME]

> LIVING DOCUMENT. This is the single source of truth for parameter IDs, ranges, and smoothing.
> Parameter IDs must NEVER change after first release — see warning below.
> Edit in place when parameters are added/changed during development (pre-release only).

---

## ⚠️ Post-Release Rule

Once v1.0.0 ships, parameter IDs in this file are FROZEN. Adding a new parameter is fine
(append it). Renaming or removing an existing ID breaks every saved session and preset
in the wild. If you must deprecate one, mark it `[DEPRECATED — DO NOT REUSE THIS ID]`
and leave it in the layout for backward compatibility.

---

## Parameter Table

| ID (constexpr) | Name (UI) | Range | Default | Skew | Smoothed | Sonic Effect |
|---|---|---|---|---|---|---|
| `gain` | Gain | -24 to +24 dB | 0 | linear | yes (50ms) | |
| | | | | | | |

---

## Switches / Toggles

| ID | Name | Type | Default |
|---|---|---|---|
| | | bool/choice | |

---

## MIDI Learn Map

| Parameter ID | CC Assignable | Notes |
|---|---|---|

---

## Smoothing Assignments

| Parameter | SmoothedValue Type | Time Constant | Rationale |
|---|---|---|---|

---

## Version Sentinel

```cpp
constexpr int kParamLayoutVersion = 1;
```
Bump only if the parameter layout structure itself changes in a way that affects
deserialization (rare — usually layout is additive and doesn't need a bump).

