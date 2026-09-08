# Audible plectrum attack — 8 September 2026

The fourth pass strengthens the short released-string edge of a hard pick on
wound strings. The existing loaded modal displacement still supplies the
ringing body. Pick Hardness scales the separate edge from its unchanged soft
endpoint to 2.5 times its former amplitude at the hard endpoint. The bridge
hand then applies the same existing Palm attenuation. This is an explicit
voicing decision for the requested clearer attack, not a measured pick-force
or material calibration.

The change applies to actual Sustain, Palm, natural-harmonic and fresh-Slide
plectrum contacts. Hammer and legato Slide have no new plectrum edge. Plain
strings retain their established attack: a high-E ablation showed their modal
path already supplies the bright onset, and increasing the broad pulse mostly
changed interference with that path. Dead and Pinch retain their existing
recording and squeal-contrast voicing. The natural-harmonic change in this pass
shares these ordinary pick mechanics.

## Isolated before/after evidence

The source before this change exactly matches the frozen third-pass engine
(`7efd9f2257f6f9c8db48839aacd5c94585fd557e0a94638539805c853f28a0e0`).
The existing 47-score renderer used the same settings, events, seed, raw gain
and Modern high-gain chain in both versions. These figures isolate the pick
edge from the fourth pass's other changes, at Pick Hardness 0.85:

| Phrase | Tap | 0–20 ms attack RMS change | Attack centroid before → after | Whole-file RMS change |
| --- | --- | ---: | ---: | ---: |
| E1 hard Sustain | Dry | +2.97 dB | 256 → 847 Hz | −0.07 dB |
| E1 hard Sustain | Modern | +0.89 dB | 199 → 368 Hz | +0.02 dB |
| F#1 hard Sustain | Dry | +2.33 dB | 233 → 714 Hz | −0.09 dB |
| F#1 hard Sustain | Modern | +1.02 dB | 187 → 294 Hz | +0.01 dB |
| E1 Palm repicks, 12 Hz | Dry | +0.15 dB | 173 → 185 Hz | <0.01 dB |
| E1 Palm repicks, 12 Hz | Modern | +0.43 dB | 178 → 213 Hz | +0.10 dB |

The final 80 ms note-release RMS changes by only −0.15/−0.12 dB in the E1
dry/Modern phrase and −0.12/−0.11 dB in F#1. The difference is concentrated in
the attack, rather than a large increase in sustained level. These are signal
descriptors, not listening scores. The
[isolated auditions](../build-realism-20260908-pass4/research-palm/edge25-comparison/listening.html)
use one constant whole-file RMS-matching gain per version. The saved
[probe summary](../Experiments/Realism20260908Pass4/pick-attack-probes.json)
records the comparison and waveform hashes.

The uncensored clean F#2 preview remains a useful limit on claims. Its 0–30 ms
centroid is about 272 Hz. The isolated model moves from 276 to 349 Hz, while
its following 30–80 ms centroid moves only from 258 to 264 Hz. This attack
voicing therefore does **not** improve that recording's early spectral match.
The source is one uncontrolled player/pick/setup, and the other two previews
have censored onsets. No reference-fit or perceptual-realism result is claimed.

## Rejected alternatives and checks

- Removing Palm's existing bridgeward pick relocation gave 0.6–1.4 dB more
  low-chug body, but failed the unchanged muted-versus-open tail-contraction
  guard. It was not promoted.
- A uniform fourfold edge increase raised the E1 dry attack centroid to
  1734 Hz and attack RMS by 6.69 dB. It was rejected as excessive.
- Doubling local pick scrape mostly added noise, with smaller attack changes.
  It was not promoted.

The isolated ordinary-pick candidate passed the complete engine suite's
physical and audible rails: tuning, output bounds, Palm decay, pressure,
passivity, aliasing and callback invariance. Eleven whole-source single-coil
spectral snapshots changed with the intended excitation change; their pickup
topology checks still passed. Integration refreshes those deterministic source
snapshots separately from the unchanged pickup and sound-behavior guards.

[RealismPickAttackTests](../Tests/RealismPickAttackTests.cpp) removes only the
edge through a test seam and observes actual dry and Modern output. It checks
a distinct wound-string onset, bounded late-body contribution, hardness
contrast, the absence of a Hammer plectrum edge, finite output/headroom, and
sample-identical callback partitions at 44.1, 48, 96 and 192 kHz. The normal
engine tests retain the broader tuning, chord, muting and pickup checks.
A coefficient-only negative control restores the old edge amplitude while
keeping the integrated fourth-pass source otherwise intact. It fails the
new test at all four rates: the E2-at-fret-12 edge contribution falls from
6.49–6.60 dB to 1.33–1.40 dB, below the unchanged 4 dB output guard.
