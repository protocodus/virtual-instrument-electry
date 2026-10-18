# Picked fret handoff validation — 18 October 2026

The user requested an autonomous correction of the ordinary picked fret-change
glide, comparisons with real dry guitars, and a main merge only after green
checks and addressed feedback. This is an engineering improvement to note
transitions; it is not a claim of perceptual parity with an eight-string guitar.

## Selected behavior

Baseline `49f42e347fa021535e4892a357796e7533d3d42e` sends a small picked
fret change through the continuous bend follower. Its moving read head scans
the intervening ringing history, adding an unintended pitch sweep. The selected
DSP, `60f7becdcfcaa2a15ef3648c61e22411cf4202a7`, instead crossfades two stationary feedback reads.
Both use the same ringing line, damping/dispersion filters and pickup history.
The nonnegative complementary smoothstep weights have exact source and
destination endpoints. There is no output fade, reset, additional voice,
buffer copy, allocation, random draw or new control.

The handoff begins at the fretting event, before the scheduled pick, and uses
the existing plectrum Contact duration. That duration is an engineering proxy,
not a measured fret-contact law. Its independent clock survives note ends and
pick cancellation. An interrupted handoff finishes its audible mixture before
starting the latest queued destination; the new pick can wait for the previous
contact's remainder. Same-clock replacements retain the original, never-heard
source. Stopping-hand filtering and temporary harmonic/thumb touches are
evaluated at each endpoint before mixing. Explicit slides and hammer gestures
retain their intended continuous trajectories.

The existing large-jump path already retuned immediately. Its 0.28 retained
amplitude and corresponding energy/tension attenuation remain intact. A
trial that removed this choke raised the alternating-fret control's raw RMS
by about 5.9 dB and its level-matched maximum sample step by about 5.7 dB;
that expansion was rejected. A queued same-destination manual repick must not
apply this choke again. Independent review reproduced that prototype defect
through the public per-string repick event and verified the final guard.

See the [primary-source rationale and limitations](../research/refret-handoff-2026-10-18.md).
Fixed-tap mixtures can cause temporary comb coloration. A frozen response
bound does not prove passivity of the changing feedback system.

## Pitch and lifecycle evidence

`Electry.Refret` evaluates the actual complex cubic-read transfer, including
the two allpass groups and loss-filter phases. During a handoff it observes
the weighted sum of both branches; the new `currentDelay` alone is not treated
as the sounding pitch. Forty-eight small-refret cases cover both directions,
three string registers and four host rates. The unchanged baseline fails all
48 release-pitch witnesses.

The selected suite passes 63,111 assertions. The maximum absolute release
error against the live energy-raised target is 2.98849861 cents; the maximum
residue beyond the independently observed intentional tension span is
0.0000636685 cents. The existing attack-energy law can raise pitch by up to
six cents and is retained. These are frozen-loop state diagnostics, not
short-window measurements of nonlinear guitar audio.

Additional checks cover all five picked styles, deliberate Hammer/Slide,
same-note controls, same-clock replacement, queued small/large moves, public
per-string repicks, delayed strum cancellation, sustain, reset/reprepare,
released MPE pitch ownership and exact one-versus-64-frame chunking at normal
and endpoint rates. Queued release checks require the destination endpoint,
zero handoff remainder and an unlocked delay. Existing large-jump state
assertions and spectral/tuning rails were not weakened.

## Recorded comparisons

The room at `http://127.0.0.1:8722/` contains two original Guitar-TECHS source06
E-string phrases and two additional source12 descents: G3–F3–E3 on the D string
and G5–F5–E5 on the high E string. The source-only selection preceded candidate
audio. Chord-overlapped alternatives were excluded. The original two source
rows and their 1.1-second crops are unchanged.

The [portable selection receipt](../research/refret-reference-selection-2026-10-18.json)
binds official archive members, CRCs, published archive MD5, original WAV/MIDI
SHA-256, native PCM crops, decoded annotation events, physical string mapping,
license and attribution. No uncomputed full-archive SHA is claimed. These are
CC BY 4.0 six-string Sire T7 recordings, with .010–.046 strings and bridge/neck
pickup blend, through a UMC202HD. Model geometry and player force differ.

Each source recording has one development-derived timing offset. The new
high-E transfer retains its approximately 4.45 ms onset residual. Model B
uses A's exact integer crop; no candidate onset fitting, per-note alignment,
time warp, pitch shift, EQ, denoising or per-window gain is applied. Original
MIDI velocities are annotation dynamics, not measured pick force. Each take
gets one constant whole-crop RMS trim plus shared sample-peak headroom.

