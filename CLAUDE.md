# CLAUDE.md — [PLUGIN NAME] Plugin Project

> Fill every [BRACKETED FIELD] before first use. This file is always in context in Claude Code.
> Skills are loaded on demand — CLAUDE.md routes to them. Do not paste skill content here.

---

## 1. Project Identity

| Field | Value |
|---|---|
| Plugin Name | [PLUGIN NAME] |
| Manufacturer | [MANUFACTURER NAME] |
| Author / Developer (if different from Manufacturer) | [used for About-screen credit] |
| Manufacturer Code | [4-CHAR e.g. LCSC] |
| Plugin Code | [4-CHAR e.g. PLGB] |
| Version | [e.g. 1.0.0] |
| Category | [Distortion / EQ / Compressor / Reverb / Delay / Modulation / Synth] |
| Formats | AU + VST3 |
| JUCE Path | [/Users/you/DEV/JUCE] |
| JUCE Version | [8.0.12] |
| Project Root | [/Users/you/DEV/PluginName] |
| Build Output | [/Users/you/DEV/PluginName/Build — same root as Project Root by default; confirmed at start of Phase 2] |
| AU Install Path | [~/Library/Audio/Plug-Ins/Components/] |
| VST3 Install Path | [~/Library/Audio/Plug-Ins/VST3/] |
| macOS Target | [12.0] |
| Xcode Version | [16.x] |
| CPU Target | Universal Binary (arm64 + x86_64) |
| Windows Target | [10 / 11, x64 — VST3 only, built via GitHub Actions, no local Windows toolchain] |

---

## 2. Current Status

```
CURRENT PHASE : [0]   ← one line of state, updated every session
PHASE STATUS  : [NOT STARTED / IN PROGRESS / COMPLETE]
BLOCKING ISSUE: [none / describe if blocked]
```

This is the ONLY thing about "phase" that is stateful. The phase roadmap itself
(Section 3 below) is fixed and never changes — it's the process, not the state.

---

## 3. Phase Roadmap & Skill Router (Fixed Process — Never Edit This Table)

This describes the process, not the project's current state. It never needs updating.
Claude reads this to know which skill to load and what each phase's gate condition is.
**Never skip a phase. Never load a skill for a future phase.**

| Phase | Name | Skill to Load | Gate Before Starting |
|---|---|---|---|
| 0 | Project Setup | *(no skill — Section 6 below)* | Toolchain installed |
| 1A | DSP Design | `dsp-research` | Phase 0 complete |
| 1B | Software Architecture | `plugin-architecture` | DSP_Design.md approved |
| 2 | CMake + JUCE Skeleton | `build-system` | DSP_Design.md + Software_Architecture.md exist |
| 3 | Parameter Architecture | `plugin-architecture` | Phase 2 compiles, Build_Config.md current |
| 4 | DSP Implementation | `dsp-implementation` | Parameters.md exists and approved |
| 5 | Advanced DSP | `dsp-implementation` | Phase 4 DSP classes implemented + unit tested |
| 6 | UI Design (HTML) | `plugin-ui` | Phase 5 DSP stable |
| 7 | UI Implementation | `plugin-ui` | HTML prototype approved in UI_Design.md |
| 8 | Preset System | `plugin-architecture` | Phase 7 GUI complete |
| 9 | Optimization | `plugin-optimization` | Presets.md current, unit tests passing |
| 10 | QA & DAW Validation | `plugin-qa` | Golden reference tests passing (`dsp-testing`) |
| B | Beta Testing | *(no skill — `Docs/BetaTesting.md`)* | Phase 10 hard blockers pass |
| R | Release & Distribution | `plugin-release` | Beta exit criteria met |

**Testing (continuous, not a phase):** Load `dsp-testing` after any DSP file change, and mandatorily before Phase R.
**Debugging (any phase):** Load `plugin-debugging`
**Pre-code gates:** Phase 4 opens by producing and getting approval on `Docs/State/DSP_Implementation.md`
(engine-by-engine build spec) before any DSP `.cpp` is generated. Phase 7 opens the same way with
`Docs/State/UI_Implementation.md`. Neither is optional, regardless of plugin size — see § 8.

---

## 4. Non-Negotiable Rules (Always Enforced)

These apply in every phase, every file, every session:

### Audio Thread
- Zero heap allocations in `processBlock` — pre-allocate everything in `prepareToPlay`
- Zero locks or mutexes on audio thread
- Zero file I/O or logging on audio thread
- `ScopedNoDenormals` is the FIRST line of `processBlock`
- Cache all `getRawParameterValue` pointers in `prepareToPlay` — never inside `processBlock`
- Circular buffers use power-of-2 sizes with bitmask indexing — no `%` modulo
- Use `applyGainRamp`, never `applyGain`, at block boundaries

