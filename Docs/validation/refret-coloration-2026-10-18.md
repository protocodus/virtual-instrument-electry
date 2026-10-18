# Fixed-tap fret handoff coloration

18 October 2026. Shipping A is current main
`d5fc768412b0d6441f34a70c4e98658e0d1bf5b4` (DSP 1.2.4); candidate B is
`7d29d0e7be7f2e2afb7f5b8b4bb35285671fe8bd` (DSP 1.2.5). Later test/document
commits preserve that DSP snapshot. This continues the
[fret-handoff work](refret-handoff-2026-10-18.md), whose selected B is now the
new shipping A. The previous room's old A is not the baseline for this task.

## Change and limits

Only the two-tap handoff's cubic control curve changes. Minimum-degree C1
velocity shoulders join the legacy peak speed `3/2` to rest; integrating them
gives C2 weight endpoints and joins. Unit total weight derives a shoulder
width of one third of Contact. Mirrored float evaluation avoids cancellation
near one. Both stationary read
positions, Contact duration, queued destinations, expression smoothing,
large-jump choke, pickup/filter history and new pick Release remain intact.
There is no new output fade, gain compensation, buffer reset or tuning control.

For ideal frozen taps, `|H|²=1-4w(1-w)sin²(omega*delta/2)`. Integrating
`w(1-w)` over the unchanged clock gives 449/3780 instead of 9/70: a 7.6132%
reduction. This is a reduction in integrated frozen cancellation exposure,
not a measured percentage of audible realism. The deepest midpoint notch
remains and the continuous maximum slope stays `3/2`. The integrated squared
slope rises 8.93%, and local slopes differ from cubic. A frozen convex bound
is not a proof of time-varying energy passivity. Actual transient tests are
therefore required; no constant is fitted to a recording.

The separately tested quintic `c6dae72` reduces ideal deficit 15.8%, but raises
peak fade speed 25% and squared-speed integral 19.05%. An ideal lengthening
impulse example changes copy energy from 0.938416 to 1.055355 for that
quintic, illustrating why frozen-response algebra is insufficient. The
selected profile gives up some deficit reduction to stay closer to shipping
in rapid-play tests. Its complete `Source/` tree is byte-identical to slope
prototype `6b08b9a`; final audio is rebuilt against the actual selected
revision above. Neither candidate is uniformly lower in transient energy.

The [research](../research/refret-coloration-2026-10-18.md) compares auxiliary
taps, phase/correlation normalization, power-state allpasses and physical fret
state remapping. A convex three-tap candidate reduces some low-order losses
but creates deep new partial holes; it was rejected analytically before audio
rendering. Neither higher tap count nor more endpoint derivatives establishes
a uniformly better string transition.

## Real recordings and current sound color

The four frozen Guitar-TECHS phrase excerpts retain all 14 annotated attacks,
their source MIDI, string ownership, one initial source offset and the original
shared A-derived crop. A and B use identical production-path events, seed,
parameters and output rate. Whole-file RMS matching and one shared peak cap
are the only gain operations. There is no EQ, denoising, pitch change,
per-note alignment, time warp or per-window gain. Source12's approximately
4.45 ms transfer-onset residual remains. Original recordings stay outside Git
and are not runtime excitation. Attribution and source hashes remain in the
[portable selection receipt](../research/refret-reference-selection-2026-10-18.json).

The [current dry color analysis](../research/dry-guitar-color-2026-10-18.md)
compares held, fresh and repicked notes plus seven isolated controls. The
Sire phrase pairing has a persistent low/mid harmonic imbalance even before
any fret change; the SG bridge pairing is much closer. Construction, pickup
position/selector, string type, unknown human force and release remain
confounds. These observations do not identify global corrective gains or
justify changing the instrument's default build. The fade change leaves that
larger tonal question open.

## Registered attack comparisons

The source-bound observer uses exactly the previous windows and gain protocol.
Ranges below include all 14 attacks. Step and high-band measures use the
whole-crop level-matched signal; RMS ratios use unscaled production output.

| Window relative to registered onset | B/A maximum sample-step range | B/A 8 kHz high-pass RMS, dB | B/A raw RMS range |
| --- | ---: | ---: | ---: |
| -5 to +10 ms | 0.991762–1.000090 | -0.010243 to +0.004150 | 0.999696–1.004345 |
| 0 to 30 ms | 0.991762–1.000099 | -0.010308 to +0.030341 | 0.999712–1.004172 |
| 20 to 80 ms | 0.992527–1.000099 | -0.010321 to +0.085433 | 0.999681–1.003713 |

The wider-window high-band increase is reported rather than hidden by the
primary window. These are descriptive artifact checks, not perceptual
thresholds or proof of a listener preference. The candidate does not achieve
its result through blanket output attenuation. Same-note 8 Hz and 12 Hz
repeats, rapid Palm changes and the alternating-octave large-jump control
remain byte-identical. Returning/overlapping small-fret probes change as
intended. Descending real phrases are included.

## Rapid-play tradeoff

