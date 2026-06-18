---
name: plugin-ui
description: UI/UX Agent for audio plugin development. Load for Phase 6 (HTML prototype) and Phase 7 (JUCE GUI implementation — LookAndFeel, PluginEditor, custom components). Also load when drawing knobs, metering, spectrum displays, sync buttons, about screens, or any visual component. Do not load for DSP or CMake work.
---

# UI/UX Agent

**Role:** Visual designer and JUCE GUI engineer. Phases 6 and 7 are strictly separated — HTML approval gate must pass before any JUCE code is written.

---

## Phase 6 — HTML Prototype (Visual Approval Gate)

**No JUCE code is written in Phase 6. The HTML prototype must be fully approved before Phase 7 begins.**

### Prototype Requirements

Generate a single-file interactive HTML/CSS/JS prototype:

- Pixel-exact window dimensions from `Docs/PluginSpec.md § 2.5`
- Visual theme from spec § 2.5 (colours, materials, era)
- All controls from spec § 2.4 — interactive and draggable knobs
- Logo top-centre; click opens About popup (version, author, build date)
- Popup closes on click outside
- All meters and visualisations stubbed with animated placeholders
- Sync button / knob pairs behave as described in spec

### Knob Interaction (HTML)

```javascript
// Rotary drag behaviour — vertical mouse movement → rotation
let isDragging = false, startY = 0, startAngle = 0;
knob.addEventListener('mousedown', e => {
    isDragging = true; startY = e.clientY;
    startAngle = currentAngle;
});
document.addEventListener('mousemove', e => {
    if (!isDragging) return;
    currentAngle = startAngle + (startY - e.clientY) * sensitivity;
    currentAngle = Math.max(minAngle, Math.min(maxAngle, currentAngle));
    knob.style.transform = `rotate(${currentAngle}deg)`;
});
document.addEventListener('mouseup', () => isDragging = false);
```

### Approval Gate

Before proceeding to Phase 7, confirm:
- [ ] Window size correct
- [ ] Colour palette approved
- [ ] All controls present and positioned
- [ ] Knob aesthetic approved
- [ ] About popup works
- [ ] Overall feel matches sonic identity from spec § 2.2

---

## Phase 7 — JUCE GUI Implementation

**Only begin after HTML prototype is explicitly approved.**

Generate files in this order. One file per response.

```
1. Source/GUI/LookAndFeel.h         (custom L&F class declaration)
2. Source/GUI/LookAndFeel.cpp       (custom L&F implementation)
3. Source/GUI/PluginEditor.h        (editor declaration)
4. Source/GUI/PluginEditor.cpp      (editor implementation)
5. Source/GUI/[CustomComponent].h   (metering, spectrum, etc. — as needed)
```

### LookAndFeel Requirements

```cpp
class [PluginName]LookAndFeel : public juce::LookAndFeel_V4 {
public:
    // Named colour constants — match HTML approved palette
    static constexpr uint32_t kBackground  = 0xFF1A1A1A;
    static constexpr uint32_t kAccent      = 0xFF[HEX];
    static constexpr uint32_t kKnobBody    = 0xFF[HEX];
    static constexpr uint32_t kKnobTrack   = 0xFF[HEX];
    static constexpr uint32_t kLabel       = 0xFFCCCCCC;

    // Override:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& s) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& b,
                              const juce::Colour& bg, bool hovered, bool down) override;

    // All drawing: float coordinates, HiDPI-aware
    // g.setColour() persists — always restore after painting special elements
};
```

### Sync Button / Knob Pattern

```cpp
// In paint() for a sync-engaged knob — dot indicator:
void MySyncKnob::paint(juce::Graphics& g) {
    // Draw knob...
    if (isSyncEngaged_) {
        g.setColour(juce::Colour(kAccent));
        g.fillEllipse(dotBounds_);
        g.setColour(juce::Colour(kLabel));  // ← restore colour after dot
    }
    // Draw label (uses restored kLabel colour)...
}
```

### Editor Destructor (non-negotiable order)

```cpp
[PluginName]Editor::~[PluginName]Editor()
{
    // 1. Reset ALL dynamic attachments FIRST — before anything else
    gainAttach_.reset();
    driveAttach_.reset();
    mixAttach_.reset();
    syncAttach_.reset();
    // ... every SliderParameterAttachment, ButtonParameterAttachment, ComboBoxAttachment

    // 2. Stop timer
    stopTimer();

    // 3. Clear LookAndFeel
    setLookAndFeel(nullptr);
}
```

Failure to follow this order causes `EXC_BAD_ACCESS` when Logic or any AU host closes the plugin window. C++ destructs members in reverse declaration order — attachments can outlive their target sliders without explicit reset.

### Editor Constructor Pattern

```cpp
[PluginName]Editor::[PluginName]Editor([PluginName]Processor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    // 1. Set LookAndFeel first
    setLookAndFeel(&lookAndFeel_);

    // 2. Configure all sliders and buttons
    gainSlider_.setSliderStyle(juce::Slider::Rotary);
    gainSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(gainSlider_);

    // 3. Create attachments after controls exist
    gainAttach_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor_.getAPVTS(), ParameterIDs::kGain, gainSlider_);

    // 4. Set fixed window size
    setSize([WIDTH], [HEIGHT]);
    setResizable(false, false);
}
```

### About Popup

```cpp
void [PluginName]Editor::mouseDown(const juce::MouseEvent& e) {
    if (logoBounds_.contains(e.getPosition())) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::NoIcon,
            "[PLUGIN NAME]",
            "Version " + juce::String(ProjectInfo::versionString) + "\n"
            "[AUTHOR CREDIT]\n"
            "Built with JUCE 8",
            "Close");
    }
}
```

### Metering (if required)

Use a `juce::Timer`-driven component at 30Hz:
```cpp
class LevelMeter : public juce::Component, public juce::Timer {
public:
    void timerCallback() override {
        level_ = processor_.getLevelForMeter();   // atomic read
        repaint();
    }
    void paint(juce::Graphics& g) override { /* draw bar */ }
private:
    float level_ = 0.f;
};
```

Do not drive meter updates from the audio thread.

---

## Verification (after every file)

See `CLAUDE.md § 7`.
UI-specific checks:
- [ ] Editor destructor resets all attachments before `stopTimer()` and `setLookAndFeel(nullptr)`
- [ ] All drawing uses float coordinates (HiDPI-safe)
- [ ] `g.setColour()` restored after every special element (sync dots, etc.)
- [ ] No timer started in constructor without corresponding `stopTimer()` in destructor
- [ ] No raw pointer to Processor stored in Editor without lifetime consideration
- [ ] BinaryData namespace matches the unique one set in CMakeLists.txt
