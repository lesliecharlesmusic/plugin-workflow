---
name: plugin-qa
description: QA Agent for audio plugin development. Load for Phase 10 — generating the full QA validation checklist, running DAW compatibility tests, auval validation, crash resilience testing, and sign-off before release. Also load when a plugin passes individual phase tests but needs full system validation. Do not load for implementation or debugging of specific bugs.
---

# QA Agent

**Role:** Quality assurance and release gate engineer. You produce test checklists and validate against them. You do not fix bugs — discovered bugs go to the debugging skill.

**When active:** Phase 10 (requires `Docs/State/Build_Config.md` current and unit tests passing).

---

## Phase 10 — Full QA Validation

Read `Docs/State/Build_Config.md`, `Docs/State/DSP_Design.md`, and `Docs/PluginSpec.md` before generating the checklist.

Generate a project-specific checklist using the template in `references/daw-validation-checklist.md`.

Customise every section for:
- Actual parameter count and names from spec § 2.4
- Actual DAWs being tested
- Actual sample rates and buffer sizes to cover
- Whether oversampling, MIDI learn, IR loading, or sidechaining are present

---

## Phase 10 — macOS / Logic Diagnostic Protocol

Run this flowchart BEFORE editing any code when the plugin fails to load or misbehaves:

```
1. Is there a known-good reference plugin on this machine?
   YES → diff its Info.plist against yours; compare isBusesLayoutSupported line by line
   NO  → proceed to step 2

2. Does auval pass?
   auval -v aumf [PLUG CODE] [MFR CODE]
   YES → binary is fine; suspect host cache → go to step 3
   NO  → code problem; load plugin-debugging skill

3. Do auval and Logic disagree about channel capabilities?
   YES → this is the Logic cache fingerprint
        → bump version number (1.0.0 → 1.0.1)
        → run full cache-clear ritual (below)
        → do NOT touch code
   NO  → check universal binary (step 4)

4. Is the binary universal?
   lipo -info [path to .component/Contents/MacOS/PluginName]
   Expected: "Architectures in the fat file: ... are: x86_64 arm64"
   THIN → rm -rf Build/ → reconfigure → rebuild → rm -rf destination → cp -R
   UNIVERSAL → proceed to step 5

5. Load in AudioPluginHost (JUCE tool) — simpler host, easier to isolate
   FAILS → definitely a code/config problem; load plugin-debugging skill
   PASSES → Logic-specific issue; check track type, AU type (aumf vs aufx)
```

### Cache-Clear Ritual

```bash
rm -rf ~/Library/Audio/Plug-Ins/Components/[PLUGIN NAME].component
rm -rf ~/Library/Caches/AudioUnitCache
rm -rf ~/Library/Caches/com.apple.audio.InfoHelper
rm -rf ~/Library/Caches/com.apple.audio.AudioComponentRegistrar 2>/dev/null
sudo killall -9 AudioComponentRegistrar 2>/dev/null
# Fully quit Logic with Cmd+Q (not just close window)
# Relaunch Logic
# If still stale: reboot Mac
```

---

## Sign-Off Requirements

All items in the full checklist must be checked before proceeding to Phase R (Release).

### Hard blockers (must pass — no exceptions):
- [ ] `auval` zero failures
- [ ] Plugin loads in Logic Pro (AU)
- [ ] Plugin loads in Reaper (VST3)
- [ ] No `EXC_BAD_ACCESS` when closing plugin window in Logic
- [ ] `lipo -info` shows universal binary
- [ ] Silence in = silence out at maximum gain
- [ ] All parameters automate without clicks
- [ ] No crash after 2 hours continuous playback
- [ ] Preset save and load round-trips cleanly

### Mandatory pre-sign-off gate:
- [ ] Golden reference regression run via `dsp-testing` skill — no unexplained failures
- [ ] All DSP unit tests pass

### Soft requirements (document if not met):
- [ ] CPU ≤ 30% with 20 instances at 128-sample buffer
- [ ] Tested in Ableton Live
- [ ] Tested in Cubase
- [ ] All sample rates: 44.1, 48, 88.2, 96 kHz
- [ ] All buffer sizes: 64, 128, 256, 512 samples

---

## After Phase 10

When all hard blockers pass:
1. Record results in `Docs/State/Build_Config.md` (verification table)
2. Append to `Docs/State/Changelog.md`: `[date] [Phase 10] QA: all hard blockers pass — ready for beta`
3. Update `CLAUDE.md § 2` current phase to Beta Testing
4. Follow `Docs/BetaTesting.md` for the beta round before Release
5. Only after beta exit criteria are met: load `plugin-release` skill

Read the full checklist in `references/daw-validation-checklist.md` to customise for this project.
