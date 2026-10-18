# Tone, controls and reference identifiability — 2026-10-18

The published model's large Sire fundamental deficit is real, but a substantial
part can already be produced or removed by its existing pickup and picking
position controls. A proposed source filter that improves those particular
ratios was rejected because it damages selector-matched SG tone, Palm balance
and eight existing behavior constraints. No production defaults, DSP, test
thresholds or frozen comparison rooms changed in this investigation.

The [portable measurements](tone-identifiability-2026-10-18.json) retain the
full control definitions, physical geometry, all control-envelope rows, 24 body
windows at three ages, all six admitted upper-band prefixes and candidate
failures. Baseline is `6c83ff453ff7ac346598e8e73367ced128a90838`, whose DSP and
all 11 default-age raw WAV hashes equal the previous final
`46f4a49127c53e148ef83f88000e1495f8e2a068`. Original masks and shared crops
remain fixed; no EQ, per-window gain, retiming or source substitution was used.
H1 share below means H1/(H1+H2+H3) power, not an identified body resonance or an
authenticity percentage.

## Pickup and picking position

P3 comparisons use the heavy extended instrument at Guitar Build .8, scale
.85, gauge 1, pickup type .32, tone .7 and the default Pick Position .18.
The complete resolved engine parameters are in the JSON. Reference Sire T7
pickup selector, hand position, force and string age are unidentified. The
model's explicitly selected Bridge is a declared comparison hypothesis.

Pick Position is independent of Guitar Build. Its numeric midpoint .5 was
included in a public-control envelope and then held fixed across all four P3
phrases, alongside the original and Neck-only configurations. This is a
sensitivity experiment, not an inferred source setup or proposed default.

| Held window | Real | Published Bridge | Neck only | Neck + midpoint pick |
|---|---:|---:|---:|---:|
| P3 B2 fresh, 80 ms | .7702 | .0974 | .3755 | .7161 |
| P3 C#3 repick, development | .9463 | .1107 | .4733 | .8775 |
| P3 C#3 repick, repeat | .9705 | .1118 | .4747 | .8781 |
| P3 G3 fresh, 80 ms | .7988 | .1108 | .2960 | .5714 |
| P3 G5 fresh, 80 ms | .9725 | .3808 | .9124 | .9855 |
| P3 F5 change, 80 ms | .6331 | .3251 | .8950 | .9848 |

Across all ten fixed P3 windows, Neck plus midpoint reduces mean absolute H1
error .61175 → .15435, H2/H1 error 15.085 → 7.021 dB and H3/H1 error
21.242 → 8.250 dB. H1 and H2 improve in nine windows, H3 in ten. F5 overshoots;
these controls do not establish a general source correction.

Known-selector SG recordings provide counterevidence. Retaining their matching
Bridge/Both/Neck selectors, moving only the pick knob .18 → .5 increases H1
mean absolute error .03126 → .09879 and H3/H1 error .5113 → 6.8860 dB, worse in
all six windows. H2 improves in five, demonstrating why H2 alone is an
insufficient target. For SG Bridge, H1 .0846 → .2088 against .1055 real.

The P3 model's scale is .701675 m. Bridge and Neck pickup centers are 38.20 and
157.56 mm from the bridge, coil spacing 12.92 mm and per-coil aperture 4.8 mm.
The nominal picking point moves 75.009 → 177.173 mm. Capturing actual contact
coordinates inside `startExcitation` gives B2 71.854 → 174.018 mm and the final
C#3 repick 73.044 → 175.208 mm, with identical seeded stroke offsets. Eight
instrumented renders were sample-identical to uninstrumented outputs. A MIDI
dispatch snapshot can precede the scheduled contact and is explicitly excluded
from physical geometry evidence.

Conventional/light scale and gauge alone give B2/C#3 shares .0948/.1090,
compared with .0974/.1107 published. Bypassing incidental artifacts, contact
noise, body and sympathetic paths gives .0997/.1119. These tested changes do
not explain the Sire deficit. All initial envelope rows are preserved rather
than retaining only favorable settings.

A separate weighted least-squares fit of twelve harmonic sine/cosine pairs
confirms the spectral observation: B2 H1 .77647 real/.09736 model, C#3
.94633/.11073. Weighted residual powers are below .7%. Only diagnostic line
frequencies were fitted; no reference or model audio was altered.

## String age is a separate confound

Only public String Age 0/.30/1 changes in the next probe. Every original body
window and previously admitted upper-band prefix remains present. The default
.30 raw WAV hash matches frozen published audio in all eleven cases.

