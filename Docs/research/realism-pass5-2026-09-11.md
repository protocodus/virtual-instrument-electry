# Everyday playing realism — pass five, 11 September 2026

This pass targets ordinary picked notes, short stops, fretted palm-muted riffs
and strummed power chords. Baseline is `3baafa6`. Five mechanisms are assessed
independently before the complete model is rendered against that baseline.
These are voicing/model changes and signal measurements, not a claim of a
blind listening win or parity with a named commercial instrument.

## Five techniques

1. **Coherent pick-induced pitch settling.** A picked string begins slightly
   sharp and relaxes as its excess tension decays. The existing energy-based
   candidate now uses a conservative six-cent maximum,
   velocity/geometry-dependent excitation, and no new random pitch oscillator.
   The fundamental and upper partials must agree on the direction of motion.
2. **Winding texture in a normal pick.** A small coherent ridge component joins
   the existing short broadband scrape on wound-string Sustain. Its direction
   follows up/down picking; frequency and phase follow the existing winding
   spacing, crossing speed and pick position. Pick Noise still controls it,
   including an exact zero and continuous approach to zero beside finger noise.
3. **Frequency-dependent note stops.** Ordinary Sustain releases lose their
   high modes during the existing 10–22 ms hand closure. A positive-weight,
   symmetric average inside the string loop adds no phase delay and cannot
   amplify a mode. The 125 microsecond span and 0.75 depth are voicing choices,
   not a claimed measurement of a finger pad.
4. **Speaking-length-aware palm damping.** As a fretted string shortens, a
   near-bridge hand interacts with more of its fundamental motion and less
   modal mass. The hand contribution follows a normalized localized-contact
   law while the open-string calibration and empirical loss-band shape stay
   intact. This tightens fretted chugs instead of leaving a long rounded tail.
5. **Contact speed across a strum.** The solved wrist travel now controls each
   string's contact/release duration and scrape speed as well as its onset.
   The leading string is the reference. Pulse-area normalization preserves
   modal displacement; MIDI velocity is unchanged. Zero spread and solo
   contacts keep unity speed. A smooth transition over the first 3 ms of Spread
   prevents a discontinuity near zero; an infinitesimal positive Spread
   produces exactly the zero-Spread waveform.

## Reference evidence

See [the reference audit](realism-reference-pass5-2026-09-11.md) for actual
recording provenance, licenses, checksums, estimator tests, window definitions
and model comparisons. The ordinary DI references constrain the *direction and
scale* of pitch relaxation. They do not identify the model's force constant,
pick contact texture, hand geometry or commercial sound quality.

A prototype that simply moved the empirical palm loss dip was rejected: its
passivity solver reduced dip depth and freed upper modes. The retained approach
changes the hand's decay contribution while preserving the calibrated dip.

## Matched audition

`Experiments/Realism20260911/RenderRealism.cpp` renders 19 common phrases as
simultaneous dry and fixed Modern high-gain taps. Both versions have the same
settings, seeded variation, score, event times and 44.1 kHz host rate. The
manifest records string selection, velocities, hardness, spread and all MIDI
controls. FX in these comparisons are deliberately enabled for the second tap.

`AnalyzeRealism.py` validates manifests and float WAVs, measures the raw signals,
and makes listening copies using one constant gain per complete file to match
paired RMS with safe shared peak headroom. There is no transient normalization
or dynamics processing in the matching step. Comparisons remain an invitation
to audition, without fabricated listening scores.

Local output: `build-realism-20260911/comparison/listening.html`.

## Measured result

- On the three ordinary E2 DI references, median aggregate pitch-offset error
  falls from **4.680 to 2.753 cents** and slope error from **10.463 to 4.661
  cents/s**. All twelve pickup/metric comparisons improve. These are unmatched
  recordings, so this supports the missing motion rather than a realism rating.
- Isolated note-stop tests reduce 1.5–7 kHz energy by **1.1–4.9 dB dry** and
  **1.0–2.8 dB through Modern**, while dry 30–400 Hz body changes by at most
  0.35 dB in the measured E1/E2/E4 cases.
- Wound-pick texture changes the dry onset waveform by roughly -27 dB relative
  to baseline; onset level changes stay within **0.05 dB**. This is subtler
  than the palm/strum changes and should be auditioned as texture.
- In the combined common-phrase renders, fret-5/fret-12 chug RMS falls
  **2.14/5.51 dB** because the tail shortens. Open-string chugs remain exactly
  identical. Audition files compensate overall level using constant gains.
- Combined down/up/alternate power-chord RMS changes are only
  **+0.14/+0.09/+0.06 dB**; the attack differences survive the same Modern chain.
- The complete model's measured upper-band floor is **151.194 dB below its
  spectral peak**, passing the original 150 dB limit. Maximum-velocity open
  tuning and chord-spread limits remain unchanged and pass.

The isolated pitch change costs a median **1.04% more CPU on held eight-string
chords** and **6.22% on four-Hz chord repicks** over nine alternating matched
rounds. Host contention is visible in the retained trials; these are relative
costs, not percentage points of total CPU. The combined engine remains below
its existing realtime guardrails. See the reference package for all raw trials.

## Validation

The new contact/rhythm suites cover callback-size identity, silent/reset state,
physical contact ownership, zero-control continuity, zero/tiny Spread identity,
contact-speed reservation and reanchoring, upper-mode passivity and zero added
phase, dry and Modern output, and 44.1/48/96 kHz. Existing timing and engine
coverage also exercises higher sample rates, MPE, slides, tuning and feedback.

The spectral snapshot numbers were refreshed for the intentional moving-pitch
and pick-source changes, preserving their existing tolerances. Independent
pickup-notch/contrast, tuning, ultrasonic-floor, reference-range and passivity
checks were retained. Slide and released-MPE test expectations now distinguish
frozen finger/bend motion from independently relaxing attack tension, while
retaining the independent 60 ms release-loss law.

All **22 core checks pass**: the initial combined run passed 21 and exposed
one stale released-MPE oracle; the corrected engine suite passes on rerun.
All **four native processor/VST3/CLAP/AU checks pass**. The focused timing,
everyday contact and rhythm suites also pass under **ASan/UBSan** with no
reported findings. Standalone, VST3, AU and CLAP Release artifacts were rebuilt
for macOS arm64. This pass does not claim a Windows or universal-binary run.

The listening page validates all 19 phrases / 38 paired tap comparisons,
76 PCM audio links, sample rates, lengths and peak headroom. Raw audition
maxima are -11.51 dBFS before and -11.68 dBFS after. Open-string palm controls
are sample-identical in both taps. Current source hashes and definitions are
in `Experiments/Realism20260911/source-provenance.json`.

Local validation logs:

- `build-realism-20260911/core-ctest.log` and `core-engine-final.log`
- `build-realism-20260911/native-ctest.log`
- `build-realism-20260911/sanitize-ctest.log`
- `build-realism-20260911/comparison/comparison.json`

Local products are under `build-ui-20260908/native/Electry_artefacts/Release/`.
No subjective preference scores were assigned; the final preference check is
listening to the matched musical phrases.


## Listening feedback and next focus

The user confirmed an improvement and identified open → muted → open as the
next area to refine. The current `open_to_muted_rhythm` clip includes Note Off
and a 50 ms gap before the next open attack; it does not isolate a continuous
Palm → Sustain repick on a held string or a pressure-only palm lift. The next
comparison should cover both transitions without resetting the ringing string,
including their timing, tone recovery and level continuity.
