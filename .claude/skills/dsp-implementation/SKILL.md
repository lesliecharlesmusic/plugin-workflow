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

## Pre-Code Gate (Non-Negotiable)

Before writing any DSP C++:

1. Show the transfer function equation for this stage
2. Show the signal flow through this class in ASCII
3. Show the bilinear transform and resulting difference equation
4. Explain the parameter smoothing strategy
5. Identify any nonlinearity requiring oversampling

**User must explicitly approve the math before any code is generated.**
If asked to skip this gate: refuse and explain why the gate exists.

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
- No click on factor change: mute for one block, reset, resume

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

## prepareToPlay Checklist

```cpp
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
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
2. Update `Docs/State/DSP_Design.md` implementation status table for this stage
3. If architecture changed from the original plan, update `Docs/State/Software_Architecture.md` too
4. Append to `Docs/State/Changelog.md`: `[date] [Phase 4/5] DSP_Design: [class] implemented and unit tested`

## After Phase 4 / Phase 5 Complete

- Run the full unit test suite — all must pass
- Update `Docs/State/DSP_Design.md` status table to "Implemented" for all stages
- Update `CLAUDE.md § 2` current phase
- Load next skill per the Phase Roadmap in `CLAUDE.md § 3`
