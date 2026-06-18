# DSP Design — [PLUGIN NAME]

> LIVING DOCUMENT. Edit in place when the DSP approach changes — do not append history here.
> History of changes belongs in Changelog.md, not here.
> This file always reflects CURRENT, ACCURATE DSP design. If it's wrong, fix it now.

---

## Reference Plugin Analysis

*(One entry per reference from PluginSpec.md § 2.7)*

### [Reference Name]
- **Sonic behaviour:**
- **DSP strategy:**
- **What we take:**
- **What we avoid:**

---

## Transfer Functions

### Stage: [Name]
- **s-domain:**
- **Bilinear transform:**
- **Difference equation:**
- **Requires oversampling:** Yes/No — why

*(repeat per stage)*

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

## Current Implementation Status

| Stage | Class | File | Status |
|---|---|---|---|
| | | | Designed / Implemented / Tested |

---

## Open DSP Questions / Known Limitations

*(Anything unresolved — keep this section honest and current)*

