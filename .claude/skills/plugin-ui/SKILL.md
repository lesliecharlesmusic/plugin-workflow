---
name: plugin-ui
description: UI/UX Agent for audio plugin development. Load for Phase 6 (HTML prototype) and Phase 7 (UI Implementation — LookAndFeel, PluginEditor, custom components). Also load when drawing knobs, metering, spectrum displays, sync buttons, about screens, or any visual component. Do not load for DSP or CMake work.
---

# UI/UX Agent

**Role:** Visual designer and JUCE GUI engineer. Phases 6 and 7 are strictly separated — HTML approval gate must pass before any JUCE code is written.

---

## General Design Defaults (apply in both Phase 6 and 7 unless the spec overrides them)

- **Typography:** strict 3-tier hierarchy — hero readout, secondary readout, label — and stop there; more tiers blur together. Medium weight reads better than regular on dark themes; reserve bold for data values. Pick a hard floor for body text size on the target background and test it on the actual target display, not just a high-DPI dev screen.
- **Consistency across equivalent elements:** any set of conceptually-equivalent readouts (e.g. five meters that are "the same kind of thing") must share one font size/weight and one refresh rate — a mismatch reads as a bug even when each element is correct in isolation.
- **Meters / data visualizations:** stacked colour zones (green/amber/red), not whole-element colour change — only the portion in each zone shows that zone's colour. Align reference lines (0 line, baseline) across all parallel meter/graph groups in the same panel. Never show a scale-based visual without printed markings. Any counter that can grow unbounded needs an explicit display ceiling so layout never breaks at high values. Global tick rate: `kMeterTickHz = 30` (~33.3ms) — every meter bar polls at this same rate; asymmetric ballistics easing (`kMeterRiseMs = 15`, `kMeterFallMs = 250`) rides on top of that same tick, it is not a separate timer.
- **Window/panel behaviour:** a fixed, non-resizable window is the strong default — it trades flexibility for zero scaling bugs and pixel-perfect alignment, which matters more than resizing for most plugin UIs. Pop-out/auxiliary panels must never move or resize the main window when toggled. Minimal header (branding/status only) + footer toolbar (controls) keeps the eye on the content area.
- **Interactive-control opacity:** every clickable/draggable control (knob, button, combo
  box, slider, handle) defaults to `kIdleOpacity = 0.88f`, rising to `kHoverOpacity = 1.0f`
  on mouse-over.
- **Hover/focus highlight:** draw it *around* the control's bounds (an outer ring/glow
  extending beyond the hit area) — never a colour wash painted on the control's own
  surface. A highlight that reads as "on" the control is easy to mistake for a state
  change in the control itself; keeping it strictly outside avoids that ambiguity.

---

## Screenshot-Audit Protocol (highest-value QA step for GUI work)

Compiling and passing `auval`/`pluginval` tells you nothing about clipped labels, misaligned
scales, or invisible hover states — only a screenshot does.

- Screenshot-audit the HTML prototype before requesting Phase 6 approval
- Screenshot-audit again after **every** page-level GUI change in Phase 7, not just at milestone gates
- Fix, re-screenshot, repeat until clean
- Once the prototype is locked and approved, treat it as the spec to diff the real JUCE implementation against
- Disregard the `DevPhaseLabel` corner overlay (`CLAUDE.md § 4` Debug Instrumentation) when
  auditing — it's temporary dev instrumentation, not part of the design, and must never be
  added to the HTML prototype itself

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
- [ ] Screenshot-audit passed clean (see Screenshot-Audit Protocol above)

---

## Phase 7 — UI Implementation (JUCE GUI)

**Only begin after HTML prototype is explicitly approved.**

### Step 0 — Produce UI_Implementation.md (gate before any JUCE code)

Read `Docs/State/UI_Design.md` (must be approved) and the locked HTML prototype. No visual
design decisions happen here — those already happened in `UI_Design.md` and the approved
prototype; this is pure translation to a build spec.

