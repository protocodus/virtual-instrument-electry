# Tone source snapshots, 2026-10-18

The unit-area, equal-mean-delay Bessel release and fresh-string intrinsic loss
correction intentionally change the complete picked source. The single-coil
partial and octave fixtures include that source, so their previous centers no
longer describe the combined candidate. They are regression snapshots, not pickup
measurements or targets fitted to dry recordings. Pickup topology, notch depth,
dark-versus-bright contrast, low-register body, velocity dynamics and all other
behavioral tests remain separate requirements.

`Tools/CaptureToneSnapshots.py` independently compiled the immutable production
DSP at `7a7b76f858678180948ca64fc59586829a9fea56`, with its default flags and
with `ELECTRY_BUTTERWORTH_LOSS_SHELF=0`. It preserved the original fixture render
calls, windows and assertions, added 17-digit observation output, and invoked
the existing pickup, velocity and Dead-note fixtures. It did not convert their
failures into passes. The same observer on shipping
`3df6a10b77b8bee0c04ab20bda34b28b81f2ec84` reports zero original failures.
The receipts, exact observations, pre-refresh failure messages, compiler and
source hashes are in [the accompanying JSON](tone-source-snapshots-2026-10-18.json).

| Immutable source state | Original snapshot failures | Other failures in these three fixtures |
| --- | ---: | ---: |
| Shipping `3df6a10` | 0 | 0 |
| Combined candidate `7a7b76f`, shelf on | 18 | 0 |
| Combined candidate `7a7b76f`, shelf off | 13 | 0 |

The refresh replaces 16 complete-source partial centers and 12 octave centers
for each independently observed state. The partial tolerances remain 0.75 dB
through partial 16 and 2.5 dB above it. The humbucker octave tolerance remains
12 dB, and the single-coil tolerances remain 1.25/2.5 dB under the same existing
signal-floor condition. The independent 12 dB pickup-contrast rail, 3 dB chord
balance rail, low-E body floor and render protocols are unchanged. Historical
branches for other experimental feature combinations retain their prior centers;
these observations do not calibrate those combinations.

One numerical-floor octave center needs an architecture-specific snapshot. The
note-28 single-coil 8--16 kHz band is -101.661721 dB on native ARM and
-97.841925 dB on the x86_64 build; with the shelf off it is -101.615803 and
-97.853588 dB. The independently observed x86 centers are selected on x86_64,
including its Windows architecture macro. The tolerance remains 2.5 dB. All
other centers are shared. The x86 observations use the unchanged original
observer linked to the x86_64 slice of the universal production DSP library;
their compile arguments, library and output hashes are in the same JSON.

The new unit-area release reduces the short-window source crest. Shipping's
measured full-velocity peak is -25.883081 dBFS; both observed candidate states
measure -28.134267 dBFS. The explicit source snapshot is therefore recentered at
-28.134267 dBFS with the same +/-1.5 dB tolerance. A separate assertion retains
the old absolute upper ceiling, `-25.690 + 1.5 = -24.190 dBFS`. The 18 dB overall
velocity span, 3 dB upper-half span, 16-step monotonicity check, spectral-dynamics
rail and all other velocity requirements retain their original thresholds.
There is no output gain adjustment in this change.

To reproduce the observations from the repository root:

```sh
python3 Tools/CaptureToneSnapshots.py --repo . \
  --revision 7a7b76f858678180948ca64fc59586829a9fea56 \
  --output build-snapshot-observer/default
python3 Tools/CaptureToneSnapshots.py --repo . \
  --revision 7a7b76f858678180948ca64fc59586829a9fea56 \
  --define ELECTRY_BUTTERWORTH_LOSS_SHELF=0 \
  --output build-snapshot-observer/loss-off
```

These observations establish why the old deterministic centers became stale.
They do not establish perceptual realism or authorize weakening a performance
guard; the frozen real-DI comparisons and full test suites assess those claims.

After refreshing the centers, the complete Engine suite passes on native ARM
with the shelf both on and off. The pickup fixture also passes in both states
on native ARM and the x86_64 build under Rosetta. Binary, final test-file and
verification-log hashes are recorded in the JSON. No DSP source was edited
by this snapshot refresh.
