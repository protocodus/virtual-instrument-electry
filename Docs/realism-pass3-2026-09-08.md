# Five more guitar realism changes — 8 September 2026

The first two passes were committed and pushed to `main` as
[`809aecc`](https://github.com/protocodus/virtual-instrument-electry/commit/809aecc40c38c08cc5449e4b2c22e9a1f5e282e8).
This third pass uses that exact commit as its before version. Five further
changes run in the shipping DSP with the existing controls.

## The five changes

1. **Bass partials shed energy sooner.** A small passive loss section adds
   intermediate-frequency decay to the two extended bass strings while
   preserving their fundamental and 3.6 kHz decay anchors. It makes a 25%
   blend toward a linear material-loss curve, subject to the loop's existing
   gain ceiling. The six conventional strings, Dead and sympathetic renderer
   retain their earlier calibration. A broader candidate shortened established
   open-string sustain too far and was rejected. The
   [reference report](realism-reference-pass3-2026-09-08.md) documents this
   approximation, source evidence, rejected scope and remaining gap.
2. **Slides arrive with the finger.** The smooth physical slide trajectory
   previously passed through another 6 ms delay follower. A continuous ramp
   now advances the string period with that finger movement. Ordinary wheel
   bends and vibrato retain their smoothing; extreme MPE pitch clamps cannot
   create artificial slide movement. The previously documented fast descent
   improves from 41.91 cents of arrival lag to **0.00312 cents**. The new
   six-trajectory tests stay below **0.16 cents** at all four sample rates.
3. **The pinch thumb follows pick release.** Pinch contact is armed while
   the plectrum holds the string and starts at the first released-excitation
   sample. The former thumb hold timer started before release. Repicks retain
   an existing touch until the next release, and a replacing gesture or Note
   Off cancels the pending thumb. The existing 90 ms hold and 130 ms lift
   remain; these are not newly measured thumb latencies. Natural-harmonic
   fingers are still positioned before picking.
4. **Fingers touch an area, not a mathematical point.** A positive-weight
   three-position average models a finite pad around the contact centre.
   Higher modes now lose some energy in the material beside a node instead
   of all surviving it perfectly. Natural harmonics use an 8 mm pad; pinch
   uses a narrower 4 mm thumb edge. These are conservative geometric voicing
   estimates. The initial 8 mm thumb patch dulled the existing squeal contrast
   too much, so its width was reduced while keeping that contrast guard.
   Physical width follows speaking length and live wave speed independently
   of the delay line's filter-phase correction.
5. **Natural harmonics respect the picking hand's position.** The harmonic
   articulation previously overwrote Pick Position with a fixed value. The
   plectrum now remains where the player put it, while the other hand still
   touches the midpoint node. Moving the pick changes which surviving modes
   are excited, including on higher frets, without moving that finger.

The existing point-touch model remains the zero-width limit. The finite pad
uses three-point Gauss–Legendre spatial averaging with positive weights
4/9 and 5/18 on each side. Its static contraction is tested through the actual
cubic reads. This is a distributed version of the existing temporal contact
approximation, not a complete bidirectional finger/string collision solver.
The independent roles of plucking position and finger contact are also
explicit in [Bilbao et al., *Real-Time Guitar Synthesis*, §§2.1 and 2.5](https://www.pure.ed.ac.uk/ws/portalfiles/portal/470239305/BilbaoEtal2024RealTimeGuitarSynthesis.pdf).
That paper does not calibrate the widths used here.

## Recording comparison

The same three CC0 eight-string previews remain development references, with
unknown setup and censored F#1/C2 attacks. Their late RMS changes are:

| Note | Real | Before | After |
| --- | ---: | ---: | ---: |
| F#1 | −10.16 dB | −2.96 dB | −4.42 dB |
| C2 | −7.60 dB | −3.37 dB | −4.76 dB |
| F#2 | −10.88 dB | −4.00 dB | −5.27 dB |

Late RMS is 500–1000 ms relative to each note's own 0–50 ms window. All three
late-envelope and all three upper/low harmonic-cooling descriptors move toward
the recordings. Mean absolute descriptor differences fall by 22.5% and 19.7%,
respectively. These numbers describe signal differences; they are not a
perceptual realism score. Upper-partial sustain still differs substantially.
The [reference report](realism-reference-pass3-2026-09-08.md) contains all
descriptors, primary sources and the isolated loss comparison.

## Tests and auditions

All **18 core DSP/tooling tests** and **four native arm64 plugin checks** pass.
VST3, AU, CLAP and Standalone builds succeed. The three new suites also pass
AddressSanitizer and UndefinedBehaviorSanitizer at 44.1, 48, 96 and 192 kHz:

- [Timing](../Tests/RealismTimingTests.cpp): slide arrival, continuous movement,
  output-pitch estimation, redirected/re-picked slides, MPE clamps, ordinary
  bends, thumb timing and cancellation, callback identity and reset.
- [Harmonics](../Tests/RealismHarmonicTests.cpp): independent pick/pad geometry,
  frets 0/6/12/22, live bends and String Age, actual cubic-operator gain, predicted
  node-adjacent mode loss, rendered differences and callback identity.
- [Loss](../Tests/RealismLossTests.cpp): complete pole/biquad response, preserved
  endpoints, extra middle loss, both polarisations, bypass scope, live motion,
  passivity and poisoned filter-state clearing at sympathetic handoff.

Negative controls fail the intended assertions: **72** for the old timing
model, **88** when restoring fixed harmonic pick position and point contact,
and **84** when disabling the extra material loss. Independent anchor,
passivity and unrelated-path controls retain their meaning.

Existing slide arrival bounds were strengthened to 2 cents. Pinch geometry
observers now wait for physical thumb contact, retaining their exact spatial
assertions. All original squeal partial and spectral-contrast bounds pass.
Two affected low-E1 single-coil octave-band snapshots were updated, with
their tolerances and humbucker contrast unchanged. The second-pass decay
observer now measures the complete filter, including its new biquad.

The [renderer](../Experiments/Realism20260908Pass3/RenderRealism.cpp) retains
the previous 39 phrases and adds eight harmonic-position, interrupted-contact
and fast-slide phrases. Every phrase has matched dry and Modern high-gain
versions. The [analyzer](../Experiments/Realism20260908Pass3/AnalyzeRealism.py)
uses the unchanged first-pass signal measurements and constant-gain RMS
matching, and presents the new phrases first.

Open `build-realism-20260908-pass3/comparison/listening.html` for **188 audio
players**. Its original float WAVs, MIDI/control manifests, CSV/JSON measurements
and source hashes remain under the same build directory. Pairs have identical
scores and settings. Low-E1/F#1 chug whole-file RMS changes stay below 0.005 dB
dry and 0.003 dB through Modern.

The inherited unity-output-gain headroom limit remains explicit. The full-level
harmonic stress phrases peak at **+3.61 dBFS in both versions**; float files
preserve those samples. Modern outputs and all listening copies stay below
full scale. Listening copies use one constant gain per complete file and
shared extra attenuation when needed, without dynamics processing.

The existing realtime guards pass. In the integrated 96 kHz run, eight active
strings measured 0.157× realtime in the worst pickup/output configuration and
0.121× in the default configuration. These workload figures were recorded
during validation and do not establish a before/after speed comparison.

```sh
cmake -S . -B build-realism-20260908-pass3/current \
  -DCMAKE_BUILD_TYPE=Release -DELECTRY_BUILD_PLUGIN=OFF \
  -DELECTRY_BUILD_UNIVERSAL=OFF -DBUILD_TESTING=ON
cmake --build build-realism-20260908-pass3/current --parallel
ctest --test-dir build-realism-20260908-pass3/current --output-on-failure
python3 Experiments/Realism20260908Pass3/AnalyzeRealism.py \
  build-realism-20260908-pass3/baseline \
  build-realism-20260908-pass3/candidate \
  build-realism-20260908-pass3/comparison
```
