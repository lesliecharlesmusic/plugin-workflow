# Audio Plugin Building — Claude Code Workflow

A complete Claude Code workflow package for building professional-grade JUCE 8 audio
plugins (AU + VST3, macOS + Windows, Apple Silicon + Intel) from architecture through
release. Built around a CLAUDE.md project brain, on-demand skills, living state
documents, and a mandatory testing gate — designed to keep token usage efficient while
enforcing real-time-safety, build, and QA discipline at every phase.

This is a **product development workflow**. It produces a shippable plugin. If you're
looking for the open-ended DSP research counterpart to this repo, see
[dsp-research-lab](#) (separate repository).

---

## Why This Exists

I'm not a professional software engineer. I'm a musician, producer, and recording/mixing
engineer. This workflow started as a way to teach myself DSP and audio programming in
C++, and to understand how audio plugins actually get built, while making small tools
that genuinely help my own creative process. It's a hobby project, built for learning
and personal use, not a commercial effort. If you're coming at this from a
musical background rather than a computer science one, this was built with you in mind.

I have real respect for the engineers doing this professionally, the people behind the
plugins we all use every day. Their work reflects years of formal training, experience
earned the hard way, and judgment that no workflow can shortcut. This process can help
me aim toward that standard of craft, but it can't replace it. AI can accelerate how
fast I learn and how much I can build. It can't generate wisdom, experience, or the
years it takes to earn either.

---

## What's Here

```
.
├── CLAUDE.md                  Always-in-context project brain: identity, rules, phase router
├── Docs/
│   ├── PluginSpec.md           Fill-in spec form — the source of truth every skill reads from
│   ├── CrossToolHandoff.md     Templates for moving work between Claude/Gemini/ChatGPT
│   ├── GitHub.md               CI/CD setup guide (GitHub Actions Windows build)
│   ├── BetaTesting.md          Pre-release field testing process
│   ├── GoldenReference/        Audio regression test fixtures
│   └── State/                  Living documents — DSP/UI decisions + build specs, architecture, parameters, etc.
├── Tests/                      Catch2 unit test scaffold
└── .claude/skills/             10 on-demand skills covering the full plugin lifecycle
```

## The Model

Unlike a fixed-phase checklist, this workflow tracks **state**, not just **process**:

- `CLAUDE.md § 3` is the fixed roadmap — it never changes, it just describes the phases
- `CLAUDE.md § 2` is the one stateful line — which phase you're currently on
- `Docs/State/*.md` are living documents, edited in place as architecture evolves —
  replacing the brittle "per-phase handoff" pattern that goes stale the moment you revise
  something mid-phase
- `Docs/State/Changelog.md` is the only append-only file — the historical record of *why*
  things changed
- DSP and UI each split into a Design doc (decisions — what and why, no code) and an
  Implementation doc (a build spec — how). The Implementation doc is a mandatory pre-code
  gate produced right before its code phase starts, regardless of plugin size — not
  optional ceremony for "simple enough" plugins

## Skills

| Skill | Phase | Purpose |
|---|---|---|
| `dsp-research` | 1A | DSP design document (decisions only), high-level transfer functions, signal flow |
| `plugin-architecture` | 1B, 3, 8 | Software architecture, parameters, presets |
| `build-system` | 2 | CMake, JUCE skeleton, universal binary, CI |
| `dsp-implementation` | 4, 5 | Pre-code gate (`DSP_Implementation.md`) + core DSP, oversampling, real-time safety |
| `dsp-testing` | continuous | Unit tests + golden reference regression — runs after every DSP change |
| `plugin-ui` | 6, 7 | HTML prototype gate + pre-code gate (`UI_Implementation.md`) + JUCE GUI implementation |
| `plugin-optimization` | 9 | SIMD, CPU profiling, branch reduction |
| `plugin-qa` | 10 | Full DAW validation checklist, auval, crash resilience |
| `plugin-debugging` | any | Forensic debugging — compiler errors, crashes, artifacts |
| `plugin-release` | R | Versioning, signing, distribution bundles, GitHub release |

## Quick Start

1. Unpack into your project root (where `CLAUDE.md` and `.claude/` need to live)
2. Fill in `Docs/PluginSpec.md` and `CLAUDE.md § 1`
3. Open the folder in your IDE, open Claude Code from inside that workspace
4. First message: `Read CLAUDE.md and confirm you have it. List the phase roadmap from § 3.`

Full first-session script in `Getting_Started.pdf` (repo root) or just ask Claude Code to
read `CLAUDE.md § 6` and walk the Phase 0 checklist with you.

## Non-Negotiable Rules (Enforced Throughout)

- Zero allocations, locks, or file I/O on the audio thread
- Own all filter state — never rely on JUCE's private internals
- Every automatable parameter is smoothed
- Editor destructor resets attachments before `stopTimer()`/`setLookAndFeel(nullptr)`
- No DSP `.cpp` or JUCE GUI file before its Implementation doc (pre-code gate) is approved
- One file generated per response, verified before the next
- Golden reference regression must pass before every release — failures are investigated,
  never silently overwritten

See `CLAUDE.md § 4` for the complete list.

## Requirements

- JUCE 8.x
- CMake 3.22+
- Xcode (macOS) — Visual Studio builds handled via GitHub Actions CI, no Windows machine needed
- Claude Code

## License

[Add your license here]
