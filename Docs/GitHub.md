# GitHub CI/CD Setup Guide

> Follow once per project. GitHub Actions automatically builds Windows VST3 on every push to main.
> Replace all [BRACKETED FIELDS] with your values.

---

## 1. Create the Repository

1. Go to github.com → sign in
2. Click **New** (green button)
3. Fill in:
   - Repository name: `[your-plugin-name]` (lowercase, hyphens)
   - Description: optional
   - Public or Private: your choice
   - **Do NOT** tick "Add README", "Add .gitignore", or "Choose a license"
4. Click **Create repository**
5. Copy the repository URL: `https://github.com/[USERNAME]/[repo-name].git`

---

## 2. Generate a Personal Access Token

1. Profile photo (top right) → **Settings**
2. Bottom of left sidebar → **Developer settings**
3. **Personal access tokens** → **Tokens (classic)**
4. **Generate new token (classic)**
5. Fill:
   - Note: `Mac push token`
   - Expiration: 90 days or No expiration
   - Scopes: tick **repo** (ticks all sub-boxes)
6. **Generate token**
7. **Copy immediately** — GitHub shows it once only
8. Format: `ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`
9. Save in a password manager — never commit to the repo

---

## 3. Connect Project to GitHub

```bash
cd [YOUR PROJECT PATH]

# Tell git who you are (once ever)
git config --global user.name "[YOUR NAME]"
git config --global user.email "[YOUR GITHUB EMAIL]"

# Initialise and connect
git init
git remote add origin https://github.com/[USERNAME]/[repo-name].git

# Create .gitignore
cat > .gitignore << 'EOF'
Build/
*_build/
DerivedData/
.DS_Store
*.xcodeproj/xcuserdata/
*.xcworkspace/
*.o
*.d
*.a
EOF

# First push
git add .
git commit -m "Initial commit"
git push -u origin main
# When prompted: username = [GITHUB USERNAME], password = [TOKEN from step 2]
```

---

## 4. GitHub Actions Workflow File

```bash
mkdir -p .github/workflows
```

Create `.github/workflows/build.yml`:

```yaml
name: Build Plugin

on:
  push:
    branches: [ main ]

jobs:
  build-macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v4

      - name: Configure CMake
        run: |
          cmake -B build -G Xcode \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
            -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

      - name: Build AU
        run: |
          xcodebuild \
            -project build/[PLUGIN_NAME].xcodeproj \
            -target [PLUGIN_NAME]_AU \
            -configuration Release \
            ONLY_ACTIVE_ARCH=NO \
            ARCHS="arm64 x86_64" \
            build

      - name: Build VST3
        run: |
          xcodebuild \
            -project build/[PLUGIN_NAME].xcodeproj \
            -target [PLUGIN_NAME]_VST3 \
            -configuration Release \
            ONLY_ACTIVE_ARCH=NO \
            ARCHS="arm64 x86_64" \
            build

      - name: Upload macOS artifacts
        uses: actions/upload-artifact@v4
        with:
          name: [PLUGIN_NAME]_macOS
          path: |
            ~/Library/Audio/Plug-Ins/Components/[PLUGIN_NAME].component
            ~/Library/Audio/Plug-Ins/VST3/[PLUGIN_NAME].vst3

  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4

      - name: Configure CMake
        run: |
          cmake -B build -G "Visual Studio 17 2022" -A x64 `
            -DCMAKE_BUILD_TYPE=Release

      - name: Build VST3
        run: |
          cmake --build build --config Release --target [PLUGIN_NAME]_VST3

      - name: Upload Windows artifact
        uses: actions/upload-artifact@v4
        with:
          name: [PLUGIN_NAME]_Windows
          path: build/**/*.vst3
```

**Replace `[PLUGIN_NAME]` with your exact CMake project name in every target reference.**

```bash
git add .github/workflows/build.yml
git commit -m "Add GitHub Actions CI/CD"
git push
```

---

## 5. Windows Build Guards

Add these to any Apple-only code:

```cpp
#if JUCE_MAC
  #include <Accelerate/Accelerate.h>
#endif
```

For FetchContent paths that differ on Windows:
```cmake
if(NOT APPLE)
  file(WRITE "${CMAKE_BINARY_DIR}/missing-header.h" "")
endif()
```

---

## 6. Daily Workflow

```bash
cd [PROJECT PATH]
git add .
git commit -m "describe change"
git push
# → triggers CI build automatically
# → download Windows artifact from Actions tab
```

---

## 7. Branching Strategy

```bash
# Development always on version branch
git checkout -b v1.0.0-dev

# Merge to main to trigger CI build
git checkout main
git merge v1.0.0-dev
git push              # ← triggers Actions

# Tag a release
git tag v1.0.0
git push origin v1.0.0
```

**Actions only run on pushes to `main`.** Develop on version branches.

---

## 8. Tagging a Release

```bash
git add .
git commit -m "v1.0.0 final"
git push
git tag v1.0.0
git push origin v1.0.0
```

In GitHub web UI: **Releases → Create a new release** → choose tag → add changelog → **Publish**.
This creates a permanent download URL.

---

## 9. Common Problems

| Problem | Fix |
|---|---|
| Terminal rejects GitHub password | Use the token from step 2, not your GitHub password |
| `git push` rejected "non-fast-forward" | `git pull --rebase` then `git push` |
| Actions tab shows nothing | You're on a dev branch — merge to main first |
| Windows build: missing header | Guard with `#if JUCE_MAC` |
| Token expired | Generate new token (step 2); update Keychain if prompted |
| `git` says "unable to auto-detect email" | Run `git config --global user.email "[EMAIL]"` |

---

## 10. Quick Reference Commands

```bash
git status                                    # what changed?
git add .                                     # stage all
git commit -m "message"                       # commit
git push                                      # push + trigger CI
git log --oneline -5                          # recent commits
git checkout -b branch-name                   # new branch
git checkout main && git merge branch-name    # merge to main
git tag v1.0.0 && git push origin v1.0.0      # tag release
```
