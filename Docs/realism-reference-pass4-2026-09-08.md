# Pass-four sustain experiment: independent reference measurements

The 0.65 material-loss fraction moves all six existing low-string decay
descriptors toward the three real previews and retains a more balanced F#2
upper-partial trajectory than the full 1.00 candidate. It changes the body-normalized
Modern sound as well as dry decay. It does not establish that the remaining
sustain sounds like a real guitar, and it does not supply time-varying pitch.

The isolated baseline is the pass-three source copied under
`build-realism-20260908-pass4/baseline-src/`; the two candidates vary only
`bassMaterialLossFraction` from 0.25 to 0.65 or 1.00 in the root agent's isolated
copies. All use the same production renderer, manifests, guitar settings and
Modern chain. A separate integrated comparison below uses the full fourth-pass
candidate. Engineering promotion checks are reported in the main fourth-pass
report; this note records the audio-domain evidence.

## Existing real recordings and method

The source is cabled_mess's CC0 clean/dry eight-string F#-string pack, captured
with an RME Babyface/Cubase workflow:
[F#1 525010](https://freesound.org/people/cabled_mess/sounds/525010/),
[C2 525008](https://freesound.org/people/cabled_mess/sounds/525008/), and
[F#2 525009](https://freesound.org/people/cabled_mess/sounds/525009/).
The same downloaded public MP3 previews and WAV hashes from
[the first-pass reference report](realism-reference-2026-09-08.md) are reused;
no new corpus is accessed.
They are development examples, not untouched holdouts. Pickup, gauge, tension,
force and finger termination are unmatched; F#1/C2 onsets are censored. F#2's
H11 tracker approaches its bracket boundary. No eight-string contact constant
or general perceptual score can be identified from these three takes.

[The original analyzer](../Experiments/RealismReference20260908/analyze.py)
gives the unchanged descriptors:
first crossing of 25% of maximum centered 2 ms RMS; local H1–H12 peaks tracked
in nonoverlapping +/-0.4 f0 windows; nine 160 ms Hann windows beginning at
50–850 ms. The [fourth-pass analyzer](../Experiments/RealismReferencePass4_20260908/analyze_sustain.py)
adds whole-window envelope and Modern band measurements. Its
[summary receipt](../Experiments/RealismReferencePass4_20260908/sustain-summary.json)
hashes both analyzers, isolated source files, manifests, previews and wet/dry
WAVs. It retains twelve-partial decay estimates, fit residuals and bracket
alarms, plus the full median pitch and upper/low ratio traces. All audio stays
outside the repository.

## Dry sustain

Late RMS is 500–1000 ms relative to the initial 0–50 ms. Upper/low cooling
is the H5–H12/H1–H3 amplitude-ratio change between the first and last tracked
windows. More negative values mean faster loss.

| Note | Descriptor | Real | Baseline 0.25 | 0.65 | 1.00 |
| --- | --- | ---: | ---: | ---: | ---: |
| F#1 | Late RMS, dB | -10.16 | -4.42 | -6.47 | -8.00 |
| C2 | Late RMS, dB | -7.60 | -4.76 | -6.69 | -8.11 |
| F#2 | Late RMS, dB | -10.88 | -5.27 | -6.99 | -8.24 |
| F#1 | Upper/low cooling, dB | -15.41 | -2.75 | -5.63 | -8.22 |
| C2 | Upper/low cooling, dB | -12.85 | -4.04 | -7.84 | -11.44 |
| F#2 | Upper/low cooling, dB | -17.66 | -8.27 | -15.64 | -23.01 |

The full contribution continues toward the F#1/C2 cooling targets but passes
F#2 by 5.35 dB. The 0.65 setting misses that F#2 target by 2.02 dB. This
supports 0.65 as the less aggressive cross-note choice among these experiments;
it is not a new fitted material coefficient. The recordings could be consistent
with a different loss curve, modal balance, string age or guitar altogether.

Fundamental decay is intentionally retained. F#1 H1 remains approximately
-2.36 dB/s against the recorded -7.07 dB/s. C2 H1 already agrees reasonably
(-3.36 versus -3.23 dB/s). Therefore shortening all fundamentals uniformly to
match F#1 would worsen C2. At F#2, H2/H3 still decay more slowly with 1.00
(-10.3/-17.1 versus -29.5/-27.5 dB/s), despite excessive aggregate upper/low
cooling. Matching one aggregate ratio can conceal wrong individual partials.

## Modern sustain, normalized to body

Each file receives a conceptual fixed gain so its own 150–350 ms body RMS is
0 dB; no dynamic or per-window gain is applied. Windows remain aligned to the
dry audio onset through the identical amp chain. Hann Welch spectra use up to
8192 samples, 50% overlap and a 65536-point FFT. Band powers are integrated
from the PSD. These are model-to-model comparisons: no paired real DI/reamp
source exists here.

| Note | Late 500–1000 ms centroid, baseline -> 0.65 | 500–2000 Hz change | 2–6 kHz change | Late RMS/body, baseline -> 0.65 |
| --- | ---: | ---: | ---: | ---: |
| F#1 | 173.5 -> 158.4 Hz | -5.20 dB | -11.71 dB | -0.24 -> -1.37 dB |
| C2 | 195.4 -> 174.2 Hz | -5.34 dB | -12.52 dB | +0.22 -> -0.60 dB |
| F#2 | 219.6 -> 190.7 Hz | -5.12 dB | -6.26 dB | -0.47 -> -0.69 dB |

The full 1.00 contribution reduces the same 500–2000 Hz band by roughly
9.7–9.9 dB. The less aggressive 0.65 therefore removes appreciable sustained
upper-band energy even after matching body level; it is not just a quieter
file. The isolated bandwidth figures are descriptive and cannot certify an
audible realism preference.

## Requested attack-to-body guard context

Using sample peak over 0–200 ms divided by RMS over 200–700 ms, after detected
audio onset:

| Note | Real | Baseline | 0.65 | 1.00 |
| --- | ---: | ---: | ---: | ---: |
| F#1, censored | 17.54 dB | 14.59 dB | 15.93 dB | 16.99 dB |
| C2, censored | 13.94 dB | 13.09 dB | 14.35 dB | 15.33 dB |
| F#2 | 15.60 dB | 12.08 dB | 13.15 dB | 13.98 dB |

A universal 15 dB ceiling is below two real examples, but these recordings
include physical pick/noise components and are not matched to a noise-disabled
engine fixture. This is context for reviewing a legacy guard, not evidence for
an arbitrary new limit. No E1 or B1 recording was added to justify an exact
E1/B1 threshold.

## Remaining sustained-tone cue

The median H1–H5 frequency trace, each partial referred to its own final
850–1010 ms window, starts at +20.47/+3.67/+6.48 cents in real F#1/C2/F#2.
The baseline starts at approximately 0/0/+0.13 cents; 1.00 remains about
0/+0.07/+0.32 cents. Stronger loss changes partial amplitudes while leaving
the modeled sustain almost frequency-stationary. Those real traces show
mostly descending pitch, not simply random jitter. They do not identify a
universal pitch law or justify adding chorus or broadband noise to blur lines.

[Lindroos, Penttinen and Valimaki's 2011 primary electric-guitar study](https://www.researchgate.net/publication/220386570_Parametric_Electric_Guitar_Synthesis)
(author-deposited full text; [university publication record](https://research.aalto.fi/en/publications/parametric-electric-guitar-synthesis/))
measures partial decay changes, force-related pitch glide and two-polarisation
beating separately. Its measured low-E beat interval is approximately five
seconds, longer than these preview sustains can reliably characterize. The
paper's mechanism distinction supports retaining separate questions about
spectral decay, coherent pitch evolution and polarisation balance. It does not
show that all real sustained partials need broad spectral lines. Electry
already models dispersion and two polarisations; their mere presence does not
prove the observed temporal trajectories agree.

The user prioritizes the sustained guitar character. These measurements support
a meaningful damping improvement, while explaining why darkening alone can
leave a static, synth-like residual impression. Existing rejected attack-pitch
experiments and the lack of controlled force/tension captures remain relevant;
no broader pitch model was implemented or recommended for immediate promotion
in this subtask.

## Full fourth-pass candidate

The complete candidate in `build-realism-20260908-pass4/candidate/` uses the
selected 0.65 contribution alongside the other fourth-pass changes. These
held probes compare it with the same baseline, independently of the isolated
loss trial above.

| Note | Baseline late RMS | Integrated late RMS | Baseline cooling | Integrated cooling | Integrated peak/body |
| --- | ---: | ---: | ---: | ---: | ---: |
| F#1 | -4.42 dB | -7.49 dB | -2.75 dB | -5.56 dB | 20.33 dB |
| C2 | -4.76 dB | -6.92 dB | -4.04 dB | -7.71 dB | 16.03 dB |
| F#2 | -5.27 dB | -7.03 dB | -8.27 dB | -15.54 dB | 13.41 dB |

The stronger integrated F#1 attack raises peak/body from the isolated trial's
15.93 to 20.33 dB, above this censored reference's 17.54 dB. Its late RMS
relative to the initial 50 ms appears another 1.02 dB closer to the reference,
but its upper/low cooling hardly changes. That extra apparent improvement
comes substantially from the stronger attack denominator; it must not be
reported as another improvement in sustained decay. C2's integrated peak/body
is also above its censored reference. These are consequences of the requested
clearer pick, not measured matches to an identified picking force.

After body normalization, integrated Modern 500–1000 ms centroids are
156.9/172.9/189.9 Hz for F#1/C2/F#2, versus baseline 173.5/195.4/219.6 Hz.
The 500–2000 Hz band drops 6.53/6.13/5.40 dB, and late total RMS/body becomes
-1.42/-0.62/-0.68 dB. The pitch traces remain almost stationary. No paired
real amplified target or perceptual test is present, so this establishes
changed spectral evolution and a remaining limitation.

Reproduce the packaged receipt from the repository root after rendering the
three isolated conditions and the full candidate:

```sh
python3 Experiments/RealismReferencePass4_20260908/analyze_sustain.py
```

The script accepts `--build-root`, `--reference-root` and `--output` for another
artifact location. It reuses the original analyzer rather than substituting
new spectral definitions between passes. The integrated source/build receipt
is maintained with the main fourth-pass validation; this reference receipt
attests the measured WAV and manifest bytes.
