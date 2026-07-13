---
name: dsp-testing
description: DSP Testing Agent for audio plugin development. Load when writing unit tests for DSP math (filter coefficients, gain curves, envelopes), setting up golden reference audio regression tests, or checking for silent sonic regressions after a refactor or bugfix. Load this BEFORE Phase 9 optimization and again before every release — it catches math errors and tonal drift that auval and DAW loading cannot detect. Also load when the user asks "did this change the sound" or "is this still correct" about any DSP code.
---

# DSP Testing Agent

**Role:** Correctness verifier, independent of any DAW or host. You test the math and the sound in isolation, fast, and reproducibly. This is the gap between "compiles and doesn't crash" and "is actually still correct."

**Why this exists:** `auval` and DAW loading prove a plugin is *stable*. They prove nothing about whether a refactor silently changed a filter's Q, shifted a gain curve, or altered the tone of a saturation stage. Professional shops (Valhalla, FabFilter) catch this with two techniques: pure unit tests on the math, and golden-reference audio diffing. Neither requires opening a DAW.

**When active:** After any DSP class is written or modified (Phase 4, 5, 9, and every bugfix). Mandatory gate before every release (Phase R).

---

## Two Test Categories

### A. DSP Unit Tests (fast, math-level, runs in CI)
Tests pure functions and classes — no JUCE plugin wrapper, no audio thread, no host. Runs in milliseconds. Catches: wrong coefficient formula, off-by-one in smoothing, incorrect gain curve, broken bilinear transform.

### B. Golden Reference Regression (audio-level, catches tonal drift)
Renders a fixed set of input WAVs through the plugin's offline render path and diffs against saved "known good" output. Catches: anything that changed the sound, even if the math "looks right" and nothing crashes.

**Both are needed.** Unit tests catch logic errors fast; golden reference catches the things logic can't predict (interaction effects, accumulated floating point drift, an oversampling change that subtly recolors the tone).

---

## A. DSP Unit Tests

### Setup (once per project)

Add to `CMakeLists.txt`:
```cmake
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable([PLUGIN_NAME]Tests
    Tests/BiquadTests.cpp
    Tests/SmoothingTests.cpp
    Tests/[OtherDSPClass]Tests.cpp
)
target_link_libraries([PLUGIN_NAME]Tests PRIVATE
    Catch2::Catch2WithMain
    [PLUGIN_NAME]   # link against the DSP code, not the AU/VST3 target
)
```

Tests link against your DSP classes directly — they must be testable without a `juce::AudioProcessor` wrapper. This is itself a useful architecture constraint: if a DSP class can't be unit tested without instantiating the whole plugin, it's too coupled to the JUCE wrapper.

### What to test

For every DSP class generated in Phase 4/5, write a corresponding test file covering:

**Filter coefficient correctness**
```cpp
TEST_CASE("Low-pass biquad coefficients sum correctly at DC", "[biquad]") {
    BiquadCoeffs c;
    calcLowPass(c, 1000.f, 0.707f, 48000.f);
    // At DC (0 Hz), a properly normalised low-pass should pass with gain 1.0
    float dcGain = (c.b0 + c.b1 + c.b2) / (1.f + c.a1 + c.a2);
    REQUIRE(dcGain == Catch::Approx(1.0f).margin(0.001f));
}

TEST_CASE("Low-pass attenuates above cutoff", "[biquad]") {
    BiquadFilter f;
    calcLowPass(f.c, 1000.f, 0.707f, 48000.f);
    // Feed a sine well above cutoff, measure RMS in vs out
    float rmsIn = 0.f, rmsOut = 0.f;
    for (int i = 0; i < 4800; ++i) {
        float x = std::sin(2.0 * M_PI * 8000.0 * i / 48000.0);
        float y = f.processSample(0, x);
        if (i > 1000) { rmsIn += x*x; rmsOut += y*y; }   // skip transient
    }
    REQUIRE(std::sqrt(rmsOut) < std::sqrt(rmsIn) * 0.5f);  // attenuated
}
```

