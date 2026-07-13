---
name: dsp-implementation
description: DSP Implementation Agent for audio plugin development. Load for Phase 4 (Core DSP Engine) and Phase 5 (Advanced DSP — oversampling, IR loading, MIDI learn). Also load whenever writing processBlock logic, biquad filters, waveshapers, gain staging, SmoothedValue wiring, or any C++ that runs on the audio thread. After completing a DSP class, switch to the dsp-testing skill to write its unit tests before moving on. Do not load for CMake, GUI, or preset code.
---

# DSP Implementation Agent

**Role:** Real-time DSP engineer. You write audio-thread C++ that is allocation-free, lock-free, and sample-accurate. Every file goes through the math-approval gate before code is written.

**When active:** Phase 4 (core DSP) and Phase 5 (advanced DSP).

**Reference files** (read when relevant):
- `.claude/skills/dsp-implementation/references/realtime-safety.md` — audio thread rules detail
- `.claude/skills/dsp-implementation/references/biquad-patterns.md` — owned biquad implementations

---

## Pre-Code Gate — Produce DSP_Implementation.md (Non-Negotiable)

Before writing any DSP C++, read `Docs/State/DSP_Design.md` and
`Docs/State/Software_Architecture.md` — both must be approved and current. No algorithm
exploration happens here; that already happened in `DSP_Design.md`. This gate is pure
translation from decisions to a build spec.

For every engine in `DSP_Design.md`'s Signal Flow / Implementation Order, produce:

1. The bilinear transform and resulting difference equation (Direct Form I)
2. Class members — what state this engine owns
3. The `processBlock` integration point — where and how it's called
4. The parameter smoothing wiring
5. Any remaining open question for this engine — must be resolved, not deferred

**User must explicitly approve each engine's build spec before any code is generated for
that engine.** If asked to skip this gate: refuse and explain why it exists — skipping it
reintroduces algorithm decisions into the C++ pass, which is exactly what causes
mid-implementation rewrites.

Once approved, write the build spec into `Docs/State/DSP_Implementation.md`, replacing
placeholder content section by section — do not just append. Append one line to
`Docs/State/Changelog.md`: `[date] [Phase 4] DSP_Implementation: build spec approved for
[engine] — [one-line summary]`.

Every class generated below must trace to an approved entry in `DSP_Implementation.md`.
If an engine doesn't have one yet, produce it first rather than deriving the design inline.

---

## Phase 4 — Core DSP Engine

Generate files in this order. One file per response. Wait for approval between files.

### File Generation Order

```
1. Source/DSP/[PrimaryDSPClass].h
2. Source/DSP/[PrimaryDSPClass].cpp
3. Source/DSP/[SecondaryDSPClass].h  (e.g. ToneStack, ClipStage)
4. Source/DSP/[SecondaryDSPClass].cpp
5. Source/PluginProcessor.h  (update to wire DSP)
6. Source/PluginProcessor.cpp (update)
   → xcodebuild Debug after each integration
```

### DSP Class Requirements

Every DSP class must:

```cpp
class MyDSPStage {
public:
    void prepare(double sampleRate, int maxBlockSize);  // pre-allocate here
    void reset();                                        // zero state, called on transport reset
    void process(juce::AudioBuffer<float>& buffer,
                 juce::AudioPlayHead::CurrentPositionInfo* pos = nullptr);
    void setParameter(ParamID id, float value);         // called from Processor, audio thread

private:
    // Owned biquad state — Direct Form I, per channel
    struct BiquadState {
        float b0=0, b1=0, b2=0, a1=0, a2=0;
        float x1=0, x2=0, y1=0, y2=0;   // per channel — use array[2] for stereo
    };

    // SmoothedValues for every automatable parameter
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain;

    // Pre-allocated buffers
    juce::AudioBuffer<float> oversampled;  // if needed
    std::vector<float> workBuffer;         // sized in prepare(), never in process()

    double sampleRate_ = 44100.0;
    int maxBlockSize_  = 512;
};
```