| Fixed upper-band trajectory | Real dB/s | Age 0 | Age .30 | Age 1 |
|---|---:|---:|---:|---:|
| SG Bridge E2, 1.5–4 kHz | −25.00 | −20.34 | −44.53 | −76.01† |
| SG Both E2, 1.5–4 kHz | −24.40 | −20.24 | −43.89 | −78.51† |
| SG Neck E2, 1.5–4 kHz | −18.77 | −21.28 | −44.30 | −69.58† |
| P3 G3, 1.5–4 kHz | −17.97 | −29.05 | −54.87 | −71.73 |
| P3 G5, 4–8 kHz | −68.81 | −56.55 | −83.00 | −180.03† |
| P1 F2, 4–8 kHz | −21.15 | −111.22 | −111.79 | −40.32† |

† Age 1 fails one or more unchanged observability guards in the frozen prefix:
35 failed frames across the six admitted slopes, 51 across every originally
retained prefix including those without a slope admission. Values are reported
without remasking. The apparent P1 improvement is not a valid identified decay.
A band slope combines changing modal weights and is not an intrinsic modal T60.

Age 0 passes all guards and brings the six slope values numerically closer,
although P1 F2 barely changes and remains far from its reference. It also
reduces fundamental share slightly: P3 H1 mean absolute error .61175 → .63331
and SG .03126 → .03407. Palm H1 improves .05225 → .02450. Age 1 can make P3
body look closer while greatly damaging SG and Palm spectral balance. None of
these unknown-age references licenses selecting a new default or fitting the
age law.

The existing age control changes both excitation and subsequent loop loss:
initial amplitude, excitation/release bandwidth, low-frequency T60 and the
high/low decay ratio. Current upper-band differences therefore cannot all be
assigned to a missing source mechanism. The newly verified controlled
[Longitudinal recordings](premium-reference-coverage-2026-10-18.md) provide a
more suitable next test: day-1 new strings and repeated later sessions on the
same instrument, with selector/settings held fixed. Source-only segmentation
and admission must precede any comparison; this report does not claim that
those recordings have already validated the model's age law.

## Rejected equal-mean Butterworth source

Immutable prototype `035bf008628107931f8ab07ad79725a942c0ae20` changes only the
release filter's damping √3 → √2, retaining
`g = damping * (1-a)/(4a)`, unit DC area, equal DC mean latency, actual-pole tail
budget and every currently eligible style. The earlier `1b8a50b` experiment
used a fixed natural corner, so this is a different normalization. No makeup
gain, EQ or fitted constant was added.

Independent bilinear impulse calculations give maximum area and relative mean
errors below 1.72e−9. A temporary correct √2 reference preserves all original
numeric bounds and passes 1,668 assertions. The tracked Bessel-specific oracle
was not modified or used as a valid Butterworth test.

Engine qualification fails 18 checks: ten whole-source snapshot pins and
eight unchanged behavior constraints. The latter are the direct body/total
ratio, upstroke brightness, E1 low-band retention, three E1/B1 peak-to-sustain
limits, E2 Palm pressure-tail contrast and E1 Palm high-band loss. Fx passes.
No snapshot refresh or threshold change was attempted.

The tone evidence independently rejects it. SG H3/H1 error increases .511 →
2.534 dB, worse in all six held windows. Both Palm windows worsen in H1, H2 and
H3. P3 G3/G5 and P1 F2 upper slopes worsen; the last fixed P1 frame also fails
observability. Whole-crop raw RMS falls to .698–.882 of baseline even though
DC area and mean are equal. Improving H2 by weakening the upper source does
not establish an overall improvement.

## Remaining physical lead

For an ideal triangular released string with a point magnetic observation,
voltage modal amplitude is proportional to
`sin(nπp) * sin(nπx/L) / n`: both hand and pickup position strongly affect the
first three partials. The published SG Bridge nominal geometry gives H1
.09743, H2/H1 +5.331 dB and H3/H1 +7.672 dB before finite release, aperture,
nonlinearity and loaded-coil losses; real SG Bridge is .1055/+5.164/+7.157.
A universally darkened source is not supported by those data.

The prior default-off analytic triangle's bright result does not reject exact
triangle physics. A parallel investigation is checking whether its delay-line
state was installed without matching histories in the loop filters. That is
an initialization hypothesis requiring its own frozen numerical proof,
production-path prototype and behavior qualification. It is not a shipped
correction or a claim of a 95% match.
