# Eight-string realism techniques — 17 October 2026

The immediate priorities are the accuracy of the existing stiff-string solver
and an independently sourced wound-string mass experiment. Both affect the
physical low-string model and can be investigated without guessing new player
variation. The other techniques below remain useful, but the existing
[evaluation history](evaluation.md) already documents rejected contact,
pickup and Palm candidates. Repeating those candidates without new evidence
would not be a new realism result.

This note distinguishes mathematical accuracy, a named reference construction,
and resemblance to a recorded guitar. A simulated string can establish the
first; manufacturer data can constrain the second; neither establishes the
third or a competitive listening claim. Candidate switches remain explicit,
and no finding here authorizes replacing the shipping sound without the
applicable engineering and listening gates.

## Ranked experiments

| Rank | Technique and existing coverage | Missing behavior or accuracy | Finite next experiment and expected consequence | Main risk and validation |
| --- | --- | --- | --- | --- |
| 1 | **Solve the existing dispersive loop against its actual modal frequencies.** Electry already derives stiffness `B`, fits an eight-allpass cascade and compensates fundamental phase. | Fitting phase-delay deficits at selected harmonics is not the same as solving every closed-loop modal root. The independent audit finds much larger high-partial errors on short, heavy fretted strings than on opens. | Use `Tools/AuditDispersion.py` as an independent numerical oracle; compare one default-off fitter against the same stiff-string law over every string, fret, Build anchor and host rate. Better partial placement should affect upper-fret metallic ringing, harmonic attacks and chord beating. | A smaller optimizer residual need not reduce audible modal error. Require actual root errors, nominal fundamental accuracy, held bend/slide continuity, CPU and finite rendering. The mathematical reference does not identify an actual string's core. |
| 2 | **Use published linear mass for one named wound-string construction.** The current model estimates wound mass from 85% of a solid steel cylinder's external area. | Current manufacturer data differs by up to 9.60% at the `.080` reference. That changes impedance, tension and stiffness together. | A default-off `ELECTRY_REFERENCE_STRING_MASS` candidate replaces only the five wound-string masses, retaining the existing empirical core coordinates. Compare the light reference separately from the synthetic heavier-gauge hypothesis. Expect construction-dependent release/contact response and partial stretch, with a potentially larger change in the unclamped attack-tension seed. | Unit mass is not core geometry, a decay measurement or an eight-string playing capture. Keep one common mass law in the physical solve and reference impedance. Score seed saturation, tuning, partials, attacks, releases and CPU before a level-matched audition. |
| 3 | **A passive local plectrum junction that reacts to incoming motion.** Shipping already preserves the ringing string, performs a finite contact stage and shapes the release by physical string impedance. | Its retained contact loss is a placeholder, rather than a measured moving pick exchanging force with the local incoming waves. | First exercise a small passive two-rail oracle over phase, velocity, compliance and release timing. Then use the existing phase-conditioned capture contract to fit one candidate. Expected gain is the second and later stroke's body, brightness and phase dependence in rapid chugs. | Both passive-spring and moving-unilateral repick candidates already regressed source comparisons. A new implementation needs new phase-resolved evidence; passivity alone is insufficient. Preserve pre-contact audio, ownership, tuning and isolated-note behavior. |
| 4 | **Registered two-axis pickup transduction and measured loading.** Electry already has two transverse polarizations, spatial aperture/coil geometry, differentiation and loaded electrical response. | The two axes are combined through voiced sensitivity before the nonlinear map, rather than measured as a two-dimensional displacement-to-flux surface at known pickup gaps. | Fit one target pickup's registered two-axis map, de-embedding its electrical load, and compare held-out trajectories before runtime integration. Expected effects include polarization-sensitive envelope motion and attack asymmetry. | The prior measured one-dimensional flux substitution regressed the paired source score. A published six-string pickup curve cannot calibrate the `.080` string or an unknown displacement scale. Validate both polarizations, several heights, full-band attacks, selector changes and the same high-gain chain. |
| 5 | **Shared passive bridge/neck mobility.** Shipping already has coupled polarizations, sympathetic strings and measured paired body-mode frequencies/damping; its direct body contribution is deliberately quiet. | A common scalar string bus and per-string loss laws do not identify one reciprocal, frequency-dependent structural state with string-, axis- and fret-specific ports. | Build a synthetic positive-semidefinite modal-matrix oracle before using any measured target. Next fit a small measured neck/bridge mobility model. Expected effects are coherent string interaction, double decay and position-specific dead spots. | Current body poles have no signed absolute residues and deliberately supply no termination loss. Do not convert them into a mobility curve or add arbitrary resonators. Test reciprocity, no-source energy, decays, coupling versus voice count, state continuity and CPU. |
| 6 | **Amplitude-dependent winding friction and conservative string nonuniformity.** Shipping age already controls decay, and the model has uniform stiff-string dispersion and procedural winding contact texture. | A fixed exponential loss cannot represent every amplitude-dependent decay law; one uniform `B` cannot represent a spatially localized mass defect. | Keep these separate. First fit decay curvature on mechanically repeated notes; separately test modal-frequency residuals against one static spatial mass perturbation. Expected cues are a changing decay slope or fret-dependent upper-partial warping, respectively. | Existing research protocols already identify both as capture-gated. Diameter wear does not measure mass per length; a decline in level can be pickup geometry or noise floor. Reject an age LFO, random detuning or extra filters as substitutes. |
| 7 | **Setup-conditioned fretted intonation.** Electry already follows performed fret, tension gestures and stiffness, while commanding nominal equal-tempered fundamentals. | Real action, fretting extension and nut/saddle compensation can leave deterministic, string-specific residual intonation. | Use a low-dimensional physical setup model to predict withheld frets, then freeze a small per-setup cents table only if the residual repeats. Expected effect is more accurate inter-string beating in fretted chords. | No generic cents table transfers from a classical guitar or sonometer to the heavy Drop-E anchor. Require actual setup dimensions, mass, stiffness, extension, controlled fretting force, held-out frets and reinstall sessions. Avoid random detune or interpolation across unmeasured Build anchors. |