**Precision:** any long-running accumulator this class owns — envelope followers,
RMS/level integrators, gain-reduction history — accumulates in `double`; only the
final displayed/output value narrows to `float`. Decide this per accumulator now,
not after a drift bug appears after extended runtime (see `CLAUDE.md § 4` Numerical
Precision).

### processBlock Pattern

```cpp
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;  // FIRST LINE

    // Mono-in stereo-out channel duplication
    if (getTotalNumInputChannels() == 1 && getTotalNumOutputChannels() == 2)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

    // Read cached parameter pointers (cached in prepareToPlay — never getRawParameterValue here)
    const float gainVal = *gainParam_;
    smoothedGain_.setTargetValue(gainVal);

    // Process per sample or per block depending on stage
    dspStage_.process(buffer);

    // Output gain with ramp — never applyGain()
    buffer.applyGainRamp(0, buffer.getNumSamples(),
                         lastGain_, smoothedGain_.getCurrentValue());
    lastGain_ = smoothedGain_.getCurrentValue();
}
```

### Biquad Direct Form I (owned — no JUCE private state)

```cpp
inline float processBiquad(BiquadState& s, float x) noexcept {
    float y = s.b0*x + s.b1*s.x1 + s.b2*s.x2
                     - s.a1*s.y1 - s.a2*s.y2;
    s.x2 = s.x1; s.x1 = x;
    s.y2 = s.y1; s.y1 = y;
    return y;
}

// Coefficient update (bilinear transform result):
void updateCoefficients(BiquadState& s, float freq, float Q, float sampleRate) {
    // Pre-warp
    float w0 = 2.0f * juce::MathConstants<float>::pi * freq / sampleRate;
    float alpha = std::sin(w0) / (2.0f * Q);
    float cosw0 = std::cos(w0);
    float a0inv = 1.0f / (1.0f + alpha);
    s.b0 = (1.0f - cosw0) * 0.5f * a0inv;
    s.b1 = (1.0f - cosw0) * a0inv;
    s.b2 = s.b0;
    s.a1 = -2.0f * cosw0 * a0inv;
    s.a2 = (1.0f - alpha) * a0inv;
    // Do NOT reset x1,x2,y1,y2 here — only in prepare()/reset()
}
```

---

## Phase 5 — Advanced DSP

After `Docs/State/DSP_Design.md` and `Docs/State/Software_Architecture.md` reflect
completed Phase 4 work. Generate in this order:

### 5A: OversamplingManager

Generate `Source/DSP/OversamplingManager.h`:
- Wraps `juce::dsp::Oversampling<float>`
- Factor from spec: `Docs/PluginSpec.md § 2.3`
- `prepare(sampleRate, blockSize)` → calls `oversampler.initProcessing()`
- `processupsample(buffer)` → returns `AudioBlock` at oversampled rate
- `processDownsample()` → writes back to original buffer
- Reports latency: `oversampler.getLatencyInSamples()` → `setLatencySamples()`
- Factor change (discrete, not continuous) uses mute-reset-unmute, not a parallel-instance
  crossfade: ramp to silence, swap/reset the oversampler at the silent instant, ramp back
  up. Cheaper than an expensive parallel crossfade and sounds identical for anything that
  isn't automated at audio rate — which a discrete topology switch like this never is.
- The moment `getLatencySamples()` is nonzero because of this, override
  `processBlockBypassed()` explicitly (see `CLAUDE.md § 12`) — its JUCE default asserts
  bypass reports the same latency as normal processing
- **Don't double-count latency:** if another subsystem also adds delay (an intentional
  look-ahead pre-delay, for example), audit exactly which path picks up which delay from
  where — on paper — before wiring dry-path compensation. It's easy to explicitly
  compensate the dry path for both delays while the wet path picks up the oversampling
  filter's own round-trip latency for free from the subsystem itself, leaving the wet path
  delayed by less than the dry path assumes. That shows up as comb-filtering/phasing that's
  easy to mistake for something unrelated.

### 5B: IR Loader (if applicable)

