# Wound-string speaking length and sustain — second pass, 8 September 2026

One of this pass's five changes makes a higher fretted note on a wound string
decay faster than the same string played open. Previously its fundamental
T60 target was essentially constant across all 22 frets, apart from the
existing small neck dead-spot curve. The open-string calibration remains the
anchor. The new target multiplies it by the speaking-length fraction
`2^(-liveFret/12)` before adding the independent palm and fretting-hand loss
rates. Both transverse polarisations receive the same factor. There are no new
host parameters or filters.

## Physical scope

[Fleischer, *Vibration of an Electric Bass Guitar*, 2005, §6.1.1 and Figure 12](https://www.researchgate.net/publication/281639288_Vibration_of_an_Electric_Bass_Guitar)
derives an inverse-frequency decay law for wound strings with a constant
material loss factor. At unchanged tension, that predicts shorter fundamental
decay as the speaking length decreases. His measurements also distinguish
frequency- and position-selective support losses from that smooth material
trend. The work notes that air loss can matter for plain treble guitar
strings, while being negligible for bass strings.

Electry applies the length dependence only to its five wound strings. This is
a construction-law approximation applied to an existing composite decay
target, not an identification of the material loss factor of an eight-string
guitar. It does not turn the empirical dead-spot curve into measured neck
mobility. Pitch bends keep the same speaking-length coordinate; a slide or
hammer follows the engine's live fractional fret.

The existing high-frequency target remains anchored at its fixed reference
frequency. It is captured **before** scaling the fundamental target. Applying
the length factor to both targets would also shorten the decay of a fixed-Hz
component solely because the note's fundamental moved. The retained one-pole
shape is still an approximation to the full frequency dependence of internal
loss; this change does not introduce a new harmonic-loss filter. The
default-off fitted order-two low-string correction remains disabled.

## Reference direction check

The same three CC0 cabled_mess eight-string public previews from the
[first-pass reference report](realism-reference-2026-09-08.md) are reused.
Their source pages, license, preview hashes, decoding and limitations remain
documented there. They are development references from a previously used
source family, not new holdout data. F#1 and C2 have censored onsets. Their
gauge, pickup, force and hand setup are unknown, and source open F#1 maps to
fret 2 of Electry's Drop-E lowest string. These differences prevent an
absolute calibration claim.

The second-pass baseline is the working tree after the first five changes,
frozen under `build-realism-20260908-pass2/baseline-src`; it is not repository
HEAD. The isolated length candidate differs only in the new fundamental
target scaling. Both use the same 44.1 kHz renderer, velocity 0.95, lowest
physical string, 250 ms lead-in, 1.5 s hold and 750 ms release. The previous
analyzer is unchanged and its hash is recorded again.

| Note | Real late RMS | First-pass baseline | Length only | Real H1 slope | Baseline H1 | Length H1 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| F#1 | −10.16 dB | −2.78 dB | −2.96 dB | −7.07 dB/s | −2.13 dB/s | −2.39 dB/s |
| C2 | −7.60 dB | −2.48 dB | −3.37 dB | −3.23 dB/s | −2.12 dB/s | −3.36 dB/s |
| F#2 | −10.88 dB | −2.12 dB | −4.00 dB | −6.06 dB/s | −2.10 dB/s | −4.74 dB/s |

Late RMS is the 500–1000 ms window relative to 0–50 ms after detected onset.
H1 slopes use the previous local spectral-peak tracker and its nine overlapping
160 ms windows. All three overall-envelope descriptors move toward the
recordings; the C2 fundamental becomes slightly faster than that example.
The H5–H12/H1–H3 cooling changes by only −0.0004/+0.0060/+0.0286 dB.
The remaining excessive upper-partial sustain is therefore **not solved**.
Seven open-E1 dry audition scores remain byte-identical in this isolated
comparison.

Complete descriptors are retained for
[F#1](../Experiments/RealismReferencePass2_20260908/length-only-30.json),
[C2](../Experiments/RealismReferencePass2_20260908/length-only-36.json) and
[F#2](../Experiments/RealismReferencePass2_20260908/length-only-42.json).
The [provenance manifest](../Experiments/RealismReferencePass2_20260908/provenance.json)
records compiler/flags, frozen source hashes, renderer/analyzer hashes and raw
WAV hashes. Ignored raw audio remains in
`build-realism-20260908-pass2/reference/length-audio/`.
These isolated results attribute the envelope change to this one mechanism;
the complete second-pass audition contains the other four changes too.

## Engineering checks

[RealismDecayTests.cpp](../Tests/RealismDecayTests.cpp) independently evaluates
the realised loop-filter magnitude. Across 44.1, 48, 96 and 192 kHz it checks
fundamental decay versus fractional length on all eight strings, both
polarisations, the fixed-Hz upper anchor, plain-string bypass, passivity and
the distinction between pitch bend and fret movement. The solver probes hold
frequency fixed to isolate the length coordinate. Their string-age setting
keeps the target below the existing 26-second ceiling, whose clipping would
otherwise obscure the ratio.

A rendered high-fret slide followed by reachable pull-offs, full Palm pressure
and Dead notes checks finite bounded output, live-fret damping updates,
restoration of the open coordinate, exact reset silence and sample-identical
results with callback sizes 1 and 257. The suite passes against the combined
second-pass DSP. Against the first-pass DSP it fails exactly the 160 material
length assertions; its upper-anchor and plain-string controls still pass.
The new suite also passes AddressSanitizer and UndefinedBehaviorSanitizer.
The existing held-damping regression now compares a moving slide against its
speaking-length target while preserving its unchanged-T60 pitch-wheel check;
that focused regression passes with its original half-percent tolerance.

These checks establish the intended signal behavior. They do not substitute
for a matched eight-string recording session or a listening panel.
