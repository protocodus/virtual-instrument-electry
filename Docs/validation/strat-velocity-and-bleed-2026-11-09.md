# Strat velocity curve and the 0 Hz bleed's tremolo (2.4.4)

Two changes, both heard on the same listening set: how picking force grows
with MIDI velocity, and a slow swell on every note that came from the
waveguide's 0 Hz bleed.

## A tremolo on every note

### What it was

The owner heard the velocity listening set "a bit like tremolo, a wobbling
effect". Every partial of a note rose and fell together at 2.5 to 8 Hz over
its first half second, more on higher notes and harder strokes.

Measured as each envelope's RMS departure from a quadratic decay over 60 to
600 ms after the onset (Hilbert envelope low-passed at 30 Hz, sampled every
20 ms), whole band, at the neck pickup and velocity 77 (`RenderStrat --tab
--pickup 5`, 48 kHz), against the Strat recorded direct (IDMT-SMT-Guitar
dataset 1, neck pickup, the same string and fret):

| Note | 2.4.3 | 2.4.4 | Recorded Strat |
| --- | --- | --- | --- |
| E2, open low E | 0.40 dB at 2.5 Hz | 0.15 dB | 0.12 dB |
| D3, open D | 0.59 dB at 3.7 Hz | 0.06 dB | 0.05 dB |
| A3, G string fret 2 | 0.73 dB at 5.4 Hz | 0.07 dB | 0.04 dB |
| E4, open high E | 1.10 dB at 6.6 Hz | 0.06 dB | 0.05 dB |
| A4, high E fret 5 | 0.77 dB at 7.6 Hz | 0.18 dB | 0.04 dB |
| E2, velocity 127 | 0.63 dB | 0.18 dB | |
| A4, velocity 127 | 1.22 dB | 0.17 dB | |

In 2.4.3 the fundamental, 2nd and 3rd partials moved by the same amount at
the same rate: a gain on the whole note, not partials beating. What is left in
2.4.4 is the two planes' slow beating (0.4 to 2 Hz), which the recording has
too.

### Where it came from

Each mechanism was switched off in a scratch build:

- a linear pickup (the flux law read at the rest gap) removes it, so the
  pickup reads a slow swing of the string's distance from its poles;
- the coils' displacement observer forgetting its drift at 1 Hz instead of
  5 Hz deepens it, at 20 Hz halves it;
- the saddle's 0 Hz bleed (2.4.1) switched off removes it.

Dumped from the neck coil, the string's distance below 20 Hz swung ±0.84 mm
on A4 at velocity 77 (the swing itself peaks at 1.6 mm), ±0.69 on A3 and
±0.32 on the low E.

The bleed takes the waveguide's 0 Hz mode (+c on the right rail, −c on the
left, a standing slope no damper on r + l reaches) out through a 3.5 Hz
low-pass of the period mean, written back at the saddle. The loop meets that
correction once a period and sums it, so the slope falls at the bleed times
f0 per second against a lagging mean: a second-order loop, with natural
frequency sqrt(w κ) and damping sqrt(w / κ) / 2, w = 2π 3.5 Hz and κ = 0.25
f0. It rings at 2.9 Hz with damping 0.52 on the low E and at 7.6 Hz with
damping 0.22 on A4, close to the rates measured above. Its swing moves the string
slowly at the coils, and the flux law turns that into gain.

### The fix

The bleed now scales with the period, κ = w / 4 per second on every note,
which damps the loop critically: the slope decays as (1 + t/τ) e^(−t/τ),
τ = 91 ms, without swinging back. Below 20 Hz the neck coil's distance then
moves ±0.10 mm on A4, ±0.08 on A3 and ±0.04 on the low E, less than with
no bleed at all (0.17, 0.18 and 0.14).

A faster critical loop is more visible: w at 7 Hz leaves 0.11 to 0.24 dB on
E2, A3 and A4, at 14 Hz 0.23 to 0.29.

Muted strings still shed the slope: the low E's peak is under 1e-8 within
1.8 s of the mute and released strings fall silent within 2.5 s
(`Electry.StratString`, `Electry.StratPerformer`). It goes later than in
2.4.3, whose underdamped loop took it faster: on the G string at fret 5, a
stroke 0.5 s into a mute differs from one on a fresh string by 80 dB under
the note, where 2.4.3 left 126 dB. That broke one fixture, which compared a
stroke after 500 strokes and a mute against a fresh string: one stroke and 500 missed it
alike, so it measured the bleed. It now compares 500 strokes against one
with the same settle and mute, its tolerance unchanged. Neither form fails
with 2.4.1's re-centring of the force reference taken out, so it guards
many strokes against one rather than that fix; that is written beside it.