Generate `Source/DSP/IRLoader.h`:
- Load `.wav` from `BinaryData` or user file path
- `juce::dsp::Convolution` on message thread — swap atomic pointer to audio thread
- Loading on audio thread is a hard error — assert

### 5C: MIDI Learn (if applicable)

Generate `Source/Parameters/MidiLearn.h`:
- CC → parameter ID map stored as `std::array` (no allocation)
- Persisted in APVTS state (`getStateInformation` / `setStateInformation`)
- Assignment on message thread; readout via atomic on audio thread

---

## Denormal Prevention

Every IIR filter must include denormal protection:
```cpp
// Option 1: ScopedNoDenormals at top of processBlock (covers all)
// Option 2: DC offset injection for IIR feedback path
y += 1e-25f;  // inject before feedback accumulation
```

Both options are acceptable. `ScopedNoDenormals` is preferred when it covers all stages.

---

## Measurement / Detector Classes

Any windowed or gated measurement (an N-second loudness integration, an envelope that
needs to fill before it means anything) must expose an explicit `isReady()`/
`hasValidReading()` query — don't let callers infer "not ready yet" from the value still
sitting at its initial floor/default. A caller that finalizes or acts on the measurement
(not just displays it) has no way to tell "genuinely measured this and it's quiet" apart
from "never measured anything," and treating a floor-vs-floor delta as a real result is
numerically valid but semantically meaningless.

---

## prepareToPlay Checklist

`prepareToPlay()` can legitimately be called again mid-session by the host at an unchanged
sample rate (a documented AU/VST3 behavior, sometimes tied to the plugin's UI becoming
active) — guard the whole function against this redundant-call case with one check at the
top, not a bespoke guard per side effect discovered one bug report at a time. A redundant
call that isn't guarded can snap in-progress `SmoothedValue` ramps to target (audible
click if it lands mid-drag) or re-send a "latency changed" host notification that wasn't
needed (see `CLAUDE.md § 4` Discontinuity Prevention).

```cpp
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const bool isRedundantCall = everPrepared_
        && sampleRate == lastSampleRate_
        && samplesPerBlock == lastBlockSize_;
    if (isRedundantCall)
        return;   // nothing below should re-run for a same-rate re-prepare
    everPrepared_ = true;
    lastSampleRate_ = sampleRate;
    lastBlockSize_ = samplesPerBlock;

    // 1. Cache sample rate and block size
    // 2. Cache ALL getRawParameterValue() pointers
    // 3. Resize/initialise all pre-allocated buffers
    // 4. Call prepare() on every DSP object
    // 5. Reset all SmoothedValues with current parameter values
    // 6. Reset all BiquadState structs
    // 7. Call setLatencySamples() for oversampling
}
```

---

## Verification (after every file)

See `CLAUDE.md § 7` for full Verification Loop.
DSP-specific additions:
- [ ] No `new`, `delete`, `malloc`, `std::vector::resize` in `process()`
- [ ] `ScopedNoDenormals` is first line of `processBlock`
- [ ] `getNextValue()` called exactly once per sample per SmoothedValue
- [ ] Coefficient update does NOT reset filter state
- [ ] All biquad state owned (no `juce::dsp::IIR::Filter.state`)
- [ ] Oversampling latency reported to host via `setLatencySamples()`

---

## After Each DSP Class Is Complete

1. Load `dsp-testing` skill and write unit tests for the new class before moving on
2. Update `Docs/State/DSP_Implementation.md`'s Current Implementation Status table for this engine
3. If architecture changed from the original plan, update `Docs/State/Software_Architecture.md` too
4. Append to `Docs/State/Changelog.md`: `[date] [Phase 4/5] DSP_Implementation: [class] implemented and unit tested`

## After Phase 4 / Phase 5 Complete

- Run the full unit test suite — all must pass
- Update `Docs/State/DSP_Implementation.md` status table to "Implemented" for all engines
- Update `CLAUDE.md § 2` current phase
- Load next skill per the Phase Roadmap in `CLAUDE.md § 3`
