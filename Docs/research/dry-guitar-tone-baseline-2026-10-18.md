# Dry guitar body and upper sustain — 2026-10-18

The new dry recordings confirm that Electry can lack lower body at its current
near-bridge picking position. Much of that difference changes with the existing
Pick Position control: a fixed midpoint setting brings Bridge H1/H2 much closer
on both recordings without changing DSP. Upper sustain remains too short in
the measured trajectories. These observations do not identify a global source
correction, a new default, or a percentage of authenticity.

The [portable numeric record](dry-guitar-tone-baseline-2026-10-18.json) retains
all fixed profiles, gains, control values, offset distributions, provenance,
and a separate observability audit. All baseline and control-sensitivity
reference renders use immutable published source
`6c83ff453ff7ac346598e8e73367ced128a90838`.

## Recordings and fixed comparison

The [Longitudinal String Ageing dataset](https://zenodo.org/records/19823590)
provides day-1 new-string recordings from a Fender Stratocaster and Epiphone
Les Paul. All thirty open-low-E2 plucks from each player are retained in their
original order and native 48 kHz timing. The source-only annotations, crops,
480 tone windows and censoring flags were frozen before any model render.
Unknown pick force, stroke direction and hand release remain unknown; the
reconstructed controller score contains no invented NoteOffs.
After the first event, these are ringing repicks. “Fresh” below refers to
string age, not independent resets before every pluck.

Each complete recording or model take receives one constant whole-score RMS
gain, with common peak headroom per player. There is no note/window gain,
EQ, alignment or time warp. Native model contact latency stays present.
The 128 ms windows are centered at fixed offsets from .10 to 1.75 seconds; they do
not qualify initial attacks or changed-note gestures.

The first eleven model takes include fixed HB Bridge/Both/Neck profiles for
both players, three explicitly hypothetical single-coil profiles for the
Strat recording, and default-build contexts. Fresh/light profiles use Age 0,
Scale 0, Gauge 0, Tone 1 and Pick Position .18. All remaining resolved
parameters, exact events, seeds and production compile flags are recorded.
These are declared hypotheses, not identified source hardware. The default
context changes several controls and is not a mechanism A/B.

Both sources are conventional six-string instruments. The Strat's .038 low E
is below the model's .042 light low E; a typical 24.75-inch Les Paul scale is
below its 25.5-inch minimum, but the source guitar's exact dimensions were
not measured. Selector, pickup model, hand position and electrical settings
are unidentified. This extends reference coverage, not premium eight-string
validation.

## Lower body versus the third harmonic

Paired uncensored-window median power errors after whole-score matching,
model minus recording, in dB:

| Fresh HB Bridge, Pick Position .18 | Strat recording | Les Paul recording |
|---|---:|---:|
| H1 | −8.43 | −8.04 |
| H2 | −5.74 | −4.96 |
| H3 | +.33 | −.86 |
| 80–250 Hz | −3.94 | −4.45 |
| 250 Hz–1 kHz | +7.66 | +6.08 |

The high H3/H1 ratio primarily reflects weak H1/H2 in this comparison. Absolute
H3 power is much closer; reducing it alone does not address the balance.
All three fixed selectors retain excess 250 Hz–1 kHz at this pick setting:
the real fixed-offset median lies below every fresh-profile median at all
eight offsets for both players. This is an operational band comparison,
not identification of a mechanical body resonance.

The Strat recording also has a pronounced H1/H2 trough and subsequent
recovery. Its envelope crosses the fixed-profile range at some offsets;
an overall median must not be described as a uniform deficit at every time.
The Les Paul envelope is smoother. The cause of that difference is not
identified by these power measurements.

## One existing control changes the diagnosis

A second prospective protocol changes only Pick Position .18 → .5, held fixed
for every event. All three HB selectors and both complete scores are rendered
again; the original masks, timing, gain method and other parameters stay
unchanged. No best control is selected per note.

| Fresh HB Bridge | Strat .18 → .5 | Les Paul .18 → .5 |
|---|---:|---:|
| H1 power error, dB | −8.43 → −1.38 | −8.04 → −1.17 |
| H2 power error, dB | −5.74 → −.97 | −4.96 → −.42 |
| H3 power error, dB | +.33 → −.28 | −.86 → −1.83 |
| 80–250 Hz error, dB | −3.94 → −.96 | −4.45 → −.64 |
| 250 Hz–1 kHz error, dB | +7.66 → +4.36 | +6.08 → +2.06 |

The public control maps to nominal pluck fractions .1069 → .2525 before the
seeded stroke/contact adjustments. Its midpoint is not the string midpoint
and is not an inferred source hand location.

Both/Neck midpoint profiles slightly overshoot H1 and introduce H3 deficits:
−2.80/−3.21 dB for the Strat, −4.73/−5.12 dB for the Les Paul. Earlier
selector-matched SG evidence also worsened with the same midpoint change.
The full profiles therefore remain hypotheses; these results support neither
a global default change nor attributing the original deficit solely to DSP.

## Upper sustain and observability

The original source-only protocol reports the following median 1.5–4 kHz
band-power slopes, dB/s:

| Recording | Real | Fresh Bridge .18 | Fresh Bridge .5 | Default context |
|---|---:|---:|---:|---:|
| Strat | −13.83 | −18.61 | −17.19 | −35.11 |
| Les Paul | −12.81 | −18.43 | −17.31 | −35.12 |

These are descriptive mixture slopes, not intrinsic modal T60s. The default
context is a different age/build configuration. The unknown reference
settings and changing modal weights prevent treating every difference as a
missing damping mechanism.

A separate later audit applies the earlier frozen tone observer's unchanged
12 dB background/leakage margins and 6 dB Hann/Blackman–Harris agreement
bound to all 240 original windows per player for fourteen tracks: Reference
and the three HB selectors at both pick settings, 3360 rows in total. The
single-coil hypotheses and default contexts were not audited. It changes no
source mask, crop or slope.
All thirty Strat 1.5–4 kHz prefixes pass; three of thirty Les Paul source
prefixes fail the stronger check, while its model prefixes pass.

The 4–8 kHz results need greater caution. The original Strat source-only
protocol admitted thirty prefixes, but five source prefixes and eight
baseline-Bridge prefixes fail the later full-prefix checks. At midpoint,
nineteen model prefixes fail. Original numerical model slopes near −90 dB/s
remain recorded; they are not uniformly qualified upper-mode decay estimates.
The Les Paul source admits no 4–8 kHz prefix and stays unscored there.

A new, separate mask was frozen before any subsequent candidate reference
render. It requires each entire original prefix to pass jointly for the
recording and both fixed Bridge pick settings: thirty Strat and twenty-seven
Les Paul prefixes at 1.5–4 kHz, eight Strat prefixes at 4–8 kHz. Failed
prefixes remain in the record; none is shortened until it passes. Future
candidates cannot change this admission.

## The evolving body is not an identified beat frequency

A separate fixed-carrier complex observer confirms a real Strat H1 trough
around 1.09 seconds: its median depth is 11.05 dB below the .2–.4-second
level, with 5.61 dB late recovery and positive recovery in twenty-two of
thirty events. The Les Paul shows no corresponding H1 recovery. These are
ringing repicks with unknown quiet hand contact and preceding phases, not
independent zero-state releases.

Two-component descriptive models fit better than a single decay, but they
do not identify a nonzero polarization frequency split. A same-frequency,
two-decay model with common chirp fits Strat H1 better than a fixed doublet
in twenty-two of thirty cases at equal parameter count. It is itself
ill-conditioned, with large opposing coefficients; neither fit establishes
physical component energies or unique string modes. All boundary and
nonconvergence cases remain in the evidence.

The actual fresh/light E2 loop poles have only about .0255 Hz separation,
a roughly 39-second beat period during its common attack glide. That limits
what the current initially aligned branches can reproduce, but it does not
justify increasing detune to fit the observed trough. Signed old residues,
contact dynamics, strike orientation and magnetic observation remain open
physical leads. Removing the current tuning compensation would expose the
surrogate loss filter's phase, not recover a measured bridge admittance.

## Rejected upper-loss experiment

Prototype `95e5c1b2db1d393d9550c2c5e910d189169b0db0` tests one different
assumption: omitting the legacy fixed `.86 * .58` structural multiplier in
the measured-body upper-decay ratio. No replacement coefficient, gain, EQ,
source-force change or default is introduced. The new flag defaults off.
This is a hypothesis about inherited voicing, not proof of double counting.

ON/OFF C++17 builds pass; Fx and Refret pass. Engine OFF passes, while ON
fails twenty-four source/evaluation pins and one unchanged ultrasonic-floor
guard. ON gives −168.872 dBFS above 12 kHz, 128.862 dB below the spectral
peak; the guard requires below −180 dBFS or more than 150 dB below peak.
OFF passes at −186.413 dBFS. This very quiet guard failure does not establish
audible aliasing, but it remains a failed contract. No threshold or fixture
was changed, and no real-reference render was started after that failure.

The [source/pickup investigation](string-field-initialization-2026-10-18.md)
separately rejects a causal pickup-history candidate that repairs its attack
boost but worsens SG Both tone and sustain. None of these experiments changes
the published instrument or licenses a claim that note changes and repicks
are newly qualified. Production improvement still requires a coherent
mechanism, unchanged behavior gates, observability, fixed whole-score
comparisons and realtime validation together.

The local [whole-take comparison](http://127.0.0.1:8727/) contains exact copies
of the already matched Reference/A/B Bridge takes. A uses Pick Position .18,
B .5; both use the same published engine. The key is separate. No listener
vote is claimed under the user's standing autonomous-evaluation instruction.
The room is a control comparison and does not promote a new DSP/default.

A subsequent [fixed-bank modal audit](upper-modal-decay-2026-10-18.md)
separates the existing upper string loss from source weighting and observed
band slopes. It introduces no new damping coefficient or production change.
