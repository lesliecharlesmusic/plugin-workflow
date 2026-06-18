# Biquad Filter Patterns Reference

> Read when implementing any IIR filter stage.
> All patterns use owned state — no JUCE private filter internals.

---

## Why Own Your Biquad

In JUCE 8:
- `juce::dsp::IIR::Filter::state` is **private**
- `juce::dsp::ProcessorDuplicator::processors` is **private**
- `ProcessorDuplicator` has no `processSample()` — block mode only
- Per-sample coefficient updates require direct state access

Owning the biquad struct gives full control with zero overhead.

---

## Canonical Owned Biquad Struct

```cpp
struct BiquadCoeffs {
    float b0 = 1.f, b1 = 0.f, b2 = 0.f;
    float       a1 = 0.f, a2 = 0.f;     // a0 normalised out
};

struct BiquadState {
    float x1 = 0.f, x2 = 0.f;
    float y1 = 0.f, y2 = 0.f;
};

// For stereo: use array of state, shared coefficients
struct BiquadFilter {
    BiquadCoeffs c;
    BiquadState  s[2];   // [0] = L, [1] = R

    void reset() {
        for (auto& st : s) st = {};
    }

    inline float processSample(int ch, float x) noexcept {
        float y = c.b0*x + c.b1*s[ch].x1 + c.b2*s[ch].x2
                         - c.a1*s[ch].y1 - c.a2*s[ch].y2;
        // Denormal guard — optional if ScopedNoDenormals is active
        y += 1e-25f;
        y -= 1e-25f;
        s[ch].x2 = s[ch].x1; s[ch].x1 = x;
        s[ch].y2 = s[ch].y1; s[ch].y1 = y;
        return y;
    }
};
```

---

## Coefficient Calculations

### Low-Pass (2nd order Butterworth / RBJ)
```cpp
void calcLowPass(BiquadCoeffs& c, float freq, float Q, float fs) {
    float w0    = juce::MathConstants<float>::twoPi * freq / fs;
    float cosw  = std::cos(w0);
    float sinw  = std::sin(w0);
    float alpha = sinw / (2.f * Q);
    float a0inv = 1.f / (1.f + alpha);
    c.b0 = (1.f - cosw) * 0.5f * a0inv;
    c.b1 = (1.f - cosw)         * a0inv;
    c.b2 = c.b0;
    c.a1 = -2.f * cosw          * a0inv;
    c.a2 = (1.f - alpha)        * a0inv;
}
```

### High-Pass (2nd order)
```cpp
void calcHighPass(BiquadCoeffs& c, float freq, float Q, float fs) {
    float w0    = juce::MathConstants<float>::twoPi * freq / fs;
    float cosw  = std::cos(w0);
    float sinw  = std::sin(w0);
    float alpha = sinw / (2.f * Q);
    float a0inv = 1.f / (1.f + alpha);
    c.b0 = (1.f + cosw) * 0.5f * a0inv;
    c.b1 = -(1.f + cosw)        * a0inv;
    c.b2 = c.b0;
    c.a1 = -2.f * cosw          * a0inv;
    c.a2 = (1.f - alpha)        * a0inv;
}
```

### Peaking EQ
```cpp
void calcPeaking(BiquadCoeffs& c, float freq, float Q, float gainDB, float fs) {
    float A     = std::pow(10.f, gainDB / 40.f);
    float w0    = juce::MathConstants<float>::twoPi * freq / fs;
    float alpha = std::sin(w0) / (2.f * Q);
    float a0inv = 1.f / (1.f + alpha / A);
    c.b0 = (1.f + alpha * A) * a0inv;
    c.b1 = -2.f * std::cos(w0) * a0inv;
    c.b2 = (1.f - alpha * A) * a0inv;
    c.a1 = c.b1;
    c.a2 = (1.f - alpha / A) * a0inv;
}
```

### Low Shelf
```cpp
void calcLowShelf(BiquadCoeffs& c, float freq, float S, float gainDB, float fs) {
    float A     = std::pow(10.f, gainDB / 40.f);
    float w0    = juce::MathConstants<float>::twoPi * freq / fs;
    float cosw  = std::cos(w0);
    float sinw  = std::sin(w0);
    float alpha = sinw / 2.f * std::sqrt((A + 1.f/A) * (1.f/S - 1.f) + 2.f);
    float a0inv = 1.f / ((A+1) + (A-1)*cosw + 2.f*std::sqrt(A)*alpha);
    c.b0 = A  * ((A+1) - (A-1)*cosw + 2.f*std::sqrt(A)*alpha) * a0inv;
    c.b1 = 2.f*A * ((A-1) - (A+1)*cosw)                        * a0inv;
    c.b2 = A  * ((A+1) - (A-1)*cosw - 2.f*std::sqrt(A)*alpha) * a0inv;
    c.a1 = -2.f * ((A-1) + (A+1)*cosw)                         * a0inv;
    c.a2 = ((A+1) + (A-1)*cosw - 2.f*std::sqrt(A)*alpha)       * a0inv;
}
```

---

## Coefficient Update Rule

**Never reset filter state on a coefficient update.**
Resetting state (`x1=x2=y1=y2=0`) during playback causes a hard click.
Only reset state in `prepare()` and `reset()`.

```cpp
// CORRECT — update coefficients, leave state alone:
void setFrequency(float freq) {
    calcLowPass(filter_.c, freq, Q_, sampleRate_);
    // filter_.s[0] and filter_.s[1] are untouched
}

// WRONG — do not do this:
void setFrequency(float freq) {
    calcLowPass(filter_.c, freq, Q_, sampleRate_);
    filter_.reset();   // ← click! state zeroed during playback
}
```

---

## Multi-Stage Filter Chains

For Baxandall / 3-band tone stacks with multiple biquad stages in series:

```cpp
struct ToneStack {
    BiquadFilter bass, mid, treble;

    void prepare(double sampleRate) {
        calcLowShelf(bass.c,    200.f,  0.7f, bassGainDB_,   sampleRate);
        calcPeaking  (mid.c,    1000.f, 0.7f, midGainDB_,    sampleRate);
        calcHighShelf(treble.c, 5000.f, 0.7f, trebleGainDB_, sampleRate);
    }

    float process(int ch, float x) noexcept {
        return treble.processSample(ch,
                   mid.processSample(ch,
                       bass.processSample(ch, x)));
    }
};
```

---

## Oversampled Nonlinear Stage Pattern

```cpp
// 1. Upsample input block
auto oversampledBlock = oversampler_.processSamplesUp(inputBlock);

// 2. Apply nonlinearity per-sample at oversampled rate
for (int ch = 0; ch < oversampledBlock.getNumChannels(); ++ch) {
    float* data = oversampledBlock.getChannelPointer(ch);
    for (int s = 0; s < oversampledBlock.getNumSamples(); ++s)
        data[s] = waveshaper(data[s]);    // your nonlinearity here
}

// 3. Downsample
oversampler_.processSamplesDown(outputBlock);
```

Common waveshaper functions:
```cpp
// Soft clip (tanh approximation — fast)
inline float softClip(float x) noexcept {
    x = juce::jlimit(-3.f, 3.f, x);
    return x * (27.f + x*x) / (27.f + 9.f*x*x);  // Pade approx
}

// Hard clip
inline float hardClip(float x, float threshold) noexcept {
    return juce::jlimit(-threshold, threshold, x);
}

// Asymmetric clip (diode-style)
inline float asymClip(float x) noexcept {
    if (x > 0.f) return 1.f - std::exp(-x);
    return -1.f + std::exp(x);
}
```
