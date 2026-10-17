# Effects CPU optimization

Measured on 2026-09-08 against `bf652d4934f077ddafcdea337fa0aa83923a8fcb`.

The optimization retains the circuit equations, nonlinear solve tolerances,
oversampling rates and filters, cabinet processing, parameter ramps, and the
continuously clocked delay/room histories. It removes repeated work:

- The American/British phase-inverter current and Newton slope share one
  exponential when evaluating softplus and sigmoid.
- Derived drive gains are retained once their float smoothers stop moving.
- Amp-model selection and retirement of faded-out circuit state run once per
  host sample, rather than for both channels at every oversampled frame.
- Oversampling swaps pointers between its two scratch buffers instead of
  copying both arrays at each stage.
- Delay indices wrap with a bounded comparison instead of integer remainder.

## Processing-time results

Eight sequential outer runs used ABBA then BAAB order (A = baseline,
B = candidate). Each case rendered one second in 128-frame blocks with three
timed repetitions per outer run. The following reductions compare the median
of each build's four outer-run medians; positive values mean less processing
time.

| Effects | 44.1 kHz | 48 kHz | 96 kHz |
| --- | ---: | ---: | ---: |
| Pedal | 2.3% | 3.0% | 6.8% |
| American amp | 3.8% | 5.1% | 5.5% |
| British amp | 6.1% | 5.9% | 4.3% |
| Modern amp | 7.2% | 13.5% | 12.2% |
| All effects, American | 5.5% | 5.5% | 7.2% |
| All effects, British | 3.6% | 0.0% | 5.2% |
| All effects, Modern | 1.8% | 2.0% | 4.8% |
| Control/model automation | 4.2% | 4.0% | 3.6% |

For example, the 48 kHz Modern amp case falls from 1,079.8 to 933.7 ns per
stereo frame (5.18% to 4.48% of the available realtime interval). The American
case with all effects falls from 15,011.5 to 14,185.5 ns (72.06% to 68.09%).

There is timing variability, especially in the American/British circuit
cases. British with all effects at 48 kHz is effectively unchanged (-0.017%
reduction). Dry/compressor/delay/room-only cases are already inexpensive and
their small changes include noise: dry at 44.1 kHz rises from 22.33 to 22.87 ns
per frame, while dry at 48 kHz falls from 23.04 to 22.24 ns. These data support
the amp-path savings, rather than a claim that every preset improves equally.

The [complete summary](fx-cpu/summary.csv) includes all 36 cases and outer-run
minimum/maximum values. [Raw timings](fx-cpu/timings.csv) retain all eight
runs, and [sample comparisons](fx-cpu/comparison.csv) retain every audio result.

## Audio equivalence

All 36 two-second stereo comparisons at 44.1, 48 and 96 kHz are bit-identical
to the baseline: **9,028,800 float samples, zero changed samples, zero peak
error and zero RMS error**, including signed zeros. The comparison includes
all amplifier voices, the individual effects, stacked effects, automation,
quiet passages and silence tails. This is a measured result for the stated
build and stimuli, rather than a cross-platform bit-identity guarantee.

## Validation

- Native Release DSP/tool CTest suite: all 10 tests passed on the final source.
- JUCE plugin processor integration: passed.
- Effects suite with `ELECTRY_MEASURED_MODERN_CABINET=ON`: passed, including
  cabinet convolution and model-transition coverage.
- Existing effects checks retain alias rejection, circuit residuals, bypass,
  engagement, block invariance, sample-rate coverage and hostile-input guards.
- Benchmark reference round trips passed; changed audio, truncated references
  and mismatched block-size metadata were rejected as expected.
- `git diff --check` passed.

## Measurement method

`ElectryBenchmarkFx` uses deterministic, distinct stereo inputs containing
multiple tones, pluck-like bursts and noise. It covers the five individual
effects, each amplifier, each amplifier with all effects, dry bypass, and
continuous control movement with model switches. Preparation, allocation,
input generation, buffer copying and reset are outside the measured interval;
per-block parameter updates are inside it. Each case is warmed before timing.

The measurements use an Apple M1 Max, native arm64, AppleClang
21.0.0.21000101, C++20, `-O3 -DNDEBUG`, and the default cabinet configuration.
Baseline and candidate use the same harness and compiler flags. Times measure
elapsed effects processing, excluding the string engine and plugin/UI work.
The realtime percentage is elapsed processing time divided by audio duration;
it is not an operating-system CPU utilization measurement. Results are specific
to this machine and workload.

## Reproduction

Build both standalone executables from the same harness, retaining the old
DSP source and headers separately:

```bash
mkdir -p build-fx-before/Source/DSP
for file in ElectryFx.cpp ElectryFx.h DspMath.h ModernCabinetIR.h; do
    git show "bf652d4934f077ddafcdea337fa0aa83923a8fcb:Source/DSP/$file" \
        > "build-fx-before/Source/DSP/$file"
done
c++ -std=c++20 -O3 -DNDEBUG -Ibuild-fx-before/Source \
    Tools/BenchmarkFx.cpp build-fx-before/Source/DSP/ElectryFx.cpp \
    -o build-fx-before/before
c++ -std=c++20 -O3 -DNDEBUG -ISource \
    Tools/BenchmarkFx.cpp Source/DSP/ElectryFx.cpp \
    -o build-fx-before/after
```

Run timings sequentially on an otherwise idle machine, alternating which
build runs first. Keep every command's CSV output:

```bash
build-fx-before/before --seconds 1 --repeats 3 > build-fx-before/before-1.csv
build-fx-before/after  --seconds 1 --repeats 3 > build-fx-before/after-1.csv
build-fx-before/after  --seconds 1 --repeats 3 > build-fx-before/after-2.csv
build-fx-before/before --seconds 1 --repeats 3 > build-fx-before/before-2.csv
build-fx-before/after  --seconds 1 --repeats 3 > build-fx-before/after-3.csv
build-fx-before/before --seconds 1 --repeats 3 > build-fx-before/before-3.csv
build-fx-before/before --seconds 1 --repeats 3 > build-fx-before/before-4.csv
build-fx-before/after  --seconds 1 --repeats 3 > build-fx-before/after-4.csv
```

For sample comparison, both executables must use the same duration and block
size. Reference files record the stimulus version, scenario, rate, frame count,
block size and cabinet configuration; incompatible or missing files fail the
comparison. A zero error budget requests a numerical null; `changed_samples`
separately counts every bitwise difference, including signed zero:

```bash
build-fx-before/before --capture build-fx-before/reference --seconds 2
build-fx-before/after --compare build-fx-before/reference --seconds 2 \
    --max-error 0 --max-rms-error 0
```

Capture/compare adds quiet passages and a final silence interval to exercise
gain recovery and delay/room tails. Its numerical error limits are configurable;
the default peak limit is 0.00005 FS and the default RMS limit is -120 dBFS.
The harness rejects non-finite output and corrupt or incompatible references.
