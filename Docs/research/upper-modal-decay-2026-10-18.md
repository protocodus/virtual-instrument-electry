# E2 upper modal decay audit — 2026-10-18

The shipping string loop itself removes upper energy quickly. This is distinct
from the lower-body sensitivity to Pick Position established in the
[dry-recording comparison](dry-guitar-tone-baseline-2026-10-18.md). No source
waveform, control, filter or decay coefficient changes in this audit.

## Method and limits

The audit uses the previously frozen production bank snapshots from commit
`6c83ff453ff7ac346598e8e73367ced128a90838`: open E2 at .2 and 1.1 seconds,
fresh/light controls and the current default controls. These are two complete
control contexts, not a single-variable age comparison. Internal rate is 96 kHz.
The characteristic equation includes the actual cubic delay read, all eight
dispersion sections, damping, intrinsic correction, any material dip, loop gain
and the two-polarization coupling.

Modes are enumerated by unwrapped unit-circle loop phase and then solved as
complex coupled poles. The original 216 low poles reproduce within
`1.98e-16` in complex log-pole units. Independent phase enumeration reproduces
all 48 applicable low poles within `1.67e-16`. Maximum determinant residual
over the extended modes is `1.74e-13`. The [numeric export](upper-modal-decay-2026-10-18.json)
contains the protocol, bank values, roots, decomposition anchors and hashes.

These are fixed-bank linear modal T60s. They are not an eigenanalysis of a
whole evolving repick, and they are not measured material constants. A band
envelope combines unequal modal weights and changing polarization interference;
its slope must not be substituted for an individual modal decay.

## Result

At the 1.1-second snapshot, the following medians are unweighted across modes
whose frequencies fall in each band. Each band contains 29 or 46 modes per
polarization respectively.

| Controls | Band | Vertical median T60 | Horizontal median T60 |
| --- | --- | ---: | ---: |
| Fresh/light | 1.5–4 kHz | .836 s | 1.429 s |
| Fresh/light | 4–8 kHz | .242 s | .411 s |
| Current defaults | 1.5–4 kHz | .431 s | .756 s |
| Current defaults | 4–8 kHz | .142 s | .230 s |

The lower edge matters: the fresh/light mode near 4.03 kHz has T60 .352 s
vertical and .599 s horizontal, longer than the band median. It can dominate
the later band tail. The .2-second snapshots give nearly the same result.

By contrast, fresh/light H1–H3 have almost equal decay within each polarization:
approximately 21.4 seconds vertical and 36.2–36.3 seconds horizontal. Their
relative loss is insufficient to explain the much larger H1/H2 body differences
seen against the recordings. Source state, contact history and pickup/playing
geometry remain relevant there.

At 8 kHz in the 1.1-second snapshots, the cubic read contributes only about
.0007–.013 dB of loss per traversal across these contexts and polarizations.
The damping section removes
several dB, partly offset by the intrinsic correction. Dispersion is lossless
on the unit circle but shortens group delay at upper modes: the total traversal
is about 1100 samples at 8 kHz versus about 1165 around the fundamental. This
changes decay timing by several percent; it does not explain a twofold upper
sustain difference by itself.

## Consequence for the next experiment

The existing loss law is a stronger lead than changing the interpolation or
boosting the pickup output. This audit supplies no new damping target or fitted
coefficient. It does not validate the separate `95e5c1b` upper-loss hypothesis,
which remains disabled after its unchanged ultrasonic-floor guard failed.
Diagnosing that failure and comparing a defensible candidate with the already
frozen real-recording masks remain separate requirements. No claim of a newly
qualified note change, repick or production improvement follows from these poles.
