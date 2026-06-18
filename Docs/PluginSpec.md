# Plugin Specification — [PLUGIN NAME]

> Complete every field before Phase 1. This is the single source of truth for all skills.
> Skills reference this file directly rather than requiring you to re-paste spec content.

---

## 2.1 Project Identity

| Field | Value |
|---|---|
| Plugin Name | |
| Version Number | |
| Manufacturer Name | |
| Manufacturer Code | (4-char, e.g. LCSC) |
| Plugin Code | (4-char, e.g. PLGB) |
| Plugin Category | |
| Plugin Formats | AU + VST3 |
| Author / Developer | |

---

## 2.2 Sonic Philosophy

### Core Sonic Identity
*(2–5 sentences. What does it sound like? What does it feel like? DSP decisions flow from this.)*


### Sonic Descriptors
*(8–15 adjectives)*


### Sonic References
*(Hardware units, pedals, consoles, recordings that represent the target sound)*


### What to Avoid
*(Sounds, artifacts, or behaviours the plugin must never produce)*


### Genre / Use Case
*(Music styles and workflows this plugin is built for)*


### Emotional Goal
*(What should the user feel? What problem does it solve emotionally?)*


---

## 2.3 DSP Requirements

| Field | Value |
|---|---|
| Plugin Type | |
| Stereo Topology | True Stereo / Dual Mono / Mono-in Stereo-out / Mid-Side / Mono only |
| Oversampling | None / 2x / 4x / User-selectable |
| Latency Tolerance | (e.g. must feel instantaneous at 128-sample buffer) |

### Nonlinear Processing
*(Saturation, distortion, clipping, waveshaping)*


### Filtering / EQ
*(Filters, EQ stages, shelves, peaks, tone shaping)*


### Dynamics Processing
*(Compression, limiting, gating, expansion, envelope)*


### Modulation
*(LFO, envelope follower, pitch modulation, tempo-sync)*


### IR / Convolution
*(Cab sim, room sim, IR loading — or "none")*


### Special DSP Features
*(Spectral, granular, physical modelling, etc. — or "none")*


---

## 2.4 Controls and Parameters

For each control: Name | Range | Default | What it does to the sound

| # | Name | Range | Default | Sonic Effect |
|---|---|---|---|---|
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |
| 4 | | | | |
| 5 | | | | |
| 6 | | | | |
| 7 | | | | |
| 8 | | | | |
| 9 | | | | |
| 10 | | | | |
| 11 | | | | |
| 12 | | | | |

### Switches / Toggles
*(On/off buttons, mode selectors, bypass switches not listed above)*


### MIDI Learn
*(All / Specific parameters / None)*


---

## 2.5 UI Requirements

| Field | Value |
|---|---|
| Visual Theme | |
| Window Size | e.g. 900 × 400 px |
| Resizable | Fixed / Resizable with min-max / Aspect-ratio locked |
| Control Style | Rotary knobs / Sliders / Buttons / Mix |

### Metering
*(Input/output level, gain reduction, spectrum — or "none")*


### Visualisation
*(Waveform, EQ curve, envelope, spectrum display — or "none")*


### About Screen
*(What appears on logo click: version, author, build date)*


### Logo / Branding
*(Plugin logo description: text, fonts, style)*


---

## 2.6 Technical Requirements

| Field | Value |
|---|---|
| JUCE Path | |
| Project Output Path | |
| JUCE Version | |
| Xcode Version | |
| macOS Target | e.g. 12.0 Monterey |
| Windows Target | (leave blank if macOS only) |
| CPU Target | Universal Binary (arm64 + x86_64) |
| Preset System | User presets / Factory presets / Both / None |

---

## 2.7 Reference Plugins and Sources

For each reference: Name | What to take from it | What to avoid

| # | Plugin / Hardware | Take | Avoid |
|---|---|---|---|
| 1 | | | |
| 2 | | | |
| 3 | | | |
| 4 | | | |
| 5 | | | |

### Open Source References
*(GitHub repos, papers, or open-source code to study)*


### Academic / DSP References
*(Papers, textbooks, articles — optional)*


---

## 2.8 Target Users and Workflow

### Target Users
*(Who will use this plugin?)*


### Primary Workflow
*(How will they use it — tracking, mixing, sound design, live?)*


### Ease of Use Goal
*(Immediately intuitive vs deep and complex? Describe the dial-in experience.)*


### Psychoacoustic Goals
*(What should the listener perceive? More energy, wider space, glue, presence?)*

