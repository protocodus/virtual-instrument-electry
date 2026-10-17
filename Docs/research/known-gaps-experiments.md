# Electry known-gaps experiments

Experiment logs and evidence gates behind the README's [Known gaps](../../README.md#known-gaps).

## Mute/Dead body checkpoint

The previous Palm-only finite heel has been retired. User feedback identified its
rendered chugs as squishy and short of body; a same-build waveform, spectrum and
envelope A/B confirmed that the extra 100 ms contact stacked a second transient
loss on top of the already selective steady hand. Removing that layer and moving
Palm's existing modal-excitation factor from 0.74 to 0.85 restores the periodic
low body without adding gain compensation, a parameter or another resonator.
At that Palm-only correction, Open and Dead evaluator WAVs were byte-identical.

At that Palm-body correction, the unnormalised dry evaluator measured:

| default probe | retired finite heel | body correction | change |
| --- | ---: | ---: | ---: |
| E1 30-80 / 0-30 ms body | -5.4608 dB | -4.1961 dB | +1.2647 dB |
| E2 30-80 / 0-30 ms body | -4.8192 dB | -3.2264 dB | +1.5928 dB |
| E1 raw peak | -28.03 dBFS | -26.29 dBFS | +1.74 dB |
| E2 raw peak | -26.70 dBFS | -25.11 dBFS | +1.59 dB |

At that checkpoint, the noise-free engine guard independently read
-4.060/-3.150 dB of E1/E2 body. It also kept the intended selective loss: over
150-500 ms the Palm-minus-Open low/high changes were -10.327/-19.832 dB on E1
and -8.347/-22.812 dB on E2. The early Palm high/low balance remained
4.499/0.385 dB darker than Open. Light, default and hard body remained strictly
ordered, so Mute Tightness was one understandable loose-to-tight performance
axis rather than three samples.

There is one explicit tradeoff. The tracked Guitar-TECHS F2 development proxy
moves from the retired contact's -4.009 dB contraction to -0.644 dB at 44.1 kHz
(-0.644..-0.658 dB across 44.1-192 kHz), farther from the two public player cells
at -6.099/-15.290 dB. Those two conventional-guitar cells establish the
direction of selective loss, not a defensible coefficient. The old local fit was
therefore demoted from a hard target after it conflicted with low-body evidence
and the rendered result; the commissioned E1/E2 TRAIN/HOLDOUT comparison remains
the actual fit gate.

At that checkpoint, the common high-gain rapid-Mute fixture had 8.8260% of its
30-80 ms power above 500 Hz and 0.882806 harmonicity, passing its intentionally
loose body and nonperiodicity rails. The shared-hand transition remained
smooth: Palm -> Open moved the old E1 by +9.8741 dB above 500 Hz and +0.279835
percentage points; Open -> Palm moved it by -7.2850 dB and -0.154347 points.
Adjacent CC2 attack moves were 0.0409/0.0548 dB on E1/E2. The stateful Dead
medians shifted only to
-8.178/-15.086/-23.257 dB with a 4.85398 dB contextual RMSE because the earlier
Palm ring and its live damping phase are different; no Dead coefficient changed.

All demonstrations affected by that correction were regenerated from the same
code. Its checkpoint SHA-256 hashes were
`5988919af56d2ba0f669011709e3468254db8664c9dd783d434918809c5296ed`
for demo 04 and
`20c612d85fd0b81ef4ae24885a7314afbdd47af92586b80c03aa2380ec3065e5`
for demo 16. These demos remain demonstrations, not validation recordings.

## Exact-eight fret-decay direction check

