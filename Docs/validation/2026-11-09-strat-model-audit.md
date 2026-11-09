# Strat model audit, 9 November 2026

Five-hour instrument review. Baseline: `eef9db8`
(2.4.1), after checking the initially supplied checkout against upstream.
The shipping Reason path uses `StratPerformer`/`StratEngine`; the JUCE plugin
still uses the separate `ElectryPerformer`/`ElectryEngine`. They are validated
separately. No listening preference is inferred from spectral measurements.

## Corrections and physical checks

- A repick's new tilt no longer rotates an earlier stroke's stored force.
  The top plane has its own linear highpass state. Completing an interrupted
  force uses its old orientation before the new force uses its new one.
  Zero-force tilt changes now leave both rail outputs exactly unchanged.
- Replacing a fractional point touch with a finite finger contact clears the
  point's interpolation state. This is reachable when a natural harmonic is
  released before the touching finger lifts, and previously bypassed the
  intended 25 mm release contact.
- A fractional dashpot reads and scatters with the same weights `w`. For
  `S = sum(w*w)` and `r = R/Z`, its midpoint resistance gives absorption
  `a = r/(2+r*S)`. The previous `r/((2+r)*S)` made the effective resistance
  as much as twice the specified value halfway between samples. The power
  scattering eigenvalue `(2-r*S)/(2+r*S)` has magnitude at most one for
  nonnegative resistance. Independent weak-loss continuum decay checks at
  48/96 kHz agree within 0.21%; the old halfway case erred by about 100%.
- Finger roughness resets its deterministic generator, as the other motion
  state does. Reusing a reset string now matches a fresh string exactly.
  Its lateral force also follows the fretting contact's own fixed direction,
  rather than the latest picking-hand tilt. A force-free pick leaves finger
  excitation identical during onset, rubbing, and the release ramp; the
  existing 0.35 lateral magnitude is retained.
- The pickup observer uses a fixed sensitivity axis for displacement and
  velocity. The previous axis depended on the last pick's tilt, so a
  force-free pick changed the field and the output by up to 2.82 V in the
  focused witness. A fixed scalar flux potential removes that discontinuity
  without inventing a measured two-dimensional magnetic field. Existing
  measured notch and sample-rate checks remain unchanged and pass.
  The retained leaky displacement observer is a low-frequency-stabilized
  approximation: its voltage is not the exact derivative of stored flux
  at DC. No observer leak or pickup gain is refitted here.
- The unsupported one-sided velocity clip at the termination is removed.
  A fret collision needs local displacement, clearance, and a positioned
  obstacle; rail velocity alone supplies none, and the clip also operated
  at fret 21 where there is no following fret. Bare-string signed-force
  symmetry is checked at frets 0, 5, and 21. This does not claim to implement
  fret buzz; a calibrated passive collision model remains future work.
- Silent strings clear their displacement observers when they sleep, so a
  stale subaudible position does not change the next attack's pickup gap.
- Held-bend pickup geometry follows fret changes and survives reset, just
  as the bridge's held tension does. Restating the same bend after either
  operation now produces identical output rather than repairing a stale
  cache. This preserves the existing quasistatic bend estimate.
- The first explicit upstroke uses the same stroke geometry as subsequent
  upstrokes. Direction belongs to the queued event's signed force, rather
  than the global first-stroke flag.
- A newer note taking a string cancels an older queued strike on that string.
  Previously, a slow strum could later retune the string to an obsolete note
  while public note ownership still reported the new note.
  A queued, unstruck note cannot start a legato/slide line merely because
  preparation or sympathetic motion has made its string move: it falls
  back to a picked onset. Released sounding tails retain their prior
  eligibility. Legato and slide also honor a valid, reachable explicit string
  request, so an older low-string line cannot steal a replacement intended
  for a still-queued higher string.

## Whole-keyboard measurements

Fresh Strat performers, 48 kHz, 256-sample blocks, three seconds held,
notes 40–85, velocities 1/24/50/77/100/127, neck/middle/bridge: 828 renders
per variant. Baseline, corrected pickup, and removed crown proxy are all
finite and non-silent, with no early-RMS velocity inversions. The routing checks
below separately classify unassigned MIDI keys and intentionally silent keyswitches.

The first four contact/state corrections leave all 828 isolated first-pluck
renders byte-identical to baseline. Their intended differences occur on
repicks, contact changes, and reuse after reset.

