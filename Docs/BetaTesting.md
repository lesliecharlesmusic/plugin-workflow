# Beta Testing Process

> Closes the gap where you can only test on hardware/DAWs/OS versions you personally own.
> Run this between Phase 10 (QA sign-off) and Phase R (Release) for any version beyond a trivial patch.
> Costs almost nothing for a solo developer with a network of other audio people — high value.

---

## Why This Matters

Your own QA (Phase 10) only covers the DAWs, OS version, and hardware you have access to.
Professional shops always run a small beta before public release because:
- Different DAW versions behave differently (Logic 11.x vs 10.x quirks)
- Different OS versions surface different bugs (macOS Tahoe vs Sequoia vs older)
- Different Mac hardware (Intel still exists; different Apple Silicon generations)
- A second pair of ears catches things you've become deaf to from familiarity

This does not need to be formal. 2-4 people is enough to catch most field issues.

---

## Beta Tester Selection

- 2-4 people minimum
- At least one person on a different DAW than your primary (if you test in Logic, get a Reaper/Ableton/Cubase user)
- At least one person on different hardware (Intel Mac if you're on Apple Silicon, or vice versa)
- At least one person who is NOT a developer — a working musician who will use it the way a customer would, not the way you'd test it

---

## What to Send

```
Beta package for [PLUGIN NAME] v[X.Y.Z]-beta[N]

Contents:
- [PLUGIN NAME].component (AU)
- [PLUGIN NAME].vst3
- Quick install note (folder destinations + Gatekeeper bypass)
- This feedback form
```

---

## Feedback Form Template

```
[PLUGIN NAME] v[X.Y.Z]-beta[N] — Feedback

Your setup:
- DAW + version:
- macOS/Windows version:
- Mac model (if applicable):

Did it load correctly? Y/N — if no, describe:

Did you encounter any crashes? Y/N — if yes:
- What were you doing when it happened?
- Can you reproduce it?

Did any parameter feel wrong when automated (clicks, jumps, unresponsive)?

Sound quality — does anything sound off, harsh, noisy, or unexpected at any setting?

Anything confusing about the UI or workflow?

Would you change anything about the default sound or any factory preset?

Overall impression:
```

---

## Triage Process

For every piece of feedback received:

1. Log it in `Docs/State/Changelog.md` under a `[BETA FEEDBACK]` tag, even if not yet actioned:
   ```
   [date] [Beta] Feedback from [tester]: crash on rapid bypass toggle in Cubase 13 — investigating
   ```
2. Crashes and incorrect audio behaviour → load `plugin-debugging` skill, fix, re-test
3. Subjective sound preferences → discuss but don't chase every opinion; use judgment against your sonic philosophy in `Docs/PluginSpec.md § 2.2`
4. UI confusion reported by 2+ testers independently → treat as a real signal, not noise

---

## Exit Criteria (ready for Phase R)

- [ ] No crashes reported by any beta tester
- [ ] No DAW-loading failures reported
- [ ] Any reported audio artifact has been investigated and resolved or explicitly accepted
- [ ] Feedback logged in Changelog.md
- [ ] At least one full round of feedback received from every beta tester (don't ship while someone's still mid-test)

---

## When to Skip This

A trivial patch (typo fix, preset name change, manual correction) doesn't need a beta round.
Any change that touches `processBlock`, parameter ranges, or the build configuration does.