Confirm prerequisites: prototype approved, DSP stable (Phase 5 complete — parameters exist
to attach to), fonts/images embedded in BinaryData. Produce:

1. File-by-file implementation order
2. Component inventory (component, target file, status)
3. Colour/font constants — literal value + symbol name, pulled directly from the prototype
4. Data flow diagram — what crosses the Processor↔Editor thread boundary and how
5. Verification checklist (destructor order, screenshot-audit, HiDPI, etc.)

**User must explicitly approve this document before any JUCE file is generated.** If asked
to skip this gate: refuse and explain why it exists — the same reasoning as the DSP
pre-code gate (`dsp-implementation/SKILL.md`) applies here.

Once approved, write it into `Docs/State/UI_Implementation.md`, replacing placeholder
content section by section. Append one line to `Docs/State/Changelog.md`:
`[date] [Phase 7] UI_Implementation: build spec approved`.

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

### Shared Tooltip Component (build once, thread every knob into it)

The single highest-value UI decision available: **one shared tooltip component**, not a
per-knob hover label. Every knob/slider/handle-owning component threads a reference to it
into its constructor rather than building its own hover display.

Owns exactly three things:
- **Positioning** — fixed clock-position relative to the target (e.g. 2:30), just outside
  the control's bounds
- **Fade** — `ComponentAnimator::fadeOut()` on mouse-exit, smooth, ~200ms
- **Text formatting** — re-queried live on every hover/move/drag, never cached, so it
  always reflects automation or a value being dragged in real time

Visual spec — literal values, not a description:
- `kTooltipBackground = 0xFFD9D9D9` (light grey)
- `kTooltipBorder = 0xFF000000` (black), 1px
- `kTooltipCornerRadius = 4.0f` (slightly rounded)
- `kTooltipText = 0xFF000000` (black)
- `kTooltipHoverDelayMs = 400` — plain hover waits this long before appearing
- Active-drag shows immediately (no delay) — one bool distinguishing hover vs. drag is
  enough, don't build two separate code paths for this

Double-click-to-type: pre-fill a small text box with just the numeric value (no unit
suffix), all text selected, Return commits (parsed/clamped to the parameter's real
range — a bare `"4"` for a ratio control reads as `4.00`, not rejected), Escape/focus-loss
cancels. Build this into the shared tooltip class, not bolted onto individual sliders.
**Do not rely on JUCE's native `setTooltip()`/`TooltipWindow`** — inconsistent timing,
can't be skinned to this spec.

Replace JUCE's own default before it fights your custom double-click behavior:
`slider.setDoubleClickReturnValue(false, 0.0);` immediately after every attachment is
constructed (see `CLAUDE.md § 12`) — otherwise a stationary double-click both opens your
type-in flow AND silently resets the value to default first.

For custom on-canvas controls (a draggable handle on a graph, not a `Slider`), attach a
plain `juce::ParameterAttachment` directly rather than hiding a `Slider` behind the
drawing — its callback already arrives via its own internal `AsyncUpdater`, off the audio
thread, so there's no need for the extra indirection.

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

### Undo/Redo Shortcuts

Wire standard keys to the `juce::UndoManager` owned by the Processor (see
`plugin-architecture/SKILL.md` Threading Model):
```cpp
bool [PluginName]Editor::keyPressed(const juce::KeyPress& key) {
    auto& undo = processor_.getUndoManager();
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
        return undo.undo();
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier
                                    | juce::ModifierKeys::shiftModifier, 0))
        return undo.redo();
    return false;
}
```
Scope stays user-initiated changes (GUI gestures, preset loads) — host automation isn't
recorded onto the undo stack.

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

