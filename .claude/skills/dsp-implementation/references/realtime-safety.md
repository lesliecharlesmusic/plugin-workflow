# Real-Time Safety Reference

> Read this file when implementing processBlock, prepareToPlay, or any audio-thread code.
> This is the expanded detail behind the CLAUDE.md § 4 quick reference.

---

## The Hard Rules

### Allocations
The OS heap (`new`, `delete`, `malloc`, `free`) acquires a lock internally.
On the audio thread this causes priority inversion and dropouts.

**Banned in processBlock and any function it calls:**
```cpp
std::vector<float> v(n);           // heap alloc — BANNED
std::string s = "debug";           // heap alloc — BANNED
new MyObject();                    // heap alloc — BANNED
juce::AudioBuffer<float> b(2, n);  // heap alloc — BANNED (even on stack declaration)
juce::HeapBlock<float> h(n);       // heap alloc — BANNED
```

**All buffers must be declared as class members and sized in prepareToPlay:**
```cpp
// In class declaration:
juce::AudioBuffer<float> workBuffer_;
juce::HeapBlock<float> tempBlock_;

// In prepareToPlay():
workBuffer_.setSize(2, samplesPerBlock, false, true, true);
tempBlock_.realloc(samplesPerBlock);
```

### Locks
Any mutex, `std::lock_guard`, `std::mutex::lock()`, or `juce::CriticalSection::enter()` on the audio thread risks priority inversion.

**Cross-thread communication patterns (use these instead):**
```cpp
// Atomic for single values
std::atomic<float> sharedGain_ { 1.0f };

// Lock-free FIFO for structs
juce::AbstractFifo fifo_ { 64 };
std::array<MyMessage, 64> fifoData_;

// JUCE's built-in lock-free queue
juce::dsp::FixedSizeFunction<sizeof(void*) * 3, void()> pendingUpdate_;
```

### File I/O and Logging
File I/O triggers kernel calls with unpredictable latency.
`DBG()`, `std::cout`, and `juce::Logger` may allocate.

**Banned in processBlock:**
```cpp
DBG("value = " + juce::String(x));   // string alloc + file I/O — BANNED
std::cout << x;                       // BANNED
logger_->writeToLog(msg);             // BANNED
FileOutputStream f(...);              // BANNED
```

**Debugging alternative:** Use a lock-free FIFO to send values to the message thread, then log there.

---

## Parameter Pointer Caching

`AudioProcessorValueTreeState::getRawParameterValue()` traverses a tree — do not call in processBlock.

```cpp
// In PluginProcessor class declaration:
std::atomic<float>* gainParam_  = nullptr;
std::atomic<float>* driveParam_ = nullptr;
std::atomic<float>* mixParam_   = nullptr;

// In prepareToPlay() — cache once:
gainParam_  = apvts_.getRawParameterValue("gain");
driveParam_ = apvts_.getRawParameterValue("drive");
mixParam_   = apvts_.getRawParameterValue("mix");

// In processBlock — read the cached atomic:
float gain  = gainParam_->load();
float drive = driveParam_->load();
```

---

## SmoothedValue Patterns

```cpp
// Declaration:
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain_;

// In prepareToPlay():
smoothedGain_.reset(sampleRate, 0.05);          // 50ms smoothing time
smoothedGain_.setCurrentAndTargetValue(*gainParam_);  // initialise to current value

// In processBlock — per-sample update:
for (int s = 0; s < numSamples; ++s) {
    float gain = smoothedGain_.getNextValue();   // advances smoother — call ONCE per sample
    channelData[s] *= gain;
}

// Never call getNextValue() more than once per sample per smoother.
// Never call setCurrentAndTargetValue() in processBlock — only in prepareToPlay/reset.
```

### Filter Coefficient Smoothing (when cutoff is automatable)

```cpp
// Smooth the frequency value, not the coefficients directly:
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCutoff_;

// In processBlock, per block:
smoothedCutoff_.setTargetValue(*cutoffParam_);
float currentFreq = smoothedCutoff_.getNextValue();
updateCoefficients(biquad_, currentFreq, Q_, sampleRate_);
// Process block with these coefficients
// On next block: repeat with next smoothed value
```

---

## Circular Buffer Rules

Use power-of-2 sizes with bitmask to avoid modulo:
```cpp
static constexpr int kBufferSize = 4096;   // must be power of 2
static constexpr int kBufferMask = kBufferSize - 1;

int writePos_ = 0;

// Write:
buffer_[writePos_ & kBufferMask] = sample;
writePos_++;

// Read with offset:
float delayed = buffer_[(writePos_ - delaySamples) & kBufferMask];
```

Never use `%` for circular buffer indexing — it is not free and generates branches on some architectures.

---

## applyGain vs applyGainRamp

```cpp
// WRONG — constant gain, no ramp across block boundary:
buffer.applyGain(gainValue);

// CORRECT — ramp from last value to new value:
buffer.applyGainRamp(0, numSamples, previousGain_, currentGain_);
previousGain_ = currentGain_;
```

Use `applyGainRamp` whenever the gain value may have changed since the last block.
Use `applyGain` only for truly constant values (e.g. a fixed -6dB compensation that never changes).

---

## ScopedNoDenormals Placement

Denormals (extremely small floating-point values) cause catastrophic slowdowns on x87/SSE2.

```cpp
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;  // ← MUST BE FIRST LINE
    // All processing below is protected
    ...
}
```

The protection applies from the point of declaration to the end of the scope.
If declared after other code, those early lines are unprotected.
