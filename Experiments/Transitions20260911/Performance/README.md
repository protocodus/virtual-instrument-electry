# Transition CPU benchmark

The staggered one-millisecond hand-loss fitting cadence resolves the transition-time deadline regression on the tested Apple M1 Max. Ordinary held notes remain within timing noise, and all measured 64- and 256-frame callbacks finish within their audio duration. This is an engine-only benchmark, not a guarantee for a full plug-in/FX chain or other hardware.

## Reproduction

Run from the repository root:

```sh
python3 Experiments/Transitions20260911/Performance/RunBenchmark.py
python3 Experiments/Transitions20260911/Performance/RunBenchmark.py --rounds 3 --block-size 64 --output Experiments/Transitions20260911/Performance/results-64frames.json
```

The script compiles the frozen `aa1f7e0` source in `build-transitions-20260911/baseline-src` and current source with identical `clang++ -O3` flags. Shipping measured-body, decoupled-release and energy-pitch definitions are enabled for both. Binaries stay in the ignored build directory. `--candidate`, `--baseline`, `--output`, `--block-size` and `--rounds` select another comparison. Source hashes and every individual run are in the JSON files.

## Method

Apple M1 Max, arm64, 96 kHz, engine only; FX and host processing are excluded. Eight open Drop-E strings (MIDI 28, 35, 40, 45, 50, 55, 59, 64) are struck at velocity 0.9 with the default guitar build, Bridge/Mono and zero strum spread. Every voice remains active for every timed render. Initial preparation and 200 ms of established ringing are excluded. Each scenario then renders two seconds.

Five scenarios compare sustained strings with no changes, alternating Sustain/Palm chord repicks at four or sixteen transitions per second, and pressure-only CC2 changes between zero and 0.85 at the same rates. The latter add no excitation. A transition is one hand-state change; a complete open/mute cycle contains two transitions. Repicks release/reacquire MIDI ownership at the same sample, retaining physical string state without a reset or silent gap.

One complete paired warmup is discarded. Seven measured baseline/candidate pairs at 256 frames alternate execution order; the bounded 64-frame check uses three pairs. Thread CPU time is primary, so time spent descheduled by concurrent builds does not masquerade as DSP work. Wall time is also retained. Both include event dispatch and inexpensive measurement overhead. The moving interval is the first 80 ms after each hand change. Callback percentiles exclude partial fragments split at event boundaries; aggregate timing retains all samples and events.

## Final results

Ratios express seconds of CPU per second of audio; 1.0 consumes the entire available duration. Values are medians across the paired rounds. Percentage changes are medians of the per-round ratios, so they need not equal a quotient of separately reported medians.

| Scenario | Baseline CPU/audio, 256 | Final CPU/audio, 256 | Paired change | Final callback p95, 256 | Final callback p95, 64 |
|---|---:|---:|---:|---:|---:|
| held | 0.105 | 0.106 | +0.7% | 0.111 | 0.109 |
| style_4hz | 0.119 | 0.146 | +23.1% | 0.268 | 0.285 |
| cc2_4hz | 0.111 | 0.132 | +18.5% | 0.249 | 0.241 |
| style_16hz | 0.124 | 0.228 | +82.8% | 0.287 | 0.312 |
| cc2_16hz | 0.112 | 0.200 | +78.3% | 0.265 | 0.288 |

Normal four-transition-per-second hand movement adds about 19–23% to engine CPU in this dense eight-string fixture. The 16 Hz stress adds about 78–83%, while remaining below 0.23× average CPU/audio. The largest callback observed across every final run is 0.783× at 256 frames and 0.661× at 64 frames; these individual maxima are retained separately from the medians. No measured callback exceeds 1.0×.

The remaining cost is concentrated while the hand moves. Each string’s nested passive loss solve now runs at roughly 1 ms, with phases staggered across strings. Contact coordinates still move at control rate, exact endpoints refit immediately, and pitch/fret/parameter refits remain direct. This reduces work while keeping every applied filter inside the existing passive solver’s bounds and retaining analytic phase compensation. The associated contact probe retains an 11.1 dB improvement in Palm→Open maximum sample step over the baseline; see `../Hand/README.md`.

## Rejected full-control-rate implementation

`results-initial.json` preserves the first combined candidate. It solved the nested damping fit every 16 internal samples during hand movement. Ordinary held notes cost approximately the same as baseline, but 4 Hz style and CC2 movement cost 0.361×/0.331× CPU/audio on average and approximately 1.62× at callback p95. At 16 Hz it averaged 1.168×/1.068×, exceeding realtime outright. Average whole-file headroom had hidden a genuine low-latency deadline problem.

`results-cadence1ms-trial.json` records a three-pair exploratory run before the final deferred-style guard refinement. Final claims use `results.json` and `results-64frames.json`, whose source hashes match the finished candidate.

The benchmark is intentionally dense: all eight strings meet the same hand movement. Fewer voices will be cheaper, but holding a full chord while moving CC2 is a valid use case. This is distinct from the existing loose 8× portable runaway ceiling; passing that ceiling alone does not establish low-latency operation. No platform-specific percentage assertion is added to CI because shared-runner timing cannot support a tight threshold reliably.
