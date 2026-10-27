# Bit-identical CPU optimization — 24 October 2026

Four changes to the string engine and the effects lower their CPU cost without
changing a single output sample. They remove repeated work only: no equation,
constant, tolerance, oversampling rate or filter changes, and every claim of
identity below is a zero-error sample comparison, not a listening judgement.
Product version 1.2.9 carries them; the sound is that of 1.2.8.

| Commit | Change | Measured effect |
| --- | --- | --- |
| `8a513aa` | The amplifier reads the triode transfer table inline, fetching it once per oversampled frame instead of three out-of-line lookups that each re-check the table's one-time initialisation; the clamped, non-negative table position is truncated rather than floored bit by bit. | Modern amp −10.6%, Modern with all effects −7.3% FX CPU |
| `ecc036e` | `PolarisationLoop::readFractional()`, the string loop's moving read, rounds its clamped delay up with integer arithmetic, which is exactly `std::ceil` in its range. | −5.5% CPU on five string-heavy dry takes |
| `2ce229e` | The pitch compensation keeps each loop's last bridge-hand and bass-material dip coefficients, keyed on the exact bits of depth and shape, instead of re-solving the biquad on every control tick in which the period moves. | −3.3% CPU on the dry palm-muted power-chord take |
| `e968624` | The power-grid current lookup answers the usual non-conducting case before fetching its table; the grid-current and power-tube pair lookups truncate their clamped positions. | American −1.7%, British −1.9%, American with all effects −3.3% FX CPU |

## Overall result

Baseline `39300f0` against `e968624`, both built from the same harness and
flags (Linux x86-64, GCC 13.3, `-O3 -DNDEBUG`, the shipping feature defines).

**Whole engine plus effects.** All 23 `ElectryRenderDemos` technique takes
(the committed demonstration score, 44.1 kHz, 256-sample blocks, the
shipping `ElectryEngine` → `ElectryFx` path with the live acoustic return),
six alternated rounds: the median round fell from 26.71 to 25.23 seconds of
thread CPU, a **5.8% reduction** (range 5.0–6.4% across rounds).

**Effects alone.** `ElectryBenchmarkFx --input guitar`, 0.5 s × 3 repeats
per case, ABBA-alternated paired runs (six at 48 kHz, four at 44.1 and
96 kHz), median of the paired process-CPU ratios:

| Scenario | 44.1 kHz | 48 kHz | 96 kHz |
| --- | ---: | ---: | ---: |
| American amp | 4.4% | 3.7% | 4.6% |
| British amp | 4.6% | 4.6% | 3.3% |
| Modern amp | 10.8% | 10.3% | 8.2% |
| All effects, American | 3.3% | 4.8% | 3.0% |
| All effects, British | 1.8% | 5.0% | 3.7% |
| All effects, Modern | 4.6% | 7.5% | 5.2% |
| Control/model automation | 5.0% | 4.8% | 4.5% |

Bypass, pedal-only, compressor, delay and room do not use the changed code;
their medians scatter on both sides of zero (−5.5% to +18% on cases that cost
20–40 ns per frame) and are timing noise, not results.

## Audio identity

- **Effects.** `ElectryBenchmarkFx --capture` from the baseline and
  `--compare --max-error 0 --max-rms-error 0` from each candidate, over all 12
  scenarios × 44.1/48/96 kHz × stereo/mono/switching/guitar input ×
  Standard/High oversampling: 288 two-second cases, **72,230,400 float
  samples, zero changed samples** (the comparison counts every bitwise
  difference, signed zero included). Run for each FX commit and for the final
  revision against the baseline.
- **Engine and effects.** All 23 technique takes rendered by baseline and
  candidate in one process and compared sample by sample: **zero changed
  samples** in every take, for each commit against its parent and for the
  final revision against the baseline.

These are measured results for the stated compiler and machine; they are not a
cross-platform bit-identity guarantee. The changes do not reorder or contract
any floating-point expression, so other conforming compilers should agree.

## Validation

After every commit: the complete `build-dsp` build including
`ElectryDSPCxx17` (the Rack's C++17, `ELECTRY_EMBEDDED_TARGET`, no-exceptions
flags) and the full `unit` tier, 41 tests, all passing; the Python tool tests
used a virtual environment with NumPy and SciPy. `Electry.ElectryEngine` hit
CTest's 300-second limit once while sharing the loaded machine with parallel
tests, so it was run directly after each commit and passed (about 3.5 minutes
wall, 81 seconds CPU). The
`audio` tier of the DSP build passed on the final revision. The plug-in build
was not available on this machine (no JUCE checkout and no ALSA development
headers), so its `Electry.AudioDemos` and parameter-matrix tiers were not
run; the plug-in plays the same `ElectryEngine`/`ElectryFx` code measured
here.

## Rejected

- **Forcing the phase-inverter helpers inline** (`phaseInverterCurrentAndSlopes`,
  `phasePower`, `lookupPowerGridCurrent`): bit-identical but 6% *slower* on the
  American and British amps.
- **Caching the triode table pointer in `prepare()`**: 4.1% fewer instructions
  on the Modern amp, but no time change beyond noise (+0.1% and +1.2%).
- **Sharing the sine and cosine of the fundamental across the pitch
  compensation's phase terms**: about 1% of instructions on the dry takes,
  below what this machine can resolve; not implemented.

Anything that changes arithmetic (reciprocal multiplication for the circuit's
constant divisions, earlier Newton exit, coarser control rates) was out of
scope: it cannot be bit-identical.

## Measurement method

The machine was a shared four-core Linux VM with other builds running
(load average 5–17), so every comparison is paired and alternated, and timed
by CPU rather than wall clock:

- **Takes.** Baseline and candidate `Source/DSP/` trees are compiled into one
  executable, each with `-Delectry=electry_base` or `-Delectry=electry_cand`,
  together with `Tools/RenderDemos.cpp` (its `main` renamed). Process-wide
  tables are built before timing. Each round renders the selected takes with
  both builds, alternating which goes first, and times each render with
  `CLOCK_THREAD_CPUTIME_ID`; the report is the median of the per-round
  candidate/baseline ratios. The same executable's compare mode renders every
  take with both builds and counts bitwise differences.
- **Effects.** Two `ElectryBenchmarkFx` executables built from the same tool
  source against each tree, run alternately (ABBA); the reported figure is the
  median of each pair's `median_cpu_ns_per_frame` ratio.
- **Instruction counts** (Valgrind Callgrind, collection limited to the
  render or `ElectryFx::process`) corroborate the smaller changes and guided
  the profiling.

An identical-build A/A run of the take harness moved single rounds by up to
1.8% (median 0.45%), and single effects pairs varied far more, so medians
within about ±2%, or not repeated in a second campaign, were treated as noise.
