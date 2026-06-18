# DAW Validation Checklist Reference

> Template for Phase 10. Customise every [FIELD] for your project.
> All items must have a test procedure, not just a checkbox.

---

## 1. AU Validation (macOS)

| Test | Procedure | Pass |
|---|---|---|
| auval zero failures | `auval -v aumf [PLUG] [MFR]` — must print "AU VALIDATION SUCCEEDED" | [ ] |
| Logic Pro loads plugin | Relaunch Logic after cache clear; insert on audio track | [ ] |
| Logic: instrument track | Insert on instrument track (requires aumf type) | [ ] |
| Logic: bus track | Insert on a bus/aux | [ ] |
| Logic: stereo in/out | Send stereo signal; confirm stereo processing | [ ] |
| Logic: mono in/out | Send mono signal; confirm mono processing | [ ] |
| Logic: window close | Open and close plugin window 10x; no crash | [ ] |

---

## 2. VST3 Validation

| Test | Procedure | Pass |
|---|---|---|
| Reaper loads (VST3) | Add to FX chain on audio track | [ ] |
| Ableton Live loads | Add to device chain | [ ] |
| pluginval (all checks) | `pluginval --validate [path].vst3 --strictness-level 10` | [ ] |
| Universal binary | `lipo -info [path].vst3/Contents/MacOS/[PLUGIN]` → x86_64 arm64 | [ ] |

---

## 3. Audio Correctness

| Test | Procedure | Pass |
|---|---|---|
| Silence in = silence out | Send silence; confirm output is silent at all gain settings | [ ] |
| No DC offset | Analyse output of pink noise input in RMS meter | [ ] |
| Bypass is transparent | A/B bypass with matched gain; null test should show near silence | [ ] |
| No clicks on bypass | Toggle bypass 20x rapidly during playback | [ ] |
| Oversampling no click | Switch oversampling factor during playback (if applicable) | [ ] |
| Phase correlation | Confirm goniometer shows expected width (no accidental polarity flip) | [ ] |

---

## 4. Parameter Automation

Test every parameter listed in `Docs/PluginSpec.md § 2.4`:

| Parameter | Automation no zipper | Extremes no click | Default recalled | Pass |
|---|---|---|---|---|
| [PARAM 1] | [ ] | [ ] | [ ] | [ ] |
| [PARAM 2] | [ ] | [ ] | [ ] | [ ] |
| ... | | | | |

**Procedure for each:**
1. Draw automation lane: full sweep from min to max over 4 bars
2. Listen for zipper noise (stepped changes) → fail if present
3. Draw step automation: jump to min and max in same bar
4. Listen for clicks → fail if present
5. Confirm `Double-click to reset` restores default

---

## 5. Sample Rate Coverage

Test at each rate with Logic and Reaper:

| Rate | Loads | No artifacts | Correct pitch | Pass |
|---|---|---|---|---|
| 44100 | [ ] | [ ] | [ ] | [ ] |
| 48000 | [ ] | [ ] | [ ] | [ ] |
| 88200 | [ ] | [ ] | [ ] | [ ] |
| 96000 | [ ] | [ ] | [ ] | [ ] |

**Procedure:** Change project sample rate; insert plugin; play test signal; confirm output.

---

## 6. Buffer Size Coverage

| Buffer | No xruns | No artifacts | Pass |
|---|---|---|---|
| 64 samples | [ ] | [ ] | [ ] |
| 128 samples | [ ] | [ ] | [ ] |
| 256 samples | [ ] | [ ] | [ ] |
| 512 samples | [ ] | [ ] | [ ] |

**Procedure:** Change I/O buffer size in DAW preferences; play 2 minutes; monitor for xruns.

---

## 7. Preset System

| Test | Procedure | Pass |
|---|---|---|
| Save user preset | Set non-default values; save as "Test Preset" | [ ] |
| Load user preset | Reload project; confirm "Test Preset" recalls all values | [ ] |
| Factory preset recall | Load each factory preset; confirm correct | [ ] |
| Preset persistence | Save session; close DAW; reopen; confirm preset recalls | [ ] |
| Preset version compat | Manually edit version field in preset XML; confirm graceful handling | [ ] |

---

## 8. MIDI Learn (if applicable)

| Test | Procedure | Pass |
|---|---|---|
| Assign CC | Right-click parameter → MIDI Learn; send CC; confirm assignment | [ ] |
| CC controls param | Move CC after assignment; confirm parameter follows | [ ] |
| Assignment persists | Save and reopen session; confirm CC still assigned | [ ] |
| Clear assignment | Right-click → Clear MIDI Learn; confirm CC no longer affects param | [ ] |

---

## 9. CPU Stress Test

| Test | Procedure | Pass |
|---|---|---|
| 20 instances — Reaper | Load 20 on separate tracks; play 2min; record CPU% | [ ] |
| CPU target met | ≤ 30% on M1/M2-class Mac at 128-sample buffer | [ ] |
| No xruns at 20x | Monitor Reaper xrun counter for 2 minutes | [ ] |

Record result: `[PLUGIN] v[X.Y.Z] — 20x @ 128 buf = [X]% CPU (M[X] Mac)`

---

## 10. Crash Resilience

| Test | Procedure | Pass |
|---|---|---|
| Logic window close | Open/close plugin window 20x; no crash | [ ] |
| Rapid param automation | Automate all params simultaneously at high speed; 2min | [ ] |
| Rapid bypass toggle | Toggle bypass at 4Hz for 2 minutes | [ ] |
| Sample rate change | Change rate while plugin active and playing; no crash | [ ] |
| Session reload | Save and reload Logic session 5x; plugin state correct | [ ] |
| 2hr soak test | Play continuously for 2 hours; monitor CPU and listen | [ ] |

---

## 11. UI Validation

| Test | Procedure | Pass |
|---|---|---|
| All controls render | Open plugin; verify all knobs/buttons visible | [ ] |
| Knob dragging | All knobs respond to vertical drag | [ ] |
| Value display | Verify tooltips or value popups (if present) | [ ] |
| About popup | Click logo; popup appears with version, credit | [ ] |
| Popup close | Click outside About popup; it closes | [ ] |
| Meter tracking | Meter follows input level (if applicable) | [ ] |
| HiDPI display | Test on Retina display; no blurry elements | [ ] |
| Resize (if enabled) | Resize within bounds; layout adapts correctly | [ ] |

---

## Sign-Off

```
QA Engineer: [name]
Date: [date]
Plugin version: [X.Y.Z]
Build: [Release / Debug]

Hard blockers: ALL PASS / FAIL (list failures)
Soft requirements: [X/Y passed]

Signed off for release: YES / NO
Notes: [any known issues deferred to next version]
```
