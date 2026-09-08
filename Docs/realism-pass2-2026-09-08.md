# Five further guitar realism changes — 8 September 2026

This continues the [first five contact changes](realism-2026-09-08.md).
All five additions run in the shipping DSP with the existing controls.
The before version is the complete first-pass working tree, frozen before
this work under `build-realism-20260908-pass2/baseline-src`, rather than git HEAD.

## Five implemented ideas

1. **Shorter wound strings lose their fundamental sooner.** Fundamental T60
   now follows the live speaking-length fraction, `2^(-fret/12)`, on the five
   wound strings. The open-string calibration and fixed-Hz upper decay target
   remain the anchors; independent hand losses still combine as rates. Plain
   strings bypass this construction law, and a pitch bend does not imply a
   shorter string. The [reference report](realism-reference-pass2-2026-09-08.md)
   explains the constant material-loss approximation and its limits.
2. **Slides travel over the actual neck.** Duration follows centimetres of
   finger travel, and pitch follows a smooth movement in speaking length.
   Equal fret intervals higher on the neck take less time. Scale length also
   affects travel. Interrupted slides, pickups, damping and picking during a
   slide share that same live position. The default nut-to-second-fret time
   remains 44.8 ms; the existing 30 ms–1.2 s limits remain.
3. **Winding ridges produce a coherent slide texture.** Filtered roughness
   gains a small periodic component whose frequency follows finger speed
   divided by winding spacing. Signed phase follows reversals and survives
   redirected slides and picking-hand repicks. A filtered-energy follower
   keeps the texture from becoming an accidental noise-level boost. Plain
   strings retain their exact original broadband friction path. The 18%
   amplitude blend is a voicing choice, not a measured contact coefficient.
4. **A stationary fretting finger does not land on every pick.** Repeated
   picking on a held fret now retains its finger contact, including when
   Palm/Dead audio has retired while the key remains held. Lifting, replacing
   or moving the finger restores the contact transient. A scheduled strum
   preserves an unconsumed landing and consumes it once. Plectrum noise still
   belongs to every actual pick.
5. **Articulation makeup changes continuously on a ringing string.** The
   observation gain previously jumped as soon as an articulation changed.
   A 2 ms pole now traverses 95% of the change in 6 ms. Fresh notes still start
   at their calibrated gain; continuing notes preserve the existing gain at
   contact. Both increasing and decreasing transitions use the same timing.

