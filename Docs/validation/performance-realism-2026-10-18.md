# Performance realism validation — 18 October 2026

The user prioritized note changes, repeated notes and reuse of one physical
string, and authorized autonomous candidate selection. The selected ownership
correction fixes a reproduced event-lifetime defect. Real DI comparisons are
descriptive performance references, not a calibrated claim that an eight-string
instrument matches the six-string recordings.

## Returning notes and displaced ends

Main `aab8bcaac863739a086c5178dead3c2ea432cdf1` stops the newer A in
`A on → B on → A on → old A off` on one string. A public Performer witness at
48 kHz / 64 frames changes from `keyDown=1, releasing=0` to
`keyDown=0, releasing=1` at the older A's end. The correction in `976931c`
retains outstanding accepted owners per sounding pitch and expression channel
independently of the physical string. An old displaced end cannot modify the
new finger, release noise, vibrato, bend freeze or legato travel.

The focused suite has 48 combinations of four styles, three host rates,
legacy/member expression and scalar/chord calls. Each stale-end waveform must
equal a copy with that obsolete end omitted, sample for sample. The genuine
final end must still release. Further cases cover same-note overlap, sustain,
panic/reset, picking-hand repicks, rejected allocation, chord reassignment,
explicit masked duplicate identities and a 65,536-owner flood. The new test
object linked against frozen old DSP fails 147 assertions; the corrected DSP
passes. No existing tolerance or sound target was relaxed.

The count array costs 8,704 bytes per engine plus 272 bytes of broadcast-group
flags: 8,976 bytes per engine, or about 17.53 KiB for two engines. Explicit
string masks can put one legacy MIDI identity on multiple physical strings;
MIDI supplies no per-string owner ID there. That group's existing broadcast
end is preserved rather than inferring an ordering from aggregate counts.
Once its last physical owner leaves, a later unique identity regains
displaced-owner protection. An independent review caught and corrected the
inverse-order case where a newer duplicate is stolen before the older one
ends; dedicated regressions cover both directions and uneven overlaps.
It adds no allocation, lock, I/O, random draw, parameter or floating-point
processing to the render loop. Existing explicit-mask broadcast Note Off
behavior is preserved. The host-neutral sources compile as embedded C++17.

## Independent demo-score lifetime checks

The full audio tier exposed missing ends in six pre-existing imperative scores:
03: E2 twice; 15: E1 once; 17: E4/G4/A4/C5 once each; 18: E4/B4 once each;
21: E4 twice/G4 once; 22: E1 once. These are deficits relative to all starts,
not new note events added by the engine. Correct the source scores by ending
each owner in their existing release group, with no changed starts, waits,
velocities, styles or controller trajectories.

The renderer now counts score owners separately from physical voices. A stolen
string cannot hide an unmatched MIDI start, and an appended take cannot borrow
another take's end. A negative control in the audio test proves that an
unbalanced stolen start fails, that the parent's unrelated end cannot fix it,
and that its actual matching end passes. All 23 complete scores must pass this
stronger check.

The blues lead's final legato E4 previously retained one physical owner after
the release group at frame 389,844. It rang until a later B4 stole the string
at frame 424,240, about 0.780 seconds later at 44.1 kHz. Ending that leaked
owner at the intended group changes its tail deliberately; do not describe
this score correction as audio-neutral. The old-DSP before/after render
receipts distinguish this change from the other corrected scores.

## Actual recordings and controlled comparisons

The [portable selection receipt](../research/performance-reference-selection-2026-10-18.json)
records nine selected excerpts from nine source assets, including archive
members, CRC/SHA-256, original PCM crop hashes, attribution and CC BY 4.0
licensing. The working room at `http://127.0.0.1:8721/` links:

- Six controlled model performance probes: A/B/A, 8/12 Hz same-note repeats
  with 0/2/10/30 ms gaps, overlapping one-string changes, larger fret changes
  and rapid Palm/open transitions.
- Two actual Guitar-TECHS P3 direct-input E-string phrases,
  B2 → C3 → C#3 → C#3, with original annotation timing and velocity. A single
  development-derived latency offset is shared with the repeated excerpt.
  Its approximately 0.5 ms onset residual remains visible. Model B uses A's
  crop exactly; later attacks are never separately aligned.
- Seven real ordinary/Palm note references from EG-IPT and Guitar-TECHS.

Reference is actual audio; A is unchanged main; B is the identified candidate.
The real guitar, pickups, strings, player force and note ends are not identical
to the model's declared controls. Fishman MIDI velocity is annotation dynamics,
not a force measurement. The repeated phrase is a structural repeat check on
the same player/instrument, not independent held-out hardware. Conservative
crop ends exclude the following chord's precursor.

