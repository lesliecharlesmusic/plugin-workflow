# DSP Implementation — [PLUGIN NAME]

> LIVING DOCUMENT. The pre-code gate — bridges `Docs/State/DSP_Design.md` and the C++ in
> `Source/DSP/`. Every open question below must be resolved before any DSP `.cpp` file is
> generated. Edit in place as engines are built. History belongs in `Changelog.md`.
> No algorithm exploration happens here — that already happened in `DSP_Design.md`. This
> document is translation-ready: once an engine's entry is approved, its C++ is pure
> translation, not design.

---

## Prerequisites

- [ ] `Docs/State/DSP_Design.md` approved and current
- [ ] `Docs/State/Software_Architecture.md` approved and current

---

## Engine-by-Engine Build Spec

*(One entry per stage in `DSP_Design.md`'s Signal Flow / Implementation Order)*

### [Engine Name]

- **Bilinear transform:**
- **Difference equation (Direct Form I):**
- **Class members / owned state:**
- **`processBlock` integration point:**
- **Parameter smoothing wiring:**
- **Open questions:** *(must be empty before this engine's code starts)*

*(repeat per engine)*

---

## Current Implementation Status

| Engine | Class | File | Status |
|---|---|---|---|
| | | | Spec written / Approved / Implemented / Tested |

---

## Open Questions / Known Limitations

*(Anything unresolved — keep this section honest and current)*