The winding mechanism follows the periodic winding/velocity relationship in
[Pakarinen, Puputti and Välimäki, *Virtual Slide Guitar*, 2008](https://aaltodoc.aalto.fi/bitstreams/b890243c-fd3e-435c-a755-b98bd860507a/download).
Phase integrates control-rate velocity, freezes when friction is inaudible,
and caps ridge frequency at 40% of the internal sample rate. This models
audible texture, not exact microscopic registration across silent movement.
The gain pole removes an artificial observation discontinuity; it does not
claim to identify another physical pickup component.

## Real-recording comparison

The same three CC0 eight-string development references are reused. Their
unknown setup and two censored attacks prevent absolute calibration. The
isolated speaking-length change moves overall decay toward all three:

| Note | Real late RMS | First-pass baseline | Second pass |
| --- | ---: | ---: | ---: |
| F#1 | −10.16 dB | −2.78 dB | −2.96 dB |
| C2 | −7.60 dB | −2.48 dB | −3.37 dB |
| F#2 | −10.88 dB | −2.12 dB | −4.00 dB |

Late RMS is 500–1000 ms relative to the note's own 0–50 ms window. These three
complete second-pass dry renders are byte-identical to the isolated length
candidate, so the comparison is attributable to that mechanism. Upper-partial
cooling barely changes and remains a significant gap. Full descriptors,
primary physical sources and recording provenance are in the
[reference report](realism-reference-pass2-2026-09-08.md).

## Verification and auditions

All **15 DSP/tooling CTest targets pass**. Native arm64 VST3, AU, CLAP and
Standalone builds succeed; all **four plugin processor/artifact checks pass**.
The three new suites cover:

- [Decay](../Tests/RealismDecayTests.cpp): independently measured solved-filter
  T60, both polarisations, all eight strings, fractional frets, fixed-Hz upper
  decay, plain bypass, pitch bends, live refretting and hand-loss passivity.
- [Gestures](../Tests/RealismGestureTests.cpp): physical distance and scale,
  moving pitch/fret consistency, redirected slides, retained repick trajectory,
  note-off freeze, new/held/retired/delayed finger contacts and callback identity.
- [Texture and gain](../Tests/RealismTextureTests.cpp): spectral detection of
  winding periodicity, energy preservation, exact plain-string bypass,
  phase reversal/reset, fresh-note gain and bidirectional gain continuity.

All three pass at 44.1, 48, 96 and 192 kHz, and under AddressSanitizer and
UndefinedBehaviorSanitizer. Callback sizes include 1, 17, 128, 257 and 511.
Restoring immediate makeup and removing the ridge term fails 76 targeted
assertions while the plain/energy controls remain valid. The first-pass DSP
fails the 160 new wound-length assertions while retaining their controls.

Older slide observers now derive physical position and duration independently
instead of assuming the retired semitone trajectory. Comparable-speed arrival
guards retain their original 35-cent bound. The actual minimum-time descending
slide is also measured explicitly: **41.91 cents of lag at nominal arrival**,
then within the same **2-cent bound after 50 ms**, with no wrong-way settling.
That transient delay-follower lag remains a limit of the fastest gestures.

The stateful Open/Palm/Dead/Dead phrase remains within the unchanged real-hit
ranges and 5 dB contextual-error guard. Its documented reproducibility medians
are refreshed to −6.886/−13.261/−21.334 dB; contextual RMSE improves from
4.767 to 4.298 dB. No Dead damping coefficient was retuned. See the corresponding
context in [evaluation.md](evaluation.md).

The new [renderer](../Experiments/Realism20260908Pass2/RenderRealism.cpp) contains
39 scores, including the previous 24 plus physical slides across the neck,
reversals, repicks during slides, a plain-string control, finger replacement,
rapid articulation switches and sustained frets 0/6/12/22. Each produces dry
DI and the same Modern high-gain chain. The
[analyzer](../Experiments/Realism20260908Pass2/AnalyzeRealism.py) reuses the
unchanged first-pass measurements and level-matching code.

`build-realism-20260908-pass2/comparison/listening.html` contains **156 audio
players**: 39 scores × dry/Modern × before/after. Paired listening copies use
one constant RMS-matching gain per complete file, with shared attenuation
where headroom requires it. No dynamics processing is added. The raw float
files, manifests, complete CSV/JSON measurements and frozen source provenance
remain in the same build directory. These are auditions and signal checks,
not a completed listening panel.

The new stress phrase exposes existing headroom behavior: at the renderer's
unity output gain, raw dry `articulation-ring-through` peaks at +3.25 dBFS
before and +3.03 dBFS after. Float WAVs preserve those samples without clipping;
all Modern outputs and the listening copies stay below full scale. The
shipping default engine output gain is 0.5. The stress result is retained
explicitly rather than used to justify a limiter or to claim every raw score
fits a unity ceiling. Low E1/F#1 chug whole-file RMS changes stay within
0.21 dB dry and 0.13 dB through Modern; fresh open-E1 sustain and releases
remain byte-identical.

The existing realtime CPU guards pass. The integrated run measured an
eight-string 96 kHz worst-case render ratio of 0.124× realtime and the default
pickup/output configuration at 0.100×. Three interleaved whole-renderer timing
pairs are saved in `renderer-timing.json`, but their wall times range from
4.32 to 13.27 seconds and are too variable to support a before/after speed
claim. They include output-file writing and are not a DSP microbenchmark.

Reproduce the core checks and audition analysis:

```sh
cmake -S . -B build-realism-20260908-pass2/current \
  -DCMAKE_BUILD_TYPE=Release -DELECTRY_BUILD_PLUGIN=OFF \
  -DELECTRY_BUILD_UNIVERSAL=OFF -DBUILD_TESTING=ON
cmake --build build-realism-20260908-pass2/current --parallel
ctest --test-dir build-realism-20260908-pass2/current --output-on-failure
python3 Experiments/Realism20260908Pass2/AnalyzeRealism.py \
  build-realism-20260908-pass2/baseline \
  build-realism-20260908-pass2/candidate \
  build-realism-20260908-pass2/comparison
```
