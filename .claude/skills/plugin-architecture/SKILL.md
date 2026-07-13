---
name: plugin-architecture
description: Plugin Architecture Agent for audio plugin development. Load for Phase 1B (Software Architecture Document), Phase 3 (Parameter Architecture and ParameterIDs.h), and Phase 8 (Preset System). Also load when designing class structure, threading model, APVTS layout, ownership model, or preset file format. Do not load for DSP implementation or GUI code.
---

# Plugin Architecture Agent

**Role:** Software architect. You design class structure, threading models, and file layouts. You produce architecture documents and non-DSP source files.

**When active:** Phase 1B, Phase 3, Phase 8.

---

## Phase 1B — Software Architecture Document

Produce before any code. Read `Docs/State/DSP_Design.md` (must be approved and current).

### Required Sections

**1. Folder & File Structure**
Full tree. Every `.h` and `.cpp` that will exist. No orphan files.

**2. Class Inventory**
| Class | File | Single Responsibility | Thread |
|---|---|---|---|
For every class. No class has two responsibilities.

**3. Separation of Concerns**
Explicitly map:
- DSP Engine → audio thread only
- Parameter System (APVTS) → owned by Processor, read on both threads
- GUI Layer → message thread only
- Preset System → file I/O on message thread only
- State Management → lock-free atomic or FIFO crossing thread boundary

**4. Threading Model**
ASCII diagram showing what lives on which thread and how values cross:
```
AUDIO THREAD              MESSAGE THREAD
processBlock()    ←——→   Editor / APVTS listeners
DSP classes               PresetManager
SmoothedValues            LookAndFeel
```
- Standard cross-thread pattern, default to this shape for every "audio thread detected
  something, message thread/GUI/host needs to react" case rather than inventing a new
  mechanism per feature: **audio thread sets a plain atomic/flag → a small class derived
  from `juce::AsyncUpdater` does the actual work (host notification, GUI update) on the
  message thread when triggered.**
- Any `Timer` driving logic that must keep running when the editor is closed (anything
  that reacts to host automation, not just a GUI click) belongs on the `AudioProcessor`
  itself — a private, always-running `juce::Timer` from construction — never inside a GUI
  component's own `Timer`, which stops the moment the editor closes

**5. APVTS Layout Plan**
- Parameter ID naming convention
- Smoothing assignments per parameter
- How listeners attach (Editor vs Processor)
- Attachment lifetime and ownership
- `juce::UndoManager` ownership — who constructs it (owned by the Processor, passed into
  the APVTS constructor), and confirm scope: it tracks user-initiated changes (GUI
  gestures, preset loads), not host automation playback — undoing automation doesn't
  make sense and isn't in scope

**6. Ownership & Memory Model**
- Who owns what (unique_ptr vs raw)
- Destructor ordering rationale
- No naked `new` or `delete`
- Owner of every derived/smoothed display value (calibrated meter reading, eased display
  number) — name it explicitly here. The moment more than one GUI component needs the same
  derived value, this section is what stops a second component from re-deriving it (see
  `CLAUDE.md § 4` State Ownership) — that duplication is one of the most common recurring
  bug classes in real-time audio/UI code.

**7. Oversampling Integration**
- Where in the signal chain
- Who owns the `juce::dsp::Oversampling` object
- How latency is reported to host

**8. Preset Architecture**
- File format (XML vs binary)
- Storage location (OS documents folder)
- Factory preset delivery (BinaryData)
- Forward-compatibility version field

**9. MIDI Learn Architecture** *(if applicable)*
- CC assignment storage
- Persistence model
- Thread safety of CC → parameter mapping

Once approved, write this document into `Docs/State/Software_Architecture.md`
(replace placeholder content section by section). Append one line to
`Docs/State/Changelog.md`: `[date] [Phase 1B] Software_Architecture: initial architecture approved`.

---

## Phase 3 — Parameter Architecture

**Input required:** `Docs/State/Build_Config.md` (Phase 2 must be current) and
`Docs/State/Software_Architecture.md` (APVTS layout plan section).

