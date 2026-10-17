# Electry development checkpoints

Per-change checkpoint notes moved out of the README, newest first. The [README](../../README.md) carries the concise version history.

## 2026-09-08 faster effects and oversampling choice

- Replaced repeated pedal Newton iterations with a rate-specific prepared
  inverse and accelerated the American/British PI scalar functions with
  derivative-consistent cubic tables. The pedal capacitor history, coupled PI
  equations, convergence tolerance and direct-math fallbacks remain intact.
- Added Standard/High FX Oversampling as appended host parameter 29 and a
  keyboard-accessible selector in the FX header. New instances and factory
  rigs use Standard; older saved states without the field migrate to High.
  Standard retires one stage and accepts the measured high-drive aliasing
  tradeoff; High keeps the original adaptive rate policy and alias target.
- Prepared both gain banks before rendering and crossfaded live changes over
  about 20 ms while retaining shared compressor, delay and room histories.
  The FX benchmark now selects either mode, accepts actual modeled-guitar
  input and reports process CPU alongside elapsed time. The integrated
  Standard complete chains saved about 63–69% FX CPU on the measured M1 Max
  guitar workload; High saved about 36–43% with tiny numerical differences.

## 2026-09-05 string precision and retained hand state

Fractional string reads and pick-image writes now preserve their exact delay
fraction independently of the circular-buffer cursor. Ringing repicks retain
the saddle resonator's history, and Palm-held hammer-ons, pull-offs and slides
retain the planted hand's attack absorption as well as its decay. These correct
existing numerical and physical-state inconsistencies without adding controls
or changing fitted constants.

The [September research and validation note](research-realism-2026-09.md)
reviews primary work through August 2026, records the focused before/after
regressions and interpolation spectrum, and identifies the remaining high-band
amp-aliasing and controlled-capture limits. Generated listening comparisons and
logs are in `build-realism-20260905/`. Neither those comparisons nor passing
engineering tests establish competitive realism superiority.

## 2026-09-01 Palm spatial-contact experiments (rejected pre-source)

- Tested two reversible bridge-hand spatial models before opening any selected
  corpus audio. A three-junction folded-waveguide candidate failed eleven local
  production rails, including uncompensated `+12.5 cent` pitch error; its
  proposed benchmark harness also failed an independent fail-closed audit.
- Replaced that attempt with an exactly energy-accounting, phase-compensated
  one-delay Mach-Zehnder loss cell. Its focused passivity and lifecycle checks
  passed, but unchanged musical rails rejected it: late Palm decay, the
  Open-to-half pressure contraction and E2 upper-mode loss all missed their
  bounds. The point model's unity nodes and weak near-bridge low-mode overlap
  explain that string-dependent failure pattern.
