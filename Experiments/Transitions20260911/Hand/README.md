# Finite bridge-hand contact probes

The final hand transition preserves the old string while the heel lands or lifts. Positive hand-contact/pressure coordinates move at control rate; nested passive loss fits run at a staggered one-millisecond cadence, with direct endpoint and physical pitch/fret refits. Freshly struck silent strings retain their established mute calibration.

## Reproduction

```sh
python3 Experiments/Transitions20260911/Hand/RunProbes.py
```

The default baseline is the frozen `aa1f7e0` source under `build-transitions-20260911/baseline-src`. `--baseline` and `--candidate` accept equivalent complete source trees. The runner builds each probe with the same shipping definitions, uses that tree’s own regression source, and stores binaries under the ignored build directory. `results.json` contains all outputs, exit statuses and source hashes, so the measurements survive without the temporary files.

`Probe.cpp` selects the existing shared-hand ownership/audio, live damping/pitch and Dead reference checks. On the current candidate it also selects the repeated finite-contact tests: sibling contacts, same-string repicks and CC2-only movement at 44.1/48/96 kHz. `Energy.cpp` compares an existing E2 ring through six 0↔0.85 CC2 transitions with a never-muted twin, then checks silent hand movement.

## Final findings

- The old-E1 Palm→Open maximum sample step over the first 30 ms falls from 0.00161323 to 0.000447109, an 11.1 dB reduction. Its unchanged-ring control is 0.000388749. The initial full-control-rate candidate measured 0.000434401; the one-ms scheduling change retains almost all of the improvement.
- Open→Palm maximum step is essentially unchanged: 0.000851393 to 0.000856938. The changed ring loses 6.90 dB of >500 Hz energy against a continued-open twin. Lifting gains 5.23 dB against continued muting: the surviving partials can ring freely without injecting new excitation.
- The live contact reaches 95% engagement/lift at 12/24 ms for 48/96 kHz and 12.154/24.127 ms at 44.1 kHz. Both coordinates reach their exact targets within 80 ms. These are design response times, not player-specific measurements.
- The largest per-control-tick pitch step across the final matrix is 0.0832 cents for repicks, 0.00560 cents for CC2 and 0.00320 cents for held siblings. Every existing ownership and live-pitch check passes.
- The final stateful Open/Palm/Dead/Dead median is −6.91362/−13.4691/−21.7443 dB, inside the unchanged reference ranges and reproducibility limits. No reference tolerance was enlarged for this pass.
- Maximum circulating-line energy relative to a never-muted twin is 1.00033/1.00018/1.00069 at 44.1/48/96 kHz: at most +0.0030 dB. This sums delay-line samples and is only a state-energy proxy; it is not a formal proof for the full time-varying filter system. Silent hand movement remains exact digital silence.

`original-isolated-results.json` preserves the first isolated hand-only probe output, before the integrated tension fix and CPU scheduling change. It is historical evidence; final source hashes and measurements are in `results.json`. The expensive full-control-rate version was rejected for callback deadline overruns; see `../Performance/README.md` for the final 64/256-frame measurements.