Use a `juce::Timer`-driven component at the shared `kMeterTickHz = 30` (~33.3ms). The
asymmetric rise/fall ballistics (`kMeterRiseMs = 15`, `kMeterFallMs = 250`) are an easing
curve evaluated once per tick, not a second timer — every meter bar in the plugin polls
and eases on this same 30Hz tick, per the Consistency-across-equivalent-elements default
above:
```cpp
class LevelMeter : public juce::Component, public juce::Timer {
public:
    void timerCallback() override {
        const float target = processor_.getLevelForMeter();   // atomic read
        const float ms = target > displayLevel_ ? kMeterRiseMs : kMeterFallMs;
        const float coeff = 1.0f - std::exp(-1000.0f / (ms * kMeterTickHz));
        displayLevel_ += (target - displayLevel_) * coeff;
        repaint();
    }
    void paint(juce::Graphics& g) override { /* draw bar using displayLevel_ */ }
private:
    float displayLevel_ = 0.f;
};
```

Do not drive meter updates from the audio thread.

---

## Floating Popups in AU/VST3 Hosts (CallOutBox, ComboBox)

`juce::CallOutBox::launchAsynchronously(content, area, nullptr)` and `juce::ComboBox` both
create independent top-level OS windows (NSWindow on macOS). In an AU host (Logic Pro) this
causes three specific failures:

- **They do not track the plugin window.** When the host moves the plugin, the popup stays
  at its original screen position.
- **Clicks inside the plugin do not dismiss them via JUCE's modal manager.** Logic embeds
  the plugin editor in its own NSView and handles the click before JUCE sees it — both
  `MouseListener` and `ModalComponentManager` dismissal paths fail silently.
- **`ComponentListener::componentMovedOrResized` also fails** for detecting host-window
  movement — the plugin component's local bounds don't change when Logic's window moves,
  only its screen coordinates do.

**Reliable patterns:**

1. *Tracking movement:* poll `parent->getScreenPosition()` at 60 Hz (piggyback on an existing
   timer), compute the delta, and call `CallOutBox::updatePosition()` to move the callout's
   underlying NSWindow. This is the only path that works across hosts.
2. *Dismissing a CallOutBox on click inside the plugin:* poll `box->getPeer()->isFocused()`.
   Clicking inside the plugin hands OS focus back to Logic's NSWindow, so the callout's peer
   losing focus is the dismiss signal. Add a ~5-frame grace period after launch so the
   callout has time to acquire focus before monitoring starts.
3. *Dismissing ComboBox dropdowns on window move:* there is no public API to reposition a
   showing `PopupMenu` — the only option is dismissal. Call
   `juce::PopupMenu::dismissAllActiveMenus()` in the editor's 60 Hz timer whenever
   `getScreenPosition()` changes. One check in one place covers every ComboBox in the plugin.

**Implementation pattern — self-managing watcher:** allocate the watcher with `new` (no
owner); it self-deletes via `MessageManager::callAsync` once the watched `CallOutBox` is
gone. Use `juce::Component::SafePointer<CallOutBox>` to detect deletion safely without a
dangling pointer, and implement `ComponentListener::componentBeingDeleted` to handle the
parent being torn down.

**What does NOT work in an AU host:** `ComponentListener::componentMovedOrResized` for
host-window movement; `MouseListener` with `wantChildEvents = true` for detecting clicks
inside the plugin; `getTopLevelComponent()` as the CallOutBox parent (changes visual style —
constrains bounds, different arrow behaviour — and was rejected).

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
- [ ] Any `CallOutBox`/`ComboBox` tested against host-window movement and click-inside-plugin dismissal in Logic
- [ ] Screenshot-audit taken and clean (see Screenshot-Audit Protocol above)

---

## After Each GUI File Is Complete

1. Update `Docs/State/UI_Implementation.md`'s Component Inventory status for this component
2. Append to `Docs/State/Changelog.md`: `[date] [Phase 7] UI_Implementation: [component] implemented`

## After Phase 7 Complete

- Update `Docs/State/UI_Implementation.md` status table to "Implemented" for all components
- Update `CLAUDE.md § 2` current phase
- Load next skill per the Phase Roadmap in `CLAUDE.md § 3`
