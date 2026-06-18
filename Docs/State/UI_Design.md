# UI Design — [PLUGIN NAME]

> LIVING DOCUMENT. Reflects the CURRENT approved visual design and GUI implementation state.
> Edit in place when design changes. History belongs in Changelog.md.

---

## Approved Visual Spec

- **Window size:**
- **Colour palette (hex):**
  - Background:
  - Accent:
  - Knob body:
  - Knob track:
  - Label:
- **Control style:**
- **HTML prototype approved:** Yes/No — date

---

## LookAndFeel Implementation Status

| Element | Status |
|---|---|
| Custom rotary knob | |
| Custom button | |
| Sync dot indicator | |
| Colour constants | |

---

## Component Inventory

| Component | File | Status |
|---|---|---|
| PluginEditor | `Source/GUI/PluginEditor.h/.cpp` | |
| LookAndFeel | `Source/GUI/LookAndFeel.h/.cpp` | |
| LevelMeter | | |

---

## Editor Destructor Checklist (verify every time editor changes)

- [ ] All dynamic attachments reset first
- [ ] `stopTimer()` second
- [ ] `setLookAndFeel(nullptr)` last

---

## Known UI Issues