**Smoothing correctness**
```cpp
TEST_CASE("SmoothedValue reaches target within expected time", "[smoothing]") {
    juce::SmoothedValue<float> sv;
    sv.reset(48000.0, 0.05);  // 50ms
    sv.setCurrentAndTargetValue(0.f);
    sv.setTargetValue(1.f);
    for (int i = 0; i < 2400; ++i) sv.getNextValue();  // 50ms at 48kHz
    REQUIRE(sv.getCurrentValue() == Catch::Approx(1.0f).margin(0.01f));
}
```

**Gain curve correctness**
```cpp
TEST_CASE("Gain parameter maps dB correctly", "[parameters]") {
    REQUIRE(dbToGain(0.f)  == Catch::Approx(1.0f));
    REQUIRE(dbToGain(-6.f) == Catch::Approx(0.5012f).margin(0.001f));
    REQUIRE(dbToGain(6.f)  == Catch::Approx(1.9953f).margin(0.001f));
}
```

**Waveshaper boundary behaviour**
```cpp
TEST_CASE("Soft clip never exceeds unity at extreme input", "[waveshaper]") {
    REQUIRE(std::abs(softClip(100.f)) <= 1.01f);
    REQUIRE(std::abs(softClip(-100.f)) <= 1.01f);
}

TEST_CASE("Waveshaper is odd-symmetric (no even harmonics added unintentionally)", "[waveshaper]") {
    for (float x = 0.01f; x < 1.0f; x += 0.1f)
        REQUIRE(softClip(-x) == Catch::Approx(-softClip(x)).margin(0.0001f));
}
```

**Denormal safety**
```cpp
TEST_CASE("Biquad does not produce denormals from near-zero input", "[denormal]") {
    BiquadFilter f;
    calcLowPass(f.c, 1000.f, 0.707f, 48000.f);
    float x = 1e-30f;  // denormal-range input
    for (int i = 0; i < 1000; ++i) {
        float y = f.processSample(0, x);
        REQUIRE(std::isfinite(y));
    }
}
```

### Before "fixing" code to satisfy a failing test, re-derive the spec math

A failing test is not automatically a code bug — a channel-count mixup or a backwards
assertion in the *test itself* produces the same red output as a real regression. Before
changing implementation code to make a test pass, re-derive what the spec actually says
the correct answer is (e.g. BS.1770 loudness legitimately reads stereo content ~3dB
louder than mono for identical per-channel content — that's correct behavior, not a bug,
and "fixing" the code to match a wrong test expectation would introduce a real one).

### Running tests

```bash
cmake -B Build -G Xcode
cmake --build Build --target [PLUGIN_NAME]Tests
./Build/Tests/[PLUGIN_NAME]Tests
```

Add to GitHub Actions (`build.yml`) as a job that runs before the plugin build — fail fast on math errors before spending CI minutes on a full plugin build.

---

## B. Golden Reference Regression Testing

### Setup (once, after Phase 4 core DSP is stable)

```
Docs/GoldenReference/
├── inputs/
│   ies   ├── sine_100hz.wav
│   ├── sine_1khz.wav
│   ├── sine_8khz.wav
│   ├── white_noise.wav
│   ├── impulse.wav
│   ├── silence.wav
│   └── music_excerpt.wav      (real musical material — bass DI or similar)
├── expected/
│   ├── v1.0.0/
│   │   ├── sine_1khz_default.wav
│   │   ├── sine_1khz_maxdrive.wav
│   │   ├── white_noise_default.wav
│   │   └── ...  (one render per input × representative preset)
│   └── [updated only when a sonic change is INTENTIONAL and approved]
└── render_and_diff.py
```

### Render Script

Generate `Docs/GoldenReference/render_and_diff.py`:

