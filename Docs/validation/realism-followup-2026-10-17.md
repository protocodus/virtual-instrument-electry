# Eight-string realism follow-up — 17 October 2026

The development instrument now uses published wound-string reference mass,
the selected stretched-modal fitter, and a continuous stiffness fit when a
hammer-on or slide arrives. The numerical and runtime tradeoffs are recorded
below. This supersedes the current-state
claims in the [initial checkpoint](realism-2026-10-17.md), which remains a
historical record of the first experiments and failed checks.

## Decision and implemented behavior

The user selected B in the original four comparisons, then authorized
autonomous candidate choices. The [decision log](../decisions.md) records
the exact scope. No further subjective approval is required for this work.
That preference is not evidence that a numerical model matches a real guitar.

`ELECTRY_REFERENCE_STRING_MASS` and `ELECTRY_MODAL_DISPERSION_FIT` default ON in
CMake and source-only builds.
The measured light reference is the named XL .024/.032/.042/.060/.080 wound
construction; plain-string mass and empirical axial/bending core estimates
remain separate. The existing heavier-gauge geometric-similarity assumption
is explicit. This is not identification of a particular .098 string.
Both mass and modal options are emitted as explicit 0/1 definitions on normal
and embedded C++17 targets, so OFF remains an effective legacy comparator.
Evaluation manifests now record both flags and validate them against CMake.

Legato correction `7b76928` replaces the endpoint's invalid-coordinate
sentinels with an explicit exact-geometry refit. The final fret still refits
damping and dispersion, while the valid preceding coordinate preserves the
bounded continuation search and existing phase-history translation. No new
excitation, voice ownership action, parameter, allocation or voiced constant
is introduced. Fresh/discrete geometry still uses its existing global search.

The new regression exercises ascending/descending Hammer and Slide on forced
physical E2 at 48/96/384 kHz. It checks exact final damping/fitted fret,
the existing coefficient-neighborhood bound, and finite stereo samples.
Unchanged shipping fails two of twelve endpoint cases; the initial modal
candidate fails eleven, reaching a coefficient jump of 0.806828. The corrected
shipping and modal builds both pass twelve. Reported waveform steps are
diagnostics, not a calibrated audibility threshold.

The combined suite also exposed a real no-op refit defect in the sympathetic
path at 48 kHz host rate: `(currentDelay + previousPhase) - samePhase` rounded
2043.8251953125 to 2043.8250732421875, one float ULP, while the target and
filter histories stayed unchanged. Computing the phase difference before
adding it to the delay makes an unchanged refit exactly idempotent. The test
now requires exact delay equality, strengthening its former `1e-4` bound;
material-state retention and fresh-handoff/reset checks remain intact. This
is a numerical history correction, not a claimed audible one-ULP effect.

## Diagnosed tests and current validation

The initial eight mass-candidate assertions were not eight physical defects:

- Float `exp2(x)-1` lost `4.56e-8` at the six-cent cap, beyond the existing
  `2e-8` conversion tolerance. The test now derives the bound with double
  `expm1` and retains that tolerance.
- E2's full-force raw seed crosses the unchanged cap with reference mass.
  The test now checks the force law on raw seeds for E1/B1/E2 and checks every
  committed value against `min(seed, cap)`, requiring both capped and uncapped
  cases. No velocity or pitch rail is relaxed.
- A raw delay is only part of the string's period. At one observed vibrato
  state it grew while filter phase shortened more; the complete frozen loop
  sharpened by 0.27343 cents. The fixture now independently solves the actual
  cubic read, allpasses and loss-filter phase; original onset, depth,
  centering and rate rails remain intact.
- The final three assertions were complete-source spectral snapshots. The
  chosen mass change alters upstream string/excitation behavior. Only those
  three centers are refreshed for mass ON/modal OFF: partial 16 of A2 and E2
  single-coil 4–8/8–16 kHz bands. Previous flag branches, tolerances and
  independent coil-topology/contrast checks remain unchanged.

