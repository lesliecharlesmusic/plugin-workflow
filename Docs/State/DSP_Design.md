# DSP Design — [PLUGIN NAME]

> LIVING DOCUMENT. Edit in place when the DSP approach changes — do not append history here.
> History of changes belongs in Changelog.md, not here.
> This file always reflects CURRENT, ACCURATE DSP design. If it's wrong, fix it now.
> Decisions only — what and why. No difference equations, no class design, no C++.
> The pre-code build spec (per-engine difference equations, class members, `processBlock`
> wiring) lives in `Docs/State/DSP_Implementation.md`, produced at the start of Phase 4.

---

## Reference Plugin Analysis

*(One entry per reference from PluginSpec.md § 2.7)*

### [Reference Name]
- **Sonic behaviour:**
- **DSP strategy:**
- **What we take:**
- **What we avoid:**

---

## Competitive Landscape

*(Research Round 2 — professional plugins beyond the references above, sourced via web
search, not training-data recall)*

### [Competitor Name]
- **Sonic behaviour / feature set vs our planned approach:**
- **Documented criticism / feedback:**
- **Does our approach already address this gap, or should it adjust:**

*(repeat per competitor — no cap on how many)*

---

## Frontier Techniques Considered

*(Research Round 3 — DSP techniques/algorithms/papers not yet widely adopted in
commercial plugins, anchored to credible sources: DAFx, JAES, arXiv audio/DSP)*

### Safe to Adopt Now
*(Sound theoretical grounding, feasible in real time, clear fit for this plugin)*

### Experimental / Risky
*(Interesting but unproven — CPU cost, stability, or DSP soundness uncertain.
User chooses whether to pursue any of these.)*

---

## Transfer Functions (High-Level)

### Stage: [Name]
- **s-domain:**
- **Requires oversampling:** Yes/No — why

*(repeat per stage — the bilinear transform and difference equation are resolved later,
in `Docs/State/DSP_Implementation.md`, not here)*

---

## Signal Flow

```
IN → [Stage 1] → [Stage 2] → [Stage 3] → OUT
        ↓             ↓
     [params]      [params]
```

---

## Oversampling Strategy

- **Factor:**
- **Anti-aliasing filter:**
- **Latency at 128-sample buffer:**
- **Justification:**

---

## Implementation Order

*(Which engine gets built first in Phase 4/5, and why — dependency order, riskiest-first,
etc. Feeds directly into `DSP_Implementation.md`'s build spec.)*

1. 

---

## Open DSP Questions / Known Limitations

*(Anything unresolved at the decision level — keep this section honest and current)*