The first two rows have finite numerical/engineering work in this pass. The
remaining rows are ranked follow-ons, with a deliberately small next step;
they are not claims that those mechanisms are absent in every form or that
their production coefficients have been identified.

## Independent dispersion finding

The physical reference is the fundamental-normalized uniform stiff string:

```text
f_n = n f_1 sqrt((1 + B n^2) / (1 + B)).
```

An independent audit of the actual closed-loop phase roots found a roughly
209-cent worst upper-partial error at harmonic 16 on a short, heavy, high-fret
case. At the default low-E construction, harmonic 16 was about +0.69 cents on
the open string and -27.83 cents at fret 12. These are numerical model errors
against the declared stiff-string law, not measurements of an electric guitar
or a listener's assessment. The full sweep and final candidate results belong
to the generated audit reports; this note does not convert the initial maximum
into a final improvement claim.

The first default-off group-delay/minimax prototype reduced the worst case to
about 110 cents while worsening the median from roughly 2.31 to 3.75 cents;
many individual modes also worsened. It is therefore not recommended or
promoted. A revised coefficient search is still an experiment, and its final
error distribution, continuity and CPU results must be reported before any
promotion decision.

[Kemp's stiff-string analysis](https://research-repository.st-andrews.ac.uk/bitstream/handle/10023/19673/Kemp_2020_SNAS_Inharmonicity_CC.pdf?sequence=1)
also distinguishes external construction, mass and flexural stiffness. Better
numerics must not be presented as having measured the empirical wound core.
If a fitter changes its two coefficients during a ringing slide or bend, its
history/phase handoff remains part of the correctness problem.

## Current manufacturer mass provenance

The source is the live [D'Addario String Tension Pro calculator](https://www.daddario.com/pages/string-tension-pro-string-tension-calculator),
retrieved on 17 October 2026. Its HTML embeds
[recommendation.min.js](https://embed.cartfulsolutions.com/daddario-string-tension-finder/recommendation.min.js),
which fetches
[strings_sample.json](https://embed.cartfulsolutions.com/daddario-string-tension-finder/data/strings_sample.json)
and [strings_dataset.json](https://embed.cartfulsolutions.com/daddario-string-tension-finder/data/strings_dataset.json).
The script passes `Mass_UL` directly as `UW` to the imperial tension function:

```text
T_lbf = UW (2 L_inches f_Hz)^2 / 386.088.
```

Imperial results are labeled pounds and metric results multiply by `0.453592`.
This establishes `Mass_UL` as lb/in. The independent SI conversion uses the
exact definitions `1 lb = 0.45359237 kg` and `1 in = 0.0254 m`, then evaluates
`T = mu (2 L f)^2` and `Z = mu (2 L f)` in SI. It does not need an unknown
scale length from a rounded product tension table.

The frozen selected records are `U1AGFNP024`, `U1AGFNP032`, `U1AGFNP042`,
`U1AGFNP060` and `U1AGFNP080`. Their matching fields are XL nickel-plated
steel, round wound, steel hex core, small ball end and `PackagedSingle =
NW024/032/042/060/080`. Duplicate packaged variants can differ in the final
decimal places; the named records remove that ambiguity.

| Reference | `Mass_UL`, lb/in | Converted mu, kg/m | Existing proxy excess |
| --- | ---: | ---: | ---: |
| NW024 | .0001082688980358360 | .0019334624432032757 | +0.7239% |
| NW032 | .0001902095534263840 | .0033967559895793364 | +1.9252% |
| NW042 | .0003140532008355160 | .005608351798152271 | +6.3431% |
| NW060 | .0006414349593603170 | .011454724543980310 | +6.2585% |
| NW080 | .0011055562555720800 | .019742987485561633 | +9.6006% |

The existing proxy is `0.85 * 7850 * pi * d^2 / 4`, with external diameter
`d` in metres. For a `.080` string at 28 inches tuned to E1 it gives
74.3253 N, versus 67.8146 N with the selected reference. Holding all other
construction inputs fixed, reference impedance is 8.760% lower and `B` is
9.601% higher. Because the existing seed uses a force-derived displacement and
axial elastic scale, its unclamped tension increment scales as `mu^-3` under
this mass-only intervention: about +31.655% for this string. That is a model
consequence, not a measurement of a guitarist's force or pitch glide.

The source also contains a separate `NW090` entry, but no `.098` anchor was
identified. The candidate's common `mu proportional to gaugeScale^2` morph is
therefore an explicit geometric-similarity hypothesis away from the light
set. Its five named XL strings plus the existing plain-string model form a
virtual reference construction; it is not advertised as the manufacturer's
NYXL0980 set, whose gauges and material differ. The JSON identifies no measured
manufacturing tolerance, core diameter, winding slip or flexural modulus.

The older indexed D'Addario tension-chart PDF contains different unit masses
and now redirects to the calculator. This pass uses the directly retrieved
live JSON, not search snippets or a mixture of old and new tables. The receipt
hashes freeze the actual retrieved inputs rather than promising an immutable
upstream URL:

```text
strings_sample.json, 575306 bytes, 1926 records
855d638563bb07bcb156bb359a75ff3fdaf83a15d27cfa840b7950c675c10ee3

strings_dataset.json, 885620 bytes, 2128 records
b49684a6911516ae947a4728616c6f383c6f176dd553c516cd11bf00b6cdf0b7

recommendation.min.js, 1510902 bytes
3df58f74c9c2f3609f271050baeab3a6d4529431133a5d42f67aa20a60f17442
```

## Mass candidate engineering check

`Tests/ReferenceStringMassTests.cpp` observes the real engine through its
existing friend seam and compares tension and impedance to independent
double-precision SI fixtures. It never calls the production mass helper. The
240 cells cross all eight strings, open/octave frets, host rates 44.1/48/96/192/
384 kHz and gauge coordinates 0/.5/1. The three plain strings retain their
existing solid-steel expectations. Only gauge zero has the identified wound
reference; the other two are labeled hypothesis checks.

Separate short real-engine renders cover 240 picked/stopped fret-5 notes,
velocities .35 and 1, and fifteen matched free/pressed low-E pairs. They check
finite bounded output, nominal targets, the existing six-cent attack bound,
fretting-finger stop timing and registered bridge-hand stop timing. Seed/cap
occupancy is reported as an observation; the suite does not invent an expected
guitar distribution or change the cap to accommodate the candidate.

Direct native C++17 OFF and ON runs both passed, taking approximately 0.70 s
each. Across the physical fixtures, worst relative tension/impedance errors
were `4.44e-7 / 3.54e-7` OFF and `3.56e-7 / 2.72e-7` ON. Across the 240 contact
cells, the unclamped seed reached the existing cap in `155` OFF versus `160`
ON cells; maximum raw seed changed from `0.09802341726` to `0.1290532758`.
Maximum observed committed `q` remained `0.006954599638`, inside the existing
six-cent bound. The aggregate maximum audio peak was `0.08154702187` in both
runs; this aggregate is dominated by a control case and is not a null claim
for wound-string waveforms. Free fret-5 stop closure stayed at 10 ms; registered
pressed closures lay between 21.987 and 21.991 ms.

Those short checks do not establish decay fitting, audible preference,
cross-platform equivalence, complete plugin validation or production
promotion. The full unit/audio suites, reproducible single-intervention WAVs,
whole-file RMS matching and the user's by-ear decision remain separate work.

## Why the other techniques remain bounded

[Evangelista and Smith's passive plucking formulation](https://dafx10.iem.at/proceedings/papers/EvangelistaSmith_DAFx10_P21.pdf)
provides reciprocal two-port scattering and a spring/mass/damping interaction
with threshold release. That is a useful numerical oracle and structural
constraint. It does not identify the tip trajectory, compliance or release
threshold of Electry's target eight-string performance, and cannot reverse the
existing failed repick comparisons on its own.

[Novak et al.'s pickup experiment](https://www.dafx.de/paper-archive/2018/papers/DAFx2018_paper_39.pdf)
supports a static displacement-to-flux law followed by differentiation and
shows dependence on pickup gap. Its shaker experiment covers perpendicular
motion at 60–400 Hz on three pickups. Electry still needs a registered target
displacement scale, two-axis trajectories and circuit loading; importing a
curve into normalized internal units is not enough.

[Bank and Karjalainen](https://www.dafx.de/paper-archive/2010/DAFx10/BankKarjalainen_DAFx10_P60.pdf)
construct passive admittance matrices from positive-real scalar sections and
positive-semidefinite modal weights, then convert them to a robust reflectance
matrix for waveguides. This motivates a shared structural state and gives an
independent passivity test. The measured neck-mobility work linked by the
[McGill/Yamaha project](https://www.cirmmt.org/en/research/projects/yamaha-rnd_guitar-analysis?set_language=en)
identifies a relevant measurement route; it does not provide Electry's missing
eight-string residues.

The separate [known-gaps research](known-gaps-experiments.md) already records
capture-gated winding friction, worn-string spatial mass, pickup back-action,
intonation and support-friction memory. For setup intonation,
[Anderson and Beausoleil](https://doi.org/10.1121/10.0026483) explicitly separate
resonant length, density, tension and stiffness. These are identification
problems with useful predicted signatures, rather than reasons to add a
generic imperfection knob.

Full nonlinear finite-difference strings remain a credible offline reference
or larger architecture experiment.
[Bilbao et al.](https://www.dafx.de/paper-archive/2024/papers/DAFx24_paper_50.pdf)
demonstrate energy-based string/finger/fretboard interaction in realtime; that
does not establish the workload for eight Drop-E strings, Double and Electry's
complete amp path. A full solver should earn its additional runtime cost and
state complexity against the bounded numerical corrections first.