The fixed pickup observer changes early RMS by +0.001 to +0.381 dB (median
+0.112 dB), and attack centroid by −5.2 to +14.7 Hz. Removing the crown
velocity proxy in isolation changes early RMS by at most +0.612 dB,
broadband decay by −0.076 to +0.432 dB/s, and attack centroid by at most
6.35 Hz; median changes are zero. The largest neighboring early-level
step remains 3.486 dB at the physical G-to-B string transition, rather than
an isolated rogue note. These broadband slopes are not modal T60 fits.

Routing over all 128 MIDI keys is finite and matches the public mapping:
ordinary Strat sounds 40–85, harmonic Strat 40–95, and the legacy plugin
sounds written notes 40–98. Keyswitches, unassigned keys, and repicks with no
owned string are correctly silent. The legacy sweep covers all 59 pitched
keys at six velocities: 354 finite renders, no early-RMS velocity inversions,
peaks −49.0 to −20.6 dBFS at its configured −6 dB output gain, and broadband
decay 3.85–10.04 dB/s. This separate engine's source is unchanged.
Its largest adjacent-level step, 4.42 dB at written 70→71, is confirmed as
string 5 fret 3 to string 6 open, rather than an isolated failed note.

All six physical Strat strings at every fret 0–21, velocities 31/127, and
44.1/96/192 kHz add 792 finite renders with no velocity inversions. Within
one string, the largest adjacent early-level step is about 2.1 dB. The
44.1-vs-96 kHz early-level difference stays within 0.46 dB; 192-vs-96 kHz
reaches −0.95 dB early and −2.06 to +1.23 dB late, concentrated at fret 21.

That highest-fret rate discrepancy exposes an inherited spatial limit:
physical positions use `Fs/c`, while the stored rails omit the lumped
termination filters' phase delay. A neck coil physically before the fret
can therefore exceed `lastUsablePosition()` and clamp inward. On the G
string at fret 21 the clamp is 8.0 mm at 96 kHz and 2.7 mm at 192 kHz;
the low E also clamps at fret 20. Baseline `eef9db8` and candidate report
identical requested and clamped coordinates. Removing the clamp reads
outside the live string; uniformly rescaling every port redistributes the
lumped dispersion without establishing the correct local two-wave phase.
A distributed spatial dispersion model remains necessary to resolve this,
and the survey does not claim perfect rate invariance or no limitations.

The tension observer also retains a calibrated one-plane approximation:
`measureStretch()` estimates excess length from the poles-plane history,
although a normal pick drives both transverse planes. Simply summing the top
plane would approximately double the calibrated attack-pitch contribution;
the force mapping and two-plane energy model need to be reidentified together.
This review does not claim to resolve total two-plane stretch energy. Likewise,
large stiffness changes can refit the dispersion allpass coefficient while
retaining its direct-form state. A normalized energy-preserving transition
needs a separate control-trajectory qualification before replacing that model.

All 49 existing DSP unit/audio gates pass. The latest focused string,
engine, and performer suites also pass after the new regressions, including
12,288 randomized valid MIDI/control events across 44.1/48/88.2/96/176.4/192
kHz with output-buffer guards, finite output, and the shipping headroom
bound. This is deterministic stress coverage, not exhaustive proof.
All three focused suites also pass Clang 19 AddressSanitizer,
UndefinedBehaviorSanitizer, and float-cast-overflow checks. The first run
exposed a pre-existing test-fixture overread: truncating `0.7*44100` rendered
30,869 samples while the 0.2+0.5 s spectral window read 30,870. The fixture
now rounds its requested sample count and checks window bounds explicitly;
no model tolerance or test timeout was relaxed.

## Review of the supplied comparison proposals

The attachment is useful for hypotheses, but parts describe older code:
the engine already has a shared neck and no longer has `tensionShare=0.09`.
`onFret()` includes an open string's rigid nut termination, so its name did
not actually exclude open strings from the removed velocity proxy.
Its proposed first-order allpass cannot independently set a 900 Hz corner
and a 3–13 microsecond DC delay; it would also reverse the current split's
sign when inserted on the specified plane. The existing twin-split plan
instead discusses a second-order mass/spring termination with unidentified
loss. No replacement is justified by those constants.

A symmetric finite pickup aperture preserves point-comb zeros, multiplying
the spatial mode by the aperture transfer. A 10 mm box changes low-E H10
by about −0.085 dB, not the proposed eight-decibel improvement. Proposed
release exponents, slip warps, noise depths, and additional finger locations
likewise need calibration independent of unmatched sample-library spectra.
The linear tip oscillator's quarter-period is amplitude independent;
velocity saturation alone is not proof its equation is wrong.