- Both candidate implementations were removed exactly and the shipping Palm
  path was restored. No selected WAV, high-gain render or real/model benchmark
  was opened. The negative results and evidence boundaries are in the
  [evaluation contract](evaluation.md#pre-source-rejection-of-the-distributed-palm-junction-experiment).

## 2026-09-01 EG-IPT artificial-harmonic coordinate audit (source-inconclusive)

- Froze a fail-closed comparison against the CC BY 4.0
  [EG-IPT corpus](https://doi.org/10.5281/zenodo.15205644) before downloading
  any selected member. The single tested mechanism kept the plectrum at the
  shipping coordinate and moved only the following touch by one shared signed
  fixed-metre offset; an equal-complexity control moved pick and touch together.
  Neither branch added voice state, sample-loop work, a control or a dependency.
- The final PLAN and pre-decode intake SHA-256 hashes are
  `2c8e5984fe419391005f0f7c67b4094ebf92e921be55fd3402f03a3f545b2d91`
  and `139b018c5d320b621b0a40e92e118b9e8174d1c93000f2814256b9aa9dfdf507`.
  They bind the exact 30/30 selection/holdout split, broad source-only YIN,
  source observability, raw and shared-high-gain gates, leave-one-pickup-out
  transfer, engineering identity/oracle tests and a non-vetoing paired
  dynamic-microphone direction report. All synthetic suites and both frozen
  source manifests passed before source access.
- The sealed extractor admitted all 30 selection `ordinario` DI members; its
  source receipt is
  `a86e9df42e71086dd48dd3ace12e14d0481dc178d9f1cb526150a01dcf4e200a`.
  The first source-only pitch/onset pass then stopped on
  `HB-neck_ordinario_02-06_5s_DI.wav`: centred onset frame 1872 at 96 kHz is
  19.5 ms, 48 frames short of the frozen 20 ms pre-roll requirement. The
  immutable inconclusive result is
  `d991777cf1912b148de532b990603114a5f7d035ba353a7a916c48f95aa095d5`.
- No pitch unseal was written. Therefore no artificial-harmonic, holdout or
  microphone member was opened; no real/model partial spectrum, nuisance fit,
  class/distance selection, high-gain score, engineering candidate build or
  A/B was run. The source does not reject the coordinate mechanism, but it
  supplies no admissible evidence to change shipping. The source branch remains
  absent and the co-located Pinch law is unchanged. Full provenance and failure
  semantics are in the
  [evaluation contract](evaluation.md#source-inconclusive-eg-ipt-artificial-harmonic-coordinate-audit).

## 2026-09-01 Guitar-TECHS pinch-offset intake (aborted)

- Tested one reversible hypothesis for Pinch: retain the existing pick point,
  but place the following thumb at one shared signed physical offset before
  fret stretch. This removes the ideal co-located
  `sin(n pi p) cos(n pi p)` constraint without adding state, per-sample work, a
  user control or a dependency.
- Froze a CC BY 4.0 Guitar-TECHS protocol before partial analysis. The original
  low-E/A/D surface stopped because four P2 pinch slots exceeded the 0.1%
  clipping ceiling. A frozen low-E-only follow-up then stopped at intake: P1
  ordinary low-E fret 1 measured 15.976619 dB onset SNR against the required
  20 dB, with no clipped frame. A strong precursor in the last 25 ms of its
  noise window—not primarily the preceding open-note tail—caused the failure.
- The fail-closed analyzer never computed a real/model partial spectrum, fit a
  pick position or thumb distance, opened an odd-fret holdout, ran the fixed
  high-gain comparison or prepared an A/B. Its independently audited SHA-256 is
  `12526ec4928cb08763e86ee3f1843a340302879b5bd9a12017b061f41bc0d54e`;
  the deterministic rejection result is
  `bd9370e6b96f3562bae3e89f58c045aae670c96b8588f50914ce765a727c6cdc`.
- A source-only follow-up confirmed more P1 intake failures, but also caught a
  near-four-second modulo-wrap error that shifts P2 ordinary by one slot. Those
  P2 counts are not used; the analyzer's first P1 failure and the decision are
  unaffected. The full diagnostic is disclosed in the evaluation contract.
- The source therefore did not falsify the mechanism, but it supplied no
  admissible evidence to ship it. The temporary macro and branch were removed.
  Focused engine/evaluation tests pass, and restored dry/high-gain metal renders
  retain their frozen hashes. The full intake and provenance record is in the
  [evaluation contract](evaluation.md#aborted-guitar-techs-fixed-metre-pinch-offset-audit).

## 2026-09-01 measured Modern 6L6 output-stage ablation (rejected)

- Added one temporary default-off branch after Modern's second triode. It
  reused the existing measured 6L6-family lookup with zero common drive, the
  legacy droop as rail scale and plate-plus-screen current demand as input to
  the unchanged sag follower; the existing transformer followed. No solver,
  state, constant or dependency was added.
- Froze `/private/tmp/electry-modern-6l6-20260901/PLAN.json` before candidate
  compilation or audio (SHA-256
  `c339c132a0d69856d2c6502edd4b0a20ac69d4ec6f50d5f6dd3eaf73c0c26f17`).
  The external `analyze_paired.py`, `PAIRED_RESULT.json` and candidate
  `process-6l6` SHA-256 hashes are respectively
  `e49aa5d5a487b9d405c7cacfc37dcb218daabbf3e2aec5bbf1a75c96fbcb2a24`,
  `f1eb99929666d28fcf9c8fc3fc2909526947d99ade3891a5820acd3a66101924`
  and `5ddd20d8f95ffb2a962b08de0ced2be876706fb0d60c4f1e705d08ed123efd0b`;
  all live beside the plan and are not committed inputs.
- On the frozen EG-IPT grid, median envelope error worsened
  6.2476765 -> 8.3466921 dB (`-33.5967%` improvement), spectral error
  17.4182643 -> 18.1519104 dB and crest error
  3.1850492 -> 3.4549171 dB. Candidate-minus-baseline cell RMS was
  -6.9449..-5.7324 dB. Only finiteness and OFF identity passed; the median,
  pair, per-cell and 1 dB level gates failed. Because the different named
  EVH/Mesa chain is direction-only, this rejected the candidate before CPU,
  A/B or full test-matrix work. The temporary flag and code were removed and
  shipping Modern is unchanged.

## 2026-09-01 paired EG-IPT Modern-cabinet audit (rejected)

- Tested the default-off phase-preserved CC0 SM57/V30 impulse against exact
  same-performance EG-IPT bridge `ordinario` and `muted` DI/dynamic-microphone
  pairs. The target chain is a Gibson SG humbucker through an EVH 5150 III 50W
  6L6, Mesa 4x12 V30 and SM57, so this is a whole-chain direction check, not an
  isolated cabinet fit or a validation of Electry's E1 string.
- Froze the protocol before inspecting candidate audio at
  `/private/tmp/electry-fx-paired-20260901/PLAN.json` (SHA-256
  `7f1b86824841831c259ca94fb1dfc566616438a1b5298865e4e892121a38036b`).
  The external analyzer is
  `/private/tmp/electry-fx-paired-20260901/analyze.py` (SHA-256
  `d52547c21cdacca2fdcd76ceca8fca0e9ddccfb73911b135a3be287d524d730f`)
  and deterministic result is
  `/private/tmp/electry-fx-paired-20260901/RESULT.json` (SHA-256
  `1aad021e713a4d027900de488a0f332133d9f57ea0b23f6bf2148f2e677f24cc`);
  none is a committed repository input.
- Normalized spectral-shape median/max error moved from
  17.418264/19.285118 to 16.255076/17.830980 dB. All eight pair/gain cells and
  both pair medians improved, but 6.677982% missed the frozen 10% median gate.
  Relative-envelope median/max error moved from 6.247677/12.160929 to
  4.965523/9.206289 dB, yet all four `ordinario` cells worsened by 0.78-1.09 dB
  while `muted` improved, breaching the 0.5 dB per-cell rail. The candidate was
  rejected without an A/B and remains default-off; the full protocol and target
  trajectories are in the
  [evaluation contract](evaluation.md#historical-cc0-modern-cabinet-audit).

## 2026-08-31 moving unilateral plectrum contact experiment (rejected)

- Built a temporary moving-boundary falsification on the proven integer
  two-rail/folded-ring junction. A signed prescribed edge, unilateral
  separation/recontact and force-threshold detach replaced the repick's legacy
  release without adding a dependency, control or more than three floats.
- The temporary harness covered fresh-attack exclusion, positive finite
  trajectory state, stable DC/Nyquist filter invariants, final detach to Idle
  with cleared contact state, finite output and 44.1/96/384 kHz lifecycles. Ten
  fresh evaluator WAVs and the first metal hit remained byte-identical; two
  complete metal renders were byte-identical to each other.
- Rejected the inferred trajectory: rapid-Palm median mean/worst error rose
  from 2.05031/3.48144 to 4.75360/6.07624 dB, clean Dead contextual RMSE rose
  from 4.81647 to 6.88066 dB, and repeated-score dry/high-gain RMS fell
  6.5383/4.6079 dB. A v2 capture must observe edge motion or force; the public
  previews cannot identify it. The failed compile flag, state and test were
  removed; only the auditable negative result remains.

## 2026-08-31 passive dynamic repick spring experiment (rejected)

- Added the default-off `ELECTRY_PASSIVE_REPICK_SPRING` falsification build:
  one Evangelista-Smith bilinear spring state projects onto the existing
  plectrum normal and scatters the already-ringing response at exact integer
  two-rail/folded-ring cells. Fresh isolated attacks remain byte-identical and
  shipping stays OFF.
- Pinned production coefficient contractivity at 44.1, 96 and 384 kHz, exact
  dynamic two-rail/folded-ring equivalence, the circular-history offsets,
  first-round-trip repicks, fresh strum pre-roll reservations, and release
  cleanup. The minimal one-state path adds no allocation or dependency.
- Rejected the fixed-edge candidate: rapid-Palm median mean/worst error rose
  from 2.05031/3.48144 to 6.93366/9.64469 dB and stateful clean Dead contextual
  RMSE rose from 4.81647 to 5.50563 dB. Its distorted Palm/Dead upper-band
  shares also moved farther from the public direction alarms. A measured
  moving/unilateral edge needs a new capture preregistration rather than tuning
  this passive topology against the previews.

## 2026-08-31 fitted pickup flux-linkage experiment and metal benchmark

- Added `ElectryRenderEvaluation --metal-benchmark`: one deterministic 40-hit
  E1/E2 Palm/Dead performance now writes unnormalized mono float32 taps before
  FX and after the fixed Modern high-gain chain, plus a machine-readable
  protocol, parameter and build-feature manifest. Source and binary provenance
  remain external. The evaluation contract renders it twice and requires
  byte-identical audio and metadata, while the established ten dry probes remain
  unchanged.
- Implemented the nonlinear shape of Novak et al.'s measurement-fitted DAFx-18
  flux-linkage proxy behind the default-off `ELECTRY_MEASURED_PICKUP_FLUX`
  flag. Absolute fitted gain is discarded: the precomputed 257-point table is
  unity-slope normalized between the paper's SH-2N humbucker and SSL-5
  single-coil fits; the render loop interpolates it, preserves the established
  neck/bridge displacement calibration, then uses the existing Faraday
  derivative and loaded pickup circuit. Equation-6-after-bound,
  calibration-wrapper, zero, interpolation and hostile-input tests cover the
  isolated experiment.
- Benchmarked open E2 against all five Clean and RAT pickup selections in the
  CC-BY EGFxSet corpus using only within-file, scale-invariant early spectrum,
  envelope, harmonicity and decay descriptors. Against the closest Clean Bridge
  row, the candidate moves centroid, upper-band level and the output-decay
  descriptor toward the recording but makes crest, early body and onset
  harmonicity worse. In the fixed metal score it changes full-file RMS by only
  +0.166 dB dry and +0.052 dB
  wet; rapid-Palm upper-band share rises by 0.149 percentage point while
  harmonicity falls by 0.0006.
- Rejected promotion. The flag-on engine still violates 32 current-engine
  regression/compatibility rails. Single local arm64 Release probes observed
  roughly 25-45% more 96 kHz eight-string render time; that is diagnostic, not a
  portable CPU estimate. Flag off is byte-identical to the shipping engine. The
  complete method, reference caveats and numbers are in
  [the evaluation contract](evaluation.md#paired-metal-render-and-fitted-pickup-flux-linkage-experiment).

## 2026-08-31 unilateral following-fret scatter and coupling audit (default off)

- Replaced the earlier default-off one-read positioned-loss surrogate with the
  rigid unilateral junction from Evangelista and Eckerholm. At the
  equal-tempered following fret, the single signed ring supplies the two
  incident travelling waves at integer paired cells. A hit applies the same
  displacement correction to both physical outgoing waves, leaves their
  differential unchanged and changes squared-wave energy by
  `(clearance^2 - displacement^2) / 2 <= 0`. The inactive side is exact
  identity. No state, dependency or fitted coefficient was added.
- Fixed a timing error exposed by maximum-force E1/E2 Palm tests: the local wave
  reached the following fret approximately 6-12 ms before the seam-derived output-energy
  follower woke. The brief contact opportunity now begins when either actual
  incident cell becomes nonzero. That restores production branch reach without
  changing the established 0.96-2.08 mm clearance, its opening envelope or the
  attack signal.
- Focused regressions pin port reciprocity, exact signed-barrier projection,
  energy nonincrease for both barrier orientations, valid cells from
  44.1-384 kHz including 96.001 kHz, bend tracking, fret-22 null behavior and
  maximum eight-string stability. A one-period modal fixture leaves H18 at
  `0.997882` gain while reducing near-antinode H9 to `0.016630`; the complete ON
  and OFF DSP suites pass.
- Promotion is rejected. At the production Metal setting (`Artifacts = 0.15`),
  the frozen 40-hit E1/E2 Palm/Dead dry and high-gain WAVs are byte-identical
  ON/OFF (`69e1cd...` and `ef32b6...`), as are all six Palm and both Dead v3
  probes. Only Open E1/E2 change, by `+0.0123/+0.0063 dB` full-file RMS, with
  difference signals `-51.66/-57.67 dB` relative to OFF. The model therefore
  cannot improve any real-reference distance on the registered metal score.
  Lowering clearance merely to manufacture exposure would be an unsupported
  fit; the physically valid scaffold remains compiled out pending measured
  action/relief and a controlled collision capture.
- A parallel no-fit audit left pickup back-action and fret-specific neck
  mobility unimplemented. Harazono et al.'s `0.8/1.3 N/m` negative stiffnesses
  fit one commercial specimen; Yamaha's later two-axis work establishes
  amplitude-dependent signed splitting but publishes no transferable curves.
  Yudasaka et al.'s 175 Hz neck result belongs to one Yamaha Pacifica and does
  not publish the complex residues needed by Electry's target eight-string.
  Those mechanisms remain measurement plans, not new arbitrary resonators or
  detunes.

## 2026-08-30 true 6 ms string-delay glide

- Played and sympathetic string delays advance their glide once per internal
  audio sample, but `prepare()` had included the 16-sample control period in
  that per-sample coefficient. The declared 6 ms time constant was therefore
  0.375 ms at every rate, making bend, legato, refret and coupled-string retune
  transitions nearly snap to their new pitch. The coefficient now uses one
  internal sample while the existing event/control target cadence is unchanged.
- A coefficient oracle covers 44.1, 48, 88.2, 96, 96.001, 192 and 384 kHz.
  Actual played and sympathetic step responses at 44.1, 96, 96.001 and 384 kHz
  must retain `e^-1` after one 6 ms time constant. The target-anchored
  recurrence plus a control-rate equality guard reaches the exact float target
  instead of stalling as far as 0.24 cent away at the highest rate. An
  inaudible ready sympathetic loop also snaps before a later drive can wake it
  at stale pitch.
- Fresh/static-note audio and the final target/filter laws are unchanged;
  transition phase and energy deliberately change. The audio loop retains the
  same two multiply-add recurrences per played voice and one per rendered
  sympathetic string. Only a cheap control-tick residue guard is added, with
  no allocation or new audio-rate branch.
- The redirected descending Slide in demo 17 now distinguishes the finger
  reaching its target from the physical delay finishing its glide. Its test
  follows both phases, requires monotone effective pitch across dispersion
  refits, bounds arrival lag below 35 cents (28.73 cents realised) and permits
  up to 50 ms for the final two-cent settling rail. The score itself leaves
  649 ms before vibrato begins.
- All nine default JUCE-free tests pass, as do complete engine runs with the
  default-off fitted-loss and positioned-fret experiments enabled. The rejected
  energy-derived attack-pitch experiment retains its two known Slide failures
  and now trips eight spectral validation rails because its excursion
  persists through the real glide; it remains compiled out of shipping builds.
  Two full same-compiler renders are byte-identical. All 23 normalised demo WAVs
  change against the old coefficient without a score or parameter change, so
  the nightly renderer will refresh the committed generated artifacts.

## 2026-08-30 complete large-jump string choke

- A retriggered string whose delay jumped by more than 25% choked both 16,384-
  sample waveguide lines to 28% amplitude, but left the one-pole damping, eight
  dispersion allpasses and active hand-loss dip at full state. Those memories
  then re-entered the newly retuned loop as an old-pitch discontinuity. The
  whole linear ringing state now receives the same amplitude scale; the
  default-off fitted-loss dip follows it when that experiment is compiled.
- The same `PolarisationLoop::scaleState()` replaces the existing hand-written
  22% sympathetic-to-played choke. It preserves phase, indices, coefficients,
  pickup/Faraday history and downstream body state instead of resetting a
  filter into a new transient. The large-jump path applies it to both
  polarisations before snapping their delays and before the next excitation.
- The regression rings lowest-string fret 2 under continuous bridge-hand
  pressure, retargets it one octave to fret 14 with contact delayed one sample,
  and pins both line energies to `0.28^2` plus every populated damping,
  dispersion and dip memory to exactly `0.28`. The unmodified line-only choke
  fails 22 assertions; the shipping and order-two builds pass, including both
  fitted-loss biquad memories. The attack-pitch experiment adds no failure
  beyond its two existing mid-Slide candidate fixtures.
- All nine JUCE-free tests pass. Two independent reviews found no omitted
  causal loop state or ordering issue. The correction adds no allocation,
  memory or sample-loop work: a large steal adds 22 scalar multiplies (26 with
  fitted loss) beside the 32,768 line samples it already scales.
- Two same-compiler full-render pairs are byte-deterministic. Exactly demos 02,
  04, 05, 07, 09, 11, 12, 13, 17, 18, 19, 20 and 23 change because they reach
  the corrected ownership/pitch-jump path; the other ten WAVs are
  byte-identical. No parameter, preset, score or coefficient changed.

## 2026-08-30 matched factory-rig popup

- Opening the RIG selector inherited JUCE's unrelated blue-grey menu and cyan
  highlight even though the closed selector uses Electry's dark control face,
  ivory text and warm selection language. The shared look and feel now gives
  that popup the same dark face and ivory text, plus Electry's established
  bright-accent/dark-text selection pair; menu behaviour and dimensions are
  unchanged.
- A renderer-level regression checks the real popup background and highlighted
  item pixels, their relationship to the closed ComboBox palette and at least
  3:1 contrast between idle and selected backgrounds. The normal 1080x860
  editor render is unchanged because the popup is transient; audio, parameters,
  layout and steady CPU are untouched.

## 2026-08-30 performed-pitch held damping

- Played-string loss is applied once per round trip, but held voices kept
  solving that loss from the written `baseFrequency` and destination fret. A
  two-semitone bend therefore shortened the same target decay from 21.456765 s
  to 18.462843 s, while a two-fret ascending Slide jumped to 24.944102 s at
  source contact and was still 22.041747 s in mid travel. The loss solve now
  follows the performed frequency and fractional finger coordinate at control
  rate; adding the invariant alone to unmodified `9c5e7c37` fails only those
  three live-coordinate rails, while the corrected shipping build passes.
- The coordinates remain physically separate. Round-trip gain, legacy modal
  body conductance, Dead's eighth-partial fit anchor and the Palm hand-dip
  centre follow sounding frequency, including tension bend, vibrato, legato
  travel and the optional attack-tension experiment. Only the provisional neck
  dead-spot term follows fractional fret. No decay or contact coefficient was
  retuned.
- Damping keeps its own last-fit coordinate because optional attack tension can
  move sounding pitch without changing the static stiffness fit. A roughly
  six-cent/fractional-fret quantum is shared with dispersion; an exact cache
  assertion proves a two-cent bend does not re-solve it. The check is confined
  to existing control ticks and adds no sample-loop branch.
- Three alternating baseline/candidate timing pairs at 96 kHz gave paired
  median changes of +2.328501% for eight-string Both/Stereo
  (0.123470x -> 0.126200x absolute medians), +2.305524% for Bridge/Mono
  (0.0985069x -> 0.1007780x), +0.974944% for eight-string Palm
  (0.142162x -> 0.143548x), -0.397272%/+0.044772% for one-active/seven-idle
  Both/Stereo and Bridge/Mono, -1.465413% for its wheel glide and +0.179040%
  for the eight-string wheel glide (0.126503x -> 0.125336x).
- Canonical Ubuntu 24.04/GCC 13.3 rendering changes exactly demos 03, 06, 14,
  15, 17, 18, 21, 22 and 23; the other fourteen demos and all ten evaluation
  WAVs plus their manifest are byte-identical. The changed SHA-256 prefixes are
  `50f3a0a9`, `66250999`, `ef89191b`, `8aa6806a`, `2ec9823f`, `475c5500`,
  `05af2122`, `1efa87d5` and `89c724e7`, respectively.
- The order-two low-string-loss build passes. The legacy-body build remains
  finite and inside its coupling bound and has exactly its base's one existing
  0.220083 dB spectral-golden failure. The optional attack-pitch build likewise
  adds no failure beyond its base's two existing mid-Slide release fixtures;
  its exact Palm oracle now evaluates the cached live attack-frequency target,
  and its strict internal coupling seam remains at or below `0.25f`.

## 2026-08-30 performed-fret pick-contact geometry

- A pick position `x` and contact width `w` are fixed metres from the bridge,
  so their delay coordinates are `2x/c` and `w/c`. A reserved pick that arrived
  during an unfinished Hammer or Slide instead combined the performed period
  with `voice.fret`, the written destination. It now recovers the existing
  fractional legato fret at contact; finger-only legato geometry is unchanged.
- Adding only the new invariant to unmodified `76f3e6ec` fails once for Hammer
  and once for Slide. The corrected engine and the complete DSP matrix pass all
  nine tests; no coefficient, parameter or persistent state was added.
- Canonical Ubuntu/GCC 13.3 renders of the merged parent and candidate change
  only demo 22 at its in-flight chord slide, beginning at 12.567506 s; the
  other 22 WAVs are byte-identical. Its SHA-256 moves from
  `1b276823cacd4a6d91d95841b93b5fac8068dc16d4d8544b5b05132e0cb9781e`
  to `6cfc639ea158be89adb6b06c96439a04d7ce6117ac46742c3e9b8c6db5a62654`.
- The branch runs only when an excitation starts and evaluates one logarithm
  only for a plectrum meeting a moving legato string; the sample loop is
  unchanged. At 96 kHz the existing candidate guard measured 0.126768x for
  eight strings in the worst Both/Stereo case and 0.150846x during an
  eight-string wheel glide.

## 2026-08-30 demo 08 clean sympathetic lifecycle

- Demo 08 struck MIDI 28, 35, 40, 47, 52 and 56 twice but sent only one
  matching Note Off per pitch. Every owner count therefore remained one through
  the advertised coupling-off comparison and the final "single low note"; all
  six strings were still physically held instead of leaving five unfingered
  strings for the low note to excite.
- The repeated chord now has two matching release batches. The coupling-off
  target gets 200 ms to reach the engine's exact-zero bypass, and the disabled
  chord gets a 400 ms post-release cleanup before coupling is restored. At the
  three corrected boundaries the held-owner count is zero; exact bypass has no
  active or sympathetic string, and the final entrance starts with no sounding
  string before MIDI 28 becomes its sole held source.
- The deterministic 44.1 kHz regression replays the exact six-note score and
  requires both overlapping releases, cleared idle loops at bypass, no release
  tail before the final section and exactly one sounding string at its entrance.
  The renderer also rejects any completed take whose visual string state still
  exposes a playable note as held, so an unbalanced score cannot silently become
  a committed WAV again.
- A same-compiler full render changes only demo 08; the other 22 WAV pairs are
  byte-identical. The corrected stereo PCM16 file is 328,543 frames
  (7.449955 s), at -3.000097 dBFS peak and -19.506759 dBFS RMS, with SHA-256
  `e88fa1b2903e239e647a0dcb31050f829ec5abbc7098ea3f115b828dc9316cf1`.
  The previous 321,928-frame file had SHA-256
  `f668df0356aaa9907021e6cac8f6870c2dd5f81ecd3d107a946a3181e74a4eea`.
- This changes an offline score and adds one bounded eight-state check per
  rendered take. Product DSP, plug-in state and realtime CPU are unchanged.

## 2026-08-30 demo 19 single-note hook description

- Demo 19's renderer description called its upper line an octave melody/hook,
  but `playPickedRun` performs the written notes sequentially and never starts
  octave pairs. The renderer now calls it the single-note high-register hook
  that it is, matching the existing overview above.
- This changes two descriptions only. The score, all 23 WAVs, product DSP and
  realtime CPU are unchanged.

## 2026-08-30 demo 22 bridge-hand release before lead

- Demo 22 called its upper line unmuted, but moved Mute Pressure from 12% to
  zero only after the 240 ms rest. At the MIDI-74 attack the target was zero
  while the smoothed, blended and applied bridge-hand state remained
  `0.119999997`, adding a `0.01737459` palm-impact velocity to the Sustain
  note. The zero target now begins before that existing rest, which also keeps
  its pre-held vibrato timing unchanged and lets the hand reach exact bypass
  before the lead.
- The regression runs both orderings at the score's 44.1 kHz/256-frame clock:
  the late-clear control retains nonzero Palm contact, while the corrected
  rest reaches exact-zero blend and produces no palm impact on MIDI 74. This
  is a score-only correction; product DSP and realtime CPU are unchanged.
- A same-compiler render changes exactly demo 22; the other 22 WAV pairs are
  byte-identical. Both versions remain 696,118 frames (15.784989 s) of
  44.1-kHz stereo PCM16. The corrected file is -3.000097 dBFS peak and
  -15.430583 dBFS RMS, with SHA-256
  `f224d266fe4e4937692f2f606a4592fdb4a8a8c43acf807eba32526fa51fad40`;
  the same-compiler baseline was
  `979c6927d13d870edb1f6c99ed409ddef3c3e96a1e47c9138ca1d23c9b60e317`.

## 2026-08-30 held owner over different-note legacy tails

- A scalar legacy Hammer or Slide could still follow a closer released voice
  sounding a different note instead of the reachable string whose fret was
  explicitly held. The existing eight-string continuation scan now also
  remembers its nearest valid held owner. That owner replaces the ordinary
  nearest result only when the result is a releasing legacy tail with no held
  owner or pending repick. Repeated targets, sustain-pedal and MPE ownership,
  pending contacts, Hammer's strict nine-fret reach, nearest selection among
  several held strings and multi-note chord assignment keep their precedence.
- In demo 17 at engine clock 212,562, the old allocator sent Slide 69 to the
  closer released MIDI-67 tail on zero-based string 5 (fret 12 to 14) while
  MIDI 62 remained held on string 4 (fret 12 to 19). The corrected gesture is
  the performed 62-to-69 Slide on string 4. Demo 21 had the same error at
  clock 286,650: its corrected Slide continues held MIDI 67 to 72 on string 5
  rather than moving the released MIDI-69 tail on string 6.
- Narrow regressions cover different-target Slide and Hammer collisions, the
  exact demo-17 opening prefix, the closest of multiple held sources and the
  distance-ten Hammer fallback. The new fixture linked against untouched
  `95350798` fails ten assertions, while the complete candidate engine suite
  passes together with the existing repeated-target, sustain, MPE, pending,
  reach and chord rails.
- A full 23-demo same-compiler render changes exactly demos 17 and 21; the
  other 21 WAV pairs are byte-identical. Demo 17 remains 1,024,540 frames of
  44.1-kHz stereo PCM16 (23.232200 s), at -3.000097 dBFS peak and
  -14.672762 dBFS RMS, with SHA-256
  `94d63831c69d72fd27fc07fc5147b855f25f666b9ed4c8aab3db9d5895ebb6a8`.
  Demo 21 remains 741,315 frames (16.809864 s), at -3.000097 dBFS peak and
  -14.986199 dBFS RMS, with SHA-256
  `53cf95e4fc329a36d5836c40d6ef2b3218946a48b48c9341cdcccb6ef9f7565f`.
- The realtime cost is a pair of integer minima and held-owner comparisons in
  the existing bounded event-time eight-string scan. There is no allocation,
  sample-loop work, parameter/state/ABI change or change to the multi-note
  chord solver.

## 2026-08-30 open-string fretboard marker

- The live fretboard's finger-position badge was also drawn for fret zero.
  At the shipped 664x114 display size its circle entered the tuning-label
  spacer and its second copy of the open pitch crowded the permanent tuning
  label. Open strings now use the existing bright string, tuning label,
  motion and level meter; the position badge is reserved for actual frets.
- The editor regression renders the silent display and a confirmed sounding
  open E1, then requires the unaffected 3x5-pixel tuning spacer to remain
  byte-identical. The old renderer fails that exact comparison. The normal
  at-rest 1080x860 editor PNG remains byte-identical at SHA-256
  `e7187094719025a49206bc6fb2e401ffb225fedbe2a1b2d5c09372ac094fc374`.
  This changes no layout, input path, accessibility semantics, audio or CPU.

## 2026-08-30 exact-zero compressor gain fast path

- The compressor's detector and envelope still follow every sample while its
  control is at exact zero, preserving a warm automation start, but its
  otherwise inaudible soft-knee power calculation is now skipped. The existing
  gain law and every non-zero setting are unchanged.
- On an M1 Max, 31 alternating paired Release trials of 1,048,576 hot stereo
  frames put the isolated zero-compressor FX path at 0.612--0.615 paired median
  candidate/baseline across 44.1--384 kHz; every row's p10--p90 stayed within
  0.607--0.623. Enabled-hot and below-threshold 96 kHz controls were 0.999 and
  0.981. A full 96 kHz eight-string metal engine-plus-FX row was 0.997 median,
  with a noisy 0.912--1.022 p10--p90, so it is a non-regression check rather
  than an end-to-end speed claim. The detector recurrence test passes and all
  23 canonical WAVs remain byte-identical.

## 2026-08-30 demo 03 physical Slide score

- Demo 03 promised Slide intervals of two frets, twelve frets up and twelve
  frets down, but only its first gesture followed the held source. In the old
  second gesture MIDI 40 remained held on zero-based physical string index 1
  while the target reused the released MIDI 42 tail on index 2; its
  `legatoFromFrequency` was
  92.4986038 Hz, so the audible move was 42→52 (ten frets), not 40→52. The old
  final gesture sent 40→40: its held-owner count became two, Slide blend stayed
  at one, source frequency and friction stayed zero, and a plectrum Contact
  started instead of a descending finger move.
- The score now states the three source/target pairs explicitly as 40→42,
  40→52 and 53→41. A 600 ms gap lets the preceding fingerless release tail
  retire before the next source is chosen. The exact 44.1-kHz lifecycle
  regression requires every source and target on zero-based physical string
  index 2 (the wound E2), zero initial Slide blend from the named source
  frequency, positive friction, zero pick excitation and a cleared destination
  owner after release.
- A full same-compiler canonical render changes exactly demo 03; the other 22
  WAV pairs are byte-identical. The corrected file is 654,884 frames
  (14.849977 s) of 44.1-kHz mono PCM16 at −3.000097 dBFS peak and
  −34.896833 dBFS RMS, with SHA-256
  `91ebed5d6c153f27a559d3af2105bd00c5108e945edf79c262c494bf47890202`.
  The previous 13.349977 s file had SHA-256
  `a87271d14df85c28ccd37f38877f7cf843694d5a9769f1d66fa515b6da538ae6`.
  No product DSP or realtime loop changed, so plug-in CPU is identical; this is
  musical-score correctness, not measurement or real-recording evidence.

## 2026-08-30 accessible exclusive-choice groups

- Each of the six titled pickup, play-style, keyswitch-mode, output-mode and
  amp-model strips now exposes a native accessibility `group` around its
  existing radio-button children. Its accessible title stays identical to the
  visible strip context, while the selected child remains the strip's only Tab
  stop. This changes no painting, layout, parameters or audio path.
- The editor contract creates every strip's native handler and requires its
  `group` role and exact title in addition to the existing 60-native/35-logical
  Tab-stop, selection, arrow-key and activation rails.

## 2026-08-30 complete accessible keyboard instructions

- The focusable MIDI keyboard's accessible title mentioned only the two
  momentary gestures and playable range, while its visible footer also explains
  all ten C0..A0 pick-stroke and play-style keyswitches. The title now derives
  from that footer verbatim, so the two instruction surfaces cannot drift and
  assistive-technology focus announces every playable keyboard region.
- The processor/editor contract creates the keyboard's native accessibility
  handler and requires its exact title to equal `"MIDI keyboard: "` plus the
  visible hint. The processor suite passes, and the 1080x860 editor render
  remains byte-identical with SHA-256
  `e7187094719025a49206bc6fb2e401ffb225fedbe2a1b2d5c09372ac094fc374`.
  No painting, layout, audio or DSP path changed.

## 2026-08-30 demo 06 whole-step wheel score

- Demo 06 described a wheel bend "up a step", but `pitchBend(0.75)` asks the
  engine's fixed ±2-semitone wheel for 1.5 semitones. The score now uses
  `1.0`, the same whole-tone command used elsewhere in the canonical renderer,
  so held MIDI 71 reaches MIDI 73 rather than an equal-tempered pitch halfway
  between MIDI 72 and 73. No DSP, effect, timing or allocation code changed.
- Same-compiler debugger traces at the end of the 750 ms hold show the old
  target and settled bend at exactly 1.5 semitones and the corrected pair at
  exactly 2.0. Before the changed command, both raw stereo prefixes contain
  66,149 frames and compare byte-for-byte; their left/right SHA-256 values are
  `dc21229e230cae734258dc69214b7713924ac2e2e83d7ca70f17254dab1518f9` and
  `bec0289be42dbc4c5f10b533ec26159d8ca15696168796e1b1b5547f7b50b6d3`.
- Exactly demo 06 changes in a full same-compiler canonical render; the other
  22 WAV pairs are byte-identical. The corrected WAV remains 568,889 frames of
  44.1-kHz stereo PCM16, at −3.000097 dBFS peak and −12.678875 dBFS RMS, with
  SHA-256
  `5fdc24468d6d46dec559a22d7c725a674f3545ea5fb57e7cf8ff4c7f6f628294`.
  Its whole-file old/new PCM null is −6.835834 dBFS because the intended pitch
  change also changes phase through the nonlinear amp, delay and room. This is
  musical-score correctness, not measurement or real-recording evidence.

## 2026-08-30 accessible numeric knob entry

- Electry deliberately keeps each rotary control's editable numeric value as a
  second Tab stop, but JUCE's stock slider value label is accessibility-ignored.
  Those 25 stops therefore exposed neither an editable-text role, their visible
  value nor an edit action. Electry now retains JUCE's decimal editor, colours
  and mouse-wheel blocking in a normal `Label`, whose native accessibility
  handler exposes the direct-entry field.
- Each slider's existing tooltip and reset guidance now begins with its
  canonical host parameter name. The current value label receives that exact
  help, while JUCE's native text-box recreation path inherits it from the
  parent slider. Regression coverage requires exactly one editable value label
  per knob, its unchanged displayed-value title, editable-text role, press
  action and canonical help, while preserving 60 native and 35 logical Tab
  stops. The test-only assertion fails all 25 fields on the prior code.
- This changes accessibility semantics only: DSP, parameters and layout are
  untouched, while copied JUCE colours keep the drawn state unchanged. The
  regenerated 1080x860 editor PNG is byte-identical at
  SHA-256 `e7187094719025a49206bc6fb2e401ffb225fedbe2a1b2d5c09372ac094fc374`.

## 2026-08-30 demo 06 reachable Slide score

- Demo 06 described a Slide from MIDI 67 to 74, but the actual prefix put 67
  at fret 17 on the open-D3 string; 74 would require fret 24, beyond the
  instrument's fret 22 limit. The score now holds MIDI 70 instead. It occupies
  fret 15 on the open-G3 string and reaches MIDI 74 at fret 19, so the four-fret
  gesture is a playable same-string Slide. Its paired Note Off moves from 67 to
  70 as well. No DSP or effect setting changed.
- Debugger traces of the same-compiler old/new renderer binaries, without
  source instrumentation, showed the old score leaving 67 held on the D3
  string while the target restruck the released MIDI-74 tail on the B3 string:
  fresh-pick amplitude `0.39315024`, zero Slide friction. With the corrected
  score, the G3 string retargets 70-to-74 with zero pick amplitude and
  Slide-friction amplitude `0.0387289524`; the old B3 tail remains active,
  releasing and key-up with start order 5. The Alternate next-stroke flag and
  chord-stroke state are unchanged across the target.
- The 169,784-frame stereo raw prefix before the edited Note On is byte-exact
  between same-compiler old/new renders. Its left/right SHA-256 values are
  `6ca8ec6d5be414691e6df2e759706dc8688869f7b697435485c97075ab475501` and
  `85f006029e7ffc11818b41dd2a6e974c5c403e2e098a9fef27840914e97b955a`.
  Exactly demo 06 changes in the canonical render; the other 22 WAV pairs are
  byte-identical. At that Slide-score checkpoint the corrected normalized WAV
  SHA-256 was
  `1f801c33d55d3500b807fd6e887f508992852c17038c9d2f4457de0fa01b2df1`.
- The normalized candidate is 568,889 frames of 44.1-kHz stereo PCM16
  (12.899977 seconds), with −3.000097 dBFS peak and −12.689882 dBFS RMS. Its
  whole-file old/new null is −9.293992 dBFS; unlike the raw prefix comparison,
  that normalized null also includes the take-wide gain change caused by the
  corrected performance. The real Slide also moves the floating hand without
  consuming an Alternate stroke, so later Harmonic and final-note string
  choices and pick directions legitimately diverge. This is a repaired
  demonstration, not measurement or real-recording evidence.

## 2026-08-30 held single-note legato string ownership

- A scalar legacy Hammer or Slide note could reuse a released voice already
  sounding the target pitch even when the performer was still holding a
  reachable source on another string. That models a repeated target instead of
  the requested fretting-hand continuation. Only an active, releasing legacy
  target with no held owner and no pending repick may now yield, and only to the
  closest active, reachable, key-held source owned by the same expression.
  Repeated held targets, member-channel binding, CC64-sustained voices, pending
  target repicks, non-legato styles and the no-held-source fallback retain their
  prior precedence.
- The deterministic regression releases MIDI 45 on the open A2 string, holds
  MIDI 40 on the open E2 string, then requests MIDI 45 as both Slide and Hammer.
  The candidate moves the held E2 string from fret 0 to fret 5, preserves the
  old A2-string release, enters the articulation-specific contact state and
  renders finite nonzero audio. The same test against `e297e849` fails because
  that allocator chooses the released target tail. The pending-state cases in
  this fixture are narrow allocator branch probes; the established
  delayed-contact lifecycle tests remain the travelling-contact coverage.
- This changes direct single-note events and the one-event `noteOnChord` fast
  path. Multi-event `noteOnChord` uses its separate global assignment score and
  is deliberately outside this checkpoint. The implementation reuses the
  existing eight-string event-time continuation scan, allocates no memory and
  does not touch a sample loop.
- At this engine checkpoint the canonical scope was empty: none of the 23 demos
  then contained a plain same-target release colliding with a reachable held
  source. Demo 06 still used its unreachable MIDI 67-to-74 pair; the separate
  score-only checkpoint above repairs that demonstration. The broader cases in
  demos 03, 17 and 21 involve released different-note sources and remain
  deliberate negative controls. A same-compiler render against `e297e849`
  confirmed all 23 WAVs byte-identical; the normalized SHA-256 manifest is
  `55bec492d0cf13cce3cc5b279365a6d52abeb8f0f1d50ffda1fd08dd33a06f61`.
- The default DSP-only CTest matrix passes all nine targets. The complete
  engine suite also passes with force-decoupled pick release disabled, the
  optional energy-derived attack-pitch model enabled and the optional order-2
  low-string loss correction enabled.

## 2026-08-30 finger-speed slide spectrum

- Pakarinen, Penttinen and Bank's measured wound-string handling-noise study
  ([JASA DOI 10.1121/1.2786607](https://doi.org/10.1121/1.2786607),
  [author manuscript](https://aaltodoc.aalto.fi/bitstreams/475276ab-f408-4ef4-a46f-5528bb8ca63d/download))
  reports both harmonic frequency and RMS amplitude proportional to sliding
  speed, with the measured gesture accelerating and then decelerating.
  Pakarinen, Puputti and Valimaki's
  [Virtual Slide Guitar](https://doi.org/10.1162/comj.2008.32.3.42)
  and its
  [NIME contact-generator companion](https://www.nime.org/proceedings/2008/nime2008_049.pdf)
  likewise drive the contact spectrum and level from sliding velocity.
- Electry's friction level already followed the normalized smoothstep-velocity
  proxy `6 b (1 - b)`, but its two-pole band remained fixed at the whole
  gesture's average centre. The band now follows that same proxy at the
  existing control rate, retaining the 200 Hz and `0.40 * internal rate`
  clamps. This deliberately does not claim an exact metre-domain finger
  derivative: Electry interpolates fret/log-frequency position, and its winding
  pitch remains the existing voicing estimate.
- Recovered-pole state tests require the unclamped `1.125 / 1.5 / 1.125`
  centre ratios at quarter/mid/three-quarter travel from 44.1 through 384 kHz,
  symmetry, both clamps, and exact zero-Finger-Noise and non-Slide bypass.
  A paired friction-only output rail requires finite, nonzero early/middle/late
  windows and no control-period first-difference spike. These are source-model
  direction and stability checks, not a spectrum fit, capture comparison or
  real-recording parity claim.
- A same-compiler canonical render changed exactly the four Slide demos 03,
  17, 21 and 22; the other 19 WAV pairs were byte-identical. Whole-file null
  RMS was -71.49, -58.44, -72.55 and -55.23 dBFS respectively, 36.43--57.37 dB
  below each candidate's RMS. Seven warmed outer benchmark pairs reversed
  baseline/candidate order: ratio-of-medians cost was 0.581% at 96 kHz and
  1.218% at 384 kHz, inside the 1%/2% Slide-stress gates. Runtime adds one
  float per voice and two exponentials per control tick only while an audible
  Slide is in flight; the sample loop is unchanged, while silent and non-Slide
  friction coefficients and output remain exact bypasses.

## 2026-08-30 roving focus for exclusive choices

- The [WAI-ARIA radio-group pattern](https://www.w3.org/WAI/ARIA/apg/patterns/radio/)
  sends Tab to the checked radio and uses arrow keys to move focus and selection
  within the group. Electry's six exclusive-choice strips already provided
  wrapping arrow navigation, but all 21 buttons were separate Tab stops. Each
  strip now exposes only its selected option to Tab; arrows, clicks and host or
  timer selection updates move that roving stop with the selection.
- This reduces the strips from 21 Tab stops to six and the editor from 50 to 35
  logical controls. JUCE's raw traversal contains 75 entries before and 60
  after because each of the 25 knobs also exposes an editable numeric value
  field; those deliberate direct-entry stops remain unchanged. Native JUCE
  focus traverser tests cover construction, all four arrow directions, wrapping,
  disabled-choice skipping, clicks, programmatic selection and all six shipped
  strips.
- A click selects the option before explicitly taking focus, while
  `setSelectedIndex()` only changes the roving stop and never steals focus from
  another control. Every button retains its native focus outline, and the
  same-renderer editor PNG is byte-identical because no layout or painting code
  changed.

## 2026-08-30 performed-pitch release damping

- Note release applies one gain per completed string loop, but armed that gain
  from the written base frequency plus legato only. A settled two-semitone bend
  therefore changed the nominal 60 ms release T60 to 53.454 ms upward or
  67.348 ms downward. The gain now uses the compensated sounding period, both
  when Note Off arms it and when an already-ringing tail changes pitch.
- Legacy wheel bend remains live after Note Off. An MPE member bend freezes with
  its lifted finger while its zone-master bend remains live; the release rate
  follows exactly that same lifecycle. The untouched implementation fails the
  three nonzero arm-time cases and both live-tail cases, while a Note-Off-only
  mutant fails exactly the latter two; the unbent and frozen-member controls
  pass both mutations.
- A same-compiler render changes only bend/legato/vibrato demos 06, 14, 17, 21,
  22 and 23; the other 17 demos are byte-identical. All ten unnormalised dry
  evaluation WAVs and their manifest are byte-identical. A dedicated held +2
  semitone dry release is 2.40 dB higher at 40--60 ms and 4.58 dB higher at
  60--80 ms, removing the former early cutoff.
- The solve is event/control-rate only and reuses existing state. Damping-only
  compensation and steady tails skip the power calculation unless the cached
  period actually moved; nine paired eight-string bent-release runs measured a
  1.0066x median candidate/baseline time ratio, with no audio-sample branch.
  The historical 60.141/60.052 ms transition audit used what was intended to
  be a six-millisecond delay smoother but was actually 0.375 ms; those two
  bounds are retired by the corrected and directly tested smoother above.

## 2026-08-30 contact-period finger release makeup

- Finger-contact modal projection already follows the period sounding at
  contact, but its release-pole makeup was still evaluated at the written
  destination. Applying the pole at the contact coordinate therefore left a
  second pitch-dependent gain: the former 48 kHz path returns an effective
  four-semitone Hammer/pull ratio of 1.347827 instead of
  `2^(4/12) = 1.259921`, 6.98% high. The makeup now uses the same already-
  validated contact period as projection and modal cutoff.
- A direct coefficient/state oracle covers 44.1, 48, 96, 192 and 384 kHz and
  requires the effective post-pole ratio above, a finite coefficient-derived
  ratio and finite audible output. A same-compiler render changed exactly the
  seven Hammer/pull demos
  03, 06, 15, 17, 18, 21 and 22; the other 16 WAV pairs were byte-identical and
  no printed peak crossed a tenth-decibel boundary. Valid plectrum contacts
  retain the identical period and arithmetic; a legato Slide's modal amplitude
  remains exactly zero.
- This removes one event-time coordinate inconsistency without a fitted
  coefficient, new state or sample-loop work. No admissible corpus recording
  holds finger force, source fret, destination fret and rig fixed while varying
  only this release coordinate, so the result is a discretisation invariant,
  not a capture fit or real-recording parity claim.

## 2026-08-29 sounding-period modal spectrum

- The two temporal poles that approximate triangular displacement's `1/n^2`
  modal falloff now use `fs/P_contact` before the existing hardness, velocity
  and articulation factors. They formerly used the written MIDI frequency even
  though projection and geometry had already resolved a pre-bent or settled
  legato source period. That changed the harmonic-number envelope by almost
  2.01 dB at high modes across a standard two-semitone bend and by about
  4.01 dB across a four-semitone source mismatch.
- An independent regression reads the pole itself: because
  `log(a) = -2 pi fc/fs`, its down/up-bend ratios must be
  `2^(-2/12) = 0.890899` and `2^(2/12) = 1.122462`. The former written-note law
  returns 1.0 at both endpoints and fails both rails. The change reuses the
  already-validated contact-period scale and adds one event-time division, no
  new fit, state or sample-loop work; an ordinary unbent attack still divides
  by exact unity.
- A same-compiler render changed exactly demos 03, 06, 17, 18, 21 and 22; the
  other 17 canonical WAV pairs were byte-identical and no printed peak crossed
  a tenth-decibel boundary. No admissible corpus take holds force, contact and
  rig fixed while varying only this period, so this closes an internal modal-
  coordinate inconsistency rather than claiming a recording fit or market
  parity. The broader unfinished-delay residual experiment remains separate.

## 2026-08-29 choice-strip arrow navigation

- Left/Up and Right/Down now move to the previous or next option in every
  exclusive-choice strip, wrapping at both ends and moving keyboard focus with
  the selection. The target button enters the same choice-action path as a
  click exactly once, so the highlight, callback and bound parameter cannot
  diverge. Disabled options are skipped; PANIC remains an ordinary action
  button and does not consume arrows.
- The plug-in regression covers all four arrow keys, both wrap directions,
  single-callback delivery, disabled-option skipping, and rejection by disabled
  or non-radio buttons. Space and Return retain their established activation
  paths. This adds no layout,
  painting, parameter, audio or steady-CPU work; it runs only for a focused
  button's key event.

## 2026-08-29 sounding-period attack projection

- The compact modal attack now normalises against the compensated period at
  physical contact, not only the newly written MIDI destination. Its event-time
  gain is
  `clamp((f_E4/f_written) (P_contact/(fs/f_written)), 0.24, 9.0)`, which
  reduces to `f_E4/f_contact`: a settled legato contact retains its sounding
  source period and a fresh pre-bent note retains its bent period. The former
  destination-only law made equal-force hammer and pull gestures aimed at the
  same note project equally even when their source loops differed by four
  semitones.
- Independent regressions require the fixed-target hammer/pull amplitude ratio
  `2^(4/12) = 1.259921` and open-E1 displacement ratios `2^(2/12) = 1.122462`
  and `2^(-2/12) = 0.890899` at the standard wheel endpoints after removing
  the release pole's exact response. The old law fails all three at 1.0. The
  upper safety rail moved from 8.25 to 9.0 because open E1 bent down two
  semitones requires 8.979696; more extreme member/master MPE remains bounded.
- A same-compiler baseline/candidate render changed exactly the seven demos
  containing hammer/pull contacts: 03, 06, 15, 17, 18, 21 and 22. The other 16
  WAV pairs were byte-identical; only demo 21 crossed a tenth-decibel peak-table
  boundary. An ordinary unbent attack forms an exact `x/x = 1` scale, and the
  added arithmetic runs only when an excitation starts, never in the sample
  loop.
- This corrects the delay-line projection's own discretisation. No admissible
  recording in the current corpus holds finger force, target fret and rig
  fixed while varying only the sounding source period, so it is not presented
  as a capture fit or a real-recording parity claim. Residual-aware projection
  during an unfinished delay glide is a broader repick model and remains a
  separate evaluation rather than being bundled into this correction.

## 2026-08-29 keyboard Space activation

- PANIC and all six exclusive-choice strips now activate with Space as well as
  Return. They already requested keyboard focus and drew a focus outline, but
  JUCE 8.0.14's base text-button key handler accepts Return only. One shared
  Electry text-button type maps Space into that same JUCE activation path, so
  asynchronous click delivery, radio selection and existing callbacks retain
  one implementation rather than acquiring a second UI action path.
- The plug-in regression proves an enabled instance accepts Space, a disabled
  instance rejects it through JUCE's existing enabled-state guard, and PANIC
  plus every button in every choice strip uses the shared type. This changes no
  bounds, colours, labels, screenshots, parameters or audio and adds no steady
  UI or render CPU work; it is one key-event comparison when a focused button
  receives input.

## 2026-08-29 bridgeward upstroke endpoints

- Up strokes now retain their established `-0.020` open-string position shift
  through the complete Pick Position range. The former private `0.03..0.45`
  clamp contradicted its own “closer to the bridge” direction: at Pick Position
  zero it moved the nominal `.025 - .020 = .005` result to `.030`, neckward of
  the `.025` Down stroke, while at Pick Position one it moved the intended
  `.460` result to `.450` and made the offset 50% too large. For a Palm Up at
  the bridge end it changed `.025 * .8 - .020 = 0` to `.030`, also beyond the
  corresponding Down contact.
- Removing that redundant clamp leaves the one physical coordinate law in
  charge: `p = clamp((p_style + dx/L_open) 2^(fret/12), 0.02, 0.98)`.
  Independent regressions pin both Sustain endpoints for Down and Up, the
  exact two-per-cent neck-end separation, and Palm Up against the shared bridge
  guard while including deterministic metre variation. The change removes one
  note-start clamp and adds no state, render-loop operation or steady CPU.
- A same-compiler baseline/candidate render left all 23 canonical WAVs
  byte-identical because their Up/Alternate contacts stay outside the removed
  private clamp. The retained two-per-cent stroke-direction offset is existing
  voicing, not a capture fit: no admissible recording here holds player, pick,
  string and absolute contact position fixed while changing only direction.
  This corrects an internal geometry contradiction rather than claiming a
  real-recording win or using a listening preference to select a coefficient.

## 2026-08-29 far-side pick geometry

- Pick and co-located thumb positions now retain the complete speaking-string
  coordinate through the midpoint and up to the symmetric two-per-cent
  endpoint/contact guard at `p = 0.98`. The former 49% ceiling was not a safety
  bound: it moved every valid `0.5 < p < 1` contact to the midpoint and erased
  relative modal phase even though ideal comb magnitude can look mirrored.
  Nominal Pick Position 0.30 at top-string fret 22 is `p = 0.57552`; the old
  clamp displaced it 16.8 mm bridgeward on the default 27.625-inch scale. At
  fret 12 the nominal neck endpoint is `p = 0.96`, which the clamp displaced
  164.9 mm. Regressions pin the far-side absolute coordinate, the unchanged
  below-midpoint path, the unmirrored 98% endpoint and both pick/thumb identity.
- The existing positive `pP` touch image remains valid: because
  `p = p_open 2^(fret/12)` while `P = P_open 2^(-fret/12)`, its extra delay is
  the fixed hand distance `pP = p_open P_open`, not an allocation that grows
  with fret. Far-side lowest-string Pinch reads remain inside the delay line at
  44.1, 48, 96, 96.001, 192 and 384 kHz under -2/0/+2-semitone bends. This is
  one event-time clamp change with no new state or render-loop operation.
- The E3 Pinch mechanism now distinguishes bridge, near-midpoint and far-side
  settings: strongest partials are eight/four/nine and tracked means are
  6.03/4.74/7.06. That non-monotone return is the co-located
  `abs(sin(2 pi n p))` sequence crossing the midpoint, not a reason to restore
  the false clamp. Public recordings do not control an absolute far-side pick
  point, so this analytically corrects geometry rather than claiming a capture
  win; a decisive fit needs mechanically repeated raw DI at measured mirrored
  positions such as `p = 0.42/0.58`, scored for complex modal phase as well as
  magnitude.
- A same-compiler baseline/candidate render changed exactly the five canonical
  demos containing affected far-side notes: 02, 06, 17, 20 and 21. The other 18
  baseline/candidate WAV pairs were byte-identical, so their committed assets
  remain untouched; the level table records only the five changed candidates.

## 2026-08-29 live status accessibility announcements

- The live engine status now sends a JUCE accessibility `titleChanged` event
  whenever its displayed text actually changes. Accessibility clients can
  therefore discover current voice and sympathetic-string counts, engine
  readiness, sample rate, MIDI mute pressure and vibrato/tremolo gestures
  without rediscovering the component. Comparing the formatted title suppresses
  redundant notifications when hidden fields change or adjacent controller
  values round to the same percentage, while optional repaint scheduling stays
  independent. This changes metadata only on status transitions; audio, layout
  and steady render CPU are unchanged.

## 2026-08-29 physical-period touch geometry

- Natural- and pinch-harmonic contacts now place their extra loop tap at `p`
  times each polarisation's complete live fundamental period. The former
  `p * currentDelay` separation omitted damping and dispersion phase: on open
  E1, raw/full ratios are 0.87470 with fresh strings and 0.79329 with old
  strings, so String Age visibly moved a nominal midpoint finger. The new
  production helper is pinned against an independent per-axis oracle, both age
  extremes, a two-semitone pre-bend and a 64-sample in-flight bend where raw
  delay has not yet reached its target. It retains the horizontal mode's
  detune, the existing smoother and every filter-phase continuity translation.
  Ordinary notes remain on the exact zero-touch branch; an active touch keeps
  the same one extra cubic read and adds only the phase-restoration arithmetic,
  then the existing lift envelope removes that work. One local whole-engine
  96 kHz run measured 0.1208x realtime in the hostile Both/Stereo eight-string
  case, below the unchanged portable guard.
- Correcting the coordinate exposed one over-specific historical regression:
  with pick and thumb co-located at the then-existing 49% clamp, their ideal
  modal product is
  `abs(sin(2 pi n p))`, making the fourth start about 5.95 dB above the octave.
  The raw-delay error had accidentally reversed that ordering. The revised
  mechanism gate required a low even partial (two or four) near the midpoint,
  a bridge partial of at least six, a lower spectral mean toward that midpoint
  and more than 6 dB of squeal lift. The later far-side checkpoint retains that
  rail at Pick Position 0.90 and adds the distinct post-midpoint result instead
  of preserving the clamp. No arbitrary thumb offset was introduced; a
  measured fixed-metre thumb/pick separation remains capture-gated.
- This is corrected fundamental-period geometry, not an exact spatial contact
  for every inharmonic partial: a single temporal tap cannot represent all
  stiff-string nodes. The correction and that claim boundary are decided
  analytically, so no A--Z preference gate selects them.
- The five canonical demonstrations containing natural or pinch harmonics were
  regenerated from the validated engine; their rounded peak-normalisation
  table entries remain unchanged.

## 2026-08-29 physical-period pick geometry

- Pick-comb centre, 0.5-1.5 mm plectrum contact width and pull-off release
  position now map physical distance through the complete sounding period
  `P = fs/f0`. The former raw-delay coordinate was `P` minus damping and
  dispersion phase, so changing String Age subtly moved a stationary hand and
  fretting changed a fixed-width tip. Regressions pin absolute centre/width on
  one string from open to fret 12, the inverse-wave-speed law under a
  two-semitone pre-bend, invariance across String Age extremes, and both
  fretted and open pull-offs. At pull-off contact the loop is still sounding at
  the source fret, so that live period is already the released finger's
  `2L_old/c` image delay; no second `L_old/L_new` ratio is applied. The shipping
  regression also pins that delay to the nominal source-note period. The raw
  line/full-period probe reads 0.94413 on open E4 and 0.87422 on default E1,
  falling from 0.87470 to 0.79329 across E1's String Age extremes, which
  quantifies the false hand motion removed. The cached-period substitution is
  event-only: it adds no render-loop operation, memory or steady CPU. This
  corrects internal geometry; it is not a claim of capture parity.
- The existing lawful CC0 Drop-E four-ghost comparison improved without
  retuning a Dead coefficient. Against the real 30-100/100-250/250-380 ms
  median of -3.57/-12.66/-20.75 dB, shipping moved from
  -8.114/-14.919/-23.040 dB to -7.466/-14.073/-22.059 dB: three-window RMSE
  fell from 3.21 to 2.51 dB. Its first-versus-repick contextual RMSE also moved
  from about 4.85 to 4.82 dB. Every individual render remains inside the same
  broad four-take real ranges; deterministic whole-source spectral snapshots
  were refreshed, but those internal snapshots are not capture targets.
- All 23 committed demonstrations were regenerated from the validated engine,
  and the rendered-peak table above was refreshed. The objective geometry and
  existing real-envelope comparison decided this correction; no A--Z listening
  gate selected it.

## 2026-08-29 full accessible knob names

- All 25 rotary controls now expose the APVTS parameter's complete canonical
  name to accessibility clients while retaining their compact faceplate labels.
  This removes ambiguous screen-reader titles such as `MUTE`, `COMP`, `AGE`
  and `OUTPUT` without adding a parallel naming table. The plug-in regression
  resolves every knob by parameter ID and checks both its component title and
  JUCE accessibility-handler title. Rendering is unchanged; this is metadata
  only, with no audio, steady CPU or visual-layout cost.

## 2026-08-29 sympathetic exact-bypass state

- Automating Sympathetic to its exact zero endpoint now clears each inactive
  vertical delay line and every retained damping/dispersion state before the
  loop is marked unconfigured. A later played note therefore cannot inherit a
  hidden ring from state the bypass declared absent. The live full-to-zero
  regression starts with a populated coupled string and pins the line and all
  nine filter memories to exact zero. This is one endpoint clear with no
  steady-state CPU or memory cost; no listening gate was needed.

## 2026-08-29 sympathetic stiff-string dispersion

- Every unfingered ringing string now runs the same formula-derived `B` and
  eight first-order allpass sections as that physical string when played open.
  The idle bank follows the global wheel's live-tension law, refits only on the
  existing six-cent or build-geometry quantum, and compensates damping plus
  dispersion phase at every smaller move. Raw delay is translated against any
  filter-phase replacement, so a rising bend stays monotone across fit cells.
- This reuses the inactive voice's existing coefficients and filter state:
  one active plus seven idle strings adds 56 first-order sections but no
  allocation or memory. Played/idle-open fit parity, populated histories,
  target tuning, live `1/T` bend scaling, gauge invalidation, current-pitch
  continuity and proportional sympathetic-to-played state choking are pinned
  structurally. The 96 kHz one-active/seven-idle render measured `0.0731x`
  realtime for Both / Stereo and `0.0539x` for Bridge / Mono, versus about
  `0.0594x` and `0.0454x` before dispersion; its full wheel glide measured
  `0.0766x`. No A--Z gate selected this same-string physical parity correction.

## 2026-08-29 sympathetic pickup-path parity

- An unfingered ringing string now reaches the selected neck and bridge buses
  through the same distinct pickup-position combs, 4.8 mm aperture and 19 mm
  humbucker coil pair as that string when played open. The spatial delays
  follow the global wheel's live `1/c` coordinate, and Pickup Type's
  exact single-coil endpoint removes both second-coil reads. The quiet idle
  loop remains one-polarisation and uses a linear low-level magnetic map.
- The implementation reuses each inactive voice's existing fixed histories,
  advances only selected pickup paths and allocates no additional memory.
  Structural tests pin played/idle geometry parity, bend scaling, both pickup
  endpoints, selector culling and the exact one-active/seven-idle topology.
  Report-only local 96 kHz renders were about `0.059x` realtime for Both /
  Stereo and `0.046x` for Bridge / Mono, remaining below 6% realtime in the
  measured worst case (the prior simplified idle path measured about `0.043x`
  and `0.039x`).

## 2026-08-29 live-wave-speed pickup geometry

- Wheel/MPE bend and fretting vibrato now carry every representable pickup
  position, 4.8 mm aperture and 19 mm coil-spacing delay in the sounding
  string's live `1/c` coordinate; pure fretting/slide motion stays neutral and
  sub-sample apertures stay at the implementation's identity floor. A
  +2-semitone bend now targets approximately `0.8909x` the resting spatial
  delay instead of leaving that delay unchanged.

## 2026-08-29 live-tension stiffness dispersion

- Wheel/MPE bend and fretting-hand vibrato now carry stiffness through the
  static stiff-string model's tension coordinate. At fixed speaking length an
  `s`-semitone gesture scales tension by `2^(s/6)` and gives `B` the unclamped
  target `2^(-s/6)`; +2 semitones therefore moves `B` to `0.7937x` instead of
  leaving it fixed.
  Pure fret/slide travel retains its established tension and `1/L^2` law;
  combined gestures follow `1/(T L^2)` within the model's existing bounds.
- Regression coverage pins global and MPE bends in both directions, quantised
  finger-vibrato refits and an opposing bend/slide at constant total pitch.
  The default-off energy-attack factor remains outside the static fit. The
  shipping and attack-candidate DSP suites pass, local full wheel-glide stress
  renders remain around `0.13x` realtime, and the correction adds no grid
  search or per-sample work. No A--Z gate selected this objective equation.
  The public recordings audited can support a descriptive direction check,
  but lack measured tension and construction controls for capture calibration.

## 2026-08-29 provenance-locked exact-eight comparison

- `CompareReferenceFixture.py` closes the mechanical gap between the existing
  successor-only exact-eight receipt validator and the frozen v3 model probes.
  A separate frozen plan must name each upstream asset, zero-based channel,
  isolated crop, the onset-search start following at least 50 ms of pre-roll,
  nominal frequency and same-note model probe; it also binds the analyzer,
  validator, receipt, model manifest and every consumed model WAV by SHA-256.
  The comparator accepts only the full ten-probe, dry-DI 44.1 kHz v3 renderer
  contract and protects every listed probe plus every fixture input from an
  aliased output path.
- The resulting `electry-evaluation/v4` record detects each onset independently
  at or after the declared search start with the same native-rate 2 ms envelope,
  requires 20 dB onset SNR (or an exactly silent measured pre-roll) and a full
  two-second tail, then reports within-file-normalized RMS decay, onset-to-peak
  timing and centered-envelope first-cycle return. Peak times use the earliest
  frame within 0.05 dB of the window maximum, avoiding arbitrary later-cycle
  choices at low E1 rates; the contract exercises the same sine at 8, 44.1, 48
  and 96 kHz. It neither downmixes nor resamples and has no fit, aggregate score,
  distance threshold or promotion verdict. The record names its Python/runtime
  host because floating analysis is only promised deterministic there. This is
  a descriptive comparison surface, not evidence of capture parity or market
  superiority. No third-party recording or generated fixture artifact is
  committed; temporary contract tests synthesize their own reference audio. A
  real result still requires a reviewed upstream-original exact-eight WAV with
  usable quiet pre-roll.

## 2026-08-29 positioned following-fret loss candidate (default off)

- The shipping Artifacts limiter sees only the vertical wave at the loop seam,
  so every partial encounters the same threshold regardless of where a real
  downstream fret would touch. Poirot et al. identify that longitudinal modal
  distribution as a collision-location cue. `ELECTRY_POSITIONED_FRET_COLLISION`
  adds one cached linear tap at the equal-tempered following fret before touch,
  dispersion and fitted damping. Its fixed-depth law leaves node modes intact:
  the production regression measures gains 0.101/1.000 for the near-antinode
  ninth and near-node eighteenth harmonics.
- The experiment retains the established contact window, clearance opening,
  zero-slope soft knee, deterministic noise and saddle rattle. It does not arm
  its following-fret branch at fret 22, where this instrument has no following
  metal fret. Dead notes retain their old distributed-hand contact, including
  at fret 22, because they have no stopped speaking fret. The OFF build
  preprocesses to the shipping branch.
- This is intentionally a bounded one-read loss surrogate, not a reciprocal
  local two-rail scatter and not a claim of nonlinear passivity. It deliberately
  retains shipping's bilateral absolute-value barrier to isolate position in
  the A/B, rather than pretending to be a unilateral fretboard obstacle. A
  correct paired fractional gather/scatter needs another outgoing write and an
  adjoint interpolation law; inventing that machinery or a per-fret action map
  without measurements would overstate the evidence.
- Both OFF and ON pass the complete DSP suite; the candidate reaches contact
  from 44.1 through 384 kHz, including 96.001 kHz, stays finite in the maximum
  eight-string stability cases, and keeps the last fret's following-fret branch
  exact-null. Across three interleaved Release runs on the current machine, its
  extra tap is confined to the 25-100 ms opportunity; the median two-second
  default case costs 2.3 percent over OFF and the hostile all-eight-string 110
  ms Palm burst costs 6.9 percent. Other full-window timings stayed inside
  run-order noise.
- No admissible controlled exact-eight collision capture is currently local.
  The thirteen CC0 exact-eight Freesound previews are lossy, uncontrolled and
  show no stable H9/H18 trajectory; they remain descriptive no-fit evidence.
  Promotion therefore requires a whole-file-RMS-matched private OFF/ON verdict
  plus a populated original-PCM reference receipt with documented action,
  relief, fret heights and attack force.

## 2026-08-29 sample-rate-invariant polarisation seam

- The two returned travelling waves encounter their contractive mixing matrix
  once per loop, but its coefficient was calculated from the rate-specific
  compensated delay. The same pitch therefore received half the cross-amplitude
  gain at a 96 kHz host that it received immediately above the engine's
  2x-to-native boundary.
- The existing `0.04` anchor and nominal linear-with-`f0` voicing are now
  expressed against the sounding period at the 96 kHz reference clock. Damping
  and dispersion phase remain responsible for tuning, but can no longer retune
  the polarisation seam. This adds no work to the per-sample path.
- The 22nd-fret high-E decay spread across the three 0.1-0.5, 0.5-1.5 and
  1.5-3.0 s windows fell from 0.80/2.64/6.65 dB to
  0.063/0.092/0.153 dB across 44.1-192 kHz, including the 96/96.001 kHz seam.
  The eight-string 96 kHz worst-case CPU ratio remained 0.1197x. No listening
  gate or absolute coupling refit was used: this checkpoint enforces one
  digital/physical invariant and does not claim measured bridge calibration.

## 2026-08-28 force-decoupled exact pick release

- A sealed, whole-file-RMS-matched dry/metal/rock/blues comparison used A =
  the former force-coupled approximate release makeup and B = the
  force-decoupled exact-digital-pole path. The user selected B by ear.
- The shipping path now keeps MIDI velocity as player force and Pick Hardness
  as plectrum contact/stiffness, preserves the established default displacement
  exactly, and normalises the implemented release pole at the live picked
  fundamental. `ELECTRY_DECOUPLED_PICK_RELEASE=OFF` retains A only as a legacy
  comparator.
- The canonical 23 demos were regenerated. The force-normalised invariant is
  pinned for unbent attacks, the standard ±2-semitone wheel and ±24-semitone
  per-note expression; the macOS release build explicitly selects the promoted
  path even when reusing an older CMake cache.

## 2026-08-27 internal-rate phase-inverter capacitors

- The American/British output-coupling-capacitor solve was called once per
  oversampled frame but upper-clamped that frame clock to the 384 kHz host-rate
  ceiling. Hosts at 88.2, 176.4 and 352.8 kHz all run the nonlinear path at
  705.6 kHz, so their 100/22 nF companion models advanced 384 kHz coefficients
  705,600 times per second.
- Before correction, a quiet full British path alternated by rate family:
  40 Hz relative to 1 kHz moved from -20.938 to -23.373 dB and 80 Hz moved from
  -0.099 to -0.890 dB. The solver now retains its actual finite internal clock
  and applies only the existing lower safety bound.
- The direct 40 Hz circuit regression failed before the fix by 2.548 dB and now
  passes a 0.05 dB 384/705.6 kHz bound; the same public-path 48/88.2 kHz rail
  also closes the production-callsite gap. A separate
  full-path sweep across 48, 88.2, 96, 176.4, 192, 352.8 and 384 kHz leaves
  only 0.00376/0.00176 dB of spread. The 44.1 kHz canonical renderer already
  ran at 352.8 kHz, below the bad clamp, so none of its twenty-three WAVs
  changes. No A–Z gate was needed because the analogue time constant and the
  clock that advances it are objective.

## 2026-08-27 continuous slide stiffness

- A slide previously selected the destination integer fret's inharmonicity at
  contact even though its delay and audible pitch travelled continuously. An
  octave ascent therefore installed four times the source `B` before the
  fretting finger had moved.
- Stiffness now uses the same live fractional fret as the legato glide. The
  established string tension stays fixed while `B` follows `1/L^2` monotonically
  through ascending and chained descending travel; the existing phase-delay
  translation keeps effective pitch continuous and one endpoint-only dirty mark
  lands on the independently settled target fit.
- Regression coverage pins contact and chained-retarget continuity, the full
  geometric `B` trajectory, exact endpoint identity and a post-slide sub-quantum
  wheel move that must not reopen the expensive dispersion search. The existing
  portable CPU guardrails remain unchanged. No A–Z gate was needed because this
  repairs a violated physical formula without adding a voicing choice, control
  or dependency.

## 2026-08-27 MPE string-owned pitch expression

- Added MIDI MPE advertisement and JUCE's fixed-size lower/upper-zone RPN
  parser. A same-sample configuration sequence conditions its attacks before
  string allocation, independent of the host's event insertion order.
- Member channels now own physical strings and use the zone's exact declared
  semitone-plus-cent pitch range; a master wheel adds its own exact declared
  range to every member. Electry retains those float ranges beside JUCE's
  integer zone-routing state. Two member channels at the same MIDI pitch
  allocate separate strings and release independently, even if the zone layout
  changes before Note Off.
- The realtime path uses a fixed 128-entry FIFO for each MIDI pitch, fixed
  16-channel owner counts and bend arrays, and rejects an excess attack before
  it can become an unreleasable engine owner. With no valid zone, explicit
  expression ID zero and multichannel MIDI remain sample-identical to the
  legacy channel-agnostic path. A released tail freezes its performed member
  interval while its zone-master interval remains live. A released pending
  contact retains that snapshot through the exact contact sample; a genuinely
  new finger keeps the old ring unchanged during pick travel, then thaws to its
  cached member bend at contact. CC 121 recentres expression and nulls the
  current RPN/NRPN selection without deleting the zone or its exact ranges;
  prepare and release likewise clear that parser transaction while retaining
  the zone and ranges. CC 121/123 retain
  whole-instrument reset/release semantics, and prepare, release, Panic, All
  Sound Off and All Notes Off cannot leave stale note ownership.
- This checkpoint maps standardized pitch only. Pressure, CC 74 timbre and
  release velocity remain unassigned until captures and a predeclared listening
  comparison justify an audible physical mapping. No A–Z gate was needed for
  the routing itself because MIDI ownership and pitch intervals are objective.

## 2026-08-27 standard-rate nonlinear antialiasing

- Raised the distortion/amplifier domain from 4x to 8x at 44.1 and 48 kHz and
  retuned the shared 23-tap halfband window from Kaiser beta 8.6 to 7.5. The
  dense kernel sweep moved from 55.45 to 75.55 dB minimum stopband rejection
  while holding worst passband deviation to 0.00145 dB.
- Twelve full-path pedal, amplifier, stacked-gain and quiet-signal probes now
  sit below -70 dB at both standard rates. The complete range is
  -70.16..-83.25 dB; at 48 kHz, Modern High-Gain moved from -61.28 to
  -72.70 dB without changing its circuit topology or voicing.
- From 48 kHz upward, the rate policy now retains the smallest oversampling
  factor that reaches 384 kHz; 44.1 kHz remains the maximum-8x exception at
  352.8 kHz. All six nonlinear profiles remain below -70 dB on both sides of
  the resulting 96/192/384 kHz stage transitions.
- Engaged gain-stage delay rose from 17.25 to 20.125 host samples (0.42 ms at
  48 kHz). The comparable 23-demo renderer costs about 55% more wall time but
  still completes at 0.122x aggregate realtime on the documented M1 Max.
  Canonical Ubuntu regeneration changed the 13 wet/amped references while all
  ten dry/reference performances remained byte-identical.
- No A–Z listening gate was run: the predeclared non-harmonic-energy and kernel
  response measurements objectively selected this correction.

## 2026-08-27 blues-rock factory rig

- Promoted the host-representable version of demo 21's neck-pickup lead voicing
  to the **Blues Rock Lead** factory rig. It restores all 28 host controls from
  a deterministic starting point while leaving Pick Stroke, Play Style and
  LATCH/HOLD under the player's control, like the existing rigs.

## 2026-08-27 source screen-grid loading

- Added the source-derived screen networks inside the American and British
  power-pair solves. American retains the RCA 6L6GC 450 V plate / 400 V screen /
  −37 V / 5.6 kOhm AB1 pair and gives each tube an AB763-derived 470 Ohm screen
  branch. That combination is explicitly a source-component hybrid rather than
  a claim to reproduce the complete Twin Reverb DC output stage.
- British keeps Mullard's 400 V / −36 V / 3.5 kOhm EL34 pair condition and its
  matching common 800 Ohm screen resistor. Both tube screen currents therefore
  pull one shared node instead of borrowing the Marshall quartet's four
  individual branches.
- The screen equations and plate load line are solved together. Their implicit
  current slopes participate in the load-line Newton step, and the resulting
  nonlinear transfer and supply demand are baked into the existing
  preparation-time tables; the audio thread gains no iterative circuit solve.
  Regression coverage checks the individual/common resistor KCL, shared-node
  identity, screen-voltage compression under drive and table agreement.
- Modern remains operation-for-operation unchanged. The American upstream
  1 kOhm / 20 uF dynamic screen node, Marshall quartet/choke supply, reactive
  loudspeaker-reflected load and full positive-grid plate-current surface remain
  explicit next circuit stages.

## 2026-08-27 output-grid blocking and recovery

*Historical metrics in this checkpoint predate the source screen-grid loading
above; its two-grid conduction, coupling and recovery topology remains.*

- Replaced the static midband handoff between each American/British phase
  splitter and power pair with the two physical branches. American uses 100 nF
  output capacitors, 220 kOhm bias returns and 1.5 kOhm grid stoppers; British
  uses 22 nF, 220 kOhm and 5.6 kOhm. TubeLib's common 2 kOhm plus `IS=1 nA`,
  `RS=1 Ohm` diode branch supplies bounded terminal-grid conduction while the
  fitted power-tube plate-current surface remains clamped at its measured
  `Vg<=0` boundary.
- Every oversampled frame jointly solves LTP current and both terminal grids
  against the two trapezoidal capacitor histories. Grid current therefore loads
  the correct unequal PI plate and leaves common negative bias memory instead
  of disappearing in a scalar differential projection. The pinned overload
  reaches 0.787/0.801 mA American/British grid current with maximum KCL residuals
  below `1e-10 A`; after 1/20 ms the shared shifts are −14.84/−7.98 V and
  −18.98/−1.14 V, preserving the slower 100 nF American recovery.
- The power pair now accepts common and differential grid drive. Its 257 × 257
  quadratic terminal-grid plane is packed by tube-swap symmetry over 13 rail
  levels (6.576 MiB for both models); trilinear interpolation stays below
  0.0003 normalized output and 0.0004 demand error and subtracts its own idle
  interpolation error before sag. The validated musical domain has headroom to
  common `[-2, 0.5]`, differential `±4` and normalized terminal grid `[-7, 0]`.
- American/British/Modern alias floors are −62.22/−65.06/−61.28 dB,
  loud-versus-quiet compression is −13.70/−15.73/−18.33 dB and the same 90%
  Drop-E fixture measures +2.86/+6.34/+5.87 dB. Modern remains
  operation-for-operation unchanged. Demo 23 alone adds near-legato overloaded
  chord turns so the different blocking recoveries are audible under one global
  normalisation; demos 01–22 retain the established Modern path.

## 2026-08-27 measured ECC81 and ECC83 phase splitters

*Historical checkpoint: its static midband output loads and scalar differential
handoff are superseded by the dynamic two-grid checkpoint above; the measured
ECC81/ECC83 models and input networks remain.*

- Replaced the American/British ideal opposed-grid driver with loaded nonlinear
  long-tailed pairs. American uses TubeLib's measured-fitted ECC81/12AT7 family
  at the Twin Reverb AB763-family 410 V, 82/100 kOhm, 470 Ohm and 22 kOhm-tail
  operating circuit; British uses its separate ECC83/12AX7 fit at the
  literature-modelled 400 V with
  the 82/100 kOhm, 470 Ohm and 10 kOhm/presence-tail Super Lead family. The
  input poles are the documented 1 nF × 1 MOhm and 22 nF × 1 MOhm networks.
- Each table point solves both plate KCL equations against the finite tail and
  midband 220 kOhm power-grid-return loads. The 769-point runtime table stays
  within the checked direct-solve error, while DC anchors, loaded KCL, unity
  local slope, asymmetric ±1 outputs, common-mode rejection and total-current
  conservation are independently pinned. Letting both grids incorrectly follow
  the live tail fails those bounds.
- The source-derived American input capacitor now tightens 90/120 Hz by
  6.80/4.40 dB relative to 1 kHz. Post-circuit trim retains the previous 90%
  Drop-E level instead of undoing that frequency shape. American/British/Modern
  alias floors are −63.27/−66.27/−61.28 dB, compression is
  −13.18/−15.43/−18.33 dB and the same riff gains are
  +2.88/+6.49/+5.87 dB.
- Modern remains operation-for-operation unchanged, so demos 01–22 retain
  their committed samples and only the globally normalised demo 23 comparison
  changes. This strict-AB1 checkpoint projects the LTP differential output into
  the existing pair table; its sub-4% boundary is regression-pinned. Separate
  output-cap/grid voltages and TubeLib's generic grid-current branch belong in
  the next commit so blocking memory is added as one coherent circuit.

## 2026-08-27 measured 6L6GC and EL34 output stages

*Historical checkpoint: the ideal phase splitter described here is superseded
by the measured ECC81/ECC83 checkpoint above; the output-tube solve remains.*

- Replaced only the American/British opposed-12AX7 output approximation with
  distinct measured-curve-fitted uTracer TubeLib families: 6L6GC beam tetrodes
  on the documented 450 V / 400 V / −37 V / 5.6 kOhm AB1 pair and EL34 true
  pentodes on Mullard's 400 V / −36 V / 3.5 kOhm fixed-bias pair. An ideal
  balanced phase splitter and Raa/4 centre-tapped load make the remaining
  topology boundary explicit.
- A preparation-time 4,097 × 17 table per family solves output and
  plate-plus-screen demand over grid drive and simultaneous plate/screen rail
  loss. Runtime bilinear interpolation stays below 0.000086 normalized output
  error and 0.000093 demand error. Its normalized drive stops at the zero-grid
  AB1 boundary (±1), rather than pretending an ideal voltage source can drive
  positive grids; supply sag now changes the rails inside that solve and
  follows tube current rather than rectified output.
- The 6L6GC/EL34 equal-slope half-drive outputs are 0.48185/0.643711. Full-path
  American/British/Modern alias floors are −62.17/−68.17/−61.28 dB,
  loud-versus-quiet compression is −13.87/−15.43/−18.33 dB and the same 90%
  Drop-E fixture measures +2.88/+6.57/+5.87 dB. Every model's full Amp travel
  remains inside the playable level rail.
- Modern remains operation-for-operation unchanged, so demos 01–22 retain
  their committed samples. Demo 23 is regenerated from the same globally
  normalised three-instance comparison to expose the new A/B output dynamics.
  Grid-current/coupling-cap memory and nonlinear phase splitting were explicit
  next stages at this historical point; source screen-grid loading now
  supersedes its fixed-screen boundary, while a reactive speaker-reflected load
  remains open.

## 2026-08-27 three complete amplifier voices

This historical checkpoint's opposed-transfer output approximation is
superseded by the measured 6L6GC/EL34 checkpoint above; the three-path surface,
tone stacks, feedback, transformer, cabinets and untouched Modern branch remain.

- Added American Clean, British Crunch and Modern High-Gain as complete
  selectable amplifier, output-dynamics, transformer and speaker/cabinet paths;
  switching crossfades independent recursive state with a 15 ms exponential
  time constant and returns to one running circuit after the weights settle.
- American and British use exact third-order Yeh/Smith passive tone-stack
  transfers, the existing measured-12AX7 circuit table, opposed-transfer output
  approximations, causal output-derived negative feedback, independent sag and
  transformer state, and distinct six-section parametric cabinet voices. The
  exact pieces and voiced approximations are identified separately; these are
  not named-amp, power-tube, cabinet-IR or microphone replicas.
- The established Modern path remains coefficient- and sample-identical. On the
  same 90% Amp Drop-E fixture, model gain spans +2.98 to +5.87 dB; pinned loud
  versus quiet compression is −5.59/−10.80/−18.33 dB, alias floors are
  −73.38/−77.14/−61.28 dB, and 48-to-384 kHz response drift stays below 0.05 dB.
- Demo 23 renders the same deterministic power-chord, chug, chord and lead
  phrase through fresh American, British and Modern instances, with one global
  normalisation and no per-model loudness hiding.

## 2026-08-27 series-connected gain circuits

- Distortion and Amp used their drive value as a permanent parallel dry/wet
  mix. Demo 20's 22% clean amp therefore bypassed its own loudspeaker with 78%
  uncabbed DI, and the 95% metal rig still leaked 5% around the cabinet.
- Zero remains the exact, cost-free bypass. At any nonzero steady setting the
  whole pedal or amplifier is now in series; the knob changes drive, and the
  existing 15 ms exponential smoothing time constant acts only as a click-free
  relay when crossing zero.
- At the UI's minimum 0.1% drive, the then-sole amp path—now Modern
  High-Gain—keeps its 8 kHz response 22.003 dB below 1 kHz. The pedal remains
  18.182 dB down at 40 Hz and 20.961 dB down at 12 kHz; each response stays
  within 0.5 dB of its full-drive circuit response. Exact bypass, aliasing,
  level, lifecycle, transition and sample-rate rails still pass.

## 2026-08-27 Hammer-latched dedicated repicks

- E6..B6 and B0 are picking-hand commands, but a latched Hammer turned them
  into finger taps: plectrum contact, contact loss, Pick Noise, Alternate
  advance and Strum travel were all bypassed. Manual and automatic note order
  advanced while both contacts stayed Down; Pick Noise 0/1 was byte-identical,
  and a two-string 3 ms B0 stroke produced delays of 0/0.
- Dedicated repicks now interpret Hammer as a neutral Sustain pick contact only
  for that attack. The global Hammer latch remains ready for later playable
  notes; Slide and every already picked style are unchanged. No state or control
  was added.
- Regression rails pin alternating manual contacts, audible Pick Noise, B0's
  next-sample contact and one shared two-string stroke with distinct nonzero
  travel delays, while preserving the live fretting finger and Hammer latch.
  Demo 22 picks MIDI 74, genuinely hammers to 76, leaves Hammer latched through
  B0's picked lead and restores Sustain afterward; its duration is unchanged.

## 2026-08-27 finger-owned vibrato lifecycle

- A#0's shared onset previously aged without a fretting finger. After 500 ms of
  pre-hold, the first stopped note began about 10.04 cents high and reached
  38.04 cents at 60 ms; a coincident gesture and note began at 0 and reached
  only 4.75 cents at 60 ms.
- Key release left the last rocking offset on a tail: an ordinary release
  reached 38.60 cents within 60 ms, a sustain-held tail reached 50.97 cents
  within 500 ms and still held 35.24 cents, and a same-event A#0 plus fretting-
  key release reached 37.19 cents within 60 ms. An off-grid final Note Off could
  retain its preceding pitch offset for up to seven host frames at 48 kHz.
- The shared onset now advances only while a physically held stopped finger
  exists. Open strings and released or sustain-held tails remain fixed; a held
  stopped sibling keeps the hand moving, while final-key release immediately
  clears the onset and voice offsets but preserves A#0 intent. Fresh/off-grid
  refrets and stopped-open-stopped legato start from rest; overlapping ownership
  of the same held or delayed note preserves the onset already under way.
- Regression coverage spans 44.1, 48, 96, 192 and 384 kHz for pre-held silence,
  an open string and the first stopped-finger tick, then pins sustain-held and
  sibling release, immediate off-grid refret, stopped-open-stopped legato and a
  delayed same-note overlap. Demo 22 now pre-holds A#0 through a 240 ms rest so
  the next stopped note exposes the fresh bloom.

## 2026-08-27 live-damping pitch correction

- Changing Mute Pressure or moving the shared bridge hand refitted damping
  filters on already-ringing strings without translating the delay coordinate
  for their new phase. A shared E1 Sustain/Palm contact could move the vertical
  loop by 22.2 cents and the horizontal loop by 11.9 cents; a direct full CC2
  state change could exceed a semitone. The resulting chirp was a physical and
  playability defect, not a voicing choice.
- The engine now preserves the prior complete-loop period when damping alone is
  refitted: it evaluates the old-to-new damping-phase difference at that period
  and applies the opposite raw-delay translation immediately. Genuine pitch
  requests from bend, vibrato and refretting remain on their existing smoother,
  and dispersion-phase translation remains a separate additive correction.
- Regression coverage measures both polarisations on shared Palm/Open contacts,
  first-sample full CC2 jumps on E1 and B2, all 254 boundaries of an adjacent
  CC2 up/down sweep at 44.1, 48, 96, 192 and 384 kHz, and CC2 during the minimum
  40 ms wheel glide at 44.1 kHz. Contact and sweep steps remain below 0.25 cent;
  the combined bend comparison remains below 0.5 cent.
- The canonical Linux renderer changed demos 02–05, 08–20 and 22 while demos
  01, 06, 07 and 21 remained byte-identical. The correction is easiest to hear
  dry at demo 04's Palm lift/replant and throughout demo 15; demos 18 and 20
  expose it in high-gain musical contexts. No A–Z voicing selection was needed
  because the complete-loop pitch invariant settled the correction.

## 2026-08-27 fixed-delay feedback correction

- Replaced host-block-sized acoustic latency with a sample-rate-derived
  5.805 ms FIFO: 256 samples at 44.1 kHz, a voiced nominal distance of roughly
  two metres. The plug-in now renders, amplifies and returns causal chunks no
  longer than that delay, including at MIDI event boundaries.
- Added a bit-exact stereo plug-in rail for 64, 256 and 1024-sample host blocks,
  with CC1 and Note On at absolute sample 137 and Note Off at sample 4099. Added
  impulse-timing and constant-FIFO-depth rails at 44.1, 48, 96 and 384 kHz.
- Before the correction, identical macOS renders selected 2.667, 5.465 and
  2.217 kHz feedback modes in demo 06 at 64, 256 and 1024 samples; demo 14's
  tail selected 0.989, 2.717 and 2.744 kHz. The corrected renderer is
  byte-identical across all three partitions. Demos 06 and 14 were regenerated;
  the other twenty same-platform WAVs remain byte-identical.

## 2026-08-27 feedback-focus correction

- Reduced only the direct loudspeaker return into idle sympathetic loops to a
  voiced one-quarter share; played/releasing voices keep the full return and
  bridge-driven sympathetic coupling is unchanged.
- Added a closed engine/amplifier rail that keeps held A4's loop at least 6.02
  dB above the idle high E and distinguishes the quarter share from a
  still-hijacked one-half share. Equal and one-half direct drive lose by 16.2
  and 11.9 dB in the selection fixture; the current fixed-delay rail leaves
  one-quarter winning by 19.1 dB.
- Corrected demo 14's stale comments: the standard MIDI pitch wheel is uniform,
  not a per-string physical whammy-bar model. Demos 06 and 14 were regenerated
  without changing their scores or durations.

## 2026-08-26 Palm-body correction

- Removed the provisional 4 mm/100 ms Palm heel after user feedback and
  controlled A/B measurements showed that it stacked an over-damped transient
  on the steady bridge-hand model and erased the 30-80 ms low body.
- Raised only the existing Palm modal-excitation factor from 0.74 to 0.85.
  Default E1/E2 body improves by 1.27/1.57 dB, Open and Dead evaluator renders
  remain byte-identical, and no gain compensation, control or dependency was
  added.
- Replaced the sparse two-cell F2 fit rail with direction, rate-invariance,
  long-band selective-loss and explicit E1/E2 body guards. The exact-eight
  commissioned TRAIN/HOLDOUT gate remains open.
- Regenerated the thirteen Palm-bearing demos and all ten evaluator probes;
  the other nine demos and four Open/Dead probe WAVs remained byte-identical.

## 2026-08-26 performance-detail checkpoint

- Removed Strum Spread's hidden 20 ms cost from normal host and on-screen
  playing. Exact-sample Note On groups now pre-anchor the known physical edge,
  begin their leading string immediately and retain only the requested
  inter-string travel; a complete one-note group is sample-identical at zero
  and nonzero spread. The causal pre-roll remains only for scalar engine clients
  that deliver an incomplete chord over successive calls.
- Made each later MIDI timestamp a new performed wrist stroke. Complete-batch
  travel is permutation- and block-partition invariant, reverses with Up,
  advances Alternate once, and cancels an un-crossed released string without a
  late attack. The deterministic demo renderer now uses that same complete
  batching contract.
- Added a 23-second all-technique solo and four original single-guitar genre
  studies covering syncopated djent, modern metalcore, odd-meter progressive
  metal and dynamic blues rock. Their scores and effect moves are deterministic
  renderer code, not copied audio or opaque backing tracks.
- Made the existing held-string repick lane visible on the live fretboard:
  clicking a held row produces one hard attack through the same bounded UI
  queue and physical repick path as MIDI E6..B6. Empty rows remain silent and
  external MIDI keeps velocity control.
- Added the adjacent momentary B0 **TRM** wrist and an append-only 4-20
  strokes/s Tremolo Rate. One shared phase repicks every physically held string
  through that same path, preserves chord-wide Alternate direction, and skips
  contacts that would overwrite an in-flight Strum traversal. A same-string
  Hammer or Slide now moves an already-reserved contact to its latest fret
  without cancelling the travelling pick. Demo 22 exposes 8, 12 and 16
  strokes/s plus moving-note, vibrato and in-flight chord-slide use.
- Made the existing plectrum predicate the complete hand boundary: a fresh
  Hammer/tap now bypasses Strum Spread, Double's wrist offset, plectrum contact
  loss and all Pick controls; a legato Slide preserves the ringing loop and
  contributes only its moving finger friction, without re-anchoring a pending
  picked chord.
- Made descending Hammer-style legato a direction-aware pull-off: it releases
  at the old fret's physical position in the lateral plane and can continue to
  an open string, while ascending hammer-ons retain their established path.
- Kept the picking hand planted through non-plectrum legato. Hammer-ons,
  pull-offs and ringing-string Slides retain existing Palm damping on their
  target and leave sibling loops untouched; a later real pick still moves the
  shared hand. Demo 15 exposes the retained heel in a dry octave hammer/pull.
- Continued chained legato from the fret the finger has actually reached. A
  mid-glide Slide or Hammer now preserves its instantaneous pitch, derives
  direction and remaining travel from the fractional live fret, translates raw
  delay after the final retained-Palm damping solve and across dispersion phase
  changes, and never visits the abandoned destination. The same phase-coordinate
  correction keeps wheel motion monotone across quantised dispersion refits.
  Demos 02-06, 09, 11-15 and 17-22 were regenerated; demo 17 exposes the
  redirected descent.
- Gave Double's seeded second engine one deterministic 0-6 ms contact offset
  per picked wrist stroke. The primary engine stays sample-exact, chords share
  one offset plus strum travel, and no additional control was added.
- Audited a CC0 SM57/V30 cabinet IR. A magnitude-only refit improved isolated
  curve error but broke complete-chain level and muted-body rails. A default-off
  phase-preserved partitioned-convolution candidate now permits the controlled
  listening comparison while the shipping zero-latency cabinet stays unchanged.
- Deferred a stronger unilateral repick contact: its passive topology is
  available, but public repetition audio cannot set its physical constants.

## 2026-08-26 MIDI tuning checkpoint

- Made the standard MIDI pitch wheel move every played and sympathetically
  ringing string uniformly over ±2 semitones and the existing Bend Time.
- Left channel pressure and polyphonic aftertouch unassigned so controller
  pressure cannot silently sharpen fretted notes.
- Added the visible momentary A#0 **VIB** key without restoring either pressure
  mapping; velocity controls width and open strings stay fixed.
- Ended the on-screen piano at the playable D6. MIDI E6..B6 per-string repick
  triggers remain available to hosts and external controllers without being
  drawn as pitched keys.

## 2026-08-26 licensed low-register mute checkpoint (superseded)

- Added reproducible direction-only comparisons against a CC0 seven-string
  baritone muted/sustained matrix and HiMMP's CC-BY score-matched rhythm DIs.
- Deepened the provisional finite contact from 1.10 to 1.25 times the solved
  bridge-hand depth and extended its hold from 70 to 100 ms. A 1.40 mapping
  was already saturated; 120 and 140 ms holds failed the retained E2
  soft-to-hard expression rail.
- Kept the 4 mm fixed footprint and rejected per-hit heel jitter: the public
  sources do not identify enough contact geometry to justify another random
  player dimension.
- Preserved the all-string tuning rails and the rapid-repick grid; the full
  100 ms hold plus 10 ms release renders eight Palm voices at 0.205x realtime
  at 96 kHz on the checkpoint machine.

This finite-contact experiment is retained as development history; the newer
Palm-body correction above removes it.

## 2026-08-26 tuning and picking-hand checkpoint (Palm portion superseded)

- Corrected hard-pick pitch across all eight strings; the default isolated
  open strings now measure within 0..2.5 cents after attack, while the low
  chord spans -4.25..+0.75 cents.
- Replaced arrival-order chord fingering with one bounded eight-string match
  at each exact MIDI sample. Host, on-screen and mixed-source permutations now
  retain the same playable four-fret shape and bit-identical player variation;
  releases, controllers and repick gestures remain ordered boundaries.
- Made live CC2 bridge-hand pressure visible in the engine status, independent
  of the host's Mute Pressure knob.
- Stabilized a provisional Palm-only finite heel: Mute Tightness moves its
  4 mm footprint centre from 4 to 20 mm at the saddle, and a passive symmetric
  six-cubic-read contact initially held 70 ms and released over 10 ms; the
  licensed low-register checkpoint above extends that hold. The 0.74 attack
  darkener and Dead path remain unchanged.
- Added the E6..B6 per-string repick range. Its picking hand can restart a
  fully decayed held Mute or Dead string without adding note ownership, and it
  follows current velocity, Pick Stroke, Play Style, CC2 and Double state.

## 2026-08-25 realism and performance checkpoint

- Corrected Dead from an over-choked 30 ms click to an independent 1.6 s
  fretting-hand contact with a short, dark periodic E1 body; Mute Pressure can
  stack while Mute Tightness remains specific to the Mute articulation.
- Made Palm contact selective, force-aware and event-accurate; CC2 is smooth,
  delayed attacks cannot act before contact, and the newest actual Palm/Open/
  Dead contact now re-solves older ringing loops without rewriting their attack
  style.
- Added `PLAY-STYLE KEYS: LATCH | HOLD`: latching remains the default,
  while HOLD makes D#0..A0 momentary over a saved base style for playable chug,
  open-accent and ghost-note phrases.
- Replaced six exposed construction axes with one Guitar Build trajectory
  through six generic anchors; the fitted Drop-E build is the 0.8 default.
- Made Output Mode one Mono/Stereo/Double host choice; Double runs two
  differently seeded deterministic mono engines before one shared FX chain.
- Replaced the pedal and amplifier memoryless clipping curves with the
  circuit-solved antiparallel-diode RC node and measured-12AX7 plate-load
  stages, retaining oversampling, sag, transformer and cabinet voicing.
- Removed the redundant OUTPUT FIELD label, widened the three output buttons,
  renamed the bridge-hand UI to Mute, and gave every section heading the same
  slightly larger, brighter treatment.
- Added two focused Palm/Dead demos, a deterministic ten-probe evaluator, the
  licensed capture intake/collection validators and a strict, runnable blinded
  packer/scorer. The terminal capture and market-comparison gates remain open.

## Prototype baseline

Initial implementation in one JUCE codebase producing VST3, Audio Unit and
Standalone:

- Eight Drop-E string voices as dual-polarisation single-delay-loop waveguides
  with third-order Lagrange fractional reads, per-note stiffness dispersion
  fitted at two partials, decay-targeted loop filters solved by bisection, and
  a contractive shared-bridge coupling matrix.
- A fretting hand with a floating position and a four-fret reach; a picking
  hand with per-stroke contact variation and constant-velocity strum travel;
  two independent latching keyswitch banks, so any of three pick strokes can
  drive any of seven play styles.
- Point-touch harmonics as a bounded low-cost temporal contact surrogate —
  natural, pinch and touch — plus slides, hammer-ons and pull-offs, dead notes,
  palm muting as an additive absorber, and incidental fret collisions.
- A uniform ±2 semitone legacy MIDI pitch wheel plus lower/upper-zone MPE
  member/master pitch composition with exact semitone-plus-cent ranges over the
  Bend Time glide, and a resonance
  wheel that drives a bounded, saturating amplifier-feedback path through a
  fixed nominal 5.805 ms acoustic return, independent of host block size;
  pressure messages, MPE CC 74 timbre and release velocity are unassigned.
- A published pickup signal structure: per-string position comb following each
  fret, an O(1) fractional moving average for its chosen rectangular aperture,
  bounded flux nonlinearity differentiated into induced EMF, and a
  loaded coil/tone circuit.
- A four-mode solid-body structural path, geometry-informed and passive, with
  a six-anchor Guitar Build macro and independent pickup/tone/body-colour
  controls.
- An FX chain — distortion; selectable American, British and Modern amplifier,
  sag, transformer and parametric speaker/cabinet paths; compression; lead
  delay; and stereo room — with the circuit-derived nonlinear stages inside an
  up-to-8× oversampled domain, and every amount defaulting to a true 0 % dry
  setting.
- Mono as the summed dry DI, Stereo as a phase-coherent divided-pickup view
  that folds coherently back to mono, and Double as two independent engines.
- Audible-work culling: a faded-out pickup is skipped entirely, Mono runs one
  shared coil chain and mirrors it, and the whole engine freezes to exact zero
  once nothing vibrates.

## September 11: ordinary-playing realism

The fifth realism pass adds winding contact texture to normal picks, upper-mode
loss during note stops, speaking-length-aware palm damping, and contact-speed
variation across strums. The energy-derived pitch candidate is promoted in a
bounded six-cent form with continuous pitch/pickup updates; it no longer uses
the coarse update steps that raised the high-frequency floor. This supersedes
the earlier default-off status described in the historical experiment notes.
`ELECTRY_ENERGY_ATTACK_PITCH=OFF` remains available for controlled comparisons.

Nineteen identical-score phrases compare the previous source and the complete
new model, dry and through Modern. Listening copies are volume matched with a
constant gain per complete file. The real-recording comparison uses ordinary
DI notes and explicitly does not claim a completed listening study or market
ranking. See [the implementation and validation record](realism-pass5-2026-09-11.md)
and [the recording audit](realism-reference-pass5-2026-09-11.md).

## September 11: continuous open / muted / open playing

An original eight-bar rolling chord riff now benchmarks repeated palm
landing and lift while the fingers keep their notes held. It includes a
continuous pressure version, a Drop-E companion, and isolated unpicked
lift/repick controls, with dry, crunch and high-gain comparisons.

The model adds finite hand motion on ringing strings, retains residual
attack tension through muted repicks, and clears cached hand loss at the
exact open pressure endpoint. See the [transition validation record](../benchmarks/transition-benchmark-2026-09-11.md)
and [reproduction commands](../../Experiments/Transitions20260911/README.md).

The [second transition refinement](transition-refinement-2026-09-11.md)
corrects the time scale of palm damping on unplayed strings and preserves
heel relaxation during an open repick. The matched follow-up retains the
original riff and adds two low-string rhythms with gaps for comparison.
