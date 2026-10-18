# Dry guitar comparisons — 18 October 2026

This pass compares Electry with recorded guitar audio, with priority on
changing notes, repeated attacks and reuse of one physical string. It follows the user's
instruction to continue autonomously and choose the candidate in subjective
comparisons. That preference does not override failed engineering checks or
make an unmeasured guitar construction a calibrated target.

The implemented change protects returning notes from older MIDI ends and
preserves explicit duplicate-string broadcast semantics. Six controlled
performance auditions and nine real-recording comparisons are available in
the working room at `http://127.0.0.1:8721/`. Real source selection is frozen
in the [portable receipt](performance-reference-selection-2026-10-18.json).
The [validation report](../validation/performance-realism-2026-10-18.md)
records the tested correction, deliberately unchanged controls, failed retiming
prototypes and the remaining acoustic handoff problem.

## Sources and scope

[Guitar-TECHS](https://guitar-techs.github.io/) provides direct guitar output
split to an audio interface, separately from its amplified microphone signal.
Its [Zenodo release](https://zenodo.org/records/14963133) and project site
identify the professional performers, instruments, pickups and string gauges;
the project site licenses the data under CC BY 4.0. The low sixth-string
ordinary and Palm recordings are useful technique references. They are
six-string guitars: they cannot identify the construction, setup or contact
behavior of Electry's lowest eight-string E1. Source MIDI also requires checking
against the sounding audio; tracker output is not calibrated pick force.

[EG-IPT](https://zenodo.org/records/15205644), also CC BY 4.0, supplies original
96 kHz / 24-bit mono direct-input ordinary sixth-string notes in bridge, neck
and combined humbucker configurations. The three already downloaded originals
are checked against their retained SHA-256 receipts. The independently played
pickup takes do not constitute simultaneous measurements of one identical
string trajectory. Their E2 pitch is an audio inference, not a filename-derived
fret annotation.

The [existing reference ladder](evaluation.md#real-eight-string-reference-ladder)
also identifies genuine eight-string clean/dry one-shots by
[cabled_mess](https://freesound.org/people/cabled_mess/packs/29585/) under CC0.
When only the public compressed preview is accessible, it is a qualified
listening reference. It is not interchangeable with the original WAV for
spectral fitting. The local 8ridgelite Natural notes are a further eight-string
direction check, with no documented raw-DI chain; they must not be labelled
verified dry recordings.

No source recording becomes an Electry sample, excitation table or factory
asset. Working listening files retain source attribution and transformation
receipts. The stronger commissioned exact-eight capture contracts in
`ValidateReferenceFixture.py` and `CompareReferenceFixture.py` remain separate
from these descriptive comparisons.

## Listening and measurement protocol

Each comparison supplies a recorded Reference, unchanged main A and an isolated
candidate B when available. A and B use the same production Performer events,
physical string, controls, seed, rate and block size, with effects disabled.
Written MIDI is one octave above the sounding note. The reference is another
performance: its pick force, pressure, timing and some geometry are unknown.
Those asymmetries are recorded rather than hidden by post-EQ or time warping.

Onset alignment is one constant offset per take. All transformations, source
crop frames, sample-rate conversion and output hashes belong in the generated
manifest. Level matching uses one constant gain per whole comparison file and
shared peak headroom. It does not independently normalize attacks or late
tails. Switching at the same elapsed time makes envelope and brightness
differences audible; it does not imply sample or phase matching.

Measure attack timing, normalized envelope and partial decay separately.
Track each partial around its own spectral peak rather than assuming an exact
harmonic FFT bin. Report fit windows, noise floor, usable dynamic range and fit
quality. A low residual in one partial's exponential fit cannot establish the
entire note's decay law: beating and the two polarizations can produce curved
or nonmonotonic envelopes. Reference and held-out clips are reported separately;
no scalar is fitted to the listening examples.

## Performance correction: stale ends after a returning note

The public production Performer reproduces a handoff defect without changing
any sound controls. On physical string 0, written A=40 and B=43, send
`A on → B on → A on → old A off`. Main forgets the first A when B takes its
string. When A returns, the delayed first A's Note Off matches the new A and
starts its release. The new note stops even though its own key is still held.
Immediate repeated A/A notes already had an overlap counter; the intervening
different pitch exposed the missing lifetime history.

The correction retains outstanding accepted MIDI owners per sounding pitch and
expression channel independently of physical allocation. Displaced older ends
are consumed before the current owner is touched. Picking-hand repicks add no
fretting-key ownership. Existing cross-string and channel rules remain intact.
This is a causal event-ownership correction: it introduces no random timing,
new contact constant, velocity curve or substituted audio.

When an explicit mask concurrently places one legacy MIDI identity on multiple
strings, its old broadcast Note Off contract remains in force for that group.
An aggregate counter cannot infer a global FIFO ordering for those separate
physical duplicates. Unique-string displaced-owner protection resumes after
the group's last physical owner leaves.

Performance auditions cover A/B/A with a stale end, rapid repeated notes with
0/2/10/30 ms gaps, overlapping changes on one string, larger fret jumps and
Palm/open changes. A and B retain identical MIDI timing and crop positions.
Unaffected cases are useful controls and are identified as such. The real DI
phrases provide a separate performance reference; a corrected MIDI lifetime
does not establish that all of the model's attacks match a human player.

## Picked fret changes: state transport before pitch cleanup

Two actual dry P3 phrases retain an overlapped B2→C3 change and a later C#3
repick on the sixth string, with source MIDI timing and one shared latency
offset. They expose a useful distinction: event ownership can be corrected
without changing a well-balanced phrase's waveform, while an ordinary picked
refret still passes through the generic six-millisecond bend follower.

An independent frozen-loop root check finds roughly -80/-158/-238 cents of
remaining interval lag at first pick Release for ascending one-/two-/three-
semitone changes. A direct new-period read and a finite ramp over the existing
pick Contact window both remove that lag. Neither transports the old ringing
state correctly enough on the repeated real phrase: the registered transient
grows strongly for one old-wave phase. The equal-duration source/model
comparison and all limitations are in the validation report. Neither prototype
is included in the product.

The next mechanism requires a spatially consistent handoff of displacement,
velocity, reflected waves and pickup/filter history when a new fret changes
the speaking length. Simply accelerating the existing delay read, erasing the
old wave, or drawing a new damping constant would not solve that state
transport. Same-pitch repicks, intentional Hammer/Slide gestures, delayed picks
and wheel/vibrato motion must retain their separate causal behavior.

## Deferred experiment: envelope delay in the loss mapping

This experiment was frozen separately when the user prioritized performance.
It is not the selected runtime change. Its initial full-engine check also
missed the existing Palm upper-body loss rail: -14.9085 dB against the
unchanged requirement below -15 dB. No threshold was loosened and no candidate
was promoted on the strength of the numerical improvement alone.

The existing decay design maps a loop magnitude to a T60 using one traversal
per fundamental period. In a dispersive loop, phase delay sets modal tuning
while group delay determines the envelope traversal time. This distinction is
explicit in Julius O. Smith's
[Digital Waveguide Theory](https://www.dsprelated.com/freebooks/pasp/Digital_Waveguide_Theory.html).
His [loop-filter identification discussion](https://www.dsprelated.com/freebooks/pasp/Loop_Filter_Identification.html)
also treats partial tuning and decay as distinct observations and warns that
other filters inside the loop belong in the design.

For a static loop with transfer H and lightly damped mode at omega, the
first-order relation is:

```text
T60 = -3 ln(10) * groupDelay(omega) / (sampleRate * ln|H(exp(j omega))|).
```

The independent complex-pole audit solves `H(exp(s)) = 1` rather than merely
restating that approximation. On current main, the default heavy low E at
frets 0 / 12 / 22 has H16 decay-to-period-mapping ratios approximately
0.949 / 0.846 / 0.703 at 48 kHz host rate. The effect concerns upper-partial
color decay; the corresponding fundamental ratios are near unity. These are
isolated vertical-loop results, excluding horizontal detuning, polarization
coupling, time-varying contact and output coloration.

The candidate preserves the existing feasible decay targets and fixed loss
sections, and changes their conversion to the one-pole/scalar loop loss.
It needs fresh dispersion geometry, consistent fundamental compensation,
bounded passive solutions and exactly repeatable no-op refits. Numerical
consistency alone does not establish greater resemblance to a real DI: the
existing voicing may already absorb part of the old mapping's error. Recorded
Reference/A/B comparisons and their measured limitations therefore belong
beside the numerical validation.
