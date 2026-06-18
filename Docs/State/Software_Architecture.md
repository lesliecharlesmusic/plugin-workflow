# Software Architecture — [PLUGIN NAME]

> LIVING DOCUMENT. Edit in place when architecture changes. Do not append history here.
> History belongs in Changelog.md.

---

## Folder & File Structure

```
Source/
├── Core/
├── DSP/
├── Parameters/
├── GUI/
└── Presets/
```
*(keep this synced with what actually exists on disk)*

---

## Class Inventory

| Class | File | Single Responsibility | Thread |
|---|---|---|---|
| | | | |

---

## Threading Model

```
AUDIO THREAD              MESSAGE THREAD
processBlock()    ←——→   Editor / APVTS listeners
DSP classes               PresetManager
SmoothedValues             LookAndFeel
```

**Cross-thread communication:**

---

## APVTS Layout

- **Naming convention:**
- **Parameter ID file:** `Source/Parameters/ParameterIDs.h`
- **Smoothing assignments:** see `Docs/State/Parameters.md`

---

## Ownership & Memory Model

- **Destructor ordering rationale:**
- **Dynamic attachment lifetime:**

---

## Oversampling Integration Point

- **Owner of `juce::dsp::Oversampling` object:**
- **Where in signal chain:**
- **Latency reporting:**

---

## Current Implementation Status

| Component | Status |
|---|---|
| CMake skeleton | |
| Parameter system | |
| Core DSP | |
| GUI | |
| Presets | |

---

## Known Architectural Debt

*(Be honest. This is reviewed in the Architecture Review.)*