Two further modal-only failures came from a harmonic-sum pitch estimator.
It searched H1–H5 on harmonic bins, so stiffness-induced stretch biased E1's
reported tuning to +8 cents while its H1 peak was +3.75 cents. The fixture now
uses the same H1 scan already used for chords and muted styles, retaining the
original 8-cent absolute and 6-cent spread limits. The combined model's actual
H1 peaks span +1.25 to +4 cents, spread 2.75 cents. A separate frozen-loop
oracle tracks the intended attack pitch within 0.000087 cents. Those phase
roots omit polarisation coupling; the H1 scan measures actual rendered audio.

The old two-point relative-deficit check enforced the former fitter's
surrogate objective. Its modal-OFF branch is retained verbatim. Modal ON now
has an independent complex-transfer root oracle for every H2–H16 in its fixed
heavy open-E1/25.5-inch/48-kHz fixture: an explicit 5-cent numerical budget,
plus 0.1-cent actual cubic/loss-loop fundamental compensation. These are
engineering approximation budgets, not guitar calibration or listening
thresholds. Mutation witnesses reject bypassed dispersion, half the required
filter order and missing fundamental compensation.

The exact combined render has five stale whole-source centers, refreshed
from that combination rather than adding isolated dB changes. The isolated
modal comparator retains its own four affected band centers. No spectral
tolerance or pickup-contrast rail changes. The maximum-rate Drop-E resource
test now checks the actual 8192-sample capacity boundary instead of an
arbitrary 9000-sample period split: the new model needs about 8960 raw samples
while the full period exceeds 10400. An 8192-sample production line still
fails after its clamp. The 10-cent rendered-pitch and actual-capacity guards
remain unchanged.

Mass-only Engine and ReferenceStringMass suites pass. A fresh default build
passes all 32 unit suites after one metadata-only repair: the first run used
an evaluation executable built before the two new manifest fields were added;
reconfigure/rebuild and the evaluation retest pass. The audio tier also passes
its 23 serial/parallel technique comparisons. Embedded C++17 compilation with
immutable tables, no exceptions, RTTI or builtins passes. Exact test logs,
including the failed metadata run, remain in the working evidence directory.

The endpoint correction received an independent source review. Review found
and repaired a test issue: checking only a maximum-step accumulator could miss
NaNs. The final fixture checks left and right samples directly.

## Further numerical findings

**Restricted dispersion topology.** The modal grid improves worst isolated
phase-resonance error but slightly worsens the median and costs more CPU.
Caching the shared frequencies' trigonometry reduces the extra work without
changing any of 864 observed rows, independently verified with mass OFF and
ON. A lexicographic early-exit trial also preserved coefficients but slowed
moving bends; it is removed. The final cache changes neither objective nor
grid. Frozen trial sources/results remain in the evidence archive.

With the selected mass law held ON, the 864-state/12,960-upper-mode sweep gives:

| Isolated vertical-loop quantity | Legacy fitter | Selected modal fitter |
| --- | ---: | ---: |
| Maximum absolute phase-resonance error | 196.496 cents | 103.036 cents |
| Median absolute phase-resonance error | 2.394 cents | 3.684 cents |
| Maximum isolated lossless dispersion error | 199.474 cents | 100.474 cents |

The modal configuration's maximum fundamental phase error is 0.000182 cents.
It is not uniformly better in every mode or cell. Both target and realization
retain the same empirical core/stiffness model; this is numerical accuracy
against that law, not measured-guitar likeness.

The final same-source 00/10/11 benchmark explicitly sets mass/modal OFF/OFF,
ON/OFF and ON/ON, with the same endpoint correction in all cohorts. Three
paired rounds reverse the middle ordering; every workload has 32 warmup and
512 measured iterations at 48/96/384 kHz. Both wall time and thread CPU are
retained. Median thread-CPU cost of ON/ON versus OFF/OFF rises 9.25/11.67/17.03%
for cold eight-string chords and 15.44/15.02/12.24% for moving eight-string
bends. Combined moving thread-p95 is 2.605/1.368/0.446 ms for 256 frames.
These are native diagnostic workloads, without Double, amp/FX or a Rack VM;
they do not certify a complete host callback budget. The old unexplained
29.567-ms wall outlier remains in the initial evidence, not erased by the
new paired measurement.

