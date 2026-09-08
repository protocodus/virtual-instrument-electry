# Eight-string reference checks — 8 September 2026

This pass newly downloaded and measured three small real eight-string previews.
They guide conservative checks on the attack and decay of the five contact
changes, rather than identify pick, hand or pickup constants. These sources
already appear in [evaluation.md](evaluation.md); they are not new holdout data.

## Audio provenance

The creator, **cabled_mess**, identifies the
[F#-string pack](https://freesound.org/people/cabled_mess/packs/29585/) as clean,
dry recordings of an eight-string through an RME Babyface into Cubase 10.5.
Each selected sound page grants CC0. The pages specify 96 kHz, 24-bit mono
originals. Original downloads require login; this pass used the public high
quality MP3 previews, decoded by FFmpeg to **48 kHz mono float WAV**. The MP3
hashes below identify the bytes actually analyzed, not the original WAVs.

| Recording | Capture fret on the F# string | Preview SHA-256 |
| --- | ---: | --- |
| [F#1, sound 525010](https://freesound.org/people/cabled_mess/sounds/525010/) | 0 | `bba14e50cf747a9e17426db0d9e378f5f24f8ddae8ed1f9772dd1fb830d8ac09` |
| [C2, sound 525008](https://freesound.org/people/cabled_mess/sounds/525008/) | 6 | `2cc521f211871371a75bcc17030ff8e9bbf9c134dae7cd6b14c04c154bd5f346` |
| [F#2, sound 525009](https://freesound.org/people/cabled_mess/sounds/525009/) | 12 | `541754d503d91831e6ba0fdd739a0452566374605b1c32eaca48f6b69029f34c` |

The three MP3s total 133,872 bytes and remain in
`/tmp/electry-realism-reference/`, alongside source-page snapshots and decoded
WAVs. No recorded audio is added to the instrument or repository. All three
frets are even, so this inspection also avoids the odd-fret holdout partition
declared by the existing dispersion-fit tool.

Neither guitar model, scale, string gauge, pickups, pick angle, velocity nor
contact marker is provided. Two clips start with substantial signal already
present. Consequently their precursor and contact-to-release duration cannot
be measured. The short ends also cannot identify a played note-off separately
from editing. A modern dataset search reconfirmed several adjacent sources but
did not establish a new exact-eight contact corpus.

## New measurements

The [analyzer](../Experiments/RealismReference20260908/analyze.py) records its
own SHA-256 and the input WAV hash in each JSON result. It uses NumPy 1.26.4
and SciPy 1.14.1. Audio onset is the first crossing of 25% of the first
second's maximum centred 2 ms RMS envelope, with zero padding at boundaries.
The onset is flagged as censored when it falls within 2 ms of the start.
Spectral descriptors use Hann windows and 8x-or-greater FFT zero padding.
An independent twelve-harmonic exponentially decaying synthetic signal checked
the analyzer: maximum recovered slope error was 0.00090 dB/s. Reducing its
amplitude to one quarter changed normalized RMS descriptors by less than
`1e-12 dB` and preserved the detected onset.

| Real note | Censored onset | 50–150 / 0–50 ms RMS | 150–500 / 0–50 ms RMS | 500–1000 / 0–50 ms RMS | Upper/low partial ratio change |
| --- | --- | ---: | ---: | ---: | ---: |
| F#1 | yes | −2.02 dB | −4.98 dB | −10.16 dB | −15.41 dB |
| C2 | yes | −1.63 dB | −3.31 dB | −7.60 dB | −12.85 dB |
| F#2 | no | −2.50 dB | −5.50 dB | −10.88 dB | −17.66 dB |

The last column is the change in root-summed H5–H12 / H1–H3 amplitudes
between 50–210 and 850–1010 ms. The tracker follows the local peak inside
non-overlapping ±0.4-fundamental brackets for each harmonic, with log-magnitude
quadratic interpolation. This avoids confusing moderate pitch drift with
harmonic-energy loss. Full partial traces, linear slope residuals and bracket
edge alarms are retained in
[F#1](../Experiments/RealismReference20260908/reference-30.json),
[C2](../Experiments/RealismReference20260908/reference-36.json) and
[F#2](../Experiments/RealismReference20260908/reference-42.json).
They are exploratory spectral descriptors, not uncertainty-qualified physical
loss estimates; weak partials, beating and preview compression remain confounds.
F#2's eleventh partial approaches its search-bracket edge; do not interpret
that line's slope as an identified physical decay constant.

All three notes lose upper-partial share as they decay. A new contact model
should preserve that direction and avoid making the low-string attack
unnecessarily brighter. These observations support a conservative ceiling on
additional contact/noise energy. They do not justify another global decay fit:
the existing engine already contains a default-off low-string loss experiment
derived from this same source family. Open-string F#1 in this recording also
maps to fret 2 on Electry's Drop-E string, so its termination differs.

The unchanged baseline was rendered at 44.1 kHz with velocity 0.95 on the
lowest physical string, 250 ms lead-in, 1.5 s hold and 750 ms release. Model
WAVs and the exact parameter/event manifest live under the ignored
`build-realism-20260908/baseline/`. The model baseline's 500–1000 / 0–50 ms
RMS changes are −2.82, −2.50 and −2.13 dB for F#1/C2/F#2; it sustains longer
than these three examples. Its 0–30 ms centroids are 534/301/280 Hz against
the previews' 186/181/272 Hz. The first two reference onsets are censored, so
their attack comparison is only a direction alarm. The model and recordings
have no matched pickup, force, age or termination; these numbers do not define
an absolute error objective. Candidate changes should avoid further brightening
the low notes, while preserving already-plausible F#2 onset color.

The same-score candidate renders satisfy that conservative direction check:

| Note | Real 0–30 ms centroid | Baseline | Candidate | Baseline → candidate 30–80 ms centroid |
| --- | ---: | ---: | ---: | ---: |
| F#1 | 186.34 Hz, censored | 534.33 Hz | 491.14 Hz | 263.73 → 256.66 Hz |
| C2 | 181.40 Hz, censored | 300.71 Hz | 291.27 Hz | 245.75 → 244.44 Hz |
| F#2 | 272.22 Hz | 280.16 Hz | 276.86 Hz | 260.20 → 259.65 Hz |

The 30–80 ms reference centroids are 178.00, 185.71 and 249.18 Hz, so that
window also moves in the reference's direction. This supports keeping the
restrained contact adjustment. It is not evidence to extend the adjustment
until these uncontrolled recordings match numerically.

First-second sustain remains essentially unchanged. Candidate normalized
500–1000 ms RMS shifts by +0.046/+0.011/+0.004 dB from baseline (slightly
farther from the three recordings), while upper/low partial-ratio cooling
changes by less than 0.002 dB. The five contact changes therefore do **not**
solve this source's faster decay. Their release/hammer mechanisms are exercised
by the separate engineering regressions and audition phrases, not by these
held sustain windows. Exact per-note candidate measurements and input hashes
are in [F#1](../Experiments/RealismReference20260908/candidate-30.json),
[C2](../Experiments/RealismReference20260908/candidate-36.json) and
[F#2](../Experiments/RealismReference20260908/candidate-42.json).

## Physical basis for the five changes

1. **String-dependent pick release timescale.**
   [Evangelista and Smith, DAFx 2010, §5](https://www.dafx.de/paper-archive/2010/DAFx10/EvangelistaSmith_DAFx10_P21.pdf)
   derive wave impedance `r = sqrt(T μ)` and a damped spring-contact pole at
   `−K/(R + 2r)`. Thus string impedance affects the response timescale at fixed
   pick stiffness and damping. Using a bounded ratio in Electry is a modeling
   inference; the paper releases the pick by relative contact/force conditions
   and does not identify Electry's total Contact duration. The implementation
   retains the loading time, scales the slip portion and preserves the sampled
   force-pulse area.
2. **Winding-aware contact detail.**
   [Pakarinen, Puputti and Välimäki, 2008](https://aaltodoc.aalto.fi/bitstreams/b890243c-fd3e-435c-a755-b98bd860507a/download)
   relate periodic handling noise to winding ridges and sliding speed. They
   also measured much quieter, less harmonic noise from plain strings. The
   ridge-crossing rate `v / winding_pitch` motivates a gauge-dependent noise
   color in Electry. This is a geometric inference: the paper's sliding tube
   is a different contact and its thicker strings also show *more* overall
   high-frequency content. That observation does not validate a lower overall
   scrape cutoff. No slide coefficient is imported as a measured pick
   coefficient, and the new cutoff remains conservative voicing checked
   against the preview attack spectra above.
3. **Remaining-vibration-aware stopping noise.** The energy removed when a hand
   stops a ringing string depends on the vibration present. Tying the existing
   damping-noise surrogate to that state is a physical consistency improvement.
   These recordings contain no controlled note-off measurements; the mapping
   is not a fitted release law. Independent finger friction can exist even on
   a quiet string and is a different mechanism from vibration-driven stopping.
4. **Hand-aware note stops.** Relaxing a stopped fret and closing a broad hand
   on an open string are distinct gestures. The implementation reuses its
   existing 10 ms finger-landing and 22 ms broad-hand closure scales for those
   endpoints, with the existing hand depth selecting between them. The final
   release decay target stays fixed; contact noise is quieter and darker for
   the broad-hand stop. Those numbers and endpoint weights are explicitly
   voiced surrogates, not newly measured release constants.
5. **Hammer impact at the landed fret.**
   [Bilbao et al., DAFx 2024, §2.1–2.5](https://www.pure.ed.ac.uk/ws/portalfiles/portal/470239305/BilbaoEtal2024RealTimeGuitarSynthesis.pdf)
   place pluck excitation, fret collision and finger force at their distinct
   physical coordinates. The finger acts locally at its contact location.
   Frets are at `L(1 − 2^(−q/12))` measured from the nut. This supports moving
   Electry's hammer excitation to the destination fret on the source speaking
   length. It does not calibrate impact strength, finger mass or contact width.

These are mechanism-level checks. No listening panel was run and no claim of
real-versus-synthetic perceptual parity follows from the descriptors.

To reproduce a reference analysis after downloading its public preview and
decoding it with `ffmpeg -i input.mp3 -c:a pcm_f32le input.wav`:

```sh
python3 Experiments/RealismReference20260908/analyze.py \
  /tmp/electry-realism-reference/525010.wav 30 /tmp/reference-30.json
```