### Filter State
- Own the biquad completely: coefficients AND state in one struct
- Direct Form I: `y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2` with your own `x1,x2,y1,y2` per channel
- Do NOT use `juce::dsp::ProcessorDuplicator` for per-sample coefficient updates
- Do NOT call `filter.reset()` on coefficient changes — only on `prepareToPlay` or transport reset

### Parameter Smoothing
- Every automatable audio-affecting parameter uses `SmoothedValue<float, Linear>`
- Call `getNextValue()` exactly once per sample
- Filter cutoff: interpolate coefficients per-block via SmoothedValue on the frequency

### Discontinuity Prevention
- Every toggle that touches the audio or detector path gets a crossfade (`SmoothedValue`,
  ~20ms default), never a hard branch — `bool ? pathA : pathB` clicks on every flip, no
  exceptions, even for toggles that don't feel "obviously audible"
- Click-free protection belongs to the *value*, not to whichever one trigger you built it
  for — the moment a value gets crossfade protection for reason A (a preset load, a
  snapshot recall), audit every OTHER path that can change the same value (its own
  knob/button, host automation, MIDI Learn) and confirm each is independently safe, don't
  assume the first fix covers all of them
- `prepareToPlay()` can legitimately be called again mid-session by the host at an
  unchanged sample rate (a documented AU/VST3 behavior) — guard every side effect inside
  it (coefficient resets, forced `SmoothedValue` snaps-to-target, forced "changed" host
  notifications) against this redundant-call case with one guard at the top
  (`lastSampleRate_`/`everPrepared_`), not a bespoke check per side effect discovered one
  bug report at a time
- Never send a host a "value changed" notification (`setLatencySamples`, bus layout, etc.)
  unless the value actually changed from what was last reported — a redundant notification
  can trigger a full plugin-delay-compensation realignment and produce an audible click

### GUI Thread Safety
- Editor destructor resets all dynamic `SliderParameterAttachment` objects **FIRST**
- Destructor order: (1) reset all attachments → (2) `stopTimer()` → (3) `setLookAndFeel(nullptr)`
- All preset load/save on message thread only
- All APVTS `copyState`/`replaceState` on message thread only
- Structural GUI reactions (a mode switch swapping which control is live, a bypass toggle
  greying out a column) poll via a message-thread `Timer` (~15Hz), never an
  `AudioProcessorValueTreeState::Listener` callback — that callback can fire from the audio
  thread during host automation, and these reactions typically allocate (rebuilding
  attachments, `setEnabled()` trees), which is unsafe off the message thread

### Build Rules
- Generate ONE file per response — explain it, wait for approval, then next file
- Run Verification Loop (Section 7) after every generated file
- Never run `xcodebuild` against empty `.cpp` files
- After every Release build: `lipo -info` must show both `x86_64` and `arm64`
- AU type: `kAudioUnitType_MusicEffect` (aumf) with `NEEDS_MIDI_INPUT TRUE`
- `COPY_PLUGIN_AFTER_BUILD FALSE` — copy manually
- After every successful build: run the Post-Build Install Ritual (`build-system/SKILL.md`)
  — copy AU/VST3 to the paths in § 1, asking explicit permission before overwriting an
  existing install of the same plugin, then clear the AU caches and run `auval`. Every
  build, not just when troubleshooting a stale-cache bug.
- Windows VST3 is built exclusively by GitHub Actions (see `Docs/GitHub.md` § 4) — there is
  no local Windows toolchain in this workflow. `lipo`/`auval` do not apply there; success is
  the `build-windows` job going green and the VST3 artifact uploading. Guard any Apple-only
  header/API with `#if JUCE_MAC` before it can break that job.

### Code Generation
- No TODO stubs — all functions fully implemented
- No prototype shortcuts
- File header comment on every file: purpose + dependencies
- `#pragma once` on every header

### Testing
- Every DSP class gets a unit test before being considered complete (load `dsp-testing`)
- Golden reference regression run before every release — failures must be explained, not silently overwritten

### Numerical Precision
- Accumulate long-running derived state (envelope followers, RMS/level integrators, gain-reduction history) in `double` — display/output stays `float`
- Decide this per accumulator up front, not after a drift bug appears; symptom only shows after extended runtime, which makes it expensive to diagnose retroactively

