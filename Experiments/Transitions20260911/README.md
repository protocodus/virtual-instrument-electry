# Continuous palm-transition benchmark

The frozen baseline is `aa1f7e0963ae488a4d0c3c7eeb3c6cb81fb749f8`.
This benchmark targets a fretting hand that stays on the guitar while the
picking hand repeatedly mutes and opens the strings. It contains an original
rolling picked-chord riff with a Southern rock feel; it is not a transcription
or a sampled performance of the song mentioned in the request.

The main phrase is eight bars of 4/4 at 105 BPM, with 250 ms lead-in and a
900 ms release tail: 19.44 seconds altogether. Its harmony is
`D | Cadd9 | G | G | D | Cadd9 | G | D`.

| Shape | Physical strings, lowest to highest | Sounding notes |
| --- | --- | --- |
| D | 4, 3, 2, 1 | D3, A3, D4, F#4 |
| Cadd9 | 5, 4, 3, 2, 1 | C3, E3, G3, D4, G4 |
| G | 6, 5, 4, 3, 2, 1 | G2, B2, D3, G3, D4, G4 |

Each bar opens with a downwards 12 ms-per-string chord rake, then uses seven
eighth-note arpeggio attacks. Subsequent attacks address physical held-string
repick commands, not repeated fretting Note Ons. D4 retains its single
fretting owner across all eight bars. Other common tones also retain their
owner when the next shape permits it. Chord changes release only fingers whose
pitch changes or whose string is no longer used.

Palm-down/open selections fall at beat offsets 0.85, 1.85, 2.85 and 3.35 inside
each bar, 85.7 ms before the following pick. The note owners remain held at all
32 boundaries. The style version latches Palm Mute or Sustain for the next
pick; the pressure version keeps Sustain selected and moves continuous CC2
pressure between zero and 0.68. A Drop-E power-chord companion reuses the
rhythm across E1, G1, A1 and D2 roots on the lowest three strings.

The six smaller takes separate related mechanisms:

- E1 and E2 each receive one pick, then three pressure contacts/lifts without
  any restrike. Each has an untouched decay control with the same schedule.
- E1 pressure mutes are followed by a lift, a 250 ms wait, then an open repick.
  This separates remaining vibration from newly injected energy.
- One fretting E1 stays held across 13 alternating open/muted attacks. Style
  selection precedes each restrike by 100 ms, exposing latch/contact timing.

The principal riffs retain the shipping 0.20 sympathetic amount. Isolated
controls set it to zero so idle-string coupling cannot replenish a string
whose energy was absorbed by the palm. Acoustic return is zero for every
take. Dry, British crunch and Modern high gain receive the same raw DI, so the
chosen audition amplifier cannot change the guitar excitation.

`manifest.json` contains every frame timestamp, sounding MIDI pitch, physical
string index, velocity, pressure command, style selection and engine/FX
parameter. It is the authoritative score. String index zero is physical
string 8, the low E1; index seven is the high E4. The plugin's playable and
repick MIDI notes are one octave higher than the engine sounding-note values.

The renderer checks the engine's actual fretting counters after every Note
On, Note Off and repick. Each addressed string must have exactly one owner;
every other string must have none. The analysis independently replays the
manifest's owners, checks all releases balance and rejects fretting changes
at in-bar palm boundaries. No resets, silent cuts or all-notes-off events
occur inside a phrase. A physical mute can lose energy permanently; opening
the hand must not artificially recreate that energy without a new pick.

## Reproduce

From the repository root, using Clang C++20 and Python 3 with NumPy:

```sh
mkdir -p build-transitions-20260911/baseline-src
git archive aa1f7e0963ae488a4d0c3c7eeb3c6cb81fb749f8 Source Tests \
  | tar -x -C build-transitions-20260911/baseline-src
clang++ -std=c++20 -O2 -DNDEBUG \
  -DELECTRY_DECOUPLED_PICK_RELEASE=1 -DELECTRY_MEASURED_BODY_RESPONSE=1 \
  -DELECTRY_ENERGY_ATTACK_PITCH=1 \
  -Ibuild-transitions-20260911/baseline-src/Source \
  Experiments/Transitions20260911/RenderTransitions.cpp \
  build-transitions-20260911/baseline-src/Source/DSP/ElectryEngine.cpp \
  build-transitions-20260911/baseline-src/Source/DSP/ElectryFx.cpp \
  build-transitions-20260911/baseline-src/Source/DSP/ElectryVisuals.cpp \
  -o build-transitions-20260911/render-before
clang++ -std=c++20 -O2 -DNDEBUG \
  -DELECTRY_DECOUPLED_PICK_RELEASE=1 -DELECTRY_MEASURED_BODY_RESPONSE=1 \
  -DELECTRY_ENERGY_ATTACK_PITCH=1 -ISource \
  Experiments/Transitions20260911/RenderTransitions.cpp \
  Source/DSP/ElectryEngine.cpp Source/DSP/ElectryFx.cpp Source/DSP/ElectryVisuals.cpp \
  -o build-transitions-20260911/render-after
build-transitions-20260911/render-before build-transitions-20260911/before
build-transitions-20260911/render-after build-transitions-20260911/after
python3 Experiments/Transitions20260911/AnalyzeTransitions.py \
  build-transitions-20260911/before build-transitions-20260911/after \
  build-transitions-20260911/comparison
```

Open `build-transitions-20260911/comparison/listening.html`. There are nine
phrases, three taps each, with same-time before/after switching and clickable
bar/hand-transition markers. Listening copies use one constant whole-file
gain per version to match paired RMS, with shared attenuation if required
for headroom. There is no per-note normalisation, limiting or added dynamic
gain. Displayed peak envelopes share the raw amplitude scale within a pair.

The raw float files and `comparison.json` remain unnormalised. Per-command
RMS, high-band energy and sample-step measurements are diagnostic descriptors,
not click detectors or perceptual realism scores. Style markers describe
selection time; the following pick applies that contact. CC2 controls the
continuous hand directly. `score.json` is copied into the listening directory
for inspection.

Initial harness validation compared the frozen baseline against itself: all
27 paired taps were bit-identical, all nine ownership traces balanced, and
the three riffs each retained ownership through all 32 hand boundaries.

Final candidate validation is recorded in
[validation-results.json](validation-results.json). All 54 raw float WAVs
and 54 PCM listening files are finite and valid; the untouched E1/E2 controls
remain bit-identical across all three taps. The largest raw/audition peak is
below 0.354, and the maximum whole-file RMS mismatch after PCM quantisation
is below 0.00003 dB. All local listening-page links resolve. When Node.js is
available, the analyzer also syntax-checks the embedded player JavaScript;
that check passed here. Visual browser review was not performed.

The benchmark review also found a cache regression in the initial candidate:
changing a non-Palm style or reversing Open / Palm / Open before a hand update
could leave the previous stroke's pressure-loss coefficients in place after
its force changed. Six focused regressions at 44.1, 48 and 96 kHz fail on that
candidate and pass after the guard/deferred-refresh fix. They live alongside
the continuous-pressure endpoint checks in
[RealismTransitionControlsTests.cpp](../../Tests/RealismTransitionControlsTests.cpp).

Whole-phrase dry RMS changes are small: -0.178 dB for the style riff,
+0.045 dB for the pressure riff and -0.135 dB for the Drop-E companion.
The comparison preserves performance dynamics; the intended difference is
how the hand transitions and how the following pick reopens the string.
