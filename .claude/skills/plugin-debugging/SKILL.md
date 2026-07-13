---
name: plugin-debugging
description: Debugging Agent for audio plugin development. Load immediately when any compiler error, linker error, runtime crash, DSP click or audio artifact, AU validation failure, or DAW loading problem occurs — regardless of which phase is active. Do not debug in the same conversation as implementation. Open a new conversation, load this skill, paste the error and file context.
---

# Debugging Agent

**Role:** Forensic debugger. You identify root causes and provide minimal targeted fixes. You do not regenerate whole files. You do not continue implementation.

**Rule:** One debugging session per conversation. Never mix debugging and implementation.

---

## Session Protocol

Paste into this conversation:
1. Exact error text (compiler output, crash log, or symptom description)
2. Relevant file(s) with line numbers
3. Phase currently active
4. Last action taken before the error

Then:
1. Explain the error in plain English
2. Identify the root cause (not just the symptom)
3. Provide ONLY the corrected lines — not the whole file
4. Explain what would have prevented this error
5. Identify if this indicates a deeper architectural issue requiring a `Docs/State/*.md` update

---

## Error Type Routing

### Compiler / Linker Errors

**Linker: "undefined symbol"**
- New `.cpp` file not added to `target_sources()` in `CMakeLists.txt`
- Fix: regenerate complete `CMakeLists.txt` (load build-system skill)

**Compiler: "private member" on JUCE filter types**
- Accessing `IIR::Filter.state` or `ProcessorDuplicator.processors`
- Fix: replace with owned `BiquadFilter` struct (see `dsp-implementation/references/biquad-patterns.md`)

**Compiler: "no member named processSample" on ProcessorDuplicator**
- `ProcessorDuplicator` is block-mode only in JUCE 8
- Fix: own the biquad and implement `processSample` directly

**Linker: "duplicate symbol BinaryData::"**
- Two `juce_add_binary_data()` targets using the default namespace
- Fix: set unique `NAMESPACE` and `HEADER_NAME` on every binary data target

**CMake configure: "Cannot find source file"**
- File listed in `target_sources()` does not exist on disk yet
- Fix: `touch` the missing file, re-run cmake configure

### Runtime Crashes

**`EXC_BAD_ACCESS` when closing plugin window in Logic**
- Editor destructor not resetting attachments before `stopTimer()`/`setLookAndFeel(nullptr)`
- Fix: add explicit `attachment.reset()` calls as the very first lines of the destructor
- See `plugin-ui/SKILL.md` → Editor Destructor section

**`EXC_BAD_ACCESS` in `processBlock`**
- Null parameter pointer (not cached in `prepareToPlay`, or APVTS not constructed yet)
- Accessing a buffer sized for a different block size
- Fix: verify all `getRawParameterValue` pointers are non-null; add `jassert(param != nullptr)`

**Crash on sample rate change**
- DSP objects not re-initialised in `prepareToPlay` on rate change
- Fix: call `prepare()` on all DSP objects inside `prepareToPlay` unconditionally

**Crash after preset load**
- APVTS `replaceState()` called on audio thread
- Fix: always call `replaceState` on message thread via `juce::MessageManager::callAsync`

### State / Dispatch Logic Bugs

**A button/flag gets "stuck" or an action silently never fires, only sometimes**
- Look for `if (A) ... else if (B) ...` where A and B are driven by independently
  triggerable real-world events (a timer completing vs. a user click, two separate
  parameters, two separate hardware/host events) that were assumed mutually exclusive but
  aren't — both landing in the same audio block means the `else if` branch never runs,
  silently dropping whatever it was responsible for (a message-thread dispatch, a UI reset)
- Fix: check the higher-priority event first and unconditionally, explicitly
  discarding/consuming the lower-priority event's pending state rather than letting
  program order accidentally decide it

### Audio Artifacts

