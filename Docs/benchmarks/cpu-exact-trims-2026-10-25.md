# Exact CPU trims — 2026-10-25

Ten mechanisms that remove repeated or out-of-line work from the string
engine and the FX chain without changing a single output bit. Every one is
graded **exact**: the float32 output of the regression stream is
byte-identical to the baseline on every factory rig and every configuration
tested. Together they cut the Rack-like build's instruction count by
**7.6 %** at the shipping FX quality and **8.6 %** at High FX oversampling;
the host GCC build falls by 5.6 %.

Baseline: `58e68a0` (1.2.8). Final: `0cdd41d`. Engine and FX behaviour,
quality settings, defaults, latency, oversampling, voice counts, noise models,
parameter ranges and every test and tolerance are unchanged.

## Protocol

The harness is `Tools/BenchmarkPerformer.cpp` (`ElectryBenchmarkPerformer`,
built with the tools); its head carries this protocol.

- **Workload.** The four factory rigs, each playing its committed demo phrase
  from `Docs/audio/midi/` through `ElectryPerformer` (Performer MIDI handling,
  eight-string engine at its 2x internal rate, FX chain), with the rig's
  factory parameters as `RenderPresetDemos` loads them: FX on for Drop-E
  Metal and Blues Rock Lead (both Modern High-Gain), dry DI for Factory
  Default and Mute / Dead DI; Blues Rock Lead in stereo output mode. MIDI
  events land on their own sample, same-sample keyswitches and CC2 condition
  the attacks, and same-sample note-ons form one chord, as in the processor.
  The engine has no seed input; its noise is deterministic from reset.
