---
name: plugin-release
description: Release Agent for audio plugin development. Load after Phase 10 QA sign-off AND beta testing exit criteria are met — for versioning, building release binaries, code signing and notarization, assembling the distribution bundle, generating the manual, and tagging the GitHub release. Do not load before Phase 10 hard blockers all pass and Docs/BetaTesting.md exit criteria are met.
---

# Release Agent

**Role:** Release engineer and distribution packager. You ensure the shipped artefact is correct, signed, bundled, and documented.

**Gate:** Phase 10 hard blockers must ALL be checked, AND `Docs/BetaTesting.md` exit criteria must be met, before this skill is used.

---

## Pre-Release Checklist

Before building the release binary:

- [ ] Version number matches in ALL locations:
  - `CMakeLists.txt` (`project VERSION` and JUCE `VERSION`)
  - `PluginProcessor` (version constant)
  - `PluginEditor` paint() / About popup text
  - `Docs/Manual.pdf` cover page
  - Distribution bundle folder names
- [ ] All Phase 10 hard blockers passed (`Docs/State/Build_Config.md` verification table is current)
- [ ] Beta testing exit criteria met (`Docs/BetaTesting.md`)
- [ ] Golden reference regression passes on the exact commit being released (`dsp-testing` skill)
- [ ] Preset compatibility test log updated in `Docs/State/Presets.md` — old presets confirmed loading
- [ ] Factory presets are final and included in `Assets/`
- [ ] `juce_add_binary_data` asset list is up to date in `CMakeLists.txt`

---

## Release Build Sequence

```bash
# 1. Clean build (mandatory — catches stale arch issues)
rm -rf Build/

# 2. Configure
cmake -B Build -G Xcode \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15

# 3. Build AU
xcodebuild \
  -project Build/[PLUGIN NAME].xcodeproj \
  -target [PLUGIN NAME]_AU \
  -configuration Release \
  ONLY_ACTIVE_ARCH=NO \
  ARCHS="arm64 x86_64" \
  build

# 4. Build VST3
xcodebuild \
  -project Build/[PLUGIN NAME].xcodeproj \
  -target [PLUGIN NAME]_VST3 \
  -configuration Release \
  ONLY_ACTIVE_ARCH=NO \
  ARCHS="arm64 x86_64" \
  build

# 5. Verify universal binary
lipo -info Build/[path]/[PLUGIN NAME].component/Contents/MacOS/[PLUGIN NAME]
# Expected: Architectures in the fat file: ... are: x86_64 arm64

# 6. Final auval
auval -v aumf [PLUG CODE] [MFR CODE]
# Expected: AU VALIDATION SUCCEEDED
```

---

## Code Signing

### Local development (no certificate needed)
Ad-hoc signing is automatic. No steps required.

### Commercial release (distributing to other machines)

Requires Apple Developer Program membership (~99 USD/year):

```bash
# Sign AU
codesign --force --deep --sign "Developer ID Application: [YOUR NAME] ([TEAM ID])" \
  --options runtime \
  ~/Library/Audio/Plug-Ins/Components/[PLUGIN NAME].component

# Sign VST3
codesign --force --deep --sign "Developer ID Application: [YOUR NAME] ([TEAM ID])" \
  --options runtime \
  ~/Library/Audio/Plug-Ins/VST3/[PLUGIN NAME].vst3

# Notarize (zip first)
zip -r [PLUGIN NAME]_vX.Y.Z_macOS.zip \
  [PLUGIN NAME].component \
  [PLUGIN NAME].vst3

xcrun notarytool submit [PLUGIN NAME]_vX.Y.Z_macOS.zip \
  --apple-id "[YOUR APPLE ID]" \
  --team-id "[TEAM ID]" \
  --password "[APP-SPECIFIC PASSWORD]" \
  --wait

xcrun stapler staple [PLUGIN NAME].component
xcrun stapler staple [PLUGIN NAME].vst3
```

---

## Distribution Bundle