All matching uses one whole-file RMS trim per take plus shared sample-peak
headroom. No EQ, denoising, time stretching, pitch shift or per-window gains
are applied. The player adds only a 10 ms switching fade; downloaded files do
not contain that fade. Attribution accompanies the working reference WAVs.
No reference audio, MIDI sample instrument or inferred calibration coefficient
is incorporated in the product.

## Further candidates and limits

A direct picked-refret delay jump corrects an unintended 6 ms pitch follower:
at the first pick Release (about 1.583 ms), one-/two-/three-semitone ascending
changes were still about -80/-158/-238 cents from their new target. Same-note
repicks had no such interval lag. The abrupt jump removes the interval lag,
but the real phrase repeat supplies a counterexample: normalized 0–10 ms
maximum sample step rises from 0.017806 to 0.058840, and 8 kHz high-pass RMS
rises by 12.503 dB. A finite read ramp over the existing pick Contact duration
also reaches the new pitch but retains a 3.28× step and +12.44 dB upper-band
rise in that repeat. Both temporary candidates are withheld. These are
engineering discontinuity diagnostics, not calibrated hardware limits or a
subjective verdict that a larger transient is always less realistic. Neither
prototype supplies a complete physical remapping of the ringing state or a
validated interrupted/delayed-contact handoff.

An independent repeat-context check uses the same 1.1-second crop for every
take, one -24 dBFS whole-crop RMS trim each, and a fixed -5..+10 ms window
around the source-registered C3 boundary. No peak search or per-note alignment
is used. The source and model remain different guitars and performances.

| Repeat C3 diagnostic | Real DI | Main A | Abrupt retune | Contact-duration ramp |
| --- | ---: | ---: | ---: | ---: |
| Maximum sample step, full-scale units | 0.006240 | 0.017144 | 0.055994 | 0.055532 |
| Causal 8 kHz high-pass RMS | 0.0001174 | 0.0002708 | 0.0010351 | 0.0010274 |

The consistent-window check supports withholding the large transient on this
particular repeat, while providing no universal threshold or perceptual parity
claim. Its observer, source hashes and complete measurements are retained in
`evidence/refret-equal-crop-context.json` within the working review directory.

The earlier group-delay loss trial remains frozen separately and unselected.
It improves consistency with the existing mathematical decay targets but misses
the unchanged Palm upper-body loss rail (-14.9085 dB versus below -15 dB).
The user then explicitly prioritized performance. The runtime ownership change
does not include that experiment or relax its failed rail.

## Reproduction and checks

Install `Tools/requirements-review.txt` in a Python 3.12 environment before
configuring the test build. The comparison tools build immutable Git snapshots
with the same driver. Working plans bind the original source hashes and
integer crops; the portable receipt identifies the lawful archive members
needed to reconstruct those plans locally.

```sh
python Tools/BuildPerformanceComparisons.py --base aab8bca --candidate f64631b \
  --output build-real-dry-review
python Tools/BuildPhraseComparisons.py --base aab8bca --candidate f64631b \
  --plan build-real-dry-review/reference-analysis/reference-plan-phrases.json \
  --output build-real-dry-review
```

Final DSP `f64631bb9b635aea3e45094acbf127f7b1ac6105` passes 36/36 unit suites
with clean Python 3.12. The embedded C++17 target builds, and the final audio run passes all 23 serial
and parallel WAVs with identical bytes and preserved unrelated files. The
comparison-tool tests independently check source/crop integrity, known RMS
gains, shared A/B offsets, retained repeat timing and exact score events.
The [machine-readable receipt](performance-realism-2026-10-18.json) binds the
final candidate, source hashes, checks, source-selection receipt and listening
manifests. Hosted checks are reported on PR #4.

The final room audit verifies all 39 delivered WAVs, including decoded format,
native rate/length, local and HTTP hashes, fixed crops and constant gains.
Maximum within-comparison RMS spread is below `4e-8` dB. All three generated
inline scripts pass `node --check`; immutable source snapshots and build/tool
hashes also match their receipts. These are integrity checks, not a subjective
listening verdict.

Working evidence lives under `build-real-dry-review/`: `performance-audit/`
contains old/new ownership witnesses, unit/audio logs, score-lifetime traces
and refret counterexamples; `evidence/` contains source/WAV/hash/HTTP/JavaScript
integrity audits. Local browser automation is disabled, so these checks do not
claim interactive browser or Reason-host listening validation. The Python
snapshot build path has not been independently exercised on Windows; the
CMake renderers and tool tests are included in hosted platform builds.
