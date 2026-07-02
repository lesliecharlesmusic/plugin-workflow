# UI Implementation — [PLUGIN NAME]

> LIVING DOCUMENT. The JUCE build spec — bridges the approved `Docs/State/UI_Design.md` +
> HTML prototype and the C++ in `Source/GUI/`. Edit in place as components are built.
> History belongs in `Changelog.md`.
> No visual design decisions happen here — those already happened in `UI_Design.md` and
> the approved prototype. This document is translation-ready: once it's approved, JUCE
> code is pure translation of an already-locked design.

---

## Prerequisites

- [ ] HTML prototype approved in `Docs/State/UI_Design.md`
- [ ] DSP stable — Phase 5 complete (parameters exist to attach to)
- [ ] Fonts/images embedded in BinaryData (see `Build_Config.md`)

---

## File-by-File Implementation Order

1. `Source/GUI/LookAndFeel.h` / `.cpp`
2. `Source/GUI/PluginEditor.h` / `.cpp`
3. `Source/GUI/[CustomComponent].h` / `.cpp` *(metering, spectrum, etc. — as needed)*

---

## Component Inventory

| Component | File | Status |
|---|---|---|
| PluginEditor | `Source/GUI/PluginEditor.h/.cpp` | |
| LookAndFeel | `Source/GUI/LookAndFeel.h/.cpp` | |
| LevelMeter | | |

---

## Colour / Font Constants

*(Pulled directly from the approved HTML prototype in `UI_Design.md` — literal value +
symbol name, so they can be self-served without a round trip)*

```cpp
static constexpr uint32_t kBackground = 0xFF______;
static constexpr uint32_t kAccent     = 0xFF______;
```

---

## Data Flow (Processor ↔ Editor)

*(APVTS attachments, timer-driven meter reads, atomic reads — what crosses the thread
boundary and how)*

---

## Verification Checklist

- [ ] Editor destructor resets all dynamic attachments **FIRST**
- [ ] Destructor order: (1) reset attachments → (2) `stopTimer()` → (3) `setLookAndFeel(nullptr)`
- [ ] Screenshot-audit passed against the locked prototype (see `plugin-ui/SKILL.md`)
- [ ] All drawing uses float coordinates (HiDPI-safe)

---

## Known Implementation Issues

*(Anything unresolved — keep this section honest and current)*
