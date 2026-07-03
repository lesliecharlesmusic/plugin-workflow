---
name: build-system
description: Build System Agent for audio plugin development. Load for Phase 2 — generating CMakeLists.txt, setting up the Xcode project skeleton, configuring universal binary builds, GitHub Actions CI/CD, or any CMake/build system task. Also load when adding new source files to the build, changing architectures, fixing configure or linker errors, or setting up the .gitignore and GitHub workflow file.
---

# Build System Agent

**Role:** CMake and Xcode build engineer. You produce build configuration files and the compilable skeleton. You do not write DSP or GUI logic.

**When active:** Phase 2 (and any subsequent CMake/build changes).

---

## Phase 2 — CMake + JUCE Skeleton

Read `CLAUDE.md § 1` for all project identity fields before generating anything.

### Step 0: Confirm the Build Output location

Default assumption: `Source/` and the `Build/` directory live in the same project root —
`Build Output` in `CLAUDE.md § 1` is `[Project Root]/Build`, a sibling of `Source/`, not a
separate location. Before generating `CMakeLists.txt`, ask the user explicitly whether
they want this changed (e.g. build artifacts redirected to a separate drive or RAM disk
for speed). If they confirm the default, proceed. If they want it changed, update
`Build Output` in `CLAUDE.md § 1` to the new path before continuing.

### Step 1: Generate CMakeLists.txt

Generate ONLY `CMakeLists.txt` first. Nothing else.

#### Required CMake configuration

```cmake
cmake_minimum_required(VERSION 3.22)
project([PLUGIN_NAME] VERSION [X.Y.Z])

# Universal binary — set at EVERY level
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)

add_subdirectory([JUCE_PATH] JUCE)

juce_add_plugin([PLUGIN_NAME]
    COMPANY_NAME "[MANUFACTURER]"
    PLUGIN_MANUFACTURER_CODE [4-CHAR]
    PLUGIN_CODE [4-CHAR]
    FORMATS AU VST3
    PRODUCT_NAME "[PLUGIN NAME]"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE           # required for aumf type
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE   # copy manually to control arch
    AU_MAIN_TYPE kAudioUnitType_MusicEffect   # aumf — all track types
    VST3_CATEGORIES "[category]"
)

# Strict warnings on plugin target ONLY — never add_compile_options()
target_compile_options([PLUGIN_NAME] PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

# Architectures on every sub-target
set_target_properties([PLUGIN_NAME]_AU PROPERTIES
    OSX_ARCHITECTURES "arm64;x86_64")
set_target_properties([PLUGIN_NAME]_VST3 PROPERTIES
    OSX_ARCHITECTURES "arm64;x86_64")
```

#### BinaryData target (guard if no assets yet)
```cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/placeholder.wav")
    juce_add_binary_data([PLUGIN_NAME]Assets
        NAMESPACE [PluginName]Assets   # unique — never default BinaryData
        HEADER_NAME "[PluginName]BinaryData.h"
        SOURCES Assets/placeholder.wav
    )
    set_target_properties([PLUGIN_NAME]Assets PROPERTIES
        OSX_ARCHITECTURES "arm64;x86_64")
endif()
```

#### FetchContent dependencies
For each third-party library:
- Pin exact version tag
- Stub platform-specific headers on non-native platforms
- Copy to expected include path after `FetchContent_MakeAvailable`

#### Test target (Catch2 — wire in from Phase 2, used starting Phase 4)
```cmake
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable([PLUGIN_NAME]Tests
    Tests/PlaceholderTest.cpp   # replace with real test files as DSP classes are written
)
target_link_libraries([PLUGIN_NAME]Tests PRIVATE
    Catch2::Catch2WithMain
    [PLUGIN_NAME]               # links against DSP code directly — no plugin wrapper needed
)
```
Add one trivial placeholder test now so the target configures and builds cleanly
before any real DSP exists — this satisfies the same "file must exist" rule as
other CMake targets. Real tests are added per the `dsp-testing` skill starting Phase 4.

#### After generating CMakeLists.txt, provide:
1. Every file listed in `target_sources()` — user must `touch` these before cmake configure
2. Every asset in `juce_add_binary_data SOURCES` — must exist before configure
3. Exact configure command:
   ```bash
   cmake -B Build -G Xcode \
     -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
     -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
   ```
4. What a successful configure output looks like

---

### Step 2: Skeleton Sequence (strict order)

Do not deviate from this order.

```
1. mkdir -p Source/{Core,DSP,Parameters,GUI,Presets} Assets/IR Tests Docs/GoldenReference/inputs
2. touch every .cpp/.h listed in target_sources()
3. Create placeholder asset (Assets/placeholder.wav — 1-sample silent WAV)
4. cmake -B Build -G Xcode            ← configure only, zero errors expected
5. Write DenormalPrevention.h         ← header-only utility
6. Write ParameterIDs.h               ← load plugin-architecture skill
7. Write ParameterLayout.h/.cpp       ← load plugin-architecture skill
8. Write PluginProcessor.h then .cpp  ← real content
9. Write PluginEditor.h then .cpp     ← real content
10. xcodebuild -scheme [NAME]_AU -configuration Debug build
11. xcodebuild -scheme [NAME]_AU -configuration Release build
12. lipo -info Build/[path]/[NAME].component/Contents/MacOS/[NAME]
    → must show: x86_64 arm64
13. Run the Post-Build Install Ritual (below) — copies AU/VST3, clears AU cache, runs auval
14. Load in Logic / AudioPluginHost to confirm it appears and opens cleanly
```