- **Instruction counts.** `valgrind --tool=callgrind` over the whole run
  (setup included, a small constant), the first 6 s of each rig at 48 kHz in
  64-frame blocks (the Rack's batch), 1,152,000 stereo frames in all.
  Counts are reproducible on the shared development machine; no timing is
  quoted, because the machine ran at a load average near 18 on 4 cores
  throughout.
- **Rack-like build (primary).** Clang 18.1.3, C++17, `-O3 -fno-builtin
  -fno-exceptions -fno-rtti`, `ELECTRY_EMBEDDED_TARGET=1`
  (`-DELECTRY_EMBEDDED_TARGET=ON`), shipping feature defines, plus `-g` for
  line attribution (no code-generation change). This is the host's nearest
  stand-in for the SDK's clang 15 `-ffreestanding -fno-builtin` build; the
  SDK itself was not available, so no figure here is an SDK-native or Reason
  CPU measurement.
- **Host build.** GCC 13.3, C++17 DSP, `-O3` Release, no `-fno-builtin`.
- **Parity.** Each candidate's float32 output file was compared byte for
  byte with the baseline's, rendered by the same harness on the baseline
  source (`58e68a0` plus the harness). Configurations: all four rigs over
  their full demo length (up to 20 s, 2,892,806 frames) at Standard and at
  High FX in both builds, and at 44.1 kHz in 256-frame blocks (Rack-like);
  from the tube-table commit on, also both FX rigs through the American and
  British amp voices (`--amp`), Standard and High, in both builds, to cover
  the power-pair path the factory rigs do not use. The final tree matches the
  baseline on all eleven configurations.

## Per-commit instruction counts

Rack-like build, Standard FX (shipping default) unless stated. Steps are
relative to the previous row, cumulative to the baseline.

| Commit and mechanism | Grade | Ir | Step | Cumulative | High FX Ir (cumulative) | Host GCC Ir (cumulative) |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| baseline `58e68a0` | — | 16,421,045,110 | — | — | 19,072,650,915 (—) | 23,478,569,002 (—) |
| `bacfe1b` integer ceiling in delay reads | exact | 16,331,201,129 | -0.55 % | -0.55 % | 18,982,806,906 (-0.47 %) | 23,184,754,611 (-1.25 %) |
| `cd312f5` IEEE square root instead of libm sqrt | exact | 16,018,940,721 | -1.91 % | -2.45 % | 18,626,509,977 (-2.34 %) | 23,184,754,588 (-1.25 %) |
| `33f5f6a` release-spread width once per rate | exact | 15,871,716,558 | -0.92 % | -3.35 % | 18,479,285,777 (-3.11 %) | 22,947,169,458 (-2.26 %) |
| `dfac0fd` dip section once per candidate depth | exact | 15,792,690,758 | -0.50 % | -3.83 % | 18,400,259,977 (-3.53 %) | 22,876,666,220 (-2.56 %) |
| `d2a334c` dispersion grid early exit | exact | 15,739,988,573 | -0.33 % | -4.15 % | 18,347,557,792 (-3.80 %) | 22,815,410,927 (-2.82 %) |
| `fa0c10b` per-bank dip-section cache | exact | 15,628,533,510 | -0.71 % | -4.83 % | 18,236,102,729 (-4.39 %) | 22,715,678,388 (-3.25 %) |
| `a32c52c` shared trig per compensation frequency | exact | 15,580,668,216 | -0.31 % | -5.12 % | 18,188,237,435 (-4.64 %) | 22,683,239,551 (-3.39 %) |
| `a68d9e6` mirrored output-decimator history | exact | 15,492,709,777 | -0.56 % | -5.65 % | 18,100,279,029 (-5.10 %) | 22,483,987,652 (-4.24 %) |
| `4bf258a` mirrored FX halfband history | exact | 15,385,174,918 | -0.69 % | -6.31 % | 17,852,166,041 (-6.40 %) | 22,391,688,901 (-4.63 %) |
| `0cdd41d` tube-table index by truncation | exact | 15,171,767,206 | -1.39 % | -7.61 % | 17,425,350,617 (-8.64 %) | 22,157,956,031 (-5.62 %) |

## Mechanisms

1. **Integer ceiling in delay reads.** The loop read, pickup taps and
   `writeAdd` split each delay at `ceil(delay)`; under `-fno-builtin` that
   was a libm `ceilf` call in every loop read. The delays are clamped
   positive and far below 2^31, where truncation is floor and the int
   conversion is exact, so `positiveCeilToInt` is `std::ceil` bit for bit
   there (checked by `DspCompatibilityTests`).
2. **IEEE square root.** `scalarSqrt` replaces every float/double
   `std::sqrt` in the engine and FX. Square root is correctly rounded, so the
   instruction and libm agree. Host clang lowers even `__builtin_sqrt` to the
   errno-setting libm call under `-fno-builtin`; the helper uses
   `__builtin_elementwise_sqrt` where the compiler has it. GCC already
   inlined sqrt (no host change). In the SDK, `std::sqrt` is a bit-serial
   software routine, so the Rack gain is expected to be larger than measured
   here.
3. **Release-spread width per rate.** `round(0.000125 * fs)` moved from the
   releasing-string boundary read to `prepare()`.
4. **Dip section once per candidate depth.** `handLossResponse` is split into
   `handLossSection` and its frequency response; the bass material-loss
   bisection and the intrinsic-shelf search build each candidate's section
   once for all their anchor frequencies.
5. **Dispersion grid early exit.** A coefficient pair whose running maximum
   error already exceeds the best cannot be selected by the minimax and its
   tie-break, so its remaining modes are not scored.
6. **Per-bank dip-section cache.** Each loop filter bank keeps its hand and
   material dip sections keyed by the exact bits of depth and shape; the
   per-tick pitch compensation (attack-pitch relaxation, bends, vibrato), the
   sympathetic retune and the refret phase reuse them.
7. **Shared trig per frequency.** Both polarisations' one-pole, shelf, hand
   and material phase terms at one compensation frequency share one float and
   one double sine/cosine pair (`PhaseFrequency`).
8. **Mirrored output-decimator history.** The 31-tap halfband decimator's
   64-sample ring is written twice, so its taps read one contiguous window
   without masks, summed in the same order.
9. **Mirrored FX halfband history.** The same for every FX oversampling
   interpolator and decimator stage.
10. **Tube-table index by truncation.** The triode, power-pair and grid
    current lookups replace a sign-general bit-level floor by the integer
    conversion, which is floor for their non-negative positions.

## Checks run on the final tree

- `ctest -L unit` in the GCC `build-dsp` build: 41 of 41 pass. The
  `Electry.ElectryEngine` suite was run on its own after the other 40 at
  `-j2`: on this contended machine it once exceeded its 300 s timeout and
  once tripped a timing guardrail when run alongside the others, and passed
  every isolated run (about 110 s). No test was skipped or changed.
- `ElectryDSPCxx17` builds with GCC and with clang.
- GCC AddressSanitizer + UndefinedBehaviorSanitizer
  (`-DELECTRY_ENABLE_SANITIZERS=ON`): the harness over all four rigs and over
  the American and British voices at High FX and 44.1 kHz/256 runs clean, and
  the sanitized `Electry.ElectryFx`, `Electry.ElectryEngine`,
  `Electry.Performer` and `Electry.DspCompatibility` suites pass.
- Not run: the `fuzz` tier (`Electry.DspFuzz` needs Clang's libFuzzer and
  sanitizer runtimes, which this machine lacks; there is no fuzz test in the
  DSP build otherwise), the multi-hour `audio` tier, and the plug-in build.

## Candidates not shipped

- **Uninitialised oversampling scratch.** `renderGainStage` zero-fills two
  eight-frame buffers per host sample that are always written before they
  are read (memcheck clean). Exact, but only -0.03 % here; not worth the
  change.
- **Rational kernels for the remaining libm transcendentals.** `atan2`,
  `atan2f`, `sin`, `cos` and `pow` in the per-tick phase compensation, loss
  solves and dispersion fit are now about 8 % of the Rack-like count. A
  Cephes-style kernel is last-bit at best and needs the one-ulp probe across
  every rig first.
- **Reciprocal multiplies and FIR partial sums.** The dispersion grid's
  per-mode division, the triode position's division and four-way partial
  sums in the halfband FIRs reorder or re-round arithmetic: last-bit or
  measured-inaudible candidates, not exact.
- **Sharing the float trig with the dispersion phase delays** in the tick
  compensation: exact, estimated near 0.2 %.
- **Idling sympathetic strings sooner.** Changes what a retrigger hears; a
  listening decision, not a null test.