An offline complex-conjugate allpass topology is a stronger architectural
direction: at the same total order eight, 27 static cells have a maximum
integer-mode error of 4.286 cents, versus a searched real-pole plateau of
40.507 cents in the default heavy fret-22 example. At the extreme `B=.003`,
four pairs still reach 10.490 cents and the fractional-order guard reaches
27.102 cents. These are different finite probes, not a global lower bound.
The [feasibility report](../research/complex-dispersion-feasibility-2026-10-17.md)
retains the exact target, ideal-delay boundary, poles, search and limitations.
No complex-pole runtime filter or interpolation table is installed.

**Decay mapping.** `AuditModalDecay.py` solves the actual complex poles of the
isolated vertical loop for the original mass-OFF observations. Across 864
states and 13,824 poles, the largest root residual is `5.40e-12`. The minimum
actual T60 divided by the fundamental-period mapping is 0.70755; median is
0.97339. At short heavy E1/fret22/-2 semitones/44.1 kHz, H16 has T60
0.35047 s versus the nominal-period 0.49533 s. The group-delay approximation
agrees with the exact pole T60 within `8.19e-6` relative across this probe.
[Smith's waveguide theory](https://www.dsprelated.com/freebooks/pasp/Digital_Waveguide_Theory.html)
distinguishes the phase delay governing tuning from group delay governing
envelope propagation. A second independent polynomial-root witness agrees
with four Newton poles within `2.6e-14` in z. This quantifies an approximation;
it does not identify new real-guitar decay targets or justify silently
recalibrating the existing voiced loss law.

**Dispersive pickup sampling.** A six-state geometry-only E1 probe compares
the current temporal taps' `k=2*pi*f_n/c0` with stiff uniform-string standing
modes `k=n*pi/L`, retaining the same sensor locations, aperture, spacing and
weights. At heavy fret22, bridge H11 differs by +14.18 dB; heavy fret12 neck
H16 differs by -6.67 dB. Opens differ by at most approximately 2.15 dB.
This is conditional spatial transfer before flux/loading, using continuous
aperture/delay equivalents; it excludes discrete interpolation and is not an
audio or recorded-guitar error. A scalar period retune cannot correct every
dispersive mode. Distributed allpass propagation is a concrete next experiment.

These modal and decay oracles observe the isolated vertical loop, omitting
the horizontal loop's detuning and nonzero polarisation coupling. They do not
give the complete static string's poles. Contact/attack motion, coupling to
other strings, pickup/body and effects add further boundaries.

## Reproducible listening and evidence

The original four-pair `build-realism-review/` is frozen. A separate
`build-legato-arrival-review/` contains two 13-second dry/Modern-amp pairs at
96 kHz/64 frames, seed `0x160c2026`, through the production Performer. A is
unchanged `93f2341` (both research flags OFF); B applies only the DSP diff of
`7b76928`. It contains ascending/descending E2 hammers and E1 octave slides.
Source archives, driver/compiler hashes, exact events, float WAVs and a
separate key are retained. Each take uses one constant whole-file RMS gain
to -24 dBFS. Trims are +16.4366/+16.6221 dB dry and +9.4788/+9.6418 dB amp.
The four raw stereo files are finite and have exactly 1,248,000 frames each;
all listening page/WAV endpoints were verified over local HTTP.

New evidence is in `build-legato-arrival-review/evidence/`, including the
original failing/passing transition runs, mass/modal follow-up failures,
decay poles, frozen spatial oracle and the complex-topology provenance archive.
The independent audits are reproducible from saved observations. NumPy/SciPy
are optional offline research/listening dependencies; the instrument and
mandatory Python audit tests do not require them. No third-party performance
audio or licensed SDK content is included.

This work is local development version 1.2.2. It does not include a plugin or
Rack release build, host/Recon playback, a Rack submodule pin, publication or
merge. Native unit/audio checks establish their declared contracts, not
recorded-guitar parity or a comprehensive perceptual realism result.
