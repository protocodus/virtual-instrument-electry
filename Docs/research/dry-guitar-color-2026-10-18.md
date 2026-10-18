# Current Electry color against real dry guitar

18 October 2026. Shipping 1.2.4 is `d5fc768412b0d6441f34a70c4e98658e0d1bf5b4`
(DSP identical to the preceding selected `634fd33`). Candidate 1.2.5 is
`7d29d0e7be7f2e2afb7f5b8b4bb35285671fe8bd`. The previous room's pre-fix A49
is not the baseline. This analysis describes spectral differences without
fitting EQ, changing default gains or claiming a listening preference. See the
[handoff mechanism research](refret-coloration-2026-10-18.md) and
[candidate validation](../validation/refret-coloration-2026-10-18.md).

## Sources and measurement

The four actual [Guitar-TECHS](https://zenodo.org/records/14963133) P3 phrases
provide 14 annotated attacks, including a same-string ascending development
phrase, its later repetition and two descending phrases. Seven additional
isolated controls comprise three [EG-IPT](https://zenodo.org/records/15205644)
SG pickup positions and four Guitar-TECHS P1 notes. Source members, hashes,
licenses, channel selections and timing remain in the existing portable
[reference receipt](refret-reference-selection-2026-10-18.json) and local
source plans. Source audio remains external to Git.

Each complete crop receives one constant RMS gain. Raw levels are retained.
There is no per-note gain, alignment, EQ, denoising or time warp. Phrase windows
remain −5..10, 0..30 and 20..80 ms around all 14 fixed source annotations.
Source12's approximately +4.45 ms transfer-onset residual and source06's
−0.478 ms repeat residual remain. They limit attack attribution.

Spectra use DC-centered Hann-windowed power. Zero padding interpolates bins;
it does not improve physical resolution. Short windows report band fractions,
centroid and rolloff, not resolved low-note harmonics. Longer windows begin
80 or 200 ms after onset and finish at the earliest matching NoteOff, next
NoteOn or crop end, minus 30 ms. Requiring at least 40 ms and six periods
leaves eight early and two late windows, selected without candidate results.
H1 is independently located within ±3% of nominal; H2–H16 energies use local
peak neighborhoods and Hann-mainlobe-width integration, relative to H1.
Residual energy outside these first 16 partial regions includes higher
partials, leakage, modulation and string/body sound; it is not an electronic
noise-floor measurement.

## Persistent differences and counterexamples

The following are held 80–200 ms observations; source06 B2's allowed window
ends at approximately 134 ms. Values are real Reference / shipping Electry.
H2 and H3 are dB relative to each recording's own H1.

| Source and control | Power centroid, Hz | H2 / H3, dB, Reference → Electry |
| --- | ---: | ---: |
| P3 source06 fresh B2 | 157 / 405 | −5.5 / −17.2 → +7.4 / +8.8 |
| P3 source06 same-pitch C#3 repick | 145 / 405 | −12.6 / −28.8 → +7.0 / +7.8 |
| P3 source12 fresh G3 | 415 / 622 | −7.1 / −12.6 → +6.6 / +7.7 |
| P3 source12 fresh G5 | 942 / 1347 | −17.9 / −19.2 → +2.3 / −7.6 |
| SG E2 Bridge, matched selector | 370 / 364 | +5.2 / +7.2 → +7.4 / +10.1 |
| SG E2 Both, matched selector | 239 / 324 | +3.7 / +3.9 → +6.7 / +5.5 |
| SG E2 Neck, matched selector | 220 / 272 | +3.7 / +2.7 → +6.9 / +4.9 |
| TECHS P1 F2 Bridge | 267 / 366 | +2.8 / +5.9 → +7.5 / +10.0 |

Across all 14 P3 20–80 ms windows, shipping's centroid exceeds the reference
by approximately 1.11–3.34×. Yet this is not simply excess treble: the model
also loses upper-band energy quickly. For example, fresh G3's 4–8 kHz held
power is approximately −106.5 dBFS versus −51.4 dBFS in the recording. A large
ratio involving such low model energy needs absolute-level context. Fresh
and same-pitch controls show that this general imbalance cannot be attributed
solely to a fret-change crossfade. Conversely, SG Bridge's close centroid and
positive real H2/H3 are a counterexample to universally boosting H1.

Hardware and performance prevent treating these numbers as calibration
errors. P3 uses a Sire T7 six-string with .010–.046 strings and a documented
bridge/neck pickup configuration; Electry uses its Bridge selector, default
heavy eight-string build 0.8, pickup-type control 0.32 and tone 0.7. The source
configuration does not establish an electrically equivalent pickup transfer.
The isolated SG selectors are explicitly matched Bridge/Both/Neck, with
model humbucker type and tone 1, but source scale, gauge and force are unknown.
P1 uses .011–.050 flatwounds; model mass references are roundwound. Annotated
velocity is not measured pick force, and source NoteOff does not isolate
intrinsic string loss from a player's stopping hand. Later source12 high-E
notes have velocities 40/33 after 127; whole-crop gain preserves that contrast.

SG matched envelopes agree within approximately 0.3 dB through 0.8 seconds,
while the P1 D4 pairing diverges much more during decay. Consequently neither
a universal damping increase nor a pickup-independent harmonic gain follows
from this corpus. Existing pickup/pick-position/tone counterfactuals and
separate excitation-versus-decay measurements are better next diagnostics.
Recording residue alone does not justify injecting model noise.

## Selected lower-slope candidate against shipping

The selected candidate changes only the handoff fade curve, retaining the
legacy peak weight slope of 1.5 and unchanged Contact duration. Its C2
shoulders reduce ideal integrated frozen comb deficit by approximately 7.61%;
that is a mechanism result, not a percentage of measured realism. The earlier
quintic has a larger theoretical reduction but increases peak slope. Broader
rapid-repick stress checks favored the conservative slope bound; the
[validation](../validation/refret-coloration-2026-10-18.md) records that choice.

All registered fresh-note raw PCM windows remain identical. Across all 14
primary −5..10 ms windows, B/A centroid is 0.998035–1.004175, matched maximum
sample step is 0.991762–1.000090, causal 8 kHz high-pass RMS changes −0.010243
to +0.004150 dB, and raw RMS is 0.999696–1.004345. Thus no blanket attenuation
explains the small result. Wider-window HF changes reach +0.030341 dB at
0–30 ms and +0.085433 dB at 20–80 ms; these maxima exceed the earlier quintic's
+0.027831/+0.063980 dB. The slope constraint does not uniformly lower every
finite-window audio metric.

The development C3's 20–80 ms band fractions change +0.1667 dB at 1.5–4 kHz,
−0.0777 dB at 4–8 kHz and +0.1336 dB at 8–16 kHz. Its later repeated C3
changes +0.1162, +0.0173 and −0.0046 dB respectively. The direction depends on
state; this is neither uniform brightening nor a fitted movement toward the
real recording. The strongest development C3 is too short for a guarded held
window, so its short-window result cannot establish steady-state improvement.

In the ten admissible held windows, raw RMS ratios stay within
0.999999968–1.000000556, centroid ratios within 0.999999720–1.000000329 and
H2–H16 relative differences within −0.002325..+0.002950 dB. Higher-band ratios
can vary more where power is near the numerical floor. The persistent
reference/model color gap remains. Four controlled performance pairs are
byte-identical: 8/12 Hz same-note repeats, ordinary alternating large jumps
and rapid Palm changes. Returning/overlapping small-fret probes change as
intended; their whole-render raw RMS rises approximately 0.065% and 0.177%
respectively, rather than falling.

## Evidence bindings

Ignored local evidence is under `build-coloration-review/evidence/`:

| Receipt | SHA-256 |
| --- | --- |
| `current-shipping-color-summary.json` | `846e6b5495c422b84b6285c997887defab3530dd41b713618e759828adc4f181` |
| `current-vs-real-phrase-color.json` | `8fe80e796ebf243d077aa9ee6b8d853d72272a797065a5e543fa4818eb188908` |
| `current-vs-real-isolated-color.json` | `34e94cbadf43dcba51495453a037adf828e5ac54924d5934e6e716f0b88ea702` |
| `final-phrase-attack-analysis.json` | `2b7183e357af19d4a2d79b4f94d1f91d7e9d4f4d5bdaa35d9b259480f5cd837b` |
| `final-color-comparison.json` | `811d58d3d30cd8a2c57a1bbedd73b4e388246813f6a426e6dfc7c49a6252cc0f` |
| `final-slope-prototype-binding.json` | `d81bdf7c1ca3296f2e658da6bf2d1324c613da0511df652bd311f626093fd766` |

The independent color observer SHA is
`3420eccdc73d7daeb94fb20d606ac2da86474f16b998ce159ca7da4a7cc85968`.
It binds the immutable A/B manifests, raw WAV hashes, exact frames and shared
feature observer. The final room at `build-coloration-review/final-room` contains four phrase
triples, six performance pairs and seven real/shipping isolated pairs. All
1,008 attack measurements, 52 color windows and full raw/listening phrase WAV
hashes agree with the immutable `6b08b9a` slope prototype; only the selected
revision/path bindings change. The seven isolated shipping
renders are also PCM-identical to the older fresh-path renders, consistent
with a handoff-only change. These development recordings cover a limited
set of guitars, registers and performances, not universal eight-string parity.
