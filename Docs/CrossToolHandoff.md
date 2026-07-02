# Cross-Tool Handoff Templates

> Use these when moving work between Claude Code, Gemini, and ChatGPT.
> The rule: research and writing can come from any tool. Audio code comes only from Claude Code.
> Every technical claim from another tool is verified on Claude before it touches the codebase.

---

## Tool Allocation Quick Reference

| Task | Tool | Notes |
|---|---|---|
| All DSP C++ | Claude Code only | Never offload |
| Architecture, threading | Claude Code only | Never offload |
| Verifying offloaded equations | Claude Code (Template 1) | Mandatory before coding |
| DSP literature research | Gemini Deep Research | Cross-check with ChatGPT |
| Summarising long docs | Gemini | Bring conclusions to Claude |
| Manual prose, marketing | ChatGPT or Gemini | Fact-check against real build |
| Logo, textures, icons | ChatGPT / Gemini image gen | Must have rights to ship |
| HTML UI mockup (Phase 6) | Claude Code or ChatGPT | Approved mockup returns here |
| Generic git/CMake Q&A | Free ChatGPT or Gemini | |
| Parameter tables, test logs | ChatGPT / Gemini Sheets | Paste final data back |

---

## Template 1 — Research Intake and Verification
### Use: When bringing Gemini or ChatGPT research into Claude Code

Paste this into Claude Code with the research report attached:

```
You are the DSP Research Agent for [PLUGIN NAME].
Below is a research report produced by [GEMINI / CHATGPT], model [NAME].

Do NOT treat any of it as correct yet. Before we use any of it:
1. List every DSP equation, coefficient, or algorithm in the report.
2. For each, mark it VERIFIED, UNSURE, or LIKELY WRONG, and explain why.
3. Flag any citation that may be misattributed or unverifiable.
4. Reject anything unsafe or impractical for real-time audio.
5. Restate trusted content in your own words as the basis for our design.

Do not write any code yet.

----- RESEARCH REPORT BELOW -----
[PASTE FULL REPORT]
```

---

## Template 2 — Spec Brief for External Tool
### Use: When sending a writing or design task to ChatGPT or Gemini

Run this in Claude Code first to generate a self-contained brief:

```
Produce a self-contained Spec Brief for an external writing/design tool.
Include, with no reference to our conversation:
1. Plugin name, manufacturer, and version number.
2. One-paragraph description of what the plugin does.
3. Full parameter list: each control, range, default, and sonic effect.
4. Signal chain in processing order.
5. Install facts: formats, folder destinations, macOS Gatekeeper and
   Windows Unblock steps.
6. Anything the external tool must NOT claim or change.
Keep it factual and complete enough to use with zero extra context.
```

Save output as `Docs/SpecBrief_vX.Y.Z.md` then paste into the external tool.

---

## Template 3 — Generic Cross-Tool Handoff
### Use: When moving any work between any two tools

```
CROSS-TOOL HANDOFF
From tool / model: [e.g. Gemini 3 Pro, Deep Research Max]
To tool: [e.g. Claude Code]
Task handed off: [what was asked]
Inputs provided: [what the source tool was given]
Outputs produced: [summary of what came back]
Confidence / gaps: [what is uncertain or unverified]
Sources / citations: [links, if any]
Next tool must: [verify X / implement Y / fact-check Z]
```

---

## Phase 6 HTML Offload (Optional)
### Use: If offloading the UI mockup to ChatGPT to save Claude Code usage

Send to ChatGPT with this brief:

```
I'm building an audio plugin UI mockup in interactive HTML/CSS/JS.
Here is the spec:

Window size: [W × H px]
Visual theme: [describe]
Controls: [list from PluginSpec.md § 2.4]
Meters: [describe or "none"]
Logo: "[PLUGIN NAME]" — top centre, click opens About popup
About popup: version [X.Y.Z], "[AUTHOR CREDIT]", closes on click outside

Requirements:
- Single HTML file (inline CSS and JS)
- Knobs are rotary, interactive, draggable (vertical mouse movement)
- Pixel-accurate to the window size
- No external dependencies

Deliver a complete single HTML file.
```

After iteration and approval in ChatGPT, bring the final HTML back to Claude Code
to start Phase 7. Paste it with:

```
The HTML prototype below has been approved.
Begin Phase 7. Load .claude/skills/plugin-ui/SKILL.md.
Produce Docs/State/UI_Implementation.md first (Step 0 — file order, component
inventory, colour/font constants, data flow, verification checklist). Do not
generate any JUCE file, including LookAndFeel.h, until that document is approved.

[PASTE APPROVED HTML]
```

---

## Manual Prose Offload
### Use: After Phase 10, to draft manual sections in ChatGPT or Gemini

1. Generate the Spec Brief in Claude Code (Template 2 above)
2. Paste Spec Brief into ChatGPT with:

```
Using the spec below, draft the following manual sections in friendly, plain prose:
- macOS installation
- Windows installation  
- Signal chain overview
- Panel reference (all controls)
[add sections from plugin-release/SKILL.md § Manual]

Do not invent technical claims. Use only the facts in the Spec Brief.
```

3. Bring the draft back to Claude Code for fact-checking:

```
The manual draft below was written by ChatGPT.
Compare every technical claim against our build:
- Parameter names, ranges, and defaults
- Install paths
- Signal chain stage names and order
Flag any error. Do not rewrite — just list corrections.

[PASTE DRAFT]
```

4. Apply corrections and finalise in your word processor.
