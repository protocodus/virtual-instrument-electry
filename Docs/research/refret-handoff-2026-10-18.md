# Picked fret handoff research — 18 October 2026

Status: **selected engineering candidate `60f7bec`** under the user's explicit
autonomous instruction. The [validation record](../validation/refret-handoff-2026-10-18.md)
reports pitch, lifecycle and real-DI evidence. This is not a listener vote or
an acoustic parity claim.
The previous [dry-reference investigation](dry-reference-comparisons-2026-10-18.md)
found that accelerating the existing delay follower removed its pitch lag but
increased short attack transients. Both the abrupt jump and Contact-duration
read-head ramp were withheld.

## Why investigate two fixed taps

A moving read head changes the sampled waveform's instantaneous frequency in
addition to changing the loop's eventual pitch. Smith derives
`omega_out = omega_in * (1 - dD/dt)` for a time-varying delay in
[Time-Varying Delay-Line Reads](https://www.dsprelated.com/freebooks/pasp/Time_Varying_Delay_Line_Reads.html).
Reducing the follower's time constant therefore does not resolve the underlying
tradeoff between a swept delay and an abrupt change in the ringing state.

Smith's [Large Delay Changes](https://www.dsprelated.com/freebooks/pasp/Large_Delay_Changes.html)
describes crossfading the old and new read pointers in one shared delay buffer.
The reads do not visit intermediate lengths, so the handoff avoids the Doppler
sweep. Only the reads need duplication during the fade; a recursive
interpolator may also require its new state to be warmed before use. Electry's
existing four-point cubic reads are FIR evaluations of already available ring
samples and have no separate recursive interpolation memory.

Jaffe and Smith give a directly related string-synthesis example in section 2.2
of [Performance Expression in Commuted Waveguide Synthesis of Bowed Strings](https://www.davidajaffe.com/writings/performance-expression-in-commuted-waveguide-synthesis-of-bo.html).
An abrupt period change creates a discontinuity that recirculates as a spurious
pluck. Their replacement reads two periods from the same memory and crossfades
the results. They explicitly qualify the algorithm's physical interpretation.
Their bowed-string transition timings are not guitar measurements and are not
adopted as Electry thresholds.

The Electry handoff blends old and new loop reads **inside the
feedback path**, using nonnegative complementary smoothstep weights over the
already existing pick Contact interval. It retains the ringing line, loop
filter memory and pickup observation history. It does not reset the string,
fade the output, add a voice, or introduce a new damping or timing control.
The existing continuous follower remains appropriate to deliberate slides and
continuous pitch expression. The finite Contact interval is a practical reuse
of current articulation timing, not a measured duration of changing frets.

## Spectral and energy limits

For two ideal fixed taps, the instantaneous frozen response is
`H(z) = (1 - w) * z^-D_old + w * z^-D_new`. Carpentier's
[SMC 2024 paper, sections 3.1–3.3](https://smcnetwork.org/smc2024/papers/SMC2024_paper_id118.pdf)
describes this method and its limitations: it introduces temporary comb
coloration and phase distortion, with cancellation notches when the two taps
have equal weight. There is no universally suitable fade time. A lower click
metric alone cannot establish that the handoff sounds natural or that the
remaining waveform reaches the target pitch promptly.

The following bound was checked independently by exact symbolic expansion
against the four coefficients in Electry's `readFractional()`. For its centered
third-order Lagrange read, let `t` be the fractional coordinate in `[0, 1]` and
`u = sin(omega / 2)^2`. Then

```text
1 - |H_t(omega)|^2
  = (4 / 9) * u^2 * t * (1 - t) * (2 - t) * (1 + t)
      * [3 + 4 * u * t * (1 - t)] >= 0.
```

Each fixed cubic tap consequently has magnitude at most one in exact
arithmetic. A convex blend of two such frozen responses also has magnitude at
most one by the triangle inequality. This is a fixed-response statement, not
a proof that the time-varying feedback system is passive or energy conserving.
When lengthening the delay, moving weights can read the same packet first at
the short tap and later at the long tap. Cubic coefficients also include
negative terms, so interpolation does not guarantee a raw per-sample maximum
bound. Repeated transitions, especially descending pitches, require stress
checks. Existing contact loss must not be described as a general stability
proof or silently strengthened to conceal a failure.

## Physical interpretation and acceptance evidence

An actual leading fret changes a boundary condition on a physical string;
crossfading two loop lengths is an efficient engineering approximation. The
[DAFx 2011 string–fret model](https://www.dafx.de/paper-archive/2011/Papers/96_e.pdf)
distinguishes approximately clamped vertical motion from horizontal slip and
friction. A two-tap handoff does not resolve that contact, inactive string
segment, or its spatial displacement and velocity fields. It must not be
presented as a calibrated moving-fret simulation.

Acceptance must combine target-pitch settling and transition measurements with
the recorded dry performances. Use the same source crop, event times, model
controls and one whole-file gain protocol for unchanged main and candidate;
retain the existing registered alignment instead of fitting each candidate's
onset separately. Report attack-step and high-band context together with
pitch and envelope behavior. The six-string reference guitars remain qualified
performance proxies for an eight-string model.

Lifecycle checks must cover both pitch directions, same-note repicks, slides,
pitch expression, delayed strums, note ends and new notes that interrupt the
handoff. A restarted crossfade cannot simply jump from the current two-tap
mixture to one of its endpoints. Any bounded representation or pending-target
policy needs explicit interruption evidence. The selected correction passes the documented pitch/lifecycle checks and
recorded counterexamples; remaining coloration and hardware limits still
apply.

## Additional recorded descending phrases

Source-only inspection of the already cached Guitar-TECHS P3 performances
identified two additional single-string passages in `directinput_12.wav`:
D-string G3–F3–E3 at 10.70–11.87 s and high-E-string G5–F5–E5 at
16.00–17.10 s. Independent decoding of the original `midi_12.mid` agrees with
all cached per-string events. No other string has an annotated owner or onset
inside either crop. Waveform and spectrogram inspection shows the selected
harmonic series rather than an accompanying chord. This does not establish the
absence of handling noise, residual body motion or sympathetic vibration.

The new source12 alignment group uses the D-string first-note onset only:
native frame `519353`, detected by the existing centered 2 ms RMS quarter-peak
oracle. Its MIDI-to-audio offset is `-0.023438561458332785 s`. The high-E passage
shares that offset unchanged; its measured onset is 213.6834 frames
(4.4517 ms) later than the common prediction. Crops end while the last selected
note is held, 66.37 and 79.22 ms before the respective following different-string
MIDI onsets, with the source waveform inspected for attack precursors. Earlier
source06 crops, events and their alignment group remain unchanged.

The [official hardware and license information](https://guitar-techs.github.io/)
identifies the same P3 performer, Sire T7, .010–.046 strings, bridge-neck pickup
setting and UMC202HD interface. The added high-E passage is a register and
string transfer check, not a repetition or an independent hardware holdout.
The MIDI velocities and note gaps remain source annotations; the weak later
high-E notes are not independently boosted.

Other apparent descending phrases were excluded: source06 contains multi-string
chord gestures, source02's final E-string descent retains G/D-string owners,
and the inspected source03 treble descents sit over held lower strings. No
isolated one-shots are spliced into a synthetic reference performance. Cached
source12 WAV/MIDI lengths, CRC32 and SHA-256 values were rechecked against the
retained official archive-member receipt. Acquisition used selective ZIP byte
ranges, so no whole-archive SHA-256 is claimed.