### macOS bundle structure
```
[PLUGIN NAME]_vX.Y.Z_macOS/
├── [PLUGIN NAME].component    (AU — universal binary)
├── [PLUGIN NAME].vst3/        (VST3 — folder, not a file)
│   └── Contents/
│       ├── MacOS/[PLUGIN NAME]
│       └── Info.plist
├── Factory Presets/
│   ├── [Preset 1].preset
│   └── ...
├── [PLUGIN NAME]_Manual.pdf
├── README.txt                 (install instructions)
└── CHANGELOG.txt
```

### macOS install instructions (README.txt content)
```
Installation:
1. Copy [PLUGIN NAME].component to ~/Library/Audio/Plug-Ins/Components/
2. Copy [PLUGIN NAME].vst3 to ~/Library/Audio/Plug-Ins/VST3/

Gatekeeper — two options:
Option A (recommended): Right-click the file → Open → Open
Option B (terminal):
  xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/[PLUGIN NAME].component
  xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/[PLUGIN NAME].vst3

3. Restart your DAW and rescan plugins.
```

### Windows bundle structure
```
[PLUGIN NAME]_vX.Y.Z_Windows/
├── [PLUGIN NAME].vst3/         ← the FOLDER is the plugin
│   └── Contents/
│       └── x86_64-win/
│           └── [PLUGIN NAME].vst3
├── Factory Presets/
├── [PLUGIN NAME]_Manual.pdf
├── README.txt
└── CHANGELOG.txt
```

Windows README must explain: the `.vst3` folder IS the plugin, not just a container.
Windows users must right-click the `.vst3` folder → Properties → Unblock, or the DAW may refuse to load it.

---

## Manual — Required Sections

Generate or commission prose for each section via `Docs/CrossToolHandoff.md → Spec Brief template`:

1. Cover page: plugin name, version, author, date
2. Table of contents
3. macOS installation (exact paths, both Gatekeeper methods)
4. Windows installation (folder structure diagram, Unblock step, per-DAW scan instructions)
5. Signal chain overview (numbered processing stages in order)
6. Panel reference (every control: name, range, default, sonic effect)
7. MIDI Learn (assign, clear, persistence) — if applicable
8. User Presets (save, load, browse, share)
9. Tips and suggested settings (3–5 practical examples)
10. Version history / changelog
11. Footer with version number and page numbers on every page

---

## Versioning Rules

```
MAJOR.MINOR.PATCH

MAJOR: Breaking change to preset format or parameter IDs (rare)
MINOR: New features, new parameters
PATCH: Bug fixes, CPU improvements, DAW compatibility fixes

DEV BUMPS: Bump PATCH on every rebuild that changes isBusesLayoutSupported()
           Logic caches by version — a bump forces re-scan
```

Version must match in ALL of these — mismatch causes user confusion and DAW issues:
- `CMakeLists.txt` → `project([NAME] VERSION X.Y.Z)` and `JUCE_VERSION "X.Y.Z"`
- `PluginProcessor.cpp` → version constant
- `PluginEditor.cpp` → About popup string
- Manual PDF cover page
- Bundle folder name
- GitHub tag
- GitHub Release title

---

## GitHub Release

```bash
# Tag the release
git add .
git commit -m "v[X.Y.Z] release"
git push
git tag v[X.Y.Z]
git push origin v[X.Y.Z]

# In GitHub web UI:
# Releases → Create a new release
# Choose tag v[X.Y.Z]
# Title: [PLUGIN NAME] v[X.Y.Z]
# Description: changelog for this version
# Upload: macOS bundle .zip and Windows bundle .zip
# Publish release
```

GitHub Actions builds Windows VST3 automatically on push to `main`.
Download the Windows artefact from the Actions tab before assembling the Windows bundle.

---

## Post-Release

- Update `CLAUDE.md § 2`: version bumped, phase status complete
- Append to `Docs/State/Changelog.md`: `[date] [Release] v[X.Y.Z] shipped — [one-line summary]`
- Save the current golden reference renders as the new baseline:
  `Docs/GoldenReference/expected/v[X.Y.Z]/` — copy from the verified release build renders
- Update `Docs/State/Presets.md` compatibility log with this version's row
- Create dev branch for next version: `git checkout -b v[X.Y.Z+1]-dev`
- Never develop on `main` — merges to `main` trigger CI builds