### State Ownership
- The moment a second component (a meter strip, a tracker view, a secondary readout) needs a derived or smoothed value another component already computes, extract it into one shared function or shared engine-side state immediately — never let a second call site re-derive it
- Duplicated derived-state math is the single most common recurring bug class in this workflow's history: each copy looks correct in isolation, then drifts out of sync with the others over time

### Collaboration Protocol
- Give the literal value + its symbol name, not a description — `kHeaderFontSize = 22px`, not "the header font." Lets the user self-serve constant changes without a round trip
- Bug reports are most useful as a numbered list naming the exact screen/element/symptom — batch several before starting fixes, then do one verification pass across all of them rather than ping-ponging one at a time
- When a requirement has more than one reasonable reading, state the interpretation and ask before implementing — cheaper than building the wrong one and redoing it

### Debug Instrumentation
- `Source/Core/DevPhaseLabel.h` holds one string constant naming the current phase (e.g.
  `"Phase 4 — DSP Implementation"`, matching the Name column in `CLAUDE.md § 3`). Created
  in Phase 2, drawn as a small, unobtrusive corner overlay in `PluginEditor` from Phase 2
  onward — this is how you confirm which phase's build you're actually looking at when
  testing in a host
- Update that constant every time `CLAUDE.md § 2`'s current phase changes — a one-line
  edit, not a GUI change, so it doesn't drag DSP-only or preset-only phases into touching
  editor code
- This overlay is **not** part of the approved visual design — it must never appear in
  the Phase 6 HTML prototype, and the Phase 7 screenshot-audit disregards it when diffing
  against the locked design
- Mandatory removal gate: `plugin-release` deletes the overlay code and
  `DevPhaseLabel.h` entirely before any release build — not a flag to flip, actually gone

---

## 5. Plugin Specification Summary

> Full spec lives in `Docs/PluginSpec.md`. This is the quick-reference extract.

**Sonic Identity:** [2-3 sentence description of the sound and feel]

**Stereo Topology:** [True Stereo / Dual Mono / Mono-in Stereo-out]

**Oversampling:** [None / 2x / 4x / User-selectable]

**Nonlinear Processing:** [brief description]

**Key Controls:** *(full list lives in `Docs/State/Parameters.md` — that is the source of truth once Phase 3 begins)*

**UI:** [brief visual theme, window size — full detail in `Docs/State/UI_Design.md`]

**Preset System:** [User / Factory / Both / None — full detail in `Docs/State/Presets.md`]

**Reference Plugins:** [list]

---

## 6. Phase 0 Checklist (No Skill Required)

Run once. Check off each item.

- [ ] JUCE 8 installed at `[JUCE PATH]`
- [ ] CMake 3.22+ installed (`cmake --version`)
- [ ] Xcode installed with command-line tools (`xcode-select --install`)
- [ ] Git installed (`git --version`)
- [ ] `pluginval` downloaded
- [ ] Logic Pro available for AU testing
- [ ] Reaper or Ableton available for VST3 testing
- [ ] GitHub repo created (see `Docs/GitHub.md`)
- [ ] `Docs/PluginSpec.md` fully completed
- [ ] This `CLAUDE.md` fully filled in
- [ ] All files in `Docs/State/` initialised (templates already in place)
- [ ] Phase status updated to: Phase 1A / NOT STARTED

---

## 7. Verification Loop (Run After Every Generated File)

```
QA self-inspection of file just generated:
1. Realtime safety violations? (allocation, locks, IO in processBlock)
2. Thread safety risks between audio thread and GUI thread?
3. JUCE 8 compatibility issues?
4. Apple Silicon / ARM64 issues?
5. Parameter smoothing — sample-accurate?
6. Denormal risks in any IIR filters?
7. Memory leaks or ownership issues?
8. Automation edge cases at min/max parameter values?
9. getRawParameterValue pointers cached in prepareToPlay?
10. Editor destructor resets attachments before stopTimer/setLookAndFeel?
11. GUI file changed? Screenshot-audit it against the locked prototype/last-approved state —
    compiling and passing auval/pluginval catches none of: clipped labels, misaligned scales,
    invisible hover states.

Rate each: PASS / WARNING / FAIL
Fix all FAILs before compiling.
```

---

## 8. State Document Protocol (Replaces Phase Handoffs)

There are no per-phase handoff files. Instead, eight living documents in `Docs/State/`
each hold the CURRENT truth for one domain. Edit them in place. Never let two documents
disagree about the same fact — if you find a contradiction, fix it immediately.

DSP and UI each split into two documents on purpose: a Design doc (decisions — what and
why, no code) and an Implementation doc (build spec — how, produced as a pre-code gate
right before the corresponding code phase starts). Each fact has exactly one home — the
Design doc never contains a difference equation or a component inventory; the
Implementation doc never contains an algorithm choice.

