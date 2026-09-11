# Idle-string hand loss during articulation changes

This experiment isolates one physical-model correction in the next transition pass. The baseline is the completed first transition pass frozen under `build-transitions-20260911-pass2/baseline-src`; the candidate is its ring-only copy under `ring-src`. The final combined model can also contain the separately tested picked-string attack change.

The sympathetic renderer multiplied a coefficient once when a wave returned around its delay loop, but that coefficient described **one sample** of hand attenuation. This made hand contact hundreds or thousands of times weaker than its stated T60, with a different error on every string and sample rate. Existing tests computed an intended per-period gain from the per-sample coefficient; that equation did not describe what the renderer actually applied.

The correction stores a positive inverse T60 and computes each idle string's scalar as `exp(-3 * ln(10) * inverseT60 * period / sampleRate)`. The live loss rate follows the same 4 ms landing / 8 ms lift constants as the played heel, and snaps to exact settled endpoints. Each ringing idle voice caches its scalar at control rate only while that rate moves; a tuning update also refreshes it. The scalar has no phase, stays between zero and one, and removes energy inside the string loop. Existing CC2 pressure loss remains in the separately solved string decay and is not added a second time here.

## Direct rendered decay evidence

`IdleDecayProbe.cpp` seeds an idle waveguide at its open fundamental, disables drive, then records values actually written by `renderSympatheticString`. A linear fit to the dB ratio of muted/open RMS envelopes measures the **additional hand loss**, independently of intrinsic string loss. It does not read a desired filter equation and report it as a measurement.

| Added hand T60 requested | Baseline measured | Corrected measured |
| --- | --- | --- |
| 0.120 s | 32.109–558.770 s | 0.11974–0.12023 s |
| 1.600 s (Dead contact) | 428.319–7459.856 s | 1.59808–1.59993 s |

The range covers low E1, A2 and high E4 at 44.1, 48, 96 and 192 kHz host rates. The very long baseline numbers are **only the erroneous added hand-loss time**; intrinsic string damping still limits the total audible ring to seconds. These synthetic probes establish time units and rate/string invariance, not a fit to newly captured guitar recordings.

`before.csv` and `after.csv` retain every measured row. The production regression also measures rendered decay, but obtains default Palm and Dead contact through ordinary keyswitch/note-on dispatch first; all 16 cases pass. The old analytic regression now reads the actual cached per-period hand gain once. The complete `ElectryEngineTests` suite passed without changing reference fixtures, tolerances or feedback expectations.

To reproduce the direct probe, compile `IdleDecayProbe.cpp` with the engine and visuals from the selected snapshot, `-std=c++20 -O2 -DNDEBUG`, and the shipping definitions `ELECTRY_ENERGY_ATTACK_PITCH=1`, `ELECTRY_MEASURED_BODY_RESPONSE=1`, `ELECTRY_DECOUPLED_PICK_RELEASE=1`. For the corrected source additionally define `ELECTRY_PERIOD_AWARE_IDLE_HAND=1`; this affects only probe access to the new physical rate state, not production DSP.

## Musical and CPU checks

`AnalyzeRing.py BASELINE_AUDIO RING_AUDIO riff-results.json` verifies the nine original take manifests match, then compares unnormalised PCM. The ring-only correction leaves all pressure-driven and zero-sympathetic probes byte-identical. For the rolling style riff, the difference relative to baseline RMS is −42.95 dB in DI and −35.48 dB after Modern FX. Whole-file level changes are under 0.01 dB. This is background cleanup; these descriptors do not establish perceived realism or promise a large change to the picked attack. The full combined audition additionally includes a low-rhythm gap test to expose idle-string residue.

`Benchmark.cpp` and `RunBenchmark.py` exercise one held A2 with up to seven idle sympathetic strings at the shipping 0.20 setting, 96 kHz / 256 frames. This explicitly covers the new idle-loop work that an eight-active-string test would miss. Three alternating paired rounds exclude a warmup and measure thread CPU time, including note dispatch. `performance.json` retains source hashes and every round.

| Scenario | Candidate realtime CPU ratio | Median paired change |
| --- | ---: | ---: |
| Held | 0.06076× | −0.23% |
| Style changes at 4 Hz | 0.06673× | +1.24% |
| CC2 changes at 4 Hz | 0.06421× | +1.57% |
| Style changes at 16 Hz | 0.07633× | +1.83% |
| CC2 changes at 16 Hz | 0.07278× | +1.48% |

A ratio of 1 is realtime. Candidate callback p99 ratios remained below 0.119×. Small paired changes include timing variation; pressure-only audio is bit-identical. The CPU script defaults to the frozen snapshots and can be pointed at final combined source with `--candidate`.
