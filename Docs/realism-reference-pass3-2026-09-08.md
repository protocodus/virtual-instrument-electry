# Bass-string partial decay — third pass, 8 September 2026

The two extended-range bass strings now shed some intermediate partial energy
sooner while keeping their previous fundamental and 3.6 kHz decay anchors.
The other six strings, Dead contact and the separately calibrated sympathetic
renderer retain their existing loss paths. This is one of the five changes in
[the third realism pass](realism-pass3-2026-09-08.md).

## Mechanism and scope

A one-pole loss filter has an approximately constant-plus-frequency-squared
low-frequency decay law; see [Bank, *Physics-Based Sound Synthesis of the Piano*,
Appendix A.1](https://home.mit.bme.hu/~bank/thesis/pianomod.pdf).
[Fleischer, *Vibration of an Electric Bass Guitar*, §6.1.1](https://www.researchgate.net/publication/281639288_Vibration_of_an_Electric_Bass_Guitar)
describes the inverse-frequency decay of wound-string modes under a constant
material loss factor. [Christian, *Parameter Estimation of Multiple Non-Linear
Damping Sources in Guitar Strings*](https://www.savartjournal.org/articles/14/about.html)
also reports frequency-proportional material damping in vacuum tests of
bronzewound strings, while warning that combined damping coefficients could not
be reliably separated by the fitted envelope model. These are physical
motivations, not identified constants for Electry's string set.

The new section makes a **25% blend toward linear loss curvature** at the
geometric midpoint between the two existing anchors. In the normalized
frequency coordinate `x`, the requested extra dB/s is proportional to
`x(1-x)` and the intrinsic high-minus-low decay rate. This is a bounded shape
approximation; it is not a full fitted material-loss kernel. A broad passive
biquad dip, with Q 0.5, supplies that extra loss. The remaining pole and scalar
are solved again after dividing both this dip and the existing hand filter out
of the original endpoint gains. A depth bisection preserves the loop's
0.99999 gain ceiling. The added phase participates in pitch compensation.

The fraction, Q and two-string scope are explicit conservative model choices.
They were not optimized against these three recordings or selected separately
for different notes. A full contribution on all wound strings moved the preview
metrics further, but failed existing open E1/B1 body and sustain checks. Even a
quarter contribution across all five wound strings disturbed established
six-string articulation and spectral checks. The retained scope passes those
audible body, mute contrast, velocity, slide-noise and sympathetic-coupling
checks with their original thresholds. Two low-E1 single-coil upper-octave
snapshots were refreshed; their tolerance and humbucker contrast remain fixed.
No default-off empirical loss experiment was enabled.

The idle-string path keeps its earlier calibration. When a played voice becomes
sympathetic, both polarisations clear the material filter's depth, shape and
memory so a later pick cannot revive stale filter state.

## Same reference recordings

The three CC0 cabled_mess clean eight-string public previews and their source
limitations are documented in [the first-pass reference report](realism-reference-2026-09-08.md).
They are reused development examples. Pickup, gauge, tension and playing force
are unmatched, F#1/C2 onsets are censored, and MP3 previews are not the original
96 kHz WAVs. No extra recordings enter the instrument.

The same 44.1 kHz score and analyzer compare the complete second-pass baseline
(commit `809aecc`) with the isolated new loss section:

| Note | Real late RMS | Before | After | Real upper/low cooling | Before | After |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| F#1 | −10.16 dB | −2.96 dB | −4.42 dB | −15.41 dB | −1.00 dB | −2.75 dB |
| C2 | −7.60 dB | −3.37 dB | −4.76 dB | −12.85 dB | −1.86 dB | −4.04 dB |
| F#2 | −10.88 dB | −4.00 dB | −5.27 dB | −17.66 dB | −4.65 dB | −8.27 dB |

Late RMS is 500–1000 ms relative to 0–50 ms after detected onset. Cooling is
the H5–H12/H1–H3 amplitude-ratio change between 50–210 and 850–1010 ms.
All six descriptors move toward the recordings. Mean absolute differences
fall by 22.5% for late RMS and 19.7% for cooling. These are descriptive
comparisons of three previously used, uncontrolled examples; substantial
upper-partial sustain differences remain and these percentages are not a
perceptual realism score.

Per-note measurements are retained for
[F#1](../Experiments/RealismReferencePass3_20260908/loss-only-30.json),
[C2](../Experiments/RealismReferencePass3_20260908/loss-only-36.json) and
[F#2](../Experiments/RealismReferencePass3_20260908/loss-only-42.json).
The [provenance receipt](../Experiments/RealismReferencePass3_20260908/provenance.json)
records the isolated source, analyzer and renderer hashes, production flags,
raw audio hashes, exact descriptors and engineering probe receipts. Raw audio
and source snapshots remain under the ignored
`build-realism-20260908-pass3/research-loss/` directory.

## Engineering checks

[RealismLossTests.cpp](../Tests/RealismLossTests.cpp) independently evaluates
the actual pole and biquad magnitudes, reconstructs an equivalent two-anchor
one-pole in double precision, and verifies that only the middle loses energy
faster. A separate 960-condition grid spans eight strings, five frets, both
polarisations, Sustain/Palm/Dead and 44.1/48/96/192 kHz. Compared with the
second-pass baseline, maximum fundamental/high anchor changes are only
0.0033%/0.0014%; maximum sampled DC–Nyquist gain is 0.999989986. All six
other strings and Dead are exact bypasses in that comparison.

The new suite also tests a moving bend/slide, hand-pressure changes and Dead
transition, finite bounded output, exact callback-size identity, reset silence,
and deliberately poisoned material-filter memory at sympathetic handoff.
The second-pass decay suite now observes the complete filter and preserves its
existing length, fixed-Hz and bend assertions. Both suites pass at all four
sample rates. A zero-fraction mutation fails all 80 midpoint-effect assertions
and four resumed-section assertions while retaining the independent anchor,
passivity and bypass controls.
