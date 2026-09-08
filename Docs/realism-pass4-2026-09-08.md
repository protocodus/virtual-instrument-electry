# Audible pick attack and ringing — 8 September 2026

This fourth pass follows the user's two priorities: a synth-like ringing tone
and an indistinct pick attack. It makes three audible changes to the shipping
DSP using the existing controls. The before version is the complete third-pass
working tree, frozen under `build-realism-20260908-pass4/baseline-src`; it is
not just Git HEAD, which contains the first two passes.

## What changes

1. **Hard picks have a clearer edge on wound strings.** Pick Hardness now
   increases the short released-string edge from its former level at the soft
   endpoint to 2.5 times that level at the hard endpoint. The loaded string
   still supplies the sustained body. Sustain, Palm, natural Harmonics and a
   Slide started without an existing note use this edge. Hammer and legato
   slides remain finger gestures. Plain strings already have a strong modal
   attack; boosting their edge did not help the measured onset. Dead and Pinch
   retain their existing contact and squeal voicing. This is a deliberate
   attack-voicing choice, not a measured pick-force coefficient.
2. **The two bass strings shed lingering upper partials faster.** The passive
   material-loss blend increases from 25% to 65%, preserving its fundamental
   and 3.6 kHz decay anchors, independent hand damping and loop-gain ceiling.
   This strengthens the previous pass's small change into a clear difference
   in how a note darkens. A full-strength candidate over-darkened the fretted
   F#2 reference and shortened established bass-note bodies too much.
3. **Natural harmonics start with a pluck.** Their old direct pulse was about
   two orders of magnitude larger than an ordinary pick edge, with a reduced
   displacement body and extra pulse brightness. The articulation now uses
   ordinary physical pick mechanics at its existing gentler player effort;
   the touching finger selects the harmonic modes. This removes the artificial
   attack spike and makes harmonic level more consistent across the fretboard.
   The finite finger pad and independent Pick Position remain active.

The [pick report](realism-pick-attack-pass4-2026-09-08.md) records the isolated
attack experiment and rejected alternatives. The
[reference report](realism-reference-pass4-2026-09-08.md) separates isolated
sustain changes from the integrated result.

## Audible signal differences

The same three CC0 clean eight-string previews remain development references,
with unknown player, pick and setup and two censored attacks. They informed the
moderate damping choice; they do not establish a general realism score. In the
isolated damping comparison, late RMS falls another 1.7–2.1 dB. Through the fixed
Modern amp, matching the 150–350 ms bodies still leaves the 500–1000 ms
500 Hz–2 kHz band approximately **5.1–5.3 dB lower** on F#1, C2 and F#2.

The isolated pick change increases E1/F#1 0–20 ms attack RMS by **3.0/2.3 dB
dry** and **0.9/1.0 dB through Modern**, with much smaller late-body changes.
Palm repicks change less because the heel absorbs that edge. A uniform fourfold
edge and extra scrape noise were rejected. The stronger pick does not improve
the sole uncensored preview's early centroid match; it addresses the user's
request for a more distinct pick sound.

In the complete 53-phrase comparison, the raw dry maximum changes from
**+3.61 to −12.08 dBFS**. This follows removal of the oversized harmonic pulse,
without adding a limiter. All audition copies are safe PCM16 with paired
whole-file RMS matching. That matching retains each note's internal attack and
decay contours; original float audio and raw measurements are preserved.

The recordings also show more pitch movement during sustain than the model.
Stronger damping changes spectral decay, but does not solve those stationary
partial tracks. No new pitch modulation or chorus was added on the strength
of three uncontrolled recordings. Listening judgments are left to the user;
these measurements do not certify perceptual realism.

## Verification and auditions

The three focused suites observe actual rendered waveforms at 44.1, 48, 96 and
192 kHz:

- [Pick attack](../Tests/RealismPickAttackTests.cpp): short-edge contribution,
  hardness contrast, retained late body, Hammer separation, dry/Modern
  headroom and callback identity. Reverting only the new boost fails four
  assertions across the four rates.
- [Natural-harmonic attack](../Tests/RealismHarmonicAttackTests.cpp): attack/body
  balance, peak, dynamics, surviving even modes and callback identity.
  The old harmonic excitation fails 100 assertions.
- [Loss](../Tests/RealismLossTests.cpp): rendered cooling on both bass strings,
  retained low-frequency body, and the previous full-filter passivity,
  preserved anchors, bypass and moving-control checks. Reverting only the
  blend to 25% fails all 16 new cooling assertions.

All 20 core DSP/tooling tests and four native arm64 plugin checks pass. VST3,
AU, CLAP and Standalone build successfully. The three focused suites also pass
AddressSanitizer and UndefinedBehaviorSanitizer. The ordinary engine checks
retain tuning, aliasing, muting, chord, pickup-contrast and realtime bounds.

Nine single-coil partial snapshots and two single-coil octave-band snapshots
were refreshed for the intentional source-spectrum change; their tolerances
are unchanged. The old requirement that a natural harmonic's broadband attack
centroid exceed a normal pick was replaced with an actual octave-selection
check. The noise-disabled bass peak/body ceiling now equals the existing
complete-output ceiling of 17 dB: disabling incidental noise does not remove
the physical pick edge. All low-partial balance, low-frequency decay and late
body bounds remain unchanged. These explicit voicing changes replace obsolete
expectations; they are not new pickup or recording calibrations.

Open `build-realism-20260908-pass4/comparison/listening.html`. The first nine
cards foreground hard, medium and soft picks, open bass sustain, natural
harmonics and pinch. The other 44 phrases are expandable. Every phrase has dry
and Modern before/after audio, using the same settings and events: **212 audio
files**. HTML links, WAV formats, headroom and paired RMS were checked locally.
Browser visual inspection was unavailable because the browser's URL policy
blocked the local file page.

Source snapshots, hashes, raw renders, manifests, validation logs and the
comparison JSON/CSV remain under `build-realism-20260908-pass4/`. The reusable
[renderer](../Experiments/Realism20260908Pass4/RenderRealism.cpp) and
[analyzer](../Experiments/Realism20260908Pass4/AnalyzeRealism.py) generate the
same score against either snapshot.

```sh
cmake -S . -B build-realism-20260908-pass4/current \
  -DCMAKE_BUILD_TYPE=Release -DELECTRY_BUILD_PLUGIN=OFF \
  -DELECTRY_BUILD_UNIVERSAL=OFF -DBUILD_TESTING=ON
cmake --build build-realism-20260908-pass4/current --parallel
ctest --test-dir build-realism-20260908-pass4/current --output-on-failure
python3 Experiments/Realism20260908Pass4/AnalyzeRealism.py \
  build-realism-20260908-pass4/baseline \
  build-realism-20260908-pass4/candidate \
  build-realism-20260908-pass4/comparison
```
