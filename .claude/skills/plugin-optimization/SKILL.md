---
name: plugin-optimization
description: Optimization Agent for audio plugin development. Load for Phase 9 — analysing DSP code for SIMD opportunities, cache locality, branch reduction, denormal review, and CPU profiling. Also load when a plugin is using too much CPU, causing xruns under load, or when preparing for commercial release performance targets. Do not implement any optimization without an approved analysis report first.
---

# Optimization Agent

**Role:** Performance analyst and SIMD engineer. You produce an analysis report first. No code changes without approval.

**When active:** Phase 9 (after `Docs/State/Presets.md` is current and unit tests pass).

---

## Pre-Implementation Gate

Generate the analysis report. Wait for explicit approval of priority ratings before writing any optimized code.

---

## Phase 9 — Optimization Analysis Report

Analyse all DSP files from `Source/DSP/`. Produce this report structure:

### 1. SIMD Vectorization Opportunities

For each DSP loop:
- Is it auto-vectorizable? (contiguous memory, no data dependencies, no branches)
- JUCE `FloatVectorOperations` candidates:
  - `multiply`, `add`, `addWithMultiply`, `clip`
  - `findMinAndMax`, `magnitude`
- ARM NEON / SSE2 manual intrinsics: only if `FloatVectorOperations` is insufficient
- Rating: **HIGH / LOW / ALREADY OPTIMAL**

### 2. Cache Locality

- Are processing buffers accessed sequentially? (cache-friendly: channel-major for short blocks)
- Are coefficients and state in the same struct? (avoids cache miss on state access)
- Are separate per-channel state arrays laid out for sequential access?
- Rating per DSP class

### 3. Branch Reduction

- Are there conditional branches inside per-sample loops? (branch misprediction cost at 192kHz)
- Can mode switches use a function pointer or lookup table instead of `if/else`?
- Can clipper threshold branches use `juce::jlimit` (branchless on ARM)?
- Rating per loop

### 4. Denormal Review

- Confirm `ScopedNoDenormals` covers all IIR feedback paths
- Any IIR filter not covered? Add `y += 1e-25f` DC offset guard
- Rating: PASS / NEEDS GUARD

### 5. Oversampling CPU Cost

- Current factor and latency
- Cost at 192kHz (4x OS = 768kHz internal rate)
- Is the anti-aliasing filter order appropriate?
- Would 2x be perceptually sufficient? (saves ~50% CPU vs 4x)
- Rating

### 6. Remaining Allocation Risk

- Final scan: any `new`, `resize`, `push_back`, `std::string` in audio-thread call graph
- Rating: PASS / FAIL (fails block release)

---

## Implementation (After Approval)

Generate optimized versions only for HIGH PRIORITY items approved in the report.
One file per response. Standard Verification Loop applies.

### JUCE FloatVectorOperations patterns

```cpp
// Instead of a scalar loop:
for (int s = 0; s < n; ++s) out[s] = in[s] * gain;

// Use:
juce::FloatVectorOperations::multiply(out, in, gain, n);

// Multi-channel add-with-multiply:
juce::FloatVectorOperations::addWithMultiply(dest, src, multiplier, n);

// Clip to range:
juce::FloatVectorOperations::clip(dest, src, -threshold, threshold, n);
```

### SIMD-friendly struct layout

```cpp
// Cache-unfriendly (interleaved channels, poor vectorization):
struct Sample { float L, R; };
std::array<Sample, 512> buffer;

// Cache-friendly (planar, vectorizes cleanly):
std::array<float, 512> bufferL;
std::array<float, 512> bufferR;
```

### Branchless mode switching

```cpp
// Instead of:
if (mode_ == Mode::Soft) output = softClip(input);
else                     output = hardClip(input);

// Use function pointer (set on parameter change, message thread):
std::atomic<float(*)(float)> clipFn_ { &softClip };

// In processBlock:
auto fn = clipFn_.load();
for (int s = 0; s < n; ++s) out[s] = fn(in[s]);
```

### Oversampling factor switching (no click)

```cpp
void setOversamplingFactor(int newFactor) {
    // Called on message thread
    pendingFactor_.store(newFactor);
}

// In processBlock — check and apply atomically:
if (int f = pendingFactor_.exchange(-1); f > 0) {
    // Mute one block, reinitialise, resume
    buffer.clear();
    oversampler_.reset();
    oversampler_.initProcessing(maxBlockSize_);
    setLatencySamples(static_cast<int>(oversampler_.getLatencyInSamples()));
}
```

---

## CPU Profiling Protocol

Before optimization, establish a baseline:

1. Open Reaper or Logic
2. Load 20 instances of the plugin
3. Set buffer size to 128 samples
4. Observe CPU meter
5. Record: `[plugin name] v[X.Y.Z] — 20 instances @ 128 buf = [X]% CPU`

After optimization:
1. Repeat measurement
2. Record delta in `Docs/State/Build_Config.md`
3. Append to `Docs/State/Changelog.md`: `[date] [Phase 9] Build_Config: optimized [X] — CPU [before]% → [after]%`

Target: 20 instances at 128-sample buffer should not exceed 30% CPU on a mid-range Apple Silicon Mac (M1/M2 class).

---

## Mandatory Gate Before Moving On

Any optimization that touches DSP code is itself a DSP change. Before considering
Phase 9 complete:

1. Load `dsp-testing` skill
2. Re-run all unit tests — must still pass with identical expected outputs
3. Re-run golden reference regression — confirm optimization did NOT change the sound
   (if it did, that's a bug, not a successful optimization, unless explicitly intended
   and logged per the dsp-testing skill's "intentional change" process)

---

## Verification

Standard Verification Loop (`CLAUDE.md § 7`) plus:
- [ ] No regression in `auval` after optimization
- [ ] No new audio artifacts (clicks, aliasing) introduced
- [ ] `lipo -info` still shows universal binary
- [ ] CPU measurement taken before and after, logged in `Docs/State/Build_Config.md`
- [ ] Unit tests and golden reference regression both pass after optimization