| Domain | File | Edited During |
|---|---|---|
| DSP decisions — signal flow, algorithms | `Docs/State/DSP_Design.md` | Phase 1A |
| DSP build spec — difference equations, class state | `Docs/State/DSP_Implementation.md` | Phase 4, 5 |
| Class structure & threading | `Docs/State/Software_Architecture.md` | Phase 1B, 2, 3, 8 |
| Parameter IDs & ranges | `Docs/State/Parameters.md` | Phase 3 (frozen after release) |
| CMake & build state | `Docs/State/Build_Config.md` | Phase 2, 9, R |
| UI decisions — layout, colours, fonts | `Docs/State/UI_Design.md` | Phase 6 |
| UI build spec — file order, component inventory | `Docs/State/UI_Implementation.md` | Phase 7 |
| Preset architecture | `Docs/State/Presets.md` | Phase 8 |
| History of WHY things changed | `Docs/State/Changelog.md` | Every phase — append only |

### At the start of every session:
```
Read Docs/State/[relevant domain files for this phase].
Read Docs/PluginSpec.md (relevant sections).
Confirm current state before generating anything.
```

### After any change to a domain:
```
1. Open Docs/State/[Domain].md
2. Update the relevant section IN PLACE — replace stale content, don't append
3. Append one line to Docs/State/Changelog.md:
   [date] [Phase N] [Domain]: what changed — why
```

### Architecture Review (run every 3-4 phases, fresh conversation)
```
Architecture Review — do not generate code.
Read all files in Docs/State/.
Identify:
1. Contradictions between state documents
2. Architectural drift from Docs/PluginSpec.md
3. Naming inconsistencies across files
4. Thread safety risks not yet addressed
5. Any value multiple documents *assume* exists (referenced as if obviously specified)
   but was never actually assigned an ID/range/default in the real source-of-truth table
6. Recommended corrections before continuing
Recommendations only — no code.
```
Do this periodically even when nothing feels wrong — at least one living document *will*
quietly stop being updated at some point while the others move on, and it's cheaper to
catch that with a scheduled pass than to discover it by contradiction later.

---

## 9. Debugging Protocol

When a build error, crash, or DSP bug occurs:

1. Stop the current phase conversation
2. Open a new conversation
3. Load `plugin-debugging` skill
4. Paste: error + relevant file + line numbers
5. Fix only the broken lines — do not regenerate whole files
6. If the fix reveals a deeper issue, update the relevant `Docs/State/*.md` file and log it in `Changelog.md`
7. Return to phase conversation with the fix applied

---

## 10. Testing Protocol

After any DSP file is written or modified:
```
Load .claude/skills/dsp-testing/SKILL.md
Write/update unit tests for the changed class.
Run the test target. All tests must pass before moving on.
```

Before every release (mandatory gate, not optional):
```
Load .claude/skills/dsp-testing/SKILL.md
Run Docs/GoldenReference/render_and_diff.py against the current build.
Any failure: investigate (load plugin-debugging) or explicitly approve
and log the change in Docs/State/Changelog.md. Never silently overwrite
golden references.
```

---

## 11. Cross-Tool Routing

| Task | Tool |
|---|---|
| All DSP C++ code | Claude Code only |
| Architecture, threading, debugging | Claude Code only |
| Verifying offloaded equations | Claude Code (use Research Intake prompt in `Docs/CrossToolHandoff.md`) |
| DSP literature research | Gemini Deep Research → verify on Claude |
| Manual prose, marketing copy | ChatGPT/Gemini → fact-check |
| Logo, textures, icons | ChatGPT/Gemini image gen |
| HTML UI mockup (Phase 6) | Claude Code (or ChatGPT, approved mockup returns here) |
| Generic git/CMake questions | Free ChatGPT/Gemini |
| Parameter tables, test logs | ChatGPT/Gemini Sheets → paste final data back |

Full handoff templates: `Docs/CrossToolHandoff.md`

---

## 12. Known JUCE 8 Gotchas (Quick Reference)

- `IIR::Filter.state` is PRIVATE — own your biquad
- `IIR::Filter.processors` is PRIVATE — cannot iterate per-channel filters
- `ProcessorDuplicator` has no `processSample()` — block-mode only
- `SmoothedValue.getNextValue()` advances the smoother — call once per sample only
- `applyGain()` is constant — use `applyGainRamp()` when gain may have changed
- `AudioBuffer` stack allocation inside `processBlock` = heap allocation — pre-allocate as member
- `HeapBlock` inside `processBlock` = heap allocation — pre-allocate as member
- `ScopedNoDenormals` scope starts where declared — must be first line
- `CallOutBox`/`ComboBox` open independent OS windows in an AU host (Logic) — they don't
  track the plugin window moving, and JUCE's `MouseListener`/`ModalComponentManager`
  dismissal paths don't fire because Logic's NSView eats the click first. See
  `plugin-ui/SKILL.md` for the polling-based tracking/dismissal pattern that actually works.