`Tools/AnalyzeRefretComparisons.py` independently verifies source and model
hashes, source-plan path resolution, identical scores and shared crops. It
reports every annotated attack in fixed −5..+10 ms, 0..30 ms and 20..80 ms
windows: maximum adjacent-sample step, causal fourth-order 8 kHz high-pass
RMS, window RMS and peak. Both raw and whole-crop-matched values are kept so
attenuation cannot silently win a transient metric. These observations are
counterexample diagnostics, not calibrated perceptual click thresholds.

Across all 14 registered primary attack windows, B/A matched sample-step
ratios range from 0.9363 to 1.0369, with high-band changes from −0.641 to
+0.0368 dB. Primary raw window RMS ranges from 0.9866 to 1.0947 times A;
this result is not produced by blanket attenuation. Wider windows retain
changes: up to 1.1063 times the sample step in 0..30 ms and +1.006 dB
high-band RMS in 20..80 ms. Those later envelope/timbre differences are not
hidden or called perceptually equivalent.

The repeated C3 counterexample uses its unchanged registered −5..+10 ms
window and one −24 dBFS whole-crop gain per take:

| C3 diagnostic | Real DI | Main A | Selected B |
| --- | ---: | ---: | ---: |
| Development maximum sample step | 0.00479407 | 0.02267641 | 0.02123087 |
| Development 8 kHz high-pass RMS | 0.00010668 | 0.00032433 | 0.00031018 |
| Repeat maximum sample step | 0.00623951 | 0.01714351 | 0.01634916 |
| Repeat 8 kHz high-pass RMS | 0.00011742 | 0.00027078 | 0.00027193 |

The earlier abrupt and moving-ramp prototypes instead reached about
0.0555–0.0560 step and 0.00103 high-band RMS in this repeat. Their failure
remains visible under the same crop/gain protocol. Real and model values are
still materially different; different hardware and force preclude fitting
parity from this table. An independent observer checked 1,008 measurements
across all four excerpts, agreeing within 5.36e−14.

Four controlled performance probes are byte-identical to A: 8 Hz repeats,
12 Hz repeats, rapid Palm/open changes and the alternating-octave large-jump
score. Returning and overlapping small-fret probes differ as intended.

## Reproduction and final checks

Use Python 3.12 with `Tools/requirements-review.txt`. Reconstruct the frozen
working plan from the licensed original members and portable receipts; source
recordings remain outside Git and are never used as runtime excitation.

```sh
python Tools/BuildPerformanceComparisons.py --base 49f42e3 \
  --candidate 60f7becdcfcaa2a15ef3648c61e22411cf4202a7 --output build-refret-review
python Tools/BuildPhraseComparisons.py --base 49f42e3 \
  --candidate 60f7becdcfcaa2a15ef3648c61e22411cf4202a7 \
  --plan build-refret-review/reference-analysis/reference-plan-phrases.json \
  --output build-refret-review
python Tools/AnalyzeRefretComparisons.py \
  --manifest build-refret-review/phrases/measurements.json \
  --output build-refret-review/evidence/phrase-analysis-final.json
```

Local validation passes all 38 unit suites, all 23 serial/parallel demo WAVs
with byte determinism, and the embedded C++17 target with its restricted flags.
The final focused rerun also passes the strengthened manual-repick regression
and both source-path/analysis suites. The exact final test file produces 62
failures against frozen baseline, including every one of the 48 release-pitch
cases. Removing only the queued-repick guard produces one targeted failure.

A new queued-large fixture initially assumed a blanket five-cent comparison
to the live target. The existing energy law permits six cents after a fresh
seed; replacing that assumption with the one-sided observed energy span and
0.05-cent numerical allowance diagnoses the actual interval lag instead.
The final checks also require the handoff to be complete at Release. No old
large-jump, spectral, stability or tuning tolerance was loosened.

All 24 delivered WAVs pass independent format/rate/length/finite checks,
local and HTTP hashes, original MIDI/score binding, fixed crops and one
constant gain. Maximum sample-domain reconstruction error is 1.5203e−7 FS;
maximum within-pair/triple RMS spread is 3.3523e−8 dB. Both generated scripts
pass `node --check`, and all eight static navigation targets return HTTP 200.
All WAV hashes are identical across the source-comment-only amendment.
The [machine receipt](refret-handoff-2026-10-18.json) binds the final source,
logs, independent observers and immutable comparison manifests. Hosted CI
and review status are recorded on the linked pull request.

Working evidence is retained in `build-refret-review/`, including the rejected
unchoked large-jump renders. The machine receipt binds the final source,
comparison manifests, observer and validation logs. No subjective listener
vote, interactive browser audio QA or Reason-host playback claim is inferred
from file integrity, analytical pitch checks or native renders.