The Reason Rack Extension's Electry pin (eef9db8) carries the 2.4.1 bleed, so
Electry in Reason has the same swell until its pin advances.

## Velocity curve

### Measure

Picking force cannot be read from a sample library's level, which is
normalised or shaped by an amp, but it can be read from the attack's pitch
glide: a string swinging harder is stretched further and starts sharper. On
this model the glide is g0 + c F² to within 0.2 cents over the force range.
The estimator is the one the ordinary stroke was fitted with
(StratPerformer): the fundamental band-passed 0.7 to 1.4 f0 (4th-order
Butterworth, zero phase), the analytic signal's phase slope weighted by its
squared magnitude, 15 to 60 ms after the onset (the first sample at a tenth
of the peak) against 0.4 to 0.8 s.

Per note, g = g0 + k F(v)² is fitted over the median of each velocity layer,
each layer placed at its zone's middle velocity; the exponent p in
F ∝ 0.05 + 0.95 (v/127)^p is the one that fits all notes best, and its
interval comes from resampling the notes (300 resamples). The same fit on the
model's own renders, at the same notes and velocities with the linear law,
gives back p = 0.97 (15 to 60 ms) and 0.98 (60 to 150 ms).

### Evidence

Read on the owner's copies on the Mac; nothing was copied off it.

- [Clean El Guitar](https://www.pianobook.co.uk/packs/clean-el-guitar/),
  Sustain Pick Dn, four layers (zone middles 20, 56, 88.5 and 116), 21 notes
  MIDI 45 to 79: p = 1.89 (1.48 to 2.44, 90 %); 60 to 150 ms, 2.45 (1.89 to
  3.20); with the layers at their zones' tops 2.55, bottoms 1.43.
- The same, Sustain Pick Up: p = 1.01 (0.69 to 1.35). The high register
  shows glides below zero that look like an artefact.
- Down and up together: p = 1.57 (1.28 to 1.91).
- [62 Reissue](https://www.pianobook.co.uk/packs/62-reissue-stratocaster/):
  through its amp the first 60 ms are not the string's and the later glide
  is buried in 1 to 3 cents of beating, so pitch cannot rank its layers.
  Their levels span 6.5 dB (5.3 to 7.5) and 9.4 dB (8.3 to 10.4) from the
  softest, where the linear law at the zone middles gives 7.0 and 9.7; the
  amp's compression makes that a lower bound.

The downstrokes ask for a steeper curve than the upstrokes and the '62's
levels, and nothing measured says which a player's MIDI velocity follows, so
it went to a listening test.

### Listening test and choice

`velocity-phrase.mid`: one note each at velocity 20, 45, 77, 105 and 127 on
the open low E, A3 on the G string and A4 on the high E; an accented 118/50
riff on the A string; an E major strum at 35 and at 120. `RenderStrat --tab
--pickup 5 --tail 0.8`, 48 kHz, with only `velocityForce` changed; whole-file
RMS matched (B −1.46 dB, C −2.53 dB, all −10.24 dB to put the loudest peak at
−1 dBFS).

- A: the linear law, 3.0 N at velocity 127.
- B: p = 1.55, the pooled fit.
- C: p = 1.9, the downstrokes.

Every letter kept the ordinary stroke at 1.88 N. The owner chose halfway
between A and B by ear: p = 1.275, inside the upstrokes' interval and at the
pooled fit's lower edge (Docs/decisions.md).

Force and level against the linear law (the deflection, so the pickup's
level, follows the force):

| Velocity | 1 | 20 | 45 | 56 | 77 | 88 | 105 | 116 | 127 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2.4.3 | 0.17 N | 0.60 | 1.16 | 1.41 | 1.88 | 2.12 | 2.51 | 2.75 | 3.00 |
| 2.4.4 | 0.18 N | 0.48 | 1.03 | 1.31 | 1.88 | 2.19 | 2.71 | 3.05 | 3.40 |
| level | +0.2 dB | −2.0 | −1.0 | −0.6 | 0 | +0.3 | +0.7 | +0.9 | +1.1 |

## Checks

`cmake --build build-dsp` (Release, GCC, `-O3`) and `ctest --test-dir
build-dsp`: every `unit` and `audio` test passes, and `ElectryDSPCxx17`
builds. The `perf` tier (`Electry.CpuBudget`) and the plug-in's audio and
fuzz tests need a plug-in build, which this session did not have.