**GATE 1** (before cmake configure): All paths in `target_sources()` must exist on disk.
**GATE 2** (before xcodebuild): All `.cpp` files must have real, compilable content.

---

### PluginProcessor.cpp — buses pattern (mandatory)

```cpp
// Constructor buses — keep plain
: AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))

// isBusesLayoutSupported — canonical pattern
bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();
    if (out == juce::AudioChannelSet::mono())
        return in == juce::AudioChannelSet::mono();
    if (out == juce::AudioChannelSet::stereo())
        return in == juce::AudioChannelSet::mono()
            || in == juce::AudioChannelSet::stereo();
    return false;
}

// processBlock — mono-in stereo-out handling
if (getTotalNumInputChannels() == 1 && getTotalNumOutputChannels() == 2)
    buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
```

---

### CMake Change Protocol

When adding a new `.cpp` file:
- Ask Claude to regenerate the COMPLETE `CMakeLists.txt` as a replacement
- Never hand-edit individual lines in `target_sources()`
- After any architecture change: `rm -rf Build/` before re-running cmake

---

### Code Signing (local development)

Do NOT set any of these in CMake:
- `CODE_SIGN_IDENTITY`
- `HARDENED_RUNTIME_ENABLED`
- `APP_SANDBOX_ENABLED`
- `AU_SANDBOX_SAFE`

Ad-hoc signing (Xcode default) is sufficient for local AU/VST3 development.
Developer ID + notarization is a release-only concern (see `plugin-release` skill).

---

### Binary Verification

After every Release build:
```bash
lipo -info /path/to/[PLUGIN].component/Contents/MacOS/[PLUGIN]
# Expected: Architectures in the fat file: ... are: x86_64 arm64

# If arm64 only — two possible causes:
# 1. Stale Build/ folder → rm -rf Build/ → reconfigure → rebuild
# 2. cp -R merged rather than replaced → rm -rf destination → cp -R again
```

---

### Post-Build Install Ritual (run after every successful build)

Every successful build ends here — not just Phase 2's initial skeleton. This is the only
path artifacts should reach the install directories; it's why `COPY_PLUGIN_AFTER_BUILD` is
left `FALSE` in CMake (see the config above) — manual copy keeps architecture and
overwrite behavior under control instead of Xcode silently doing it.

1. Read `AU Install Path` and `VST3 Install Path` from `CLAUDE.md § 1`.
2. If a plugin of the same name already exists at either destination, **stop and ask the
   user for explicit permission before overwriting it** — never replace an existing
   install silently.
3. Copy the fresh artifacts:
   ```bash
   cp -R Build/[path]/[NAME].component "[AU Install Path]/[NAME].component"
   cp -R Build/[path]/[NAME].vst3 "[VST3 Install Path]/[NAME].vst3"
   ```
4. Clear the AU caches so the host can't load a stale cached verdict for the build that
   just changed — do this every time, not only when chasing a stuck-cache bug:
   ```bash
   rm -rf ~/Library/Caches/AudioUnitCache
   rm -rf ~/Library/Caches/com.apple.audio.InfoHelper
   rm -rf ~/Library/Caches/com.apple.audio.AudioComponentRegistrar 2>/dev/null
   sudo killall -9 AudioComponentRegistrar 2>/dev/null
   ```
5. Run `auval -v aumf [PLUGIN_CODE] [MANUFACTURER_CODE]` — zero failures required before
   the build counts as verified. (`auval` covers AU only; there's no Apple equivalent for
   VST3 — `pluginval` is the manual fallback there, per the Phase 0 checklist.)

---

### GitHub Actions CI/CD

See `Docs/GitHub.md` for the full setup guide and workflow file.
The workflow file lives at `.github/workflows/build.yml`.
Actions run only on pushes to `main`. Develop on version branches.

The workflow has two independent jobs: `build-macos` (AU + VST3, universal binary, verified
locally too) and `build-windows` (VST3 only — AU is Apple-only). Treat them as two separate
concerns, not one shared config:

- `build-windows` uses `-G "Visual Studio 17 2022" -A x64`. That generator invokes MSBuild,
  which finds the MSVC toolchain itself — no manual `vcvarsall`/dev-cmd activation step is
  needed. (That step is only required if a workflow switches to Ninja/NMake and calls `cl.exe`
  directly — don't switch generators without adding it back.)
- There is **no local Windows toolchain in this workflow.** `lipo` and `auval` do not apply
  to the Windows artifact — the only verification available is the `build-windows` job going
  green and the VST3 `.vst3` artifact uploading (`actions/upload-artifact`). Confirm both
  before considering any phase "Windows-complete."
- Any Apple-only header or API (`Accelerate.h`, `CoreAudio`, etc.) reachable from shared
  (non-`#if JUCE_MAC`-guarded) code breaks `build-windows` even though `build-macos` passes
  clean. Since there's no local repro, a Windows-only compile failure can only be diagnosed
  from the Actions log — push the fix and re-run rather than guessing blind.

---

## After Phase 2

When skeleton compiles, loads in host, and `auval` passes:
- Update `Docs/State/Build_Config.md` with current target list, architecture flags, and verification results
- Append to `Docs/State/Changelog.md`: `[date] [Phase 2] Build_Config: skeleton compiles, auval passes`
- Update `CLAUDE.md § 2` current phase to Phase 3
- Load `plugin-architecture` skill for Phase 3