**Click on the very first parameter gesture of a session, reproduces only in a real host**
**(Logic/REAPER), never in Standalone**
- Two distinct causes produce this exact signature — check both before assuming DSP math:
  1. `prepareToPlay()` was called again by the host at an unchanged sample rate (a
     documented, legitimate AU/VST3 host behavior, sometimes tied to the plugin's UI
     becoming active) and something inside it unconditionally reset a `SmoothedValue` to
     its target or re-sent a "latency changed" host notification — see Discontinuity
     Prevention in `CLAUDE.md § 4`
  2. **Code signing, not DSP.** An ad-hoc-signed Debug binary can get rejected by `amfid`
     (Apple's file-integrity daemon) the moment a sandboxed host loads the plugin
     out-of-process (Logic hosts AU this way; REAPER doesn't, which is exactly why it
     never reproduces there). This produces a click/dropout that looks identical to a
     DSP/parameter-timing bug from the reproduction pattern alone. Pull the actual system
     log for the reproduction window before spending another pass reading source code:
     `log show --last 5m --predicate 'process == "Logic Pro"'` (or the relevant host) and
     look for `AppleMobileFileIntegrityError` / `AUHostingServiceXPC` teardown entries.
     If found: this project has no Developer ID certificate configured yet — see
     `plugin-release/SKILL.md` Code Signing.
- **Diagnostic tooling note:** prefer `log show --last <duration>` over `log stream`
  redirected to a file for after-the-fact captures — piping `log stream`'s output to a
  file can silently capture nothing, since stdout not being a terminal switches many CLI
  tools to block-buffered output that a `Ctrl-C`/`SIGINT` doesn't guarantee gets flushed.
  `log show` is a single retroactive query against the OS's rolling log buffer instead, so
  there's no buffering/interrupt timing to get wrong — reproduce the bug first, then run
  the query. Also: on some machines `log` resolves to a zsh builtin, not `/usr/bin/log` —
  a "too many arguments" error from an otherwise correctly-quoted `log show` command is
  that builtin intercepting the call; invoke `/usr/bin/log` explicitly to bypass it.

**Clicks / pops on parameter automation**
- Parameter not smoothed with `SmoothedValue`
- `SmoothedValue.getNextValue()` called multiple times per sample
- Filter coefficients updated without smoothing
- Fix: verify smoothing wiring in `processBlock`

**Clicks on bypass engage/disengage**
- Output not ramped to zero before bypass; no ramp from zero on re-engage
- Fix: implement gain ramp over one block on bypass state change

**Clicks on coefficient update**
- `filter.reset()` called during coefficient update (zeroes state)
- Fix: only reset filter state in `prepare()` and transport reset — never on parameter change

**DC offset / noise floor rising**
- Denormal accumulation in IIR feedback path
- Fix: verify `ScopedNoDenormals` is first line of `processBlock`, or add `y += 1e-25f` guard

**Aliasing on distortion/saturation**
- Nonlinearity applied without oversampling
- Fix: wrap waveshaper in `juce::dsp::Oversampling` (load dsp-implementation skill for Phase 5)

**Silence on right channel (mono-in stereo-out)**
- Missing channel duplication in `processBlock`
- Fix: add `buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples())` before DSP

### AU Validation Failures

**`auval` error: "Bus layout not supported"**
- `isBusesLayoutSupported` not implementing the canonical mono/stereo pattern
- Fix: replace with the exact pattern from `build-system/SKILL.md`

**`auval` passes but Logic won't load plugin**
- Host version cache stale — Logic cached old capability verdict
- Fix: bump version number in `CMakeLists.txt` → clean cache ritual:
```bash
rm -rf ~/Library/Audio/Plug-Ins/Components/[PLUGIN].component
rm -rf ~/Library/Caches/AudioUnitCache
rm -rf ~/Library/Caches/com.apple.audio.InfoHelper
rm -rf ~/Library/Caches/com.apple.audio.AudioComponentRegistrar 2>/dev/null
sudo killall -9 AudioComponentRegistrar 2>/dev/null
# Fully quit Logic (Cmd+Q) and relaunch
```
**Fingerprint:** `auval` passes + host disagrees = cache problem, not a code problem.
This is the escalated fix (deletes the installed component too) for when the routine
cache-clear in `build-system/SKILL.md`'s Post-Build Install Ritual — which runs after
every successful build, not just when something's stuck — wasn't enough on its own.

**`auval` error: "MusicEffect requires MIDI input"**
- `NEEDS_MIDI_INPUT` not set to `TRUE` in `CMakeLists.txt` for aumf type
- Fix: add `NEEDS_MIDI_INPUT TRUE` to `juce_add_plugin()`

**Plugin appears on some track types but not others in Logic**
- Using `kAudioUnitType_Effect` (aufx) instead of `kAudioUnitType_MusicEffect` (aumf)
- Fix: change `AU_MAIN_TYPE` to `kAudioUnitType_MusicEffect`

### Build / Architecture Issues

**`lipo -info` shows arm64 only (not universal)**
- Cause 1: Stale `Build/` folder — `rm -rf Build/`, reconfigure, rebuild
- Cause 2: `cp -R` merged instead of replaced — `rm -rf` destination before `cp -R`
- Cause 3: Architectures not set on sub-targets — check CMake for AU and VST3 sub-targets

**GitHub Actions Windows build fails: missing header**
```cpp
// Guard Apple-only headers:
#if JUCE_MAC
  #include <Accelerate/Accelerate.h>
#endif
```

**FetchContent library not found on include path**
```cmake
# After FetchContent_MakeAvailable, copy to expected path:
file(COPY "${libraryname_SOURCE_DIR}/"
     DESTINATION "${CMAKE_BINARY_DIR}/libraryname/")
```

---

## Diagnostic Checklist (start here for unknown issues)

1. Does `auval` pass? → if yes, suspect host cache not code
2. Is the binary universal? (`lipo -info`) → if thin, rebuild from clean
3. Are all `getRawParameterValue` pointers non-null in `processBlock`? → add `jassert`
4. Is `ScopedNoDenormals` the first line of `processBlock`?
5. Does the editor destructor reset attachments first?
6. Has the version number been bumped since last channel-layout change?
7. Is there a known-good reference plugin to diff against?

---

## After Debugging

1. Apply the minimal fix to the affected file(s)
2. Re-run `xcodebuild` and `auval`
3. If the fix touched any DSP class, load `dsp-testing` skill and re-run unit tests
   plus golden reference regression — a bug fix can itself change the sound
4. If the fix reveals an architectural issue, update the relevant `Docs/State/*.md`
   file in place and append a line to `Docs/State/Changelog.md`:
   `[date] [Phase N] [Domain]: bug found — [root cause] — fixed by [summary]`
5. Return to the implementation conversation with the fix applied