- `SliderParameterAttachment`'s constructor silently calls
  `setDoubleClickReturnValue(true, defaultValue)` on every `Slider` it binds — if you build
  your own double-click behavior (e.g. a type-in-value tooltip), call
  `slider.setDoubleClickReturnValue(false, 0.0)` immediately after construction, and again
  every time the attachment is rebuilt (a fresh attachment re-enables it). Symptom: a value
  reset to exactly the parameter's default on a perfectly stationary double-click.
- `processBlockBypassed()` defaults to asserting bypass reports the same latency as normal
  processing — override it explicitly the moment any feature makes `getLatencySamples()`
  nonzero, routing it through the same delayed/muted path a soft bypass already uses
- A `Rectangle` read (e.g. `getX()`) *after* `removeFromLeft()`/`removeFromTop()`/etc. have
  already mutated it returns a position from the wrong sub-region — capture any position
  needed downstream before further mutation, not by re-querying the mutated object later
- A `juce::Timer`-derived class owns exactly one interval — if it already drives job A via
  its inherited `Timer`, compose a second, separate object with its own internal `Timer`
  for unrelated job B rather than multiplexing both onto one `timerCallback()`

---

## 13. File & Folder Structure

```
[PluginName]/
├── CLAUDE.md                          ← this file (always in context)
├── CMakeLists.txt
├── .github/workflows/build.yml
├── .gitignore
├── Source/
│   ├── Core/
│   │   └── DevPhaseLabel.h    ← temporary, deleted before release (§ 4 Debug Instrumentation)
│   ├── DSP/
│   ├── Parameters/
│   │   ├── ParameterIDs.h
│   │   └── ParameterLayout.h/.cpp
│   ├── GUI/
│   │   └── LookAndFeel.h
│   └── Presets/
│       └── PresetManager.h/.cpp
├── Tests/
│   ├── BiquadTests.cpp
│   ├── SmoothingTests.cpp
│   └── OfflineRenderHarness.cpp
├── Assets/
│   └── IR/
├── Docs/
│   ├── PluginSpec.md                  ← full Part 2 spec
│   ├── CrossToolHandoff.md            ← handoff templates
│   ├── GitHub.md                      ← CI/CD setup guide
│   ├── BetaTesting.md                 ← beta process
│   ├── GoldenReference/
│   │   ├── README.md
│   │   ├── inputs/
│   │   └── expected/v[X.Y.Z]/
│   └── State/                         ← LIVING docs, edited in place
│       ├── DSP_Design.md              ← decisions (Phase 1A)
│       ├── DSP_Implementation.md      ← build spec (Phase 4/5 pre-code gate)
│       ├── Software_Architecture.md
│       ├── Parameters.md
│       ├── Build_Config.md
│       ├── UI_Design.md               ← decisions (Phase 6)
│       ├── UI_Implementation.md       ← build spec (Phase 7 pre-code gate)
│       ├── Presets.md
│       └── Changelog.md               ← the ONLY append-only file
└── .claude/
    └── skills/
        ├── dsp-research/SKILL.md
        ├── plugin-architecture/SKILL.md
        ├── build-system/SKILL.md
        ├── dsp-implementation/SKILL.md
        │   └── references/
        │       ├── realtime-safety.md
        │       └── biquad-patterns.md
        ├── dsp-testing/SKILL.md
        ├── plugin-ui/SKILL.md
        ├── plugin-qa/SKILL.md
        │   └── references/
        │       └── daw-validation-checklist.md
        ├── plugin-optimization/SKILL.md
        ├── plugin-debugging/SKILL.md
        └── plugin-release/SKILL.md
```

---

## 14. Version Control

- Develop on version branches: `v1.0.0-dev`, `v1.0.1-dev`
- Merge to `main` only to trigger GitHub Actions CI
- **Bump version on every rebuild that changes `isBusesLayoutSupported()`** — Logic caches by version
- Tag every release: `git tag vX.Y.Z && git push origin vX.Y.Z`
- Version must match in: `CMakeLists.txt`, `PluginProcessor`, `PluginEditor`, About window, manual, bundle

---

*Last updated: [DATE] | Template v2.9*