```python
#!/usr/bin/env python3
"""
Renders test inputs through the plugin offline (via JUCE AudioPluginHost
command-line render or a small custom render harness) and diffs against
the saved golden reference. Run before every release and after any DSP change.
"""
import subprocess, sys, numpy as np
import soundfile as sf

INPUTS_DIR = "inputs"
EXPECTED_DIR = "expected/v[CURRENT_VERSION]"
RENDER_BIN = "[path to a small offline render harness — see below]"

THRESHOLD_DB = -60.0  # difference below this is considered "no audible change"

def render(input_wav, preset_name, output_wav):
    subprocess.run([RENDER_BIN, "--plugin", "[PLUGIN].vst3",
                     "--input", input_wav, "--preset", preset_name,
                     "--output", output_wav], check=True)

def diff_db(expected_wav, actual_wav):
    e, sr1 = sf.read(expected_wav)
    a, sr2 = sf.read(actual_wav)
    if len(e) != len(a):
        return float('inf'), "LENGTH MISMATCH"
    diff = e - a
    rms_diff = np.sqrt(np.mean(diff**2))
    rms_ref  = np.sqrt(np.mean(e**2)) + 1e-12
    db = 20 * np.log10(rms_diff / rms_ref + 1e-12)
    return db, None

def main():
    failures = []
    test_cases = [
        ("sine_1khz.wav", "default", "sine_1khz_default.wav"),
        ("sine_1khz.wav", "maxdrive", "sine_1khz_maxdrive.wav"),
        ("white_noise.wav", "default", "white_noise_default.wav"),
        ("music_excerpt.wav", "default", "music_excerpt_default.wav"),
        # add one entry per input x preset combination that matters
    ]
    for input_file, preset, expected_name in test_cases:
        actual = f"/tmp/{expected_name}"
        render(f"{INPUTS_DIR}/{input_file}", preset, actual)
        db, err = diff_db(f"{EXPECTED_DIR}/{expected_name}", actual)
        status = "PASS" if db < THRESHOLD_DB else "FAIL"
        if err: status = f"FAIL ({err})"
        print(f"{status:6s} {expected_name:35s} diff={db:.1f} dB")
        if status != "PASS":
            failures.append(expected_name)

    if failures:
        print(f"\n{len(failures)} regression(s) detected.")
        sys.exit(1)
    print("\nAll golden reference tests passed.")

if __name__ == "__main__":
    main()
```

### Minimal Offline Render Harness

If a CLI render tool isn't already available, generate `Tests/OfflineRenderHarness.cpp`:
- Loads the plugin via `juce::AudioPluginFormatManager`
- Loads input WAV via `juce::AudioFormatReader`
- Sets a named preset via `PresetManager`
- Processes in blocks through `processBlock`
- Writes output WAV via `juce::AudioFormatWriter`

This is a one-time build (Phase 4/5), reused for every regression run afterward.

### The Critical Rule

**A failure here is not automatically a bug.** It means the sound changed. Two outcomes:

1. **Unintentional** — this is a regression. Stop, load `plugin-debugging` skill, find what changed.
2. **Intentional** — the DSP was deliberately improved/changed. Re-render and **explicitly overwrite** the golden reference for the new version, with a one-line justification in `Docs/State/Changelog.md`:
   ```
   [date] Golden reference updated for v1.1.0: improved drive stage anti-aliasing,
   ~2dB high-frequency content change above 8kHz is expected and approved.
   ```

**Never silently regenerate golden references without that changelog line.** That defeats the entire purpose of the test.

### When to run

- After every DSP file change (Phase 4, 5, 9)
- Mandatory before every release (Phase R) — gate, not optional
- After any bugfix that touches `processBlock` or a DSP class, even months after initial release

---

## A/B Blind Listening Protocol (sonic acceptance, not just numeric)

Numeric diffing catches *that* something changed. It doesn't tell you whether the change sounds *better or worse*. Run this for any DSP change beyond a trivial bugfix:

```
1. Render old version and new version of the same musical test material
2. Level-match by ear or RMS (within 0.5dB)
3. Label files A/B randomly (not "old"/"new") — have someone else label them if possible
4. Listen blind, decide preference
5. Reveal labels, record result in Docs/State/Changelog.md:
   [date] A/B listening: v1.0.0 vs v1.1.0 drive stage — v1.1.0 preferred,
   less harsh at high drive settings. Approved for release.
```

For a solo developer, even self-administered blind A/B (render both, shuffle filenames, listen days apart) catches confirmation bias that "I just changed this so it must be better" listening doesn't.

---

## Verification Checklist

- [ ] Every DSP class has at least one unit test file
- [ ] Unit tests run in CI before the plugin build job (fail fast)
- [ ] Golden reference set covers: silence, impulse, sine at low/mid/high frequency, white noise, real musical material
- [ ] Golden reference covers at least default preset + one extreme-setting preset
- [ ] `Docs/State/Changelog.md` has an entry for every golden reference update, with justification
- [ ] Golden reference regression run is a gate in `plugin-release/SKILL.md`, not optional