The [62 Reissue Stratocaster's own provenance](https://www.pianobook.co.uk/packs/62-reissue-stratocaster/)
describes a Marshall JVM1 at the edge of breakup and a Celestion Greenback
cabinet, rather than a clean guitar DI. Its nonlinear onset, HF balance,
and sustain therefore cannot directly identify string/pickup parameters.
The [Clean El Guitar page](https://www.pianobook.co.uk/packs/clean-el-guitar/)
also reports noise cleaning. These are useful listening references, not
interchangeable physical measurement chains.

Palm and dead-note checks use 230 additional renders per variant (46 notes,
ordinary with palm 0, upstroke, dead-note, and palm 0.75/1). With the corrected model, the 2–4 kHz
attack band does rise under the strongest palm, e.g. −58.1 to −45.3 dBFS
on G3 over 0–35 ms, while total attack RMS falls from −26.0 to −27.4 dBFS.
The largest later relative-band jumps occur near −90 dBFS absolute. A
passive high-resistance load becomes reflective, so these figures alone
do not prove a faulty contact or justify an invented taper.

Dead-note 0.2–0.5 s audible-band RMS (above 20 Hz) is at most −79.5 dBFS
at velocity 100, against attack peaks near −15 to −20 dBFS. Several large
whole-band residuals are dominated by subaudio, rather than the alleged
400–500 Hz whistle. Prominence above a nearly silent spectral median is
not an audibility measure. No uncalibrated 65 mm/multiple-finger mute is
added on that evidence. Levels use the shipping host's 10 V reference.

## Validation and evidence

The acoustic checkpoint is `03635993628238606adb6d70586584bb69e9f2b2`
(2.4.2). Routing, legacy and forced-string/rate surveys, articulation
measurements, final acoustic unit/sanitizer suites, the version ledger,
and all six Linux plugin gates are complete. Those gates cover processor,
factory/more audio, editor rendering, and VST3/CLAP artifacts. The final
forced-string eligibility fix is included in the acoustic checkpoint and
the frozen pre-optimization library.

The pickup CPU checkpoint is `6c9d0102871c2abfa5c93dea462a217c7d636d68`. Shared pickup
addressing preserves 131,072 direct reader cases and all 58,668,170 mono frames
of the factory parity matrix exactly. Native factory CPU improves 3.4–11.0%
in aggregate against the acoustic checkpoint, and 5.3–12.4% against the original,
over six rate/block groups. Individual measurements and sporadic callback
overruns are retained in the [CPU report](../benchmarks/strat-cpu-2026-11-09.md);
the VM measurements are not a hard real-time guarantee. The separate legacy
plugin CPU-budget gate also passes.

The final source checkpoint is `620a3098d92b77e3e93d308359669fab396bbe37`. A full
callback audit exposed repeated five-note chord work over the 192 kHz/64-frame
duration. Bounded profiles identified repeated phase calculations inside the
dispersion fit. Computing those invariant terms once reduces the two profiled
callbacks' instructions by 50.4–52.6%, preserving 6,984 independent coefficient
cases and all 56 factory renders exactly. Final `620a309` passes all 49 DSP
gates again, the strict C++17 target, all three rebuilt sanitizer suites and the
reader boundary oracle, and all six rebuilt Linux plugin/artifact/UI gates.
The parent review also passes its source-exact native wrapper contract and
strict Clang 19 x86-64/AArch64 compilation. ARM compilation is not ARM execution
or Reason SDK acceptance. Final quiet native timing reduces aggregate CPU
against `eef9db8` by 7.39% for all seven factory patches at 48 kHz/64 and 10.31%
for Quack Rhythm at 192 kHz/64. The hoist adds 3.25%/3.96% savings over `6c9d010`.
The 14 repeated chord-event medians fall from 375–502 µs to 203–288 µs, below
the 333.3 µs duration; raw matched overruns fall from 42/42 to 3/42. Sporadic
overruns remain, so this is not a universal real-time guarantee. The earlier
six-group timing table remains specific to `6c9d010`; the final follow-up is
a separate, smaller comparison with raw paired callback data retained.
Scratch binaries, source snapshots, raw renders, scripts, and stage metrics
are in `/workspace/scratch/electry-2026-11-09/`. Final comparison audio uses
`eef9db8` versus `620a309`, with identical score/rate/seed/controls and recorded
constant level matching. All three pairs (factory Init, alternating repicks,
short harmonics) prove byte identity from acoustic `0363599` to final output.
Independent file-level audit confirms finite audio, positive scalar matching,
RMS agreement within 1.98e−8 dB, and a worst 4× reconstructed peak of
−1.0000007 dBFS. The preview keys and level/source ledgers preserve raw levels;
no listening preference has been inferred from these checks.
