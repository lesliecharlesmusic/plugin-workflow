# Golden Reference Audio Files

> See `.claude/skills/dsp-testing/SKILL.md` for full usage.

## inputs/
Fixed test material — never changes once created. Generate once in Phase 4/5:

- `silence.wav` — pure digital silence, 2 seconds
- `impulse.wav` — single sample impulse, rest silence
- `sine_100hz.wav` — 2 seconds, -12dBFS
- `sine_1khz.wav` — 2 seconds, -12dBFS
- `sine_8khz.wav` — 2 seconds, -12dBFS
- `white_noise.wav` — 5 seconds, -18dBFS RMS
- `music_excerpt.wav` — real musical material relevant to plugin category
  (e.g. bass DI for a bass distortion plugin, drum bus for a compressor)

Generate with:
```bash
# Example: sox for sine generation
sox -n sine_1khz.wav synth 2 sine 1000 vol -12dB
sox -n white_noise.wav synth 5 whitenoise vol -18dB
sox -n silence.wav trim 0 2
```

## expected/v[X.Y.Z]/
Golden reference renders for a specific version. Created once per version when that
version is confirmed correct (by ear + by intent). Never edited except via the
explicit "intentional change" process in the dsp-testing skill — and always with a
corresponding Changelog.md entry.

## Do not commit /tmp render outputs
Render outputs from `render_and_diff.py` go to `/tmp` and are not committed.
Only `inputs/` and `expected/vX.Y.Z/` are version controlled.
