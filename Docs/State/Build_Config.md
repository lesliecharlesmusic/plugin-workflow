# Build Config — [PLUGIN NAME]

> LIVING DOCUMENT. Reflects the CURRENT state of CMakeLists.txt and build decisions.
> Edit in place when build configuration changes. History belongs in Changelog.md.

---

## Current CMake Targets

| Target | Type | Status |
|---|---|---|
| `[PLUGIN_NAME]_AU` | AU | |
| `[PLUGIN_NAME]_VST3` | VST3 | |
| `[PLUGIN_NAME]Tests` | Catch2 unit tests | |
| `[PLUGIN_NAME]Assets` | BinaryData | |

---

## Architecture / Platform

- **Universal binary:** arm64 + x86_64
- **macOS deployment target:**
- **Windows target:**

---

## FetchContent Dependencies

| Library | Pinned Tag | Include Path Fix Needed |
|---|---|---|
| Catch2 | v3.5.4 | no |
| | | |

---

## AU Configuration

- **AU_MAIN_TYPE:** `kAudioUnitType_MusicEffect`
- **NEEDS_MIDI_INPUT:** TRUE

---

## Current Binary Verification

| Build | lipo -info result | auval result | Date |
|---|---|---|---|
| | | | |

---

## Known Build Issues

*(Anything currently broken or fragile in the build — be honest, this saves future debugging time)*