An additional 72-case actual-engine matrix uses identical seed, gain 1,
48/384 kHz hosts, low/high strings, hardness 0/0.58/1 and ascending/descending
one-/three-semitone alternations. Each case has 32 events at 1, 12.5 or 50 ms
cadence, an initial 100 ms note and 200 ms tail. Noise, artifacts, body and
sympathetic controls are zero to expose the handoff; this diagnostic is
separate from the normal-path real-DI auditions. Every output sample and
checked string/filter/handoff state stays finite for all three engines.

At the playable 20 Hz cadence, maximum fixed-event raw RMS/step/HF increases
are +0.345/+0.679/+0.992 dB for quintic and +0.152/+0.411/+0.354 dB for the
selected profile. Subtracting each case's RMS change gives maximum step/HF
changes +0.420/+0.751 dB versus +0.278/+0.265 dB. At the worst quintic HF
case, absolute HF RMS is -80.67 dBFS shipping, -79.68 quintic and -80.33
selected; the largest step occurs 6.52 ms after NoteOn, beyond its 0.55 ms
Contact. The change is spread through ringing rather than concentrated only
at the Contact endpoint. These metrics neither prove retained harmonics nor
identify a broadband click.

Across all cadences, maximum fixed-event HF rises +2.176 dB for quintic and
+1.374 dB selected; relative to case RMS these are +1.803 and +1.197 dB.
The selected curve is more conservative, not uniformly lower: some cases
have more HF/step than quintic. The real phrases' maximum wider-window HF
increase is also slightly higher selected (+0.0854 versus +0.0640 dB).
This explicit tradeoff follows the user's attack-quality priority. Short,
finite stress runs do not establish indefinite stability or perceptual
artifact-free performance.

## Checks

Local checks pass all 38 unit suites and the serial/parallel 23-demo audio
determinism suite. The embedded target compiles the shared sources as C++17
for arm64 and x86_64 with exceptions and RTTI disabled. ASan/UBSan passes the
engine and refret suites; the final strengthened refret rerun also passes.
No previous test or tolerance was relaxed.

The refret suite now has 132,409 assertions with zero failures. Its new probe
seeds known, disjoint source/destination interpolation neighborhoods and
observes the actual pre-filter mixture stored by the first allpass. It checks
the entire finite clock at six durations against an independent direct-double
shoulder polynomial, including endpoints, convexity, monotonicity and
completion semantics. The same test snapshot linked to shipping A's cubic
engine produces 2,198 failures and to the quintic engine 2,208 failures;
the production formula is not merely copied into an observer and
then declared correct. All 48 physical-Release pitch cases retain their prior
maximum 2.98850-cent absolute error, with only 0.0000637 cents unexplained
beyond the existing intentional energy-pitch span.

An independent float audit covers all 1,180,410 weights for Contact lengths
4–1536, finding no nonfinite, out-of-range, descending or endpoint failures.
Finite-clock integrated deficit ratios range 0.923868–0.976339 versus cubic.
This numeric bound does not replace the transient-energy limitation above.

Independent source/PCM/HTTP audits pass all 38 delivered WAVs: four phrase
triples, six performance pairs and seven isolated real/shipping pairs. The
maximum constant-gain reconstruction residual is 1.69e−7 FS and maximum
within-group RMS spread is 7.37e−8 dB. Original MIDI/score binding, shared
crops, source and binary hashes, three generated scripts and ten navigation
links pass. All 1,008 recomputed attack observations agree within 5.36e−14.
New A's raw PCM matches the prior selected B `634fd33`; real-reference crops
are unchanged. Old rejected prototypes using A `49f42e3` are excluded from
this task's acceptance comparison rather than silently relabelled.

Hosted platform checks and review are tracked on the pull request; they remain
a separate merge gate from these local checks.

## Reproduction

Use Python 3.12 and `Tools/requirements-review.txt`. Restore the licensed
source members from the portable receipts to reconstruct the frozen plan.

```sh
python Tools/BuildPhraseComparisons.py --base d5fc768 \
  --candidate 7d29d0e7be7f2e2afb7f5b8b4bb35285671fe8bd \
  --plan build-coloration-review/reference-plan-phrases.json \
  --output build-coloration-review/final-room
python Tools/BuildPerformanceComparisons.py --base d5fc768 \
  --candidate 7d29d0e7be7f2e2afb7f5b8b4bb35285671fe8bd \
  --output build-coloration-review/final-room
python Tools/AnalyzeRefretComparisons.py \
  --manifest build-coloration-review/final-room/phrases/measurements.json \
  --output build-coloration-review/evidence/final-phrase-attack-analysis.json
```

The local comparison room is `http://127.0.0.1:8725/`, with separate mechanism
keys and seven additional isolated real-DI controls. Earlier rooms are frozen.
Working audio and full numerical evidence stay in `build-coloration-review/`;
the selected verdict is committed. Validation status and evidence hashes are
recorded in the accompanying machine receipt. No subjective listening vote,
interactive audio QA or Reason-host playback is inferred from these checks.
