# Residual tension through muted repicks

This is an isolated experiment against `aa1f7e0963ae488a4d0c3c7eeb3c6cb81fb749f8`.
It preserves the evidence for the tension part of the transition work. It does
**not** include the later hand-contact smoothing, pressure-controller fixes, or
the complete musical benchmark in the parent directory.

## Reproduce

From the repository root, with Git, `patch`, a C++20 `clang++`, and Python 3:

```sh
Experiments/Transitions20260911/Tension/run.sh \
  build-transitions-20260911/tension-reproduction
```

Choose a new output directory if that one already contains source snapshots.
The script archives the frozen baseline, copies it, and applies
`isolated-tension.patch` to the candidate copy. It compiles both probes against
each snapshot, renders the diagnostic clips, and runs `AnalyzeTension.py`.
Everything generated, including audio, binaries, snapshots, and the compiler
version, stays in the chosen build directory. The command does not change the
working source tree. No JUCE download or plugin build is required.

Both builds use `-std=c++20 -O2 -DNDEBUG` with the production definitions
`ELECTRY_ENERGY_ATTACK_PITCH=1`, `ELECTRY_DECOUPLED_PICK_RELEASE=1`, and
`ELECTRY_MEASURED_BODY_RESPONSE=1`. The probes reuse the frozen engine test
accessor and rendering helpers by including `Tests/ElectryEngineTests.cpp` with
its test-suite `main` renamed. They do not execute the full suite. The separate
integrated regression run is documented with the main benchmark.

The recorded result files are `before.csv`, `after.csv`, and `results.json`.
`receipt.json` records the compiler and SHA-256 hashes of the compared sources,
probe scripts, isolated patch, and result files.
The recorded run used Apple Clang on an arm64 Mac. The raw `.f32` diagnostic
format is native float32 mono, generated on a little-endian host; use a
little-endian host for the replay script and analyzer.

## What changed

Previously, the first released sample of a Palm/Dead pick erased residual
`q = ΔT/T` from the preceding sustained pick, while preserving the travelling
string state. The candidate retains that coordinate, creates no new muted
seed, and lets the existing 304.9 ms free relaxation plus physical hand loss
remove it. New Sustain seeds still commit only at physical pick release;
cancelled/delayed contact rules and the six-cent cap remain in place.

The additional hand rate comes from the existing fundamental T60 solve:

```text
extraRate = max(0, 1 / handLoadedT60 − 1 / intrinsicT60)
qHandRetention = exp(−6 ln(10) × extraRate × controlPeriod / sampleRate / 1.7)
qNext = q × freeRetention × qHandRetention
```

The T60 values use the same finite bounds as the loop solve. The factor six
converts the amplitude T60 law to energy decay. The existing horizontal loop's
T60 is 1.7 times the vertical loop's T60; using that longest-lived fundamental
gives a conservative lower bound on the extra loss. It avoids treating the
new pick's projection weights as a measurement of the preceding ring's modal
energy. There is no new muted pitch seed or adjustable relaxation constant.

This remains a conservative approximation: the residual `q` coordinate is
empirically calibrated, and a single fundamental loss rate does not measure
the complete instantaneous modal-energy distribution. The result establishes
continuity and consistent contact ownership, not a fit to a recorded player's
muting motion.

## Measurements

`TensionProbe.cpp` examines 48 combinations: host rates 44.1, 48, 96, and
192 kHz; engine notes 28/40/55 (E1/E2/G3); Palm/Dead; and 50/150 ms between the
first open pick and a true held repick. It advances to physical Release and
compares the surrounding internal samples. The CSV includes:

- The residual coordinate before and after release.
- The frequency factor's change in cents.
- Target and current frequencies implied by the delay and all compensated
  loop-filter phases, rather than raw delay movement.

The isolated candidate removes a maximum **5.0956-cent target drop** at that
boundary. The old current delay already smoothed the target change: its largest
single-sample implied-pitch change in this matrix was only **0.00970 cents**.
This is a correction to a short artificial bend, not evidence of an audio
click. A zero at this sampled boundary is not a claim that pitch never moves
elsewhere in the phrase. The regression patch also checks other control phases.

`TensionAudioProbe.cpp` renders four 1.2-second diagnostic cases at 48 kHz,
each with callback sizes 17 and 256. Noise/artifacts/sympathetic coupling and
strum spread are zero; other engine parameters keep their baseline defaults.
There is no FX processing or normalization:

| Case | Description | Candidate versus baseline |
| --- | --- | --- |
| 0 | One ordinary E1 Sustain | Identical PCM |
| 1 | One stationary E1 Palm pick | Identical PCM |
| 2 | Held E1 open → Palm → open → Palm → open | Nonzero difference |
| 3 | E1/B1/E2 chord with those repicks on E1; siblings keep ringing | Nonzero difference |

For cases 2–3, transitions occur at 50, 200, 500, and 650 ms. Their difference
RMS is approximately −15.76 and −18.90 dB relative to the baseline clip RMS.
All four candidate cases are byte-identical across the two callback sizes and
stay finite below full scale. Difference RMS measures a signal change; it does
not establish a perceptual improvement. `same_pcm` provides the exact null
check; −300 dB in `results.json` is the analyzer's display floor for that null.

The musical audition and final combined checks belong to the parent
transition benchmark, not these isolated diagnostic clips.