### ParameterIDs.h

Generate ONLY `Source/Parameters/ParameterIDs.h`:
- All parameter IDs as `constexpr StringLiteral` or `const char*`
- Naming convention: `lowercase_snake_case`
- Group by functional section with comments
- Include a version sentinel: `constexpr int kParamLayoutVersion = 1;`

After generating, explain:
1. Why IDs must never change after first release (breaks saved sessions and presets) —
   and why that's true the moment ANY save file exists on disk, even pre-release: a
   renamed/reassigned/renumbered ID silently orphans or corrupts anything already saved
   with the old ID, including the developer's own test presets and sessions
2. How IDs connect to APVTS `createParameterLayout()`
3. The chosen naming convention and why

Wait for approval. Then generate `ParameterLayout.h/.cpp` only after ID approval.

**Undo/Redo:** every plugin gets `juce::UndoManager` wired into the APVTS constructor
(`apvts_(*this, &undoManager_, "PARAMETERS", createParameterLayout())`) by default — see
Threading Model § 5 above for ownership and scope. `plugin-ui` wires the keyboard
shortcuts (Cmd+Z / Cmd+Shift+Z) once the editor exists.

Once approved, write the full parameter table into `Docs/State/Parameters.md`
(this becomes the frozen-after-release source of truth). Append to
`Docs/State/Changelog.md`: `[date] [Phase 3] Parameters: initial parameter layout defined`.

### ParameterLayout.h/.cpp

- `createParameterLayout()` returns `AudioProcessorValueTreeState::ParameterLayout`
- Use `AudioParameterFloat`, `AudioParameterBool`, `AudioParameterChoice` as appropriate
- Skew factors where perceptually justified (frequency, gain)
- `NormalisableRange` with explicit `from0to1` / `to0to1` lambdas for nonlinear ranges
- Default values from `Docs/PluginSpec.md § 2.4`

---

## Phase 8 — Preset System

**Input required:** `Docs/State/UI_Design.md` (Phase 7 GUI complete) and
`Docs/State/Parameters.md` (final parameter list).

Generate ONLY `Source/Presets/PresetManager.h` first, then `.cpp`.

### PresetManager requirements

- Save named user presets to OS user documents folder
- Factory presets embedded as `BinaryData` (generated by `juce_add_binary_data`)
- Version field in every preset file for forward compatibility
- File I/O on message thread only — assert or `jassert` if called on audio thread
- `APVTS::copyState()` / `replaceState()` on message thread only
- Preset recall glitch-free: load into temp state, then `replaceState` atomically
- Save/Load buttons in the plugin's own GUI always open the native OS file browser
  (`juce::FileChooser`), not a plugin-internal-only file list — gives the user full
  filesystem access rather than being locked into one folder. This is scoped to the
  plugin's own preset files; it's separate from and doesn't affect
  `getStateInformation`/`setStateInformation`, which the host drives automatically for its
  own session save/recall

Before generating:
1. Propose 6–10 factory preset names appropriate to the plugin category and sonic identity
2. Describe the chosen file format (XML recommended for human-readability and JUCE compatibility)
3. Describe the folder structure on disk

Wait for approval before writing code.

Once implemented, write preset format, factory preset list, and folder structure
into `Docs/State/Presets.md`. Append to `Docs/State/Changelog.md`:
`[date] [Phase 8] Presets: preset system implemented — [format chosen]`.

After PresetManager is complete, load `dsp-testing` skill if any preset-affecting
parameter math changed, to confirm round-trip correctness is covered.

---

## Verification Checklist (run after every file)

See `CLAUDE.md § 7` for the full Verification Loop.
Additional architecture-specific checks:
- [ ] No class has more than one responsibility
- [ ] No cross-thread raw pointer sharing without atomic or FIFO
- [ ] All `unique_ptr` attachments reset in correct destructor order
- [ ] Parameter IDs are `constexpr` — no runtime string construction
- [ ] `ParameterLayout` version sentinel present
