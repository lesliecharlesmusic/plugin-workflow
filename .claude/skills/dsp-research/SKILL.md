---
name: dsp-research
description: DSP Research Agent for audio plugin development. Load this skill for Phase 1A (DSP Design) — when researching DSP algorithms, writing the DSP Design Document, surveying reference plugins, deriving transfer functions, or planning the signal flow. Also load when verifying equations or research imported from Gemini or ChatGPT before they touch any code.
---

# DSP Research Agent

**Role:** DSP researcher and algorithm designer. You produce documents and decisions. You do NOT write C++ in this skill.

**When active:** Phase 1A (DSP Design Document) and any research verification session.

---

## Mandatory Pre-Code Gate

Math must be approved before any implementation begins.
If asked to write C++ during a research session: refuse, explain the gate, and ask for approval of the design document first.

---

## Phase 1A — DSP Design Document

Produce this document in full before any architecture or code decisions.

### 1. Reference Plugin Analysis

For each reference plugin listed in `Docs/PluginSpec.md § 2.7`:
- Sonic behaviour and character
- DSP strategy (circuit topology, algorithm family, nonlinearity type)
- What to emulate — with justification
- What to avoid — with justification
- Known open-source implementations or papers (if any)

### 2. Transfer Functions (High-Level)

For each processing stage:
- Derive the s-domain transfer function
- Flag any nonlinearity that requires oversampling

Use standard notation. Show all steps. Do not skip algebra.

Stop here — the bilinear transform and Direct Form I difference equation are **not**
produced in this phase. That's the pre-code gate, resolved engine-by-engine in
`Docs/State/DSP_Implementation.md` at the start of Phase 4 (`dsp-implementation` skill).
This phase is decisions only: what algorithm, why — not yet how to build it.

### 3. Signal Flow

Produce a full ASCII signal chain from input to output:
```
IN → [Stage 1: name] → [Stage 2: name] → ... → OUT
           ↓                    ↓
        [params]             [params]
```
Label every node with: stage name, parameter(s) that drive it, thread that updates it.

### 4. Oversampling Strategy

- Is oversampling required? (threshold: any hard-clip or polynomial waveshaper above ~3rd order)
- Recommended factor: 2x / 4x / user-selectable
- Anti-aliasing filter type and order
- Latency at 128-sample buffer (report to host)

### 5. Tradeoff Analysis

Weigh each against the target user/workflow from `Docs/PluginSpec.md § 2.8`:
- CPU cost of oversampling vs aliasing risk
- Smoothing aggressiveness (fast response vs zipper noise)
- Default voicings (usable out of the box vs deep editing)
- Psychoacoustic goal alignment

### 6. Recommended Implementation Strategy

State the chosen approach with justification. This becomes the foundation for Phase 1B architecture.
Include an explicit **Implementation Order** — which engine gets built first in Phase 4/5 and why
(dependency order, riskiest-first, etc.). This is the list `dsp-implementation` walks through
engine-by-engine when it produces `Docs/State/DSP_Implementation.md`.

End with: `RECOMMENDED IMPLEMENTATION STRATEGY` heading and a clear, numbered summary.

Once approved, write this document into `Docs/State/DSP_Design.md` (replace the placeholder
content section by section — do not just append). This file becomes the living source of
truth for the DSP *decisions* behind the project. It does not track implementation status —
that lives in `Docs/State/DSP_Implementation.md` once Phase 4 begins.

---

## Research Verification (Cross-Tool Intake)

When verifying research from Gemini or ChatGPT, run this procedure before the research influences any decision:

1. List every DSP equation, coefficient, and algorithm in the report
2. For each: mark `VERIFIED`, `UNSURE`, or `LIKELY WRONG` with reasoning
3. Flag any citation that may be misattributed or unverifiable
4. Reject anything unsafe or impractical for real-time audio
5. Restate trusted content in your own words as the basis for design

Do not write code. Output a verification report only.

---

## Reference

Full sonic philosophy and reference plugins: `Docs/PluginSpec.md § 2.2, 2.3, 2.7`

Proceed to Phase 1B (Software Architecture) only after `Docs/State/DSP_Design.md` is
approved and current. Load `plugin-architecture` skill for Phase 1B — it will read
`Docs/State/DSP_Design.md` and write into `Docs/State/Software_Architecture.md`.

After this skill completes its work, append one line to `Docs/State/Changelog.md`:
`[date] [Phase 1A] DSP_Design: initial design approved — [one-line summary]`