The strongest new low-cost realism lead is measured, per-partial loop loss.
[Vodka, Shapovalova, Ovcharenko and Avdieieva (2026)](https://doi.org/10.3390/vibration9030046)
estimate frequency- and fret-dependent damping from three electric basses and
show why a single two-point loss tilt cannot represent every string and fret.
Their E strings trend toward less damping higher up the fretboard, their G and
D strings trend toward more, and their A-string direction depends on the
instrument. The paper is CC BY 4.0, but its linked
[data/code repository](https://github.com/a-vodka/stingsoundgenerator) has no
licence; neither its code nor recordings enter Electry. Bass coefficients are
not transferred to an eight-string guitar.

The lawful exact-eight direction check uses the thirteen public high-quality
previews in
[cabled_mess's CC0 F#-string pack](https://freesound.org/people/cabled_mess/packs/29585/).
The author identifies one clean/dry lowest-string stroke at each fret 0-12,
recorded through an RME Babyface into Cubase 10.5. Each page identifies its
login-only original as a mono 96 kHz, 24-bit WAV. The unauthenticated previews
fetched on 2026-08-29 are mono 48 kHz MP3s; no third-party audio is committed.
Their IDs, in fret order, and SHA-256 receipts are:

```text
fret  0  525010  bba14e50cf747a9e17426db0d9e378f5f24f8ddae8ed1f9772dd1fb830d8ac09
fret  1  525013  c9f182f15d005ee0590e74db66d71be1edc7b4ff0657b2b4e63011092e3c972d
fret  2  525011  d1e7638e2462014f4e0ad927c2a1f64f878b4238f0a7db5a4543b97325cacd6b
fret  3  525003  8211ea474dd8db1bf537234dc4fafae51b1c291f67ba19b31073f485979c5953
fret  4  525004  4f2227ed70bf3208f29285ebf2b3ce04a705ed3a4937545c1e69e253320d6ebc
fret  5  525002  41999dea30b450c737131c1e6b22c00d40da69907678d5db414fc7e7d2659cd2
fret  6  525008  2cc521f211871371a75bcc17030ff8e9bbf9c134dae7cd6b14c04c154bd5f346
fret  7  525001  18cd049b1b9e0b6f3693d808094bdb8e469c750018c123b53c2bfbe012fed773
fret  8  525006  ec558d3830448600157d8214ca90f193f8c1b9cf31a64ad548b3d1f5987ba472
fret  9  525007  f92ea2b0ec9d4830ead8010a324654f7f0a3da968453a520a12fe40197afc7fe
fret 10  525005  7fad5b916ab291fcaa25499b67d21b84d269b780c7c455bc19aed244e0508ad6
fret 11  525012  a1d947ab03d148d64cedaea7a3b41796d1a6dbacca454c5bb6d70f30a205e6a7
fret 12  525009  541754d503d91831e6ba0fdd739a0452566374605b1c32eaca48f6b69029f34c
```

Every preview URL follows
`https://cdn.freesound.org/previews/525/<ID>_5450487-hq.mp3`; the individual
sound page is `https://freesound.org/people/cabled_mess/sounds/<ID>/`.
Receipts used FFmpeg/ffprobe 8.1.1. Analysis used Python 3.11.0, NumPy 1.26.4
and SciPy 1.14.1. A centred 2 ms RMS envelope sets onset at 25% of its own
first-second maximum. An 8,192-sample Hann STFT with a 512-sample hop and
32,768-point FFT tracks each expected partial inside +/-0.28 fundamental.
Theil-Sen lines fit 0.20-1.20 seconds; tracks need at least 8 dB of tail
headroom, a median residual under 4 dB and a positive loss under 120 dB/s.
The comparison below is the median amplitude loss of retained H2-H8 tracks.

Shipping Electry at commit `d74a03761fe10070d6470dcff3f4859a1253bd92`
was rendered dry at 48 kHz/256 frames, Bridge/Mono, Sustain/Down, velocity
0.95, with sympathetic coupling, bridge-hand pressure and acoustic return off.
The regression seam forced physical string 8, so F#1-F#2 are model frets 2-14;
the real F# string uses frets 0-12.

| step / note | real loss | Electry loss | Electry - real |
| ---: | ---: | ---: | ---: |
| 0 / F#1 | 23.74 dB/s | 5.15 dB/s | -18.59 dB/s |
| 1 / G1 | 23.51 dB/s | 4.89 dB/s | -18.62 dB/s |
| 2 / G#1 | 19.07 dB/s | 5.30 dB/s | -13.77 dB/s |
| 3 / A1 | 21.29 dB/s | 5.66 dB/s | -15.64 dB/s |
| 4 / A#1 | 26.50 dB/s | 6.09 dB/s | -20.40 dB/s |
| 5 / B1 | 20.04 dB/s | 5.93 dB/s | -14.11 dB/s |
| 6 / C2 | 24.92 dB/s | 6.36 dB/s | -18.55 dB/s |
| 7 / C#2 | 11.58 dB/s | 6.68 dB/s | -4.90 dB/s |
| 8 / D2 | 33.15 dB/s | 7.11 dB/s | -26.04 dB/s |
| 9 / D#2 | 27.52 dB/s | 7.60 dB/s | -19.92 dB/s |
| 10 / E2 | 31.43 dB/s | 7.94 dB/s | -23.50 dB/s |
| 11 / F2 | 38.13 dB/s | 7.65 dB/s | -30.49 dB/s |
| 12 / F#2 | 27.32 dB/s | 8.50 dB/s | -18.82 dB/s |

The real median is 24.92 dB/s and Electry's is 6.36 dB/s. The paired model
minus real median is -18.625 dB/s, with a 5,000-resample whole-fret bootstrap
95% interval of -20.402..-15.639 dB/s. Electry also rises almost perfectly
with pitch (`rho = 0.9835`) where the take series is irregular (`rho =
0.6484`). That is strong directional evidence that this slice of Electry's
upper partials ring too long and too smoothly. It is not a calibration target:
there is one MP3 stroke per fret, with unknown gauge, pick, pickup and player,
and the instruments use different actual frets.

The irregularity should not automatically be smoothed away. [Le Carrou, Pate
and Chomette's operational-modal study of a played electric
guitar](https://doi.org/10.1121/1.5130894) finds that string-modal damping is
driven mainly by the real part of the neck driving-point admittance: a string
partial near a structural mode can be selectively shortened, while the
player's grip damps the structure, lowers that conductance and can lengthen the
string decay near a dead spot. Contact point and structural mode interact, so
this supplies a physical explanation for local fret/partial departures but no
coefficient that transfers to Electry's eight-string. A later low-CPU
experiment should measure complex neck admittance with the commissioned guitar
held in playing position, evaluate its sparse modal loss offline, and compress
that response into the same minimum-phase loop filter. The public F# previews
contain neither the impedance measurement nor repeated held/free conditions,
so no player-conditioned loss is added here.

### Measured two-axis fret-termination plan

The strongest actionable follow-on is now directly measured rather than a
smoother version of the Gaussian dead-spot guess. [Yudasaka, Scavone, Rau and
Ishizaka (2025)](https://doi.org/10.1121/2.0002142) measured 24 termination
locations—all 22 frets, the nut and the bridge—in vertical, horizontal and
cross-axis configurations, five times per location. Their
[author manuscript](https://caml.music.mcgill.ca/lib/exe/fetch.php?media=publications%3Ayudasaka_ismra2025.pdf)
reports vertical-admittance changes exceeding 40 dB and neck-side admittance
equal to or greater than bridge admittance. Importing the measured
fret-specific reflection functions into a coupled digital waveguide reproduced
the measured locations of partial-decay changes. In the demonstrated 175 Hz,
fret-1/fret-13 comparison, omitting neck reflection made the synthesized decay
rates nearly identical. Above 700 Hz the real string decayed faster than the
model, plausibly because its capo added a soft termination, and the authors
explicitly leave human finger/neck contact for future work.

That result supplies a plausible, testable mechanism for the lawful exact-eight
mismatch above: Electry's upper partials decay too slowly and too smoothly
while its current `deadSpotFactor()` is one frequency-independent Gaussian per
string. It does not supply transferable coefficients. The production
experiment therefore starts with the actual target eight-string in playing
orientation. An automated impact hammer and two-axis LDV measure the same 24
locations in both direct axes and both cross directions, with at least five
accepted repeats in
free, repeatable capo and normal-grip conditions. Repeatable wire-break plucks
then record string and bridge motion with the magnetic pickup removed; a
separate registered-height installation records synchronized DI for pickup-path
validation. Gauge, tension, fret action, hand force, support and temperature
are frozen with the audio and calibration receipts.

Offline fitting compares the present scalar loss with passive two- and
four-mode 2×2 termination-admittance candidates; modal frequencies remain
shared while fret-specific residues vary. At runtime that admittance is
converted to a reflection matrix using the active string's characteristic
impedance and live tension. The existing intrinsic termination loss is jointly
refitted or subtracted so the measured neck loss is not counted twice.
Stability and contractivity are verified for the complete matrix over
frequency, supported tensions and sample rates. A cross-axis section is
admitted only if it improves the alternating-fret model-selection folds beyond
the diagonal model; the complete measurement-repeat holdout and later remount
session remain unopened for final validation. The candidate reuses Electry's
two existing polarisation loops and updates coefficients only when the physical
fret coordinate or quantized live tension changes, at the existing control
rate and never per sample. It replaces rather than stacks the Gaussian loss.
Model order and fret interpolation are chosen with alternating frets hidden.
Promotion requires at least 50% lower held-out H1-H12 median decay-rate error
than shipping, no worse maximum error, improved polarisation-beating error, stable
44.1-384 kHz stress renders and less than 2% eight-string CPU overhead at a
96 kHz host. Only then may all frets be refitted for a default-off listening
candidate.

### Capture-gated winding-friction decay curvature

[Christian's isolated-string experiment](https://www.savartjournal.org/articles/14/article.pdf)
is an overlooked reason not to assume that every wound-string partial has an
exponential envelope. Its mass-normalized oscillator is
`x_ddot + 2 zeta omega x_dot + epsilon |x_dot| x_dot + mu sign(x_dot) + omega^2 x = 0`,
and its fitted envelope separates exponential, reciprocal-time and linear
terms as `F(t) = m exp(-a t) + 2 pi a0 / (3 pi + c t) + d t + f`.
In vacuum, the two tested 0.056-inch bronze-wound strings had a linear
fundamental envelope, a mixed linear/exponential H2 envelope and exponential
higher partials; their unwrapped core wire was exponential at every measured
partial. That direction is consistent with dry friction between adjacent
turns. It is not a coefficient source: the simultaneous three-loss fits were
ill-conditioned, varied by up to 471% and sometimes returned physically
impossible negative Coulomb loss. The construction was also acoustic bronze,
not Electry's nickel/steel exact-eight set.

[Paté, Le Carrou and Fabre](https://doi.org/10.1121/1.4871360) independently
represent dry friction between successive winding turns by a complex tension
`T (1 + j delta_W)` and add `Q_W^-1 = delta_W` to wound-string loss, but their
exponential-sinusoid observation model does not test the amplitude law.
[Vodka et al.'s 2026 electric-bass measurements](https://doi.org/10.3390/vibration9030046)
likewise identify larger losses in thick wound strings and wire-contact
friction as a plausible contributor, while retaining exponential modal
damping and only one accepted recording per condition. Together these sources
justify measuring a missing mechanism; none transfers a number or proves it
audible on E1/E2.

Electry's intrinsic string path is linear between events: each polarisation
uses one fixed loop gain and one fixed one-pole loss filter for a note, so even
their mixture is only a sum of exponentials. String Age moves those fixed decay
targets but cannot make an isolated partial lose a nearly constant amount of
raw amplitude per cycle. The preregistered observable is therefore not another
T60. For calibrated displacement amplitude `A[n,k]` of partial `n` at
successive cycles `k`, measure `D[n,k] = A[n,k] - A[n,k+1]`. The null is
`D = alpha A + gamma A^2`; the nested winding-friction candidate is
`D = alpha A + beta + gamma A^2`, with all loss terms nonnegative. `alpha`
captures exponential loss, `gamma` the quadratic air-drag direction and a
repeatable pressure-independent `beta > 0` the Coulomb/linear-envelope
direction. Per-take peak normalization is forbidden because it destroys that
absolute-amplitude signature.

The acquisition uses five disconnected string-pack/remount clusters: exactly
three TRAIN and two unopened HOLDOUT. Each contains new production E1 and E2
specimens from the named exact-eight set; TRAIN also contains manufacturer-
supplied matching bare core and the production plain string as negative
controls. A heavy, measured-low-admittance frame holds the production speaking
length and tension. A nonmagnetic wire-break release drives both axes while two
synchronized optical channels measure them without an added target. This
follows the two-axis isolated-string method above and the sensor boundary in
[Jasiński et al. (2025)](https://doi.org/10.3390/s25216514): magnetic DI is a
useful product observable, not an uncoloured string-motion reference. Each
specimen receives twelve accepted randomized releases at five absolute
displacement levels fixed from the 10th, 30th, 50th, 70th and 90th percentiles
of a separate production-performance preflight, at ambient pressure and two
lower pressure plateaus frozen in the manifest. Endpoint admittance, tension,
temperature, pressure, release position and sensor calibration are captured
before and after every block.

Before any response is decoded, hashes, eligibility rules, the 80 ms onset
exclusion, H1-H8 complex demodulator, 20 dB tail-headroom stop, models, bootstrap
and split are sealed. H1 and H2 are primary; separated axis/polarisation fits
prevent beating from masquerading as curvature, while H3-H8 are regression
guards. All three TRAIN clusters must resolve positive `beta` on both wound
strings with cluster-bootstrap 95% intervals excluding zero; core/plain
intervals must include zero. The fitted `beta` must remain inside one shared
TRAIN prediction interval across pressure while `gamma` decreases toward the
lowest-pressure plateau, otherwise air drag and winding friction are not
identified and the result is inconclusive. Only then may TRAIN fit one shared
nonnegative whole-voice decrement per low physical string. It may not add a
modal bank or mode-specific coefficients.

That frozen scalar is rendered against synchronized two-axis motion and clean
DI from the two exact-once installed-guitar HOLDOUT clusters. On each E1 and E2
in each HOLDOUT, it must reduce the equal-level median H1/H2 raw-envelope error
against shipping by at least 50%, with no worse maximum H1/H2 error and no
worse H3-H8 median, pitch, polarisation-beating or late-noise-floor result. The
registered DI is also passed through one fixed, level-matched high-gain reamp
for blinded listening, but listening cannot rescue a failed physical gate. A
single broadband scalar that fails any upper-partial guard is rejected rather
than followed by a filter bank.

The cheapest stable realtime form reuses the `sqrt(outputEnergy)` already
computed on Electry's 16-sample control tick. For a TRAIN-fitted nonnegative
per-round-trip decrement `Delta` and amplitude proxy `A`, cache
`g_C = 1` when `Delta = 0`, otherwise
`g_C = max(0, 1 - Delta / A)` for `A > 0` and zero at silence, then fold `g_C`
into both polarisations' existing loop gains. Thus `0 <= g_C <= 1`, the
candidate cannot inject energy, and OFF remains byte-identical without a new
per-sample branch, multiply, filter, allocation, parameter or UI control.
The combined, smoothed energy follower is only a cheap whole-voice surrogate,
not a modal-amplitude measurement; every displacement level and every one of
the sixteen possible first-control-tick phases is therefore a separate rail.
Round-trip/fret scaling is fitted from TRAIN rather than guessed. Promotion
also requires finite monotone-energy stress renders through every interaction
at 44.1-384 kHz and 24 warmed order-balanced OFF/ON benchmark pairs: median
overhead at most 0.25 percentage points of one core at 96 kHz, upper bootstrap
bound at most 0.5 points, and no 384 kHz ON run more than 1.0 point slower.

### Capture-gated worn-string spatial mass nonuniformity

[Grzybowski, Wrzeciono and Bayat (2025)](https://doi.org/10.3390/s25133989)
played two unnamed conventional six-string sets for 30 hours on one guitar.
Afterward, three micrometer repeats at five positions found diameter variation
up to 7%, particularly on wound strings; E2 was first to develop 12th-fret
intonation trouble, after 16/21 hours for the two sets. Its fundamental stayed
stable while higher-partial and pitch-time measures changed, including a 0.82
session correlation for set A E2's late-pitch `sigma_3`. This establishes a
measurement question, not a coefficient or cause: diameter does not measure a
wound string's linear mass density, there was no exact-eight E1, unplayed
control, remount split or measured `mu(x)`, and the recordings are available
only by request.

The missing candidate is a conservative spatial perturbation
`delta_mu(x)`, not another age scalar. String Age moves fixed decay targets,
the winding-friction gate above changes amplitude loss, and the current
stiff-string `B` produces smooth uniform dispersion. For
`phi_n(x) = sin(n pi x/L)`, first-order modal perturbation instead gives
`delta_f_n/f_n = -0.5 integral(delta_mu(x) phi_n(x)^2 dx) /
integral(mu_0 phi_n(x)^2 dx)`. At fixed tension `T` and speaking length `L`, a
narrow signed mass defect `delta_m` inside the current speaking segment, at
bridge-referenced coordinate `x_w`, gives the raw shift
`delta_f_n/f_n ~= -delta_m/(mu_0 L) sin^2(n pi x_w/L)`. Fundamental-preserving
scoring instead uses `r_n = delta_f_n/f_n - delta_f_1/f_1 =
-delta_m/(mu_0 L) [sin^2(n pi x_w/L) - sin^2(pi x_w/L)]`; a defect outside the
speaking segment contributes zero. Its falsifiable cue is therefore a
mode-oscillating residual that follows one physical location across frets,
unlike `B n^2`; the static law does not justify within-note wandering, random
phase or an age LFO.

[Jasiński et al. (2025)](https://doi.org/10.3390/s25216514) provide only a
positive-control boundary: on E2, a local optical target near 0.1% of active
mass changed partial balance and whole-record RMS energy without a resolved
fundamental shift. Its explicit faster-decay example instead used a target near
1.9% of a thin string's active mass, and mass, area, stiffness and asymmetry
remained confounded. Primary motion capture must therefore add no target. The
targetless defocus method of
[Niimi et al. (2024)](https://web.tuat.ac.jp/~yatabe/papers/niimi_2024_9.pdf)
tracked both transverse coordinates at 6,400 frames/s; the exact-eight capture
adapts that geometry and records synchronized raw DI. The 28-day,
two-performer/two-guitar fixed daily routine in [Wright, McKenzie and Hamilton's
2026 longitudinal
study](https://www.research.ed.ac.uk/en/publications/a-longitudinal-dataset-for-guitar-string-ageing/)
is a replication-design lead only, not a physical model or calibration source.

Acquisition uses five independent named-production pack/remount clusters:
exactly three TRAIN and two unopened HOLDOUT. Each target exact-eight E1/E2
specimen is captured new, after controlled wear but before cleaning, after a
frozen cleaning protocol and after remounting. Time-matched unplayed E1/E2 and
the production plain string follow the same capture schedule as negative
controls. A compact known added mass at a registered coordinate and a
matched-area lower-mass fixture with both masses measured separate modal-mass
sensitivity from aerodynamic area. A robotic metal-riff routine freezes use
cycles, pick force and position. Speaking length, tension, supports, setup,
temperature and humidity are logged, while noncontact diameter mapping or
micro-CT before and after wear is followed by destructive segment-mass checks
only after acoustic capture.

Before either HOLDOUT opens, TRAIN freezes the existing uniform-`B`, termination,
damping, polarisation and pickup null; every nuisance and eligibility choice;
the setup-intonation estimate; and the complete mass-map-to-prediction rule.
Within each worn specimen, both the before-to-after mass-map change and the
`f1`-normalized acoustic change must exceed time-matched unplayed drift under
that frozen rule. Complete specimens, remounts and wear days stay held out.
On E1 and E2 independently in each of the two HOLDOUT clusters, the frozen
candidate must reduce H2-H12 median frequency residual by at least 50%, with no
worse p95 or maximum and no regression in fundamental tuning, decay or
polarisation; failure in any of those four string-by-cluster cells rejects it.
Reject it also if baseline-new, unplayed or plain-string controls show the same
fixed-metre `sin^2` signature, cleaning/remounting removes the claimed
structural effect, measured mass fails to predict it, only within-note drift
remains, or the frozen null performs equally well.

There is no implementation or tuning before that gate. The only permitted
low-CPU prototype first tries an event-time refit of the existing dispersion
allpass, with no new per-sample state or allocation; if that cascade cannot
represent the held-out spatial signature, the cheap candidate is rejected
rather than expanded into a modal bank. Disabled builds must remain
byte-identical. The 24 warmed order-balanced benchmark pairs retain the same
rails as the winding-friction experiment: median overhead at most 0.25
percentage points of one core at 96 kHz, upper bootstrap bound at most 0.5
points, and no 384 kHz ON run more than 1.0 point slower. No public audio is
ingested or committed, and this gate makes no market claim. Bend/pole dropout
is not counted as a separate new mechanism because the existing bivariate
`flux(vertical, horizontal)` pickup gate already subsumes its lateral operating
point.

### Ranked measurement-backed follow-ons

1. Capture-fit the existing wound-string dispersion cascade. [Murray and
   Whitfield (2022)](https://doi.org/10.1119/5.0064373) found good agreement
   between theoretical and measured inharmonicity for monofilament strings but
   poor agreement for wound guitar strings. `Tools/FitDispersion.py` now
   provides a non-promotable calibration gate for supplied H1-H12 frequencies:
   it fixes one joint `(f1, B_take)` observation per take, fits Electry's Drop-E
   open-string `B` from even-fret TRAIN H2-H12 only, and scores odd-fret HOLDOUT
   under the same fixed-`f1` constraint. Captured F# fret `r` maps to model fret
   `r + 2`;
   the fit changes no source and adds no render-loop work. Original-WAVE hashes
   and reviewed commercial-calibration rights fail closed, but the tool does
   not extract partials and therefore cannot prove that supplied measurements
   came from those bytes. Final promotion still needs a separate installed
   exact-eight string-set holdout, at least 50% lower median cents error, no
   worse p95 or maximum error, and rendered confirmation that the production
   allpass approximation preserves fundamental tuning.
2. Measure the target pickup as a genuinely bivariate transducer. [Novak et
   al. (2020)](https://doi.org/10.17743/jaes.2020.0002) measured distinct
   two-dimensional nonlinear functions for single-coil, pole-piece humbucker
   and rail-humbucker geometries. The first candidate is only a compact fitted
   `flux(vertical, horizontal)` map in place of Electry's scalar mixed-axis
   polynomial. [Giampiccolo, Bernardini and Sarti
   (2023)](https://re.public.polimi.it/handle/11311/1236166)
   show an explicit, non-iterative wave-digital pickup/tone/volume/cable/load
   circuit, but that larger replacement is considered only if impedance and
   clean-DI measurements first falsify Electry's current loaded circuit. The
   bivariate stage must halve held-out normalized complex H1-H5 error across
   registered heights, amplitudes and elliptical trajectories, with no worse
   worst-case condition and below 1% CPU overhead.
3. Keep transverse/longitudinal coupling exploratory. [Lee et al.'s
   differentiable planar-string model](https://papers.nips.cc/paper_files/paper/2024/hash/0232cafe8d1909a01019abe8af32f3e1-Abstract-Conference.html)
   and [Bank and Sujbert's reduced longitudinal-resonator
   method](https://www.dafx.de/paper-archive/2004/P_089.PDF) offer an offline
   nonlinear teacher and a compact resonator reduction for pitch glide and
   phantom partials, but neither establishes an exact-eight residual. No
   runtime candidate is permitted until repeated three-axis motion and
   synchronized clean-DI captures show level-dependent longitudinal motion,
   pitch glide or phantom components above noise, and a coupled model improves
   unopened captures beyond the fixed dispersion/polarisation/pickup null.

Most importantly, fret and frequency are confounded. A robust fit of loss to
log partial frequency plus fret gives a direct fret coefficient of +0.1348
dB/s/fret for the real files and +0.0207 for Electry; their difference is
-0.1141 with a paired bootstrap interval of -0.8026..+0.6117. The data do not
identify a shipping fret coefficient, and the bass paper's opposite E-string
direction reinforces that boundary.

The CPU-bounded experiment now exists behind the default-off
`ELECTRY_LOW_STRING_LOSS_CORRECTION_ORDER2` build flag. It follows the
low-order, decay-time-weighted direction of
[Bank and Välimäki](https://research.aalto.fi/en/publications/robust-loss-filter-design-for-digital-waveguide-synthesis-of-stri/),
but makes only the narrow claim this public series supports: one fixed-Hz,
second-order RBJ loss dip is added to both played polarisations of physical
string 8 for F#1-F#2 (MIDI 30-42). It is minimum phase, never exceeds unity
magnitude, and its phase is included in the fundamental delay compensation.
Open E1, every other string, notes outside that range and all sympathetic opens
are exact bypasses.

The fit used every retained H1-H12 row, not the H2-H8 median headline above.
At 48 kHz, SciPy soft-L1 (`f_scale=1`) minimised
`(shipping + correction - real) / sqrt(rows in fret)` on even frets. The
frozen result is a 441.418112 Hz centre, Q 0.40618486 and 22.9327503 dB/s peak;
the loop depth is `22.9327503 / f0` dB per round trip. Coefficients were frozen
before any odd-fret result was read. The local 149-row JSON and CSV measurement
receipts have SHA-256
`e9b790b503221bdb23b575a491046683057d09b972cf278f242016df2ba2e4aa`
and `a010206ac1effd4461d55d514d65182e88e0790d1315f37bc361c52896cf8696`.
An order-four target's small, uncertain holdout advantage did not justify a
second realtime section.

The candidate was then rendered through the real mixed-polarisation engine,
not its fitting equation. A frozen real-track mask kept all 149 comparisons
fixed so changing decay could not change which model partials were admitted.
On the six untouched odd frets, equal-fret MAE fell from 16.292 to 8.533 dB/s
and row RMSE from 19.314 to 11.200 dB/s. A 5,000-resample whole-fret bootstrap
put the median MAE change at -7.97 dB/s, 95% interval -12.70..-0.74 and
`P(change >= 0) = 0.008`. Five of six frets improved; fret 7 worsened from
4.962 to 13.836 dB/s, so this is an aggregate result, not universal parity.
With the flag off, all thirteen rendered WAVs remain byte-identical to the
frozen shipping baseline.

The active-F# cost is one double biquad per polarisation on one voice. In 24
alternating 96 kHz, 512-frame, eight-note runs, OFF and ON used 0.12325x and
0.12239x realtime at their independent medians; paired overhead averaged 0.69%
inside a -5.05..+3.77% clock-noise range, and the worst ON run was 0.12634x.
CPU is not the promotion blocker. The two evidence boundaries now follow the
same continuous fret coordinate as Hammer and Slide: a one-fret C1 smoothstep
crossfades loss between frets 1/2 and 14/15, retains the exact source section
until the finger actually moves, and lands on exact full-correction and bypass
endpoints. Each tick rebuilds the passive physical filter rather than
interpolating coefficients, retains the two independent filter states, and
translates both raw delays for the changed filter phase. Legacy and MPE bends
do not move the evidence gate.

The boundary regression covers both directions at both ends for Hammer and
Slide, chained reversal, finite output, monotone loss, fractional passivity,
legacy/MPE bends and both-polarisation stationary-pitch error below 0.25 cent
at 44.1, 48, 96, 192 and 384 kHz. All thirteen settled ON renders remain
byte-identical to the frozen candidate and all thirteen OFF renders remain
byte-identical to shipping. An intentionally continuous, alternating MIDI
29/30 boundary-Slide microbenchmark at 96 kHz and eight-sample process quanta
used 0.04459x OFF and 0.04750x ON at the six-run medians: 0.00291x realtime,
or 0.29 percentage points of one core, for coefficient motion on every control
tick. The flag therefore remains off only pending repeated licensed exact-eight
captures and phrase-level blind listening. No harmonic bank, neural network or
FDTD solver enters the realtime path.

## Conventional-E2 post-fit falsification check

After the coefficients and odd-fret result above were frozen, a no-refit
adjacent-domain audit compared the candidate with all five EGFxSet clean
`6-0.wav` open-E2 takes. This asks whether the fitted loss can transfer merely
because another string has the same pitch; it is not another calibration set.
The files were recovered from the 431,036,829-byte Zenodo archive with five
single-range requests totalling 3,428,190 bytes, then checked against both the
ZIP CRCs and mirdata MD5 values. The source receipt SHA-256 is
`edca2335844399ba54ea7c8635a2a0639399daf114325e1cc21322757d849e2a`;
no third-party audio is committed.

Before rendering or reading a decay result, the audit froze the existing five
real files, the string-8 fret-12 OFF and ON E2 renders, a fresh shipping-OFF
renderer for physical string 3 open E2, and the exact estimator used above.
The preregistration and final eight-input manifest have SHA-256
`85695336c6671cc16883dcd0e89373b9017a48f3a0c763181ee0145a7db3b72d`
and `9bf8e2ef53f986e8f3db3d6222db88b444a577da59f257896ac97f42f34dcea3`.
The fresh role-matched WAV is MIDI 40 on physical string index 2, open fret,
velocity 0.95, 48 kHz/256 frames, Mono/default build with sympathy, bridge hand
and acoustic return disabled; its SHA-256 is
`9005acb7949786116014b87fdb9f13feabc575b367a2d8de0b892580475ae993`.

The frozen H2-H8 gates were intersected across all five real takes and all
three model renders. Exactly H4-H7 survived, meeting the predeclared four-track
minimum without relaxing a threshold. A first attempt stopped before onset or
spectral analysis because SciPy could not decode the EGFxSet PCM24 container.
The disclosed decoder-only amendment, SHA-256
`0b89834deb970e947894d08e5d2b818e1a3d563676f30f825f2283d79be9e055`,
added a signed little-endian PCM24 reader; every numerical rule remained
unchanged.

| common H4-H7 comparison | median loss | median signed error | MAE | RMSE |
| --- | ---: | ---: | ---: | ---: |
| five real pickup takes | 4.586 dB/s (3.701..7.696) | - | - | - |
| string 8 / fret 12, candidate OFF | 7.105 dB/s | -0.055 dB/s | 3.808 dB/s | 4.496 dB/s |
| string 8 / fret 12, candidate ON | 30.783 dB/s | +23.193 dB/s | 23.917 dB/s | 24.405 dB/s |
| string 3 / open, shipping OFF | 7.816 dB/s | +0.630 dB/s | 3.935 dB/s | 4.728 dB/s |

Errors aggregate the five pickups by four common partials. Turning the
candidate on worsens MAE for every pickup by 16.974..21.542 dB/s and aggregate
MAE by 20.109 dB/s. The result rejects pitch-only transfer of the exact-eight
F# correction to a conventional Strat low E2 and reinforces keeping the flag
off. It does not refute the frozen exact-eight F# holdout: string construction,
scale position and fret geometry differ, while the EGFxSet pickup files are
separate normalised plucks. The OFF result is directionally much closer, not a
claim that either mapping is capture-fitted. The intersection only meets its
minimum, so it says nothing about H2, H3 or H8. Its 0.20-1.20-second slopes end
before the dataset's forced fade at 3 seconds, although that fade makes the
final-frame headroom gate permissive for the real files.

The read-only final receipt and result have SHA-256
`eb740fadf6fd7499f2703cd5f3e335b28e13ea791af0b2fc08ad46f690eacc6c`
and `d5b10c339563f1511365bcf68885013e90a4c9b55ac497aeead8022402fef84f`;
the partial and comparison CSV hashes are
`43321623cdbc174fd6259f5278a26e646b8bcce35248b98532c31ad93435d7c1`
and `6529050170f35f067803b8c94a0799d727d33d2f96c5579075e21689959e1079`.

## Attack-pitch and resolvable-beating audit

A second no-refit audit asked whether the same lawful recordings expose a
missing pitch trajectory or a resolvable modal doublet. Its protocol and input
manifest were SHA-256 frozen as
`ffa0c7c4d19eca02de0f742f49810a4b8b42bec532d64d7ca36833578cc5a27e`
and `0539cc3eb280ef04ecacc26b249933b88caac25eff724802179e5773a365e506`
before any sample-derived pitch, phase, envelope or modulation result was read.
The same 13 exact-eight previews were paired by fret with shipping-OFF and
loss-candidate-ON renders; the five EGFxSet open-E2 DIs and shipping string-6
open-E2 role control remained descriptive robustness checks.

After a 2 ms RMS onset, the frozen estimator tracked H1-H6 independently from
unwrapped analytic phase, using an 80 ms phase line every 10 ms. Each partial
was expressed relative to its own 0.75-1.20 s reference before a quality-gated
cross-partial median formed the 60-180 ms attack excess and 80-500 ms Theil-Sen
relaxation slope. A detection required at least +2 cents at attack, at most
-3 cents/s thereafter, a near-zero late residual and fixed frame/dispersion
coverage. This avoids treating a changing broadband spectral centroid as
pitch, although it still cannot isolate the physical cause.

| cohort | sufficient / files | detections | median attack excess | median slope | frozen support |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact-eight real | 5 / 13 | 5 | +6.218 c | -7.670 c/s | no: coverage below 10 / 13 |
| Electry shipping OFF | 13 / 13 | 0 | +0.007 c | +0.003 c/s | no |
| loss candidate ON | 13 / 13 | 0 | +0.004 c | -0.007 c/s | no |
| EGFxSet real E2 | 5 / 5 | 5 | +7.550 c | -14.282 c/s | yes |

All 13 exact-eight previews had a positive descriptive attack excess and a
negative slope, including the eight excluded from the formal cohort. That sign
agreement and the independent EGFxSet pass make Electry's stationary attack a
high-priority realism discrepancy, but the strict cross-dataset result remains
false. On the five sufficient exact-eight triplets, candidate attack MAE was
8.594 cents against shipping's 8.608 cents and therefore failed the frozen
at-least-1-cent and 20% improvement gate. The loss experiment neither fixes nor
materially changes pitch motion.

The companion detector required coherent 1-12 Hz periodic envelope and pitch
motion in at least two partials, with at least three observable cycles. No
exact-eight real/OFF/ON file passed and only one of five EGFxSet takes passed,
below its 4/5 gate. Median best-case H6 resolution floors were 8.712 cents for
the short exact-eight previews, 6.150 cents for their model renders and 4.879
cents for EGFxSet. Thus no fast modal doublet is supported, while Electry's
slow sub-cent polarisation beating is explicitly below this audit's resolution
and is not rejected.

The byte-deterministic rerun produced an identical result. Its read-only result
and final receipt have SHA-256
`e836d01e7c5ad96aa45c211916bbb60df11aa930474b284c2be24b02b2c2bc0b`
and `3b196778284f2d7d39c6b3743fe2c96d29a739fba3c4263ea0481f5d0b4e00c7`;
no third-party audio is committed. These unmatched recordings support testing
a bounded amplitude-dependent attack-pitch mechanism, not choosing tension,
fret/contact motion or pickup-magnet back-action, fitting a coefficient, or
changing the shipping path.

## Remaining realism gates

Engineering tests and public datasets do not replace licensed Drop-E hardware
validation. No public source found closes the licensed, controlled, genuinely
dry E1 and open-string E2 Open/Mute/Dead gate. The best lawful sources each
cover only part of it: [inspektral's CC0 seven-string baritone matrix](https://freesound.org/people/inspektral/packs/42559/)
matches muted and sustained low notes but does not identify the muting hand,
raw-DI chain or exact tuning; this pass could access only its public previews.
[HiMMP](https://himmp.net/faq.html) supplies CC-BY score-matched raw Palm DIs,
but from a conventional six-string Drop-C guitar.
[Freesound 557299](https://freesound.org/people/minus_28_and_falling/sounds/557299/)
is a real Drop-E phrase but its hand labels, chain and public-preview codec are
not controlled, while [cabled_mess's eight-string pack](https://freesound.org/people/cabled_mess/packs/29585/)
documents clean dry chromatic one-shots on its lowest F# string, but only F#1
is open and there are no mutes or Dead.
[8ridgelite](https://github.com/JamesStubbsEng/8ridgelite) adds GPL-3.0,
downloadable exact-eight open-note sustains from E1 upward. Its two low files
show a much faster attack and broadly similar first-two-second decay, but the
guitar, tuning, string/fret, channel identities and recording chain are
undocumented and there is only one stroke per pitch; it is a direction check,
not a fit set.

The expanded lawful search adds useful controls but no missing extended-range
gate. [EGFxSet](https://egfxset.github.io/) and its
[CC-BY-4.0 Zenodo deposit](https://zenodo.org/records/7044411) provide 690
five-second clean-DI captures: all 138 string/fret positions of a standard
six-string Strat at five pickup settings, made with one 2.0 mm pick and one
documented instrument-input chain. They are a strong adjacent-fret and
conventional-E2 sustain check across separately played pickup settings, but
their normalisation precludes absolute-level fitting and they contain no Palm
or Dead notes.
[GuitarJam](https://huggingface.co/datasets/Julian-br/GuitarJam/blob/main/README.md)
adds 580 CC0 raw-DI monophonic phrases for timing, dynamics and incidental-noise
listening, but has no string, fret, tuning or technique labels. Neither source
can promote a Drop-E/F# coefficient.

The formerly unavailable EG-IPT lead is now a published
[CC-BY-4.0 Zenodo dataset](https://doi.org/10.5281/zenodo.15205644). The main
corpus contains 52,320 audio files (28 h 22 min 56 s) recorded at 96 kHz/24 bit
from a six-string 2005 Gibson SG Standard: three humbucker configurations and
six simultaneous sources, including a BSS AR 133 DI through a Midas XL48. The
paper's 1,038-file Strandberg seven-string corpus is a separate generalisation
test set, not the deposited EG-IPT archive; conflating the two had made this
source look both smaller and unavailable.

A range-only audit of the 23,757,983,313-byte archive selected the literal
sixth-string note-identifier `02` DI under `ordinario`, `muted` and `snap-pizz`.
Eight files match. The HB-bridge snap-pizz technique is not absent: it has 23
sixth-string files under a different, unexplained identifier sequence beginning
at `03`, so none was guessed into the frozen `02` subset. ZIP CRC and SHA-256
checks passed after 36 HTTP range requests totalling 30,512,346 bytes, or
0.1284% of the archive. The read-only source receipt and follow-up filename
forensics have SHA-256
`1a60c1c1b4cffecd90f5c47d5f180bda92716f9653cfd84fe46573dac964a5f1`
and `31dd194e792f2ae38d23d8ef7fd249d23e46ded951053ea5f5709897a6395724`;
no third-party audio is committed.

The paper defines `_6s` as string 6 but calls `02` only a unique note
identifier; it specifies neither the numeric grammar nor the guitar tuning.
The common 23-identifier lattice is compatible with open plus frets 1-22, but
does not prove that order or standard-tuned open E2. Released-WAV normalization
is also unstated. Pitch must therefore establish note identity before analysis,
and even a confirmed E2 remains a controlled conventional six-string
technique/pickup check rather than licensed Drop-E calibration data.

### Frozen EG-IPT conventional-E2 result

The partial-frequency protocol was SHA-256 frozen as
`09fb828a5f8d4326227f24061f63a610e9c6dfb42fb596671f44aff5a254324c`
before any waveform sample was decoded. Its synthetic suite rejected silence,
clipping, a neighbouring note and a static-inharmonicity/unequal-decay false
positive, while detecting a shared H1-H6 glide. The analyzer and exact 23-file
code freeze have SHA-256
`fc03d69007edfb99a1f74b1a9fa58019fa44e169e6d8b811683cb6e4be025c17`
and
`cc5eed5fe93c7c7bc00e778fca995f76aa1054692054e5f5b0595ef2c8c28a76`.
No threshold, file, window, mapping or model control changed after that freeze.

All three `ordinario` files pass the note and coverage rails. Their late H1
references are 82.318251, 82.973623 and 82.525740 Hz (-1.863, +11.865 and
+2.495 cents from E2), establishing the E2-like endpoint for this stratum
without claiming that identifier `02` means open E2 throughout the corpus.
The frozen result is:

| source / pickup | aggregate attack | aggregate slope | H1 attack | H1 slope | coherent |
| --- | ---: | ---: | ---: | ---: | ---: |
| EG-IPT / bridge | +2.368 c | -9.596 c/s | +2.686 c | -11.053 c/s | yes, H1-H6 |
| EG-IPT / couple | +2.110 c | -9.097 c/s | +2.199 c | -10.036 c/s | yes, H1-H4 |
| EG-IPT / neck | +1.045 c | -4.970 c/s | +1.198 c | -5.545 c/s | no; below the 2-cent/four-partial rails |
| Electry / Bridge | +0.0049 c | -0.0193 c/s | +0.0090 c | +0.0033 c/s | no |
| Electry / Both | +0.0048 c | -0.0194 c/s | +0.0091 c | +0.0282 c/s | no |
| Electry / Neck | +0.0060 c | -0.0265 c/s | +0.0168 c | +0.0129 c/s | no |

Thus `ordinario` passes its preregistered real-trajectory gate: 3/3 files are
sufficient, 2/3 share a positive attack offset followed by negative
relaxation, and the cohort median attack magnitude is +2.110 cents. Shipping
Electry is effectively stationary and fails four of five compatibility rails.
Its median/max aggregate-attack errors are 2.105/2.363 cents, median/max slope
errors are 9.078/9.577 cents/s, and median H1-attack error is 2.190 cents; only
the four-cent maximum-attack bound passes.

The negative controls matter. Electry PalmMute shows only a rejected
upper-partial spectral drift: its aggregate attack is about +0.65 cent, while
H1 moves by -0.030..-0.045 cent with a positive slope. All three real `muted`
files decay before the frozen tracker can form the required 300-440 ms late H1
reference, and both `snap-pizz` files lack common partial coverage. Those
strata are inconclusive, not evidence of a wrong note or a stationary attack.

This is one separately played take per pickup/technique cell on a conventional
six-string SG. It supports a bounded, default-off common amplitude-pitch
experiment; it does not identify tension, magnet or contact causality, fit an
exact-eight coefficient, or authorize a shipping change. Two complete runs
produced byte-identical JSON and CSV with SHA-256
`c82e6cb447f27f2fb4258f5b93b42f39cf6cbf733ce1d1373a8f80b57f13e5b2`
and
`74a4806f195780ecee57bef20e76df1b046c1a4547b5834edb8a3cc8f461b866`;
the read-only final receipt is
`2760dd5c9248aca031afc7d1281471e8f2725493c1ea5affd8a20af6e1c7a5cf`.

[GOAT](https://zenodo.org/records/15690894),
[IDMT-SMT-Guitar](https://www.idmt.fraunhofer.de/en/publications/datasets/guitar.html)
and the [pitx muted pack](https://freesound.org/people/pitx/packs/898/) are
noncommercial; [GADA](https://gingerpianist.github.io/GADA_demo_page/) exposes
no dataset licence. They cannot be used to tune a commercial product.

The [cabled_mess profile](https://freesound.org/people/cabled_mess/) offers custom
recordings and is the best first commission lead; availability, Drop-E setup,
protocol compliance and commercial calibration rights still require a written
agreement. The documented
[RG8 corpus repository](https://github.com/aomartinezg/music-sheet-generator)
has neither committed raw recordings nor a licence, so it supplies no lawful
calibration audio. The
commissioned [`electry-mute-capture/v1` preflight](capture/electry-mute-capture-v1/README.md)
therefore requires 16 files per session: ten isolated E1/E2 Open, three Palm
positions and Dead probes; four rapid Palm/Dead runs; one mixed Dead E1/E2
groove; and one Palm-E1/Open-E2 lift/replant groove, at least 429,975 frames.
Commercial calibration/private-evaluation rights, stable player and guitar IDs,
and train/holdout separation are mandatory; the final engineering gate needs
at least three train clusters and exactly two untouched active holdout clusters.

### Angle-conditioned plectrum-scrape research

[Lindroos, Penttinen and Välimäki's measured electric-guitar
study](https://doi.org/10.1162/COMJ_a_00066) separates the noise made while a
plectrum slides over the string from the pulse at release. On a Fender
Stratocaster recorded simultaneously through an under-saddle piezo and magnetic
pickup, a larger plectrum/string angle produced a longer scrape at a higher
level relative to that pulse; the authors also identify this detail as
especially audible through distortion. Their analyzed bursts span 20-130 ms
and 10-40 dB below the pulse, while Electry's current hardness-derived scrape
spans about 0.8-4.8 ms. Those six-string ranges establish a missing mechanism,
not coefficients that transfer to a Drop-E eight-string.

The minimum candidate would reuse only existing event-time state.
`drawStrokeVariation()` already latches one deterministic, chord-shared
`strokeAngleOffset`; `startExcitation()` already resolves `noiseLevel`,
`noiseMs` and the wound/plain noise pole before arming `noiseAmplitude` and
`noiseLength`. The angle currently rotates only the two excitation-polarisation
weights in `updateStyleWeights()`. A later default-off trial may therefore fit
one log-domain slope for scrape/pulse level and one for duration, with exact
identity at zero angle and when Pick Noise is zero. It must not add another
random draw, filter, parameter, render-loop branch or angle-dependent bandwidth:
the paper supports duration and relative level, not a cutoff change.

That reuse has a hard falsifier. Electry's stored value is an offset in the
attack's polarisation plane; the paper varies the physical angle between pick
and string. A registered three-dimensional pick trajectory must show that
these are the same coordinate. If they are not, the two-scalar candidate is
rejected rather than relabeling the existing value or inventing another latent
control.

The capture uses the production eight-string, string set, pick, pickup height
and position, with raw bridge DI plus a high-bandwidth contact channel and an
optical or force release marker. Randomized cells cover E1, E2 and one plain
string, up/down strokes, at least two force strata, registered pick angles and
at least 20 accepted repeats per cell. Exclusions and maximum attempts are
frozen first. TRAIN fits only the two slopes; complete angle/string/direction
cells remain untouched HOLDOUT. Primary measures are contact-to-release scrape
duration and scrape-to-pulse RMS ratio. Promotion requires the registered
direction to exceed within-cell repeatability on both low wound strings, at
least 50% lower HOLDOUT median absolute error in both measures, no worse p95 or
maximum error and no direction reversal in any holdout cell. Otherwise both
slopes remain zero.

Only after that dry gate may an RMS-matched high-gain real/baseline/candidate
listening test judge audible realism. A later implementation must keep its
disabled renders byte-identical and remain finite from 44.1 through 384 kHz.
Nine or more warmed alternating ABBA/BAAB benchmark rounds must exercise eight
strings at the maximum supported contact rate, with at most 0.25% paired-median
overhead at 96 kHz and 1% worst-case overhead at 384 kHz. Event-time arithmetic
is not permission to skip that measurement.

MIDI Note Off velocity is a separate no-go. The MIDI 1.0 detailed specification
[recommends 64 when a sender has no release-velocity
measurement](https://midi.org/community/midi-specifications/writing-midi-software-send-note-off-or-zero-velocity-note-on),
so preserving a true `0x8n` value and treating zero-velocity Note On as an
unknown, neutral 64 would be an objective transport change. It does not supply
a guitar mapping. In Cuzzucoli and Lombardo's [physical plucking
model](https://www.cglib.org/wp-content/uploads/cglib.org/Musicology/Physical%20model%20of%20the%20plucking%20process%20in%20the%20classical%20guitar.pdf),
the gradual "string release" that suppresses upper modes is the finger or pick
leaving the displaced string at note onset. Their separate end-of-note damping
section varies flesh-contact damping, not noise duration or bandwidth. Until
exact-eight stopping gestures identify a direction and curve, mapping Note Off
velocity to Electry's release-noise burst remains unsupported; the existing
unassigned-release-velocity boundary stands.

### 2026-08-30 exact-eight recording checkpoint

A fresh search found no second public source that closes the controlled dry
E1/E2 Open/Palm/Dead gate. The only new lawfully reusable exact-eight signal is
a descriptive F1 anchor, not a calibration corpus.
[Vrezerino 133024](https://freesound.org/people/Vrezerino/sounds/133024/)
is CC0 and identifies a sustained lowest string of a Schecter Omen 8, reported
as gauge `0.74`; its adjacent source identifies F1. The clean recording has
short reverb and no documented fret, pickup, pick, force or path. Its login-only
original is a 44.1 kHz/16-bit stereo WAV. The unauthenticated
[HQ preview](https://cdn.freesound.org/previews/133/133024_17361-hq.mp3)
fetched on 2026-08-30 is a 43.932154-second, 995,282-byte stereo MP3 with
SHA-256 `3535e3a7b7edf8f63ee7639ed1c096f9ece1932527be57625f05bebe6a3a4e23`.
Only normalized onset-aligned first-100-ms timing and partial-frequency motion are
admissible; reverb and the unknown chain censor late decay, noise floor,
absolute level, pickup response and absolute spectrum.

The adjacent CC0
[distorted F1 recording](https://freesound.org/people/Vrezerino/sounds/133025/)
is not a DI/reamp pair: "playing again" identifies a separate performance with
heavy distortion and no reverb through an unknown nonlinear chain. Its public
[HQ preview](https://cdn.freesound.org/previews/133/133025_17361-hq.mp3)
is 25.868571 seconds and has SHA-256
`e6862bfdf643fbecf2988da56be11efbcd726fd1cc44483f1e3cee485e801b45`.
It is a listening reference only and cannot identify dry attack, damping,
pickup balance or an amp transfer from the clean file.

[Ueberschall Metal Riffs](https://www.ueberschall.com/en/product/283/Metal-Riffs)
has 400 DI phrases paired to the same amped performances, but mixes seven- and
eight-string guitars without per-file identity. Its
[licence](https://www.ueberschall.com/en/faq) makes the permitted music-production
uses conclusive, restricts duplication and adaptation outside such productions,
and separately prohibits AI/ML development. Electry calibration needs a written
grant. [JST Spirit of the Machine](https://joeysturgistones.com/products/riff-vault-spirit-of-the-machine)
does identify Drop-E eight-string DI and corresponding amped/wet phrases, but
JST's public
[permission statement](https://joeysturgistones.zendesk.com/hc/en-us/articles/40184857335444-Can-I-use-your-samples-in-my-own-music-releases)
covers royalty-free music releases subject to the included EULA, not competing
instrument calibration; it is also written-permission only.

Beyond the established commission lead above,
[Ciprian Martin](https://soundbetter.com/profiles/465257-ciprian-martin) is a
fallback: his profile names an Ibanez RG2228M with EMG pickups and D'Addario
NYXL strings and offers DI delivery. Vrezerino's
[2026 metal pack](https://freesound.org/people/Vrezerino/packs/45261/) shows
current activity and makes its surviving pre-distortion DIs and guitar identity
worth asking about. Either route needs the existing protocol and explicit rights
for commercial physical-model calibration, private evaluation and derived
measurements.

The minimal experiment is to obtain the original 133024 WAV, render Electry's
physical eighth string at F1, and triangulate both with the established dry F#1
and low open-note anchors. Normalize per file and score only the first 100 ms:
H2-H8 inharmonicity, 30-to-100-ms per-partial pitch change, median attack-pitch
slope and onset-to-2-ms-RMS-peak time. This F1 file supplies direction only; it
cannot promote a decay, spectrum or level change, and no model adjustment
advances unless it also improves the independent dry reference.

### Reactive loudspeaker-load research

Published bidirectional guitar-amplifier models support a causal electrical
speaker load inside the tube/load solve, not an impedance-shaped EQ after it.
[Pakarinen, Tikander and Karjalainen](https://dafx.de/paper-archive/2009/papers/paper_16.pdf)
and [Macak and Schimmel](https://dafx.de/paper-archive/2011/Papers/05_e.pdf)
used measured transformer/speaker data; their result does not authorize deriving
a cabinet load from nominal impedance or an acoustic recording.

A defensible small-signal free-air driver starts with the passive network
`Zs(s) = Re + s Le + (Rp || s Lp || 1/(s Cp))`, where
`Rp = Re Qms/Qes`, `Lp = Re/(2 π fs Qes)` and
`Cp = Qes/(2 π fs Re)`. The official
[Jensen C12K 8 Ω](https://www.jensentone.com/vintage-ceramic/c12k) fields
`Re=6.75 Ω`, `Le=0.84 mH`, `fs=115 Hz`, `Qms=16.4` and `Qes=0.90`
give `Rp=123.0 Ω`, `Lp=10.38 mH` and `Cp=184.5 µF`. They identify only one
rounded, low-level driver model. They do not identify its open cabinet, and
Jensen's rounded mechanical fields give slightly different derived values.
The current official Celestion
[G12M](https://celestion.com/product/g12m-greenback/) and
[Vintage 30](https://celestion.com/product/vintage-30/) pages do not publish
enough Q/complex-impedance data to derive matching British or Modern networks.
[Celestion's own guidance](https://celestion.com/blog/thinking-of-using-thiele-small-parameters-to-design-a-guitar-speaker-cab-think/)
also warns that small-signal T/S data do not characterize an open-back guitar
cabinet.

For an ideal centre-tapped transformer with turns ratio `n`, half-primary
voltage `v` and differential plate current `dI`, `vs=2v/n`, `is=n dI/2`, so
`v=(n^2/4) Zs dI`. Choosing `n^2=Raa/Znom` recovers Electry's current
`v=(Raa/4)dI` resistive load. A real candidate must therefore close the
reactive companion against the American/British power-pair current in the same
oversampled frame, feed the converged current to sag and the loaded secondary
to negative feedback. Filtering the existing fixed-load table output would
leave all three interactions wrong. Modern has no push-pull pair, `Raa` or
defined secondary port, so a post-waveshaper version cannot make the same
claim.

The passive network itself is cheap: roughly three to five states and about 20
scalar operations per internal sample and channel. The nonlinear coupling is
the blocker. A direct runtime evaluation reaches nested plate and screen roots;
adding reactive history as another dense table axis is too large. A local
Norton/tangent solve is admissible only if an offline exact implicit oracle
holds plate swing below 1% error and supply demand below 2% throughout both
models' rail, grid and load-history domains. It must also retain positive-real
impedance, stable poles, closed-loop feedback margin and less than 3% paired
96-kHz CPU overhead.

Existing EG-IPT DI/microphone pairs and acoustic cabinet IRs cannot validate
this circuit because they contain no simultaneous speaker-terminal voltage and
current. Promotion requires a calibrated series-sense capture of complex
`Z=V/I`, cold/remount/specimen repeats and multiple levels, followed by fixed
dummy-load/real-cab E1, E2, two-tone and transient reamps. Until that exists,
the result is a deliberate no-go: Electry retains its bounded resistive
American/British load and makes no reactive loudspeaker-reflected-load claim.

### Energy-derived attack-pitch research

The first-ranked low-CPU pitch experiment is now a common string-tension glide,
but not a return to the removed force-to-cents envelope. [Lee et al.'s
per-partial measurements](https://www.dafx.de/paper-archive/2009/papers/paper_86.pdf)
show the first 15 partials of one plucked guitar string relaxing along nearly
one normalized exponential trajectory; a softer pluck mainly changed its
starting offset. [Bank's energy formulation](https://dafx.de/paper-archive/2009/papers/paper_76.pdf)
relates the quasistatic tension increase directly to transverse string energy,
`T_qs = E_modulus A E_string / (2 L T0)`, and permits evaluation every 1-10 ms with
linear interpolation. That is one bounded scalar per string, not a nonlinear
solver or per-partial oscillator bank.

The wound-string scaling is crucial. [Kemp's measured string-construction
model](https://doi.org/10.1371/journal.pone.0184803) supports treating winding
tension as negligible, so the elastic term uses the steel core's `E A_core`
while inertia still uses the complete string's linear density. Using the .080
outer diameter as load-bearing area would grossly overstate Drop-E modulation
and repeats the kind of cross-string mismatch that made the earlier prototype
sharpen E1 by about 30 cents but B1/E2 by only 9-13 cents.

Broadband tuner motion is not enough evidence. In [Kemp's .132 bass-string
experiment](https://doi.org/10.1007/s42452-020-2391-2), static inharmonicity and
faster upper-partial decay produced a strong downward broadband pitch trace
while the isolated fundamental and spectrogram did not descend. Any Electry
gate must therefore normalize and track individual partial frequencies, require
the fundamental plus at least three partials to share the trajectory, and leave
its existing static dispersion/loss model responsible for purely apparent
spectral glide.

Electry's existing `outputEnergy` is not reused. Under the compile-time
`ELECTRY_ENERGY_ATTACK_PITCH` flag, an ordinary Sustain plectrum contact
instead latches a physical release candidate. For pluck fraction `p`,
two-axis peak displacement `y` and axis weights `w_v`, `w_h`, it evaluates
`E_string = T y^2 (w_v^2 + w_h^2) / (2 L p (1-p))`, then converts that
energy to the dimensionless tension ratio
`q = E A_core E_string / (2 L T^2)`. Sounding frequency follows Bank's
square-root law, `f / f0 = sqrt(1 + q)`. The winding remains inertial while
only the estimated steel core supplies axial `E A`; the complete static
dispersion fit stays at the written pitch.

The physical energy and elastic scale exist only while that plectrum contact
is pending. On real release they seed a bounded phenomenological `q`; delayed
or interrupted contacts do nothing, repicks retain the larger old/new value
without summing it, legato carries only live `q`, and reset, silence and a
large voice steal close the state explicitly. The common dynamic period is
phase-compensated in both polarisations before their fixed relative split.
`q` is capped at `2^(7/600) - 1`, exactly seven cents, and decays with a
0.3049 s time constant. That time constant is borrowed from Lee et al.'s one
hard-plucked open high-E measurement, not fitted to EG-IPT and not asserted as
an eight-string material constant. The 4.8 N full-force contact anchor and
wound-core fractions are existing heuristic/construction coordinates, not
coefficients fitted after seeing the comparison.

The v2 candidate protocol was frozen before the final candidate audio was
decoded by the analyzer, with SHA-256
`3db89fcbc411ea16d34ba89a0b11b3b15067cd7742053411cb687700f633093e`.
Its wrapper and unchanged estimator core have SHA-256
`10e78e687682541c6a28be68144a85c8073bb2bc27c50a1911b77c9eb4e4737c`
and
`745d31494b5ddca33f092a3d8208a533892c8a1dd2d21fd9263bdc3fc16d5de3`;
the exact 29-file source/audio/executable freeze is
`c50d86d4fea7603bd650b0a96eb1b8d1a21973199e46a93993e8e4c27eb760f1`.
The wrapper rebinds the byte-identical estimator core; no numeric estimator
constant or gate changed.

All six model files were sufficient, all three Sustain renders had a positive
coherent attack trajectory, and every registered absolute-error measure
strictly improved:

| error against the three ordinary EG-IPT cells | shipping | candidate |
| --- | ---: | ---: |
| median aggregate attack | 2.105 c | 0.347 c |
| maximum aggregate attack | 2.363 c | 0.980 c |
| median aggregate slope | 9.078 c/s | 2.072 c/s |
| maximum aggregate slope | 9.577 c/s | 2.569 c/s |
| median H1 attack | 2.190 c | 0.684 c |

The three Palm renders remain byte-identical with the flag on, while all six
flag-off release renders remain byte-identical to the pre-candidate baseline.
Two complete analyzer runs produced identical JSON and CSV with SHA-256
`bce5c0b612ebd2e8dea8b15dc584b6ece62705b300ab79ded1b3d6eeaa7219a8`
and
`23fe3f59f23c3bfc3b4c773b433b9f431e46c380ba5bd340b6b23bc5fad8ba62`.
Both flag-off and flag-on engine and FX suites pass. Nine alternating
ABBA/BAAB rounds at 96 kHz, block 256, all eight strings, Both pickup and
Stereo measured median overhead of 0.397% for Sustain and 0.735% for Palm;
paired-round medians were 0.403% and 0.764%, below the frozen 3% ceiling.
The CPU result has SHA-256
`982c791df5695d54512c4f4ab150b2d54297e27d7a81d2e0cdf62cd64d906f48`.

A descriptive exact-eight follow-up reused that frozen estimator core on the
[8ridgelite](https://github.com/JamesStubbsEng/8ridgelite) repository's
20-second `Natural/0_e1.wav` and `Natural/13_e2.wav` stereo files. Their
SHA-256 values are
`1a9e05a3a2eeae067bdddd4fc102d50deb0c749bddfb0632c5e1f4a2ac866c5a`
and `c95c1989892142246b291354bb808e98991a529fb52818920554581165f09018`.
The wrapper changed only the nominal `f0`, inspected each unnamed channel
separately and prepended 100 ms of zero solely to clear the estimator's onset
boundary; its SHA-256 is
`45f2bf57a016247e2f40f88a195e335fa022586ec4afec45df3d9d4c66cc02f1`.
No threshold or analysis window moved. Shipping E1/E2 model WAV SHA-256 values
are `2c877332b1a6ac3bfd6d2603c815374219030997cd430fd8710b1b6cbb0e176b`
and `ef93361c9a08ce66b2a8012774ddaf583a389e0a9101c25e04f7697e35ac4a4f`;
the candidate pair is
`dc157a90b71616358fe6b257bb022ba9782acd20507713f870504e7e84b80047`
and `c986857cde98202773583550bc84f8c707f5a96b5cac0c7c4568e349960f0948`.

One E1 channel lacked the frozen individual-partial/aggregate coverage. The
other three channel/note observations and same-compiler model renders were:

| observation | real attack / slope | shipping | candidate |
| --- | ---: | ---: | ---: |
| E1 channel 2 | +8.201 c / −31.283 c/s | +0.041 c / +0.061 c/s | +2.361 c / −9.455 c/s |
| E2 channel 1 | +1.652 c / −6.995 c/s | +0.0047 c / −0.0207 c/s | +2.020 c / −7.603 c/s |
| E2 channel 2 | +4.523 c / −17.456 c/s | +0.0047 c / −0.0207 c/s | +2.020 c / −7.603 c/s |

Across aggregate and H1 attack/slope errors, the candidate improves all 12
comparisons by 18.278% to 91.291%, but its median reduction is only 33.116%:
below the existing 50% promotion rail. Two complete executions were
byte-identical. The source provides one take per pitch and documents neither
channel identities, guitar/tuning/string nor recording chain; this was also a
descriptive follow-up, not a frozen TRAIN/HOLDOUT experiment. It strengthens
the independent exact-eight direction check, but it neither calibrates the
force/core/decay coefficients nor authorizes enabling the candidate. No
third-party audio is committed.

The default-off candidate is committed at `7807944`; its read-only final
receipt has SHA-256
`1b826e0b5683d7c035cb5bbf8a7e2caea7b0f0470eadcdb8a225f82169071144`.
This is a descriptive compatibility pass against one separately played take
per pickup on a conventional six-string SG. Muted and snap-pizzicato cells
remain inconclusive. The result does not identify the physical cause, provide
matched multi-velocity exact-eight calibration or holdout evidence, include a
user blind verdict, authorize enabling the flag, or establish market
superiority.

### Phase-conditioned repick research

The ranked metal-specific mechanism is the plectrum touching a string
that is already moving. Electry correctly keeps the ringing loop through a
same-string repick, but its short Contact phase still applies one scalar loss
to both loop polarisations at the commuted seam. It is not a local contact at
the pick position. The existing
[repick audit](evaluation.md#repick-contact-mechanism-audit) found that
this loss changes the full E1/E2 state by only about -0.002/-0.004 dB and also
proved an integer-position reciprocal two-port reference. A stronger local
junction failed the public rapid-mute proxy, so topology alone is not a licence
to tune it.

[Evangelista and Smith's passive pluck junction](https://www.dafx.de/paper-archive/2010/DAFx10/EvangelistaSmith_DAFx10_P21.pdf)
supplies the useful integer-position, low-cost structure. Electry's derived
fractional extension uses a signed interpolation vector `w`, loop state `x`
and dimensionless contact amount `0 <= gamma <= 1`. Its normalized adjoint
gather/scatter
`c = w^T x; x' = x - gamma w c / (w^T w)` changes squared state energy by
`-gamma (2 - gamma) c^2 / (w^T w)`, which cannot be positive. This proves the
delay-line Euclidean norm only, not passivity of moving delays and filter
states in the full engine. The extension reduces to the tested two-port at an
integer pick position, requires no allocation or filter bank and runs only
during the existing 0.55-3 ms Contact phase. The current whole-loop
`contactRetention` is not a measured local reflectance and must not be reused
as `gamma`. Repeated application also makes a raw per-sample `gamma` depend on
contact duration and sample rate, so any later experiment must freeze it as a
one-shot total amount or convert a measured rate to per-step amounts.

The physical evidence explains why the missing coefficient matters.
[Pluta, Tokarczyk and Wiciak's robotic re-excitation](https://www.mdpi.com/2076-3417/12/3/1659)
shows that simply adding a fresh initial condition misses pre-release damping,
ringing and lasting spectral change, while a moving one-sided boundary follows
their measured direction more closely. [Brauchler et al.'s measured string and
plectrum motion](https://acta-acustica.edpsciences.org/articles/aacus/pdf/2020/03/aacus200019.pdf)
shows that detachment phase contributes both displacement and velocity to the
two-axis release. A newer robotic
[192-micrometre depth sweep](https://arxiv.org/abs/2606.24356) finds shallow
plateaus or thresholds in several amplitude and spectral features whose depth
depends on plectrum material, but its acoustic six-string rig and remounted
plectra prevent transfer of an absolute depth to Electry.

A default-off dynamic trial now exercises the published one-state spring rather
than another memoryless loss. `ELECTRY_PASSIVE_REPICK_SPRING=ON` computes the
string wave impedance `r = sqrt(T mu)`, reuses the existing 6 kN/m medium-pick
stiffness, chooses the representative impedance-matched `R = 2r` case, and
bilinear-transforms
`rho(s) = (R s + K) / ((R + 2r) s + K)`. It projects one shared state onto the
existing normalized plectrum direction and scatters at integer folded-ring
cells, where the two-rail proof is exact. It runs only on the old ringing state
during a repick; the established release remains the new pick drive, so all ten
isolated evaluator WAVs are byte-identical between OFF and ON. Clearing the
stored spring state at detachment is dissipative. This is a fixed-edge
homogeneous-response ablation, not the paper's driven plectrum trajectory or a
unilateral intermittent collision.

The candidate is rejected. On the frozen 44.1 kHz E1/E2 rapid-Palm grid, median
absolute cell-mean error rose from 2.05031 to 6.93366 dB and median worst-cell
error from 3.48144 to 9.64469 dB. The stateful clean Dead first-to-repick
contextual RMSE rose from 4.81647 to 5.50563 dB. In the frozen 40-hit metal
score, dry RMS moved -0.0668 dB and high-gain RMS +0.0603 dB, but the dry
difference was already -18.95 dB relative to OFF and the high-gain difference
-15.89 dB. The distorted Palm upper-band share fell from 7.5622% to 7.5236%,
moving farther from the 73.0696% public muted-string descriptor; the Dead share
also fell from 8.3287% to 7.8233%. Stability and determinism are necessary, but
this fixed edge damps the wrong residual modes.

A temporary second ablation implemented the paper's more complete
moving-boundary idea. It advanced a prescribed edge along the signed
two-polarisation stroke normal, used the same reciprocal bilinear junction only
while the edge-to-string compression was positive, reset its one state during
temporary separation so chatter could recontact, and latched final release at
the existing 4.8 N / 6 kN/m compression threshold. The edge had then supplied
the stroke, so final detach went directly to Idle rather than injecting the
legacy Release pulse as a second pluck. The trajectory was an engineering
inference from existing force, compliance, position and contact-time
anchors—not a measured exact-eight trajectory. The trial needed only three
floats and no dependency or parameter.

That more physical topology also fails, and by a wider audible margin. All ten
fresh evaluator WAVs remain byte-identical, but rapid-Palm median mean/worst
errors rise from 2.05031/3.48144 to 4.75360/6.07624 dB and the clean Dead
first-to-repick RMSE rises from 4.81647 to 6.88066 dB. In the deterministic
40-hit score, the first hit remains sample-identical and the dry path first
diverges one sample after the second hit begins. Whole-file dry RMS collapses
from -36.1143 to -42.6526 dBFS and high-gain RMS from -30.2860 to
-34.8939 dBFS. Distorted Palm and Dead 30--80/0--30 ms body fall from
-2.3162/-1.7259 dB to -11.2100/-8.1954 dB, opposite the public
+0.87 and +3.12/+1.30 dB direction alarms. The model is deterministic and its
one-state junction remains stable through 384 kHz; the inferred edge motion
and release force nevertheless erase the repick's low body. The failed flag,
state and focused test were removed after recording the hashes and result.

The first falsification gate is therefore a separate commissioned 96 kHz
exact-eight session, not another conventional-guitar fit or a mutation of the
validated 44.1 kHz Palm/Dead capture contract. On one fixed measured rig it
records E1 and E2 at one hard intended force as 64 independent two-stroke pairs
per string. Four cue phases receive sixteen randomized pairs each; every phase
balances eight Down and eight Up second strokes, with the opposite first stroke.
Exactly three disconnected TRAIN and two untouched HOLDOUT performance
clusters use that same rig; other rigs are external-validity tests, not mixed
into its coefficient fit. The operational
[`electry-repick-phase/v1` capture contract](capture/electry-repick-phase-v1/README.md)
freezes the file, sensor, schedule and rights requirements.

The cue phase is only scheduling metadata. A fixed nonmagnetic felt fixture
damps the seven non-target strings while leaving E1 or E2 untouched. A
synchronized contact-onset channel and nonmagnetic two-axis LDV or equivalent
motion reference measure the actual state. The frozen primary phase is
plectrum-normal H1 displacement at contact,
with zero fixed at its positive displacement maximum (positive-real phasor) and
four fixed +/-45-degree quadrants; it is never rotated after decoding.
Tangential H1 phase remains a
recorded covariate. Pickup/gap, gauge, pick geometry, grip and pick point stay
fixed or measured. A magnetic DI is already spatially filtered; the
[electric-guitar measurement comparison](https://pmc.ncbi.nlm.nih.gov/articles/PMC12609871/)
also warns that microphones do not recover an exact string waveform and that
an LDV target can mass-load a thin string.

Before the first scored pair, the protocol, sensor-latency calibration and
analyzer are frozen. After acquisition but before any TRAIN bridge-DI response
is decoded, a first receipt binds all five completed manifests, raw hashes,
blinded integrity records, equations, fixtures, fixed quadrants, shipping
renderer/build/preset/seed and the TRAIN/HOLDOUT split. Its one primary response
is bridge-DI H1-H4 energy gain from joint 48 ms
Hann-weighted fits in `[-55, -7) ms` and `[+7, +55) ms` around measured
contact; 0-12 ms remains a
broadband attack window, not an identifiable E1 complex-partial estimate. Each
string/direction/quadrant must retain at least three eligible pairs. The real
phase effect must first resolve on both strings and directions in all three
TRAIN clusters with mutually aligned centred profiles. Only then may one
default-off fractional junction derive its sample-rate-independent contact
amount entirely on TRAIN. A second receipt freezes that candidate and both
still-unopened HOLDOUT manifests. Exact-once HOLDOUT decoding must replicate
the registered effect and aligned profiles. Promotion additionally requires
at least 50% lower
median absolute real/model cell error in each HOLDOUT with no worse maximum
cell, plus identity, energy, lifecycle, level, tuning and stability rails and
at most 1% median overhead at 96 kHz and 3% worst case at 384 kHz. The permissible
result is narrower than “realistic plectrum”: more accurate phase-dependent
open E1/E2 repicks on the captured rig and protocol.

That `v1` contract preregisters one dimensionless, sample-rate-independent
memoryless contact amount. It cannot be retroactively used to fit either
dynamic `K/R/tau` experiment. A moving-edge capture needs a new versioned
contract with synchronized edge position/velocity or contact force, plus
frozen trajectory and release equations, before any recorded response is
opened.

### Exact-eight pickup-circuit identification gate

Public high-output pickup specifications do not falsify Electry's current
2 kHz/Q1 loaded-humbucker anchor. The
[UIUC complex-impedance programme](https://courses.physics.illinois.edu/phys406/sp2017/406emi_guitar_pickup_results.html)
measured a Seymour Duncan JB specimen at 16.43 kOhm DC, 9.83 H at 1 kHz,
61 pF at 10 kHz and a 5.175 kHz unloaded peak with Q 2.843 and 0.627 MOhm
peak impedance. Its read-only
[workbook](https://courses.physics.illinois.edu/phys406/sp2017/Experimental_Results/Pickup_Data/Guitar_Bass_Pickup_Data.xls)
had SHA-256
`05eae58f51192f47fd1ecaecc26695a4e260cdc5496b5f2cc6943413b96852bd`.
The same workbook's loading sheet contains only Fender single coils, not a
loaded JB or metal eight-string pickup. The accompanying
[measurement method](https://courses.physics.illinois.edu/phys406/sp2017/Lab_Handouts/Electric_Guitar_Pickup_Measurements.pdf)
also shows why one fixed RLC is not identified: pickup inductance and loss are
frequency-dependent. Indeed, `(Rdc + jwL) || C` from those three summary cells
predicts a 6.499 kHz/Q24.43/9.816 MOhm peak rather than the measured one.

The exact-eight summaries close no more of the circuit. Seymour Duncan's
[comparison data](https://www.seymourduncan.com/blog/latest-updates/pickup-comparison-chart)
give the passive Nazgul 8 as 18.76 kOhm/3.85 kHz and Pegasus 8 as
17.31 kOhm/4.00 kHz, without L, C, Q, phase, peak level or test load. A
3.85 kHz peak permits, for example, 5, 10 or 20 H with 342, 171 or 85.5 pF;
adding the same 600 pF cable moves those ideal resonances to 2.319, 1.813 or
1.359 kHz. A family of equally admissible fits to the UIUC JB summaries,
500 kOhm volume/tone controls, a 1 MOhm input and 400--1,200 pF cable spans
about 2.81--1.47 kHz and Q 1.15--1.60. The shipping anchor is therefore not a
measurement of a named pickup, but neither can these public peaks select a
better replacement.

Active exact-eight pickups are a different model class. Fishman's official
[Fluence Modern 8 specifications](https://fishman.com/dp/fluence-open-core-modern-8-string-pickups/)
publish 630/720 Hz Voice-1 peaks, 550 Hz plus 8 kHz or 1.3 kHz Voice-2 peaks,
4.7/4.5 kHz Voice-3 peaks, 2 kOhm output impedance and 25 kOhm controls. Those
labels omit gain, bandwidth, phase and filter order, and the two-peak voice
cannot be inferred as one passive resonator. A 2 kOhm output driving even
1,200 pF has a simple RC corner near 66.3 kHz, so passive cable loading cannot
explain the published voice peaks. An active voice must not be smuggled into
the passive Pickup Type endpoint by retuning one biquad.

Promotion requires an explicitly named exact-eight family and voice. At least
three passive specimens need raw complex `Re/Im Z(f)` under unloaded, exact
harness, selector, measured-cable, interface and tone-position conditions;
active targets instead need magnetic-excitation-to-output transfer and output
impedance versus frequency, level and voice. Registered clean DIs then freeze
pickup height, string set, cable/interface and repeated levels before a
specimen/condition TRAIN/HOLDOUT split. A candidate must halve held-out
complex-transfer error with no worse p95 or maximum, win a blinded matched
reamp comparison, allocate nothing in the callback and add under 1% median
eight-string CPU at 96 kHz. The first implementation is the smallest stable
fit—normally one biquad plus one pole for passive, or at most two biquads per
active voice—not a WDF network by default. No third-party measurement file is
committed.

### Pickup-magnet back-action research

The second-ranked low-CPU direction is the force a pickup magnet applies back
to the string, not another pickup-output waveshaper. Electry already reads two
transverse polarisations through a nonlinear magnetic sensor and gives them a
small fixed split, but its pickup cannot change their restoring stiffness.
[Feinberg and Yang's measured pickup-like field](https://doi.org/10.1121/1.5080465)
changed that stiffness differently in the two axes, split their frequencies
and produced beating; their parameter-free model matched the experiment and
put a commercial-magnet effect near the order of one hertz. The later
[two-axis force/displacement measurements](https://doi.org/10.1121/10.0027283)
show that the split depends on vibration amplitude and magnetic force, so it
can evolve through the note rather than acting as one static detune. The
[official Yamaha/CIRMMT project](https://www.cirmmt.org/en/research/projects/yamaha-rnd_guitar-analysis)
establishes the programme's provenance, and its 2025
[ISMRA abstract](https://ismra2025.org/wp-content/uploads/2025/05/ISMRA-2025-Final-1.3.pdf)
reports pickup-height measurements and a single-recursion realtime model.

The smallest defensible Electry experiment is static, diagonal and first-mode,
not an invented amplitude law. Its first-order Rayleigh reduction evaluates
the signed stiffness `k_a,p = -dF_a/du_a` at static equilibrium. For axis `a`,
sounding length `L`, tension `T` and pickup-row positions `x_p`, use
`r_a = 2 L sum(k_a,p sin^2(pi x_p/L)) / (T pi^2)` and
`f_a = f0 sqrt(1 + r_a)`. Cache the two mode-shape sums at note/pitch geometry
changes, evaluate each axis's existing phase compensation at its corrected
frequency, and feed the two delay targets through their current smoothing.
Stationary notes add no render-loop work; 0.25% of the physical-string engine
is the preregistered CPU ceiling, not a promised result. Summing point rows is
an approximation too and requires target measurements to verify additivity.

The mechanical correction must remain active for both physical magnets even
when the selector culls one electrical pickup path, must not relabel or remove
Electry's existing small polarisation split, and must not add damping. A string
centered over a suitably symmetric field makes the cross-axis terms zero; an
off-centre target would require the measured symmetric 2x2 stiffness matrix.
The necessary scalar-modal condition is `1 + r_a > 0`; the matrix form requires
every eigenvalue of `I + R` to be positive, and neither condition proves
moving-delay passivity. Harazono et al. report negative-stiffness magnitudes
0.8/1.3 N/m for one two-row specimen. Mapping those to Electry's signed
`k = -0.8/-1.3 N/m` convention produces a useful -1.752574-cent arithmetic
fixture in this reduction, but does not calibrate Electry.

No published coefficient transfers to a high-output eight-string humbucker or
its wound .080 string. Promotion requires force-versus-displacement grids in
both axes for the actual pickups, per-string gaps at open and high frets,
synchronised two-axis motion plus dry DI over several velocities, and a
magnet-removed or very-low-pickup control. The evidence supports evolving
stiffness, polarisation split and beating; it does not justify inventing extra
sustain loss. Only after those curves exist should a control-rate amplitude
backbone replace the static first mode; Electry's summed retirement follower
cannot supply it because it loses axis identity.

A third, cheaper measured residual is pickup-localised partial warping.
[Harazono et al.](https://www.jstage.jst.go.jp/article/ast/33/5/33_E1148/_article)
model two humbucker poles as local negative stiffness and reproduce long-period
waveform/envelope motion with pickup-specific fitted values. Rather than add a
modal bank, a future offline solve can project a target pickup-height residual
onto Electry's existing dispersion/allpass fit at note start. No published pole
stiffness transfers to Electry's pickup, and longitudinal/phantom resonators
remain below priority until repeatable clean magnetic-DI sum/difference
partials survive a target high-gain chain.

### Setup-conditioned fretted-intonation research

A premium fretted guitar is close to equal temperament, not mathematically
identical to it. Pressing a real string changes its total length, tension,
linear density and stiffness while nut/saddle compensation only reduces the
residual. [Varieschi and Gower](https://arxiv.org/abs/0906.0127) derive that
coupled geometry and measured one compensated steel sonometer string from
-7.9 to +7.3 cents across its frets. [Anderson and
Beausoleil](https://doi.org/10.1121/10.0026483) separately decompose classical-
guitar intonation into resonant length, density, tension and bending stiffness,
with [open model and experiment data](https://github.com/beausol/classical-guitar).
Those results establish the mechanism; neither instrument supplies coefficients
for Electry's preferred 27.625-inch/.098 extended-range anchor or its
28-inch/.080 endpoint.

Electry currently commands exact nominal 12-TET note targets and ideal
`L = L0 2^(-fret/12)` speaking lengths. Stiffness dispersion therefore bends
partials without giving a fretted fundamental the deterministic residual of one
installed setup. This missing mechanism is not random detune. The first
identification experiment deliberately freezes one real guitar, string set and
setup; it does not infer a continuous Guitar Build/gauge law from that one
instrument. An Anderson-style low-dimensional static model is evaluated offline
into one 8-by-23 table of fundamental cents for each independently measured
setup: 736 bytes per table as floats, or about 4.3 KiB for all six curated build
anchors. Separate measured speaking length, elongation, unit mass, bending
stiffness, load-extension and open-tension inputs identify the physical model;
the DI pitch is its outcome, not enough data to identify those terms by itself.

The existing live-fret control-rate pitch solve, not note allocation alone,
interpolates the table in cents and multiplies the target by
`2^(cents/1200)`, so slides and legato consume the same continuous residual.
The first runtime candidate corrects only the fundamental: ideal speaking-length
pickup geometry remains explicit, rather than pretending that a cents residual
separately identified length and tension. The residual moves only the compensated
total-period target and is explicitly excluded from the nominal construction,
tension and inharmonicity coordinates, which retain their ideal unbent-fret
inputs. It is bypassed away from its measured setup. Production interpolation
across Guitar Build is forbidden until every curated scale/gauge anchor has its
own capture, the interpolation law is frozen on training anchors and one
complete setup is held out. No oscillator, filter, delay read or sample-loop
branch is added; the preregistered 96 kHz eight-string ceiling is 0.25% CPU.

Promotion is capture-gated on the actual target guitars, installed string sets
and setups. Record mechanically repeated raw DI at every string/fret while
logging scale, each saddle setback, nut position, action, relief, fret crown,
gauge and temperature. At each stopped fret, optically measure speaking length
and total extension under a preregistered jig contact position and normal force;
matched string samples supply unit-mass, flexural-rigidity and load-extension
data.
Record the tested string's own open fundamental immediately before and after
each block, with an adjacent open only as a secondary drift check. Freeze jig
acceptance and exclusion rules before capture. Use at least five accepted jig
repetitions for identification and human-fretted repeats for external validity.

Fit the low-dimensional physical model to even-fret pitch outcomes only and
score its predicted odd frets unopened. If that gate passes, refit the same
model on all frets, freeze its table, and reserve a later remove/reinstall
session for final validation. Multi-anchor interpolation additionally leaves
one complete guitar/setup unopened. This makes the odd cells predictions of a
named model rather than fitting a direct lookup and then calling its missing
entries holdout. Existing public DIs do not document or repeat these inputs, so
their pitch offsets cannot identify the table.

The residual itself must repeat above a two-cent practical floor or the feature
is rejected. Every string must show at least 50% lower held-out median absolute
cents error than exact 12-TET, with no worse p95 or maximum and at least 80%
correct residual signs among cells whose repeated residual clears that floor.
Hidden distorted chord voicings must also reduce
inter-string beat-frequency error and win an RMS-matched blind A/B. Open strings
stay exact and alternate string assignments retain their own deterministic
setup. The initial candidate adds only a static unbent offset in cents; it does
not claim that bending leaves tension and stiffness unchanged. Preregistered
low/middle/high-fret physical bends and equivalent wheel/MPE trajectories must
show no pitch-error regression before that approximation may ship, otherwise a
separately identified bent-state correction is required. The 44.1--384 kHz
tuning/stability and CPU ceilings remain hard rails. Until these captures pass,
importing published sonometer or classical-guitar residuals would be invented
realism.

The frozen perceptual gate scores ten real/model cells plus three hidden repeats
with exactly 30 listeners: 15 extended-range guitarists and 15 metal producers.
A separate licensed-leader session compares Electric Storm Deluxe, Shreddage
3.5 Hydra and Uproar RAW. Real parity is not a market win: “best sounding”
requires predeclared superiority. Until those commissioned captures and the
frozen blind comparison pass, Electry claims a research-grounded,
regression-measured model—not capture parity or market leadership.

### Capture-gated moving saddle witness point

Electry currently treats each saddle witness point as fixed. Fretting and
tension gestures move the speaking period, but the bridge endpoint itself does
not move with string amplitude. The separate saddle-rattle path adds quiet
hardware noise, and the support-friction experiment below asks whether a
gesture leaves tension memory; neither represents a contact point travelling
over a rounded saddle crown while the string vibrates.

[Ihsan, van Horssen and Tuwankotta's 2024 moving-boundary
study](https://doi.org/10.1016/j.jsv.2024.118311) supplies a genuinely different
mechanism. For a smooth endpoint obstacle `R_B`, their scalar planar free
string occupies `S(t) < X < L` and satisfies, with linear density `rho`,

```
U_tt - (T / rho) U_XX = 0
U(S(t), t)   = R_B(S(t))
U_X(S(t), t) = R'_B(S(t))
U(L, t)      = H
```

The attachment-detachment coordinate `S(t)` depends on the vibration. With
`s(t) = S(t) / L`, their `x = (X/L - s(t)) / (1 - s(t))` transformation fixes
the spatial interval but makes the boundary problem nonlinear. For small
oscillations around a parabolic obstacle, particular combinations of obstacle
length, far-end height and equilibrium contact give leading-order Dirichlet-,
Neumann- or Robin-type cases. Selected nondimensional cases are numerically
corroborated against direct PDE integration; for longer times the authors
report that only the characteristic-coordinate approximation remains close.
This is not a guitar experiment, saddle measurement or recorded-audio fit.

If a conventional guitar saddle exhibits the same class of contact motion,
`L_eff(t) = L - S(t)` gives the first-order engineering inference
`delta f / f approximately delta S / (L - S_0)` for the paper's ideal free
domain at fixed tension and linear density.
Audio-rate witness motion could therefore produce phase modulation, sidebands
and modal interaction instead of only a static intonation offset. The paper
does not establish that this motion is measurable on a metal electric-guitar
saddle, or that its scalar, one-dimensional, small-oscillation, constant-tension
assumptions transfer, or that its continuously differentiable single-hump
obstacle represents a guitar saddle.
It omits string stiffness, friction, afterlength, bridge admittance, damping and
magnetic pickup observation, and publishes no transferable crown radius or
coefficient. No public exact-eight DI found here records the endpoint closely
enough to infer `S(t)` from sound.

The mechanism must therefore be seen before it is heard into the model. Use one
production-representative eight-string with its actual rounded saddles for five
separately installed/remounted string instances assigned before acquisition:
exactly three TRAIN clusters and two hash-locked HOLDOUT clusters. Start with
open E1 and E2; any fretted extension is a separately preregistered experiment.
Define the settling procedure and log the measured three-dimensional crown
profile and
roughness, saddle material and setback, scale, gauge/core/wrap, winding
orientation, break angle, action, relief, afterlength, tension, string age,
temperature and humidity. Mechanically repeat registered safe release
displacement and force waveforms at registered amplitudes in both transverse
axes with a nonmagnetic fixture. Damp the seven non-target strings and randomise
the complete trial order before capture.

First pass a known-motion optical preflight under the same gauges, crown,
lighting and working distance. Freeze the spatial modulation-transfer function,
scale, frame rate, exposure-blur bound, bandwidth and minimum detectable
contact travel. Then synchronise two orthogonal, backlit, global-shutter
telecentric views, or a validated mirror/stereo equivalent, with untouched
bridge DI and bridge force or acceleration. Register both string centreline
views to rigid saddle fiducials and the measured crown. A common clock plus the
same optical and electrical timing event at the start and end of every take
bounds exposure-midpoint error, trigger jitter, clock drift and analog group
delay. If occlusion prevents the preflight motion from being recovered, the
gate stops.

Before TRAIN opens, freeze lens calibration, the geometric centreline/tangency
estimator that converts the two views to the neck-side `S(t)`, registered
subpixel reconstruction, regions of interest, exclusions and raw-file hashes.
Also freeze a cluster-level amplitude/phase statistic, an autocorrelation-aware
simultaneous uncertainty interval, a static/pinned-string null distribution,
minimum detectable effect and phase/sign tolerance for every primary
string/axis/amplitude cell. Keep all raw frames and channels; no algorithm,
frame, path or timing correction may change after its paired audio is seen.

The optical null keeps the production saddle and observes a pinned string plus
its rigid fiducials. A separate safe clamped witness must demonstrate `S(t)`
below the frozen systematic floor; otherwise it is called only a high-curvature
control. Crown-radius and break-angle interventions are separate, randomized
controls, never claimed to change only one acoustic property. Their actual
static witness, downforce, afterlength and bridge transfer are measured, and
mass, compliance and friction changes remain nuisance variables rather than
evidence for the mechanism.

TRAIN provisionally passes only if every primary E1/E2, release-axis and
amplitude cell resolves non-constant `S(t)` above the frozen cluster-level
effect threshold in all three clusters, with repeatable phase and sign and a
predeclared control-effect size. The reconstructed string must also meet the
paper's contact and tangency conditions within frozen uncertainty, without a
visible gap, bounce or impact; otherwise the observation is different physics.
Failure leaves the fixed endpoint in place regardless of a listening
preference. Both HOLDOUT bundles remain sealed at this stage.

TRAIN alone may then identify one runtime compression hypothesis. Start with a
memoryless map and reject it if matched instantaneous inputs with different
histories predict different `S(t)`; otherwise allow at most two additional
stable bounded scalar states per physical `Voice`, shared by both
polarisations. The input is a causal previous-sample proxy of the existing
local transverse state. Update candidate state only after both axis returns,
then apply one shared physical saddle offset additively to the next sample's two
fractional read delays; do not mutate the pitch target/smoother, create an
algebraic loop or add a second delay read. Express offset and slew bounds in
metres and seconds, then rate-convert them. Freeze reset, repick, steal, refret
and release lifecycles, plus the offset sign and the round-trip conversion from
metres to samples. The first candidate is active-voice-only: sympathetic-only
rendering forces both candidate states and its offset to zero, so the existing
one-axis sympathetic path is not silently assigned an unmeasured rule. Do not
port the paper's PDE solver into the audio thread or add a user parameter.

A zero-input non-regeneration rail disables excitation, artifacts, acoustic
feedback and inter-string bridge injection. Its frozen observable is
`R_W = sum(vertical_return^2 + horizontal_return^2)` before pickup/body
processing over each preregistered window. Matched ON/OFF renders start from
identical frozen line, filter and candidate states; the rail allows stored
candidate energy to return locally but permits no positive late-window slope or
cumulative ON-minus-OFF excess beyond the repeat-measured interpolation
tolerance, and requires bounded eventual decay. A default-off compile-time
experiment must leave the frozen render corpus byte-identical on the same build
and architecture.

Before either HOLDOUT bundle opens, freeze candidate structure, states,
coefficients and initialization; the complete optical/audio estimator; latency
correction; analysis windows; complex demodulation; H1-H12 and non-overlapping
sideband bins; phase wrapping; gain normalization; partial weights; noise-floor
exclusions; aggregation; and equivalence margins. Its normalized scalar error
`E` must include both registered complex harmonic trajectories and sidebands.
The capture manifest records this complete definition and its implementation
hash. `E_fixed` is the error of shipping fixed-endpoint Electry against that
same measured bundle under identical excitation, alignment, normalization and
estimation; it is not the physical high-curvature control.

Each HOLDOUT is then opened once as one synchronized camera/force/DI bundle.
Separately for E1 and E2 in each bundle, the direct TRAIN mechanism criteria and
tangency test must repeat, `E_candidate <= 0.5 * E_fixed`, measured `S(t)` phase
must fall inside its frozen tolerance, and fundamental cents error, T60,
transient error and energy decay must remain inside their equivalence margins.

The same candidate must win whole-file-RMS-matched dry and high-gain blind
comparisons without degrading ordinary soft notes. On one logged machine and
Release compiler, at fixed block sizes 32, 128 and 512 after warmup, at least 24
order-balanced ON/OFF pairs run in every rate/block/path cell. Before timing,
freeze one deterministic MIDI, parameter, seed and render-duration fixture;
report each cell and take the worse block-size median for each of the
one-active/seven-idle and eight-active paths. The relative paired-median runtime
increase must stay below 1% at 96 kHz in each path and below 3% for the worse
path at 384 kHz.

The stability matrix enumerates 44.1, 48, 88.2, 96, 96.001, 192 and 384 kHz;
minimum/maximum candidate state, delay and slew; open E1 through 22nd-fret E4;
registered bend extrema; reset, repick, steal, refret, release and
active-to-sympathetic transitions; and the frozen short/long render durations.
Every cell must remain finite and output-bounded, respect delay/state limits,
pass the zero-input observable and reach its eventual-decay threshold. The 50%
error reduction, equivalence margins, confidence level and CPU budgets are
Electry engineering preregistrations, not results or thresholds from Ihsan et
al. Until every gate succeeds, moving-saddle synthesis remains a falsifiable
lead, not shipping realism or a market-superiority claim.

### Capture-gated support-friction pitch memory

Electry's bend trajectory is currently memoryless. The legacy wheel or resolved
MPE member/master interval is smoothed, added to legato and vibrato, and mapped
directly to `f0`. A key-held voice returns with its controller trajectory; a
released MPE tail deliberately freezes its member interval while its master
remains live. A legacy rendered-audio regression holds an up-bent string's
centred return within 0.75 cent. These are deterministic MIDI contracts, but
none can express a tension imbalance left at a nut, stopped fret or saddle
after a real tension gesture reverses.

[Groves and Kemp](https://journals.pan.pl/Content/112808/PDF/aoa.2019.129261.pdf)
used tuner steps on a one-string rig across a Stratocaster-type saddle and found
capstan-consistent coefficients below 0.26 for wound strings and below 0.17 for
plain strings. They report that sliding almost equalises the two tensions
before friction arrests a smaller residual, and argue from practical afterlength
geometry that the sounding-length pitch consequence is small but non-zero; they
did not measure an actual bend-return trajectory and leave its full treatment
outside the paper. Surface condition strongly affected their result, while the
capstan equation is itself only an approximation for a stiff, elastic string
that slips dynamically. Kemp's
[tremolo-arm experiment](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0184803)
conditioned one non-locking Stratocaster to repeat within two cents by retuning
and repeating any unstable pull-up, so those trials cannot estimate a
greater-than-two-cent return-memory distribution. On a different Washburn,
clamping a locking nut changed one high-E pull-up response by `21 +/- 11%`.
That result establishes nut/afterlength compliance as a control variable, not a
hysteretic-return amplitude. Neither paper's coefficients or measured intervals
transfer to an extended-range guitar, and neither identifies Electry's residual.

The smallest candidate is therefore a measured play operator, not a derivation
of either paper. For each physical string `s`, let `x_s` be one already-smoothed
wheel/MPE tension gesture in semitones, `q_s` its persistent play state,
`r_s > 0` the fitted play half-width and `h_s` a signed fitted branch-correction
amplitude constrained by `abs(h_s) < r_s`:

```
q_s <- clamp(q_s, x_s - r_s, x_s + r_s)
e_s  = clamp((x_s - q_s) / r_s, -1, 1)
y_s  = x_s + h_s e_s
```

Initialising `q_s = x_s` starts this bounded stop operator at exact equilibrium.
It traces a different return branch after reversal and cannot move corrected
pitch more than `100 * abs(h_s)` cents away from the input. The full transition
between branch limits spans `2 r_s`; a smaller excursion need not leave the
full `h_s` correction. The `abs(h_s) < r_s` rail also keeps its interpolation
monotone. This is only an engineering inference to test against complete
trajectories; neither fitted number may be calculated from a published friction
coefficient. The first experiment keeps legato/fret motion and algorithmic
vibrato outside the state. Conventional finger-bend and bridge-actuated
captures are separate strata: legacy-wheel or MPE friction correction stays
disabled unless its corresponding physical gesture passes.

The first candidate admits exactly one gesture class and needs two fitted floats
plus one state float per string: 96 bytes for all eight strings. If both classes
pass separately, two independent states are forbidden because both gestures
move the same physical string; mixed-gesture captures must identify one
composed state law first. The admitted class adds at most eight O(1) scalar
updates at the existing 16-sample control tick; there is no oscillator, filter,
delay read or sample-loop branch. The corrected interval enters the existing
configured-F0 path so tension-derived dispersion and pickup wave-speed
coordinates follow it, while ideal fret length remains unchanged.

For a sounding physical string, `x_s` is the legacy interval or the resolved
key-held MPE member plus master; after Note Off its member freezes and only the
master may keep moving it. Idle member messages do not advance an unassigned
physical state. An ownership or active/sympathetic route change without a
physical gesture translates `q_s` by the simultaneous change in `x_s`, keeping
`e_s` continuous: a fresh equilibrium starts with `q_s = x_s`, while a centred
repick retains a real preceding residual. State survives Note Off and repick,
and is re-anchored to current `x_s` on enable, engine preparation/reset, an
explicit retune, or CC 121 after its controller recenter—not on an ordinary
wheel return or voice retirement. This preserves Reset All Controllers as an
emergency equilibrium command. The operator is cached once per physical
string/control tick outside active-voice updates and shared by played and
sympathetic paths. Tests must cover released-member freeze, live-master motion,
idle-channel reuse, alternate string assignment and active-to-sympathetic-to-
repick handoff without a correction jump. A disabled or zero residual returns
`x_s` before division or state mutation to remain bit-identical. The
preregistered 96 kHz overhead ceiling is 0.25% of the physical-string engine,
not a promised result.

Public recordings cannot fit the operator. The
[EG-IPT corpus](https://doi.org/10.5281/zenodo.15205644) supplies high-resolution
DI bends, but the audited `HB-bridge_bend-up-wholetone_121-14_2s_DI.wav` and
`HB-bridge_bend-down-halftone_162-14_2s_DI.wav` examples finish at their bent
targets rather than pairing a pre-gesture equilibrium with a post-return
reference. The newer
[longitudinal string-ageing dataset](https://www.research.ed.ac.uk/en/publications/a-longitudinal-dataset-for-guitar-string-ageing/)
is an external lead for age dependence only after its fixed-segment timing is
decoded, and cannot calibrate an eight-string support setup. Neither source
freezes the actuator, break geometry, afterlength, lubrication and
locked-support control needed to separate friction memory from player pitch
error.

Promotion instead requires one pre-stretched target guitar/string set and
setup, with scale, gauges and core/wrap construction, nut/fret/saddle break
geometry, both afterlengths, surface treatment, string age and temperature
logged. A calibrated transverse or bridge actuator performs randomised
low/middle/high-fret gestures at several safe amplitudes and rates, with fixed
contact position/normal force and exactly 20 accepted trials per cell. Trial
exclusions, a maximum attempted count and stopping rule are frozen before
capture. A paired locked-support or independently frozen low-memory pitch
trajectory, not raw actuator displacement, supplies the reference `x_s(t)` in
semitones so ordinary bend geometry cannot be fitted as hysteresis. Clamp state
is randomised; score matched achieved cents peaks/rates or preregistered strata,
normalise its change in bend sensitivity, and include a sham clamp plus
individually controlled supports before assigning causality. Track continuous
F0 through the return before making a gentle post-return diagnostic pluck, and
interleave time-matched no-gesture/sham trials to estimate amplitude-dependent
pitch decay, ordinary tuning drift and disturbance by the diagnostic pluck. A
temporary locked or clamped-support block is the causal control; equilibrium is
restored between blocks. Fit only training trials, then hold out complete frets
and rates. A later-day remove/reinstall session changes the contact surface and
is external robustness evidence, not a veto of the captured-setup claim.

On training trials a string is non-zero only when the absolute value of its
post-return signed median exceeds two cents, its trial-level stratified 95%
bootstrap confidence interval excludes zero and at least 80% of trials agree in
sign; otherwise `h_s = 0`. A hidden held-out decision must independently repeat
the floor, interval and sign. The mechanism is rejected entirely if no
metal-relevant wound low string passes both decisions, or if clamping fails to
halve the matched residual or push it below the floor. All promotion metrics
use held-out trials. On every admitted route the candidate must separately
reduce median absolute full-trajectory error and post-return-window error by at
least 50% against the current memoryless model, with no worse p95 or maximum in
either window and the correct reversal sign. Regression rails require exact
disabled identity, finite `r_s` above a frozen positive floor, bounded output,
`e_s = 0` after explicit retune, no drift through 1,000 alternating cycles,
fresh equilibrium attacks unchanged, and stable 44.1--384 kHz/MPE/repick
lifecycles. Only after those objective gates pass does an RMS-matched dry and
high-gain blind A/B run. The permissible claim is the captured setup's
bend-return pitch memory; if a well-set locking target remains inside two cents,
the realistic result is to ship nothing.
