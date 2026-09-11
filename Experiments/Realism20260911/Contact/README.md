# Ordinary contact candidates, 11 September 2026

These measurements isolate the two contact changes from baseline commit
`3baafa69adf1c6b80455ed6e58dcde009873c359`, before integration with the other
September 11 changes. Exact source hashes and build definitions are in
[provenance.json](provenance.json).

1. **Spectral note stops.** Note Off currently ramps only a broadband loop gain toward its 60 ms T60. The candidate adds a positive-weight symmetric history average to that same closing hand, so upper modes leave first while the low note body completes the stop. The existing 10–22 ms closure ramps a depth of 0.75; the averaging span is approximately 125 us, snapped to integer delay cells so cubic interpolation adds no relative phase. These are explicit voicing choices, not a measured finger-pad dimension or fitted recording. The operator adds no group delay. The candidate applies to Sustain damping; continuous Palm Pressure scales down the remaining contribution, while Palm/Dead styles bypass it because they already have their calibrated hand models.
2. **Wound-pick ridge texture.** Normal wound-string Sustain already scales the broadband scrape cutoff by winding pitch and pick speed. A coherent component adds the short succession of ridge crossings, within the same pick-noise envelope/control. Signed phase uses physical pick position, the existing per-stroke contact offset and winding spacing. Ridge frequency uses 0.65 times the already-scaled noise cutoff, capped at 0.35 times internal sample rate. The 0.35 amplitude mix follows the plectrum share of any simultaneous finger noise, tends continuously to zero with Pick Noise, and has unit expected source variance with the remaining roughness; finite filtered envelopes are not promised exact equal energy. Plain strings, Palm/Dead and finger gestures are unchanged. This is a contact texture approximation, not a full longitudinal string model or a second ringing oscillator.

The published angled-pick/rough-edge and winding-scratch mechanisms are described in M. Zollner, *Physics of the Electric Guitar*, §1.5, pp. 1-30 and 1-34: https://www.gitec-forum-eng.de/wp-content/uploads/2020/08/poteg-1-5-picking-process.pdf. That source does not identify our mix or release-loss constants. There is no newly matched recording or subjective listening judgment in this candidate assessment.

## Measured audible scope

At 48 kHz, identical `.9` velocity, `.85` Pick Hardness, noise off for the release test, Note Off at 80 ms. Release window is 90–180 ms. Modern uses amp .95, distortion .45, compressor .60.

| Note | Release high-band change dry/Modern | Low-band change dry | Whole-release level change dry |
| --- | --- | --- | --- |
| E1 | -1.14 / -0.96 dB | -0.02 dB | -0.04 dB |
| E2 | -2.70 / -1.41 dB | -0.09 dB | -0.18 dB |
| E4 | -4.86 / -2.80 dB | -0.35 dB | -3.34 dB |

High means 1.5–7 kHz, low means 30–400 Hz, measured through independent third-order Butterworth bands. E4 loses substantial upper-mode body; this is intentional note-stop color and remains an audition decision, not an empirical realism score. The waveform before Note Off stays exactly unchanged.

For `.5` Pick Noise, first 30 ms onset difference signals are -26.93/-26.42 dB relative to baseline dry E1/E2, and -24.48/-22.75 dB through Modern. Whole-onset levels move only +.013/-.031 dB dry and +.028/-.041 dB Modern. These signal changes are smaller than the release change; they indicate a contact-texture difference, not a stronger pick.

## Validation and integration

- `contact-tests.log`: focused suite at 44.1/48/96 kHz; finite/headroom, held-note identity, ordinary onset-level preservation, exact plain/finger/Palm/Dead bypass, exact zero Pick Noise bypass, deterministic reset, 17/257 callback identity dry and Modern, full-Nyquist production cubic-operator gain and complex zero-phase transfer sweep at short/long periods.
- `contact-existing-tests.log`: unchanged `RealismContactTests` passes at 44.1/48/96/192 kHz.
- `contact-sanitizer.log`: same focused tests built with AddressSanitizer and UndefinedBehaviorSanitizer.
- `contact-metrics.json`: all three sample rates and both audio paths.
- `contact-audio/`: raw mono float32 files named by sample rate, note, candidate/baseline, idea and dry/amp path. Baseline uses test-only ablation of the one mechanism, with identical events, parameters and all other code.

## Reproduce the isolated comparison

Run from the repository root. The frozen patch contains only these two changes;
the current shipping source may also contain the other September 11 changes.
The test renderer includes both candidate and ablated baseline paths and exports
all three sample rates in one invocation. Python analysis requires NumPy and
SciPy.

```sh
mkdir -p build-realism-20260911/contact-repro
git archive 3baafa69adf1c6b80455ed6e58dcde009873c359 Source \
  | tar -x -C build-realism-20260911/contact-repro
patch -d build-realism-20260911/contact-repro -p1 \
  < Experiments/Realism20260911/Contact/source.patch
clang++ -std=c++20 -O2 -DNDEBUG \
  -DELECTRY_DECOUPLED_PICK_RELEASE=1 -DELECTRY_MEASURED_BODY_RESPONSE=1 \
  -Ibuild-realism-20260911/contact-repro/Source \
  Tests/RealismEverydayContactTests.cpp \
  build-realism-20260911/contact-repro/Source/DSP/ElectryEngine.cpp \
  build-realism-20260911/contact-repro/Source/DSP/ElectryFx.cpp \
  build-realism-20260911/contact-repro/Source/DSP/ElectryVisuals.cpp \
  -o build-realism-20260911/contact-repro/render-contact
build-realism-20260911/contact-repro/render-contact \
  build-realism-20260911/contact-repro/audio
python3 Experiments/Realism20260911/Contact/AnalyzeContact.py \
  build-realism-20260911/contact-repro/audio \
  build-realism-20260911/contact-repro/measurements.json
```

[measurements.json](measurements.json) preserves the isolated measurements. The
`differenceDb` value for the unchanged plain-string pick is the finite numerical
floor (around -568 dB); it denotes exact sample identity. No normalization,
limiting or output EQ is added by the renderer or analyzer.
