# Independent sympathetic-loss review

Read-only review of the isolated ring candidate, before combining it with the
attack change. Source SHA-256 values are in `bend-review-results.json`.

## Findings

The baseline multiplies a gain derived for **one sample** once per delay-line
round trip. Its existing decay oracle then raises that gain to `period`, so it
checks the intended equation instead of the multiplier actually rendered.
The corrected oracle should read the per-voice loop multiplier directly.

The candidate's `lastCompensatedPeriod` stores the full `sampleRate / f0`,
including the smoothed global pitch bend. This is the correct time interval
for the new round-trip attenuation. Raw `currentDelay` and `targetDelay` omit
filter phase and would give a string-dependent decay error. The candidate
updates the hand gain after assigning the full period during bend refits.
A rapidly bending string still follows its tuning over the existing six-ms
delay motion; the stationary tests do not claim an exact instantaneous T60
during that motion.

Pressure already changes the coupled string's ordinary loop solve. The new
style loss correctly excludes pressure. Pressure-only signal fingerprints
must therefore remain identical, including zero and full pressure.

## Measured checks

`BendDecayReview.cpp` extends the sibling `Ring/IdleDecayProbe.cpp` with held
bends of −2, 0, and +2 semitones, the public global-wheel range. There are 72
cases: four host rates (44.1/48/96/192 kHz), three strings, three bends, and two
hand T60 targets (120 ms and 1.6 s). It seeds an idle waveguide, disables bridge
and feedback injection, and fits the added decay from the muted/open signal
ratio. Median relative T60 error is **0.100%**, maximum **1.373%**.
This matrix also completed with AddressSanitizer and UndefinedBehaviorSanitizer.

`PressureEndpointReview.cpp` checks 108 pressure-only cases at the same rates,
strings and bends, with pressure 0, 0.3 and 1. Its FNV-1a fingerprints cover all
float32 samples of each raw waveguide render. Results are in
`pressure-before.csv`, `pressure-after.csv` and `pressure-review-results.json`.
This is an unchanged-signal check, not a measurement of the 80 ms fundamental
target: whole raw-loop RMS can be dominated by DC/filter-start residue after
that fundamental dies. A fundamental-decay test must isolate the relevant
mode or observe the pickup path with its DC blocker.

## Commands

Run from the repository root while the isolated snapshots are available:

```sh
review_source=build-transitions-20260911-pass2/ring-src
clang++ -std=c++20 -O2 -DNDEBUG \
  -DELECTRY_ENERGY_ATTACK_PITCH=1 -DELECTRY_DECOUPLED_PICK_RELEASE=1 \
  -DELECTRY_MEASURED_BODY_RESPONSE=1 -DELECTRY_PERIOD_AWARE_IDLE_HAND=1 \
  -I"$review_source/Source" \
  Experiments/Transitions20260911Pass2/Review/BendDecayReview.cpp \
  "$review_source/Source/DSP/ElectryEngine.cpp" \
  -o build-transitions-20260911-pass2/ring-review-probe
build-transitions-20260911-pass2/ring-review-probe \
  > build-transitions-20260911-pass2/bend-review.csv
```

For the pressure check, substitute `PressureEndpointReview.cpp`. Compile its
baseline against `baseline-src`, omitting `ELECTRY_PERIOD_AWARE_IDLE_HAND`.
That define only controls the diagnostic's access to the candidate's new
state; it does not select a DSP branch. For sanitizers, replace `-O2 -DNDEBUG`
with `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`.

No production source or tests were edited by this review. The complete
benchmark remains responsible for live contact ownership, cancellation,
callback invariance, re-enable/reset behavior and the final combined build.
