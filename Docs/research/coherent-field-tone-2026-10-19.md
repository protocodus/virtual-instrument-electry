# Coherent string field: lower body and upper sustain

The new inherited-loss field reduces several lower-body errors and all eight
qualified 1.5–4 kHz decay errors against dry guitar recordings. It still has
body regressions at other fixed controls and fails every eligible 4–8 kHz
comparison. No DSP, default, version or playable performance changes follow;
production remains 1.2.6. The [complete scalar record](coherent-field-tone-2026-10-19.json)
retains every reported metric, both Neck profiles, all eight Bridge contexts,
all sixteen upper-prefix outcomes, matching gains and frozen input hashes.

## Three fixed formulations

A is unchanged shipping source `6c83ff453ff7ac346598e8e73367ced128a90838`.
The earlier material field, called oldB here, uses a complete stiff static
preload, zero initial velocity, finite C1 force unloading and physical sine
coil-aperture observations. Its open-string effective material loss gives
stronger low-partial cooling but severely overdamps 1.5–4 kHz. C keeps that
field, source, undamped frequencies, force and observation unchanged, replacing
only its loss with a fixed inherited per-axis production-bank law:

`alpha(omega) = -Fs * log(abs(H(exp(i*omega/Fs)))) / groupDelay(omega)`.

H includes real-arithmetic cubic interpolation, all eight allpass sections,
scalar gain and active loss/material filters from the frozen 115-note noteOn
grid. The law is evaluated at each exact oldB physical frequency. There is no
additional air/material term, axis multiplier, fitted blend, EQ or retuning.
The stationary uncoupled transfer is a surrogate: its historical bank controls
differ from the audition controls; actual floating-point rounding, coupling,
evolving contact and hand losses remain outside it. Production loop phase and
physical-field mode numbers are also different coordinates. C versus A changes
source support, spatial interpretation and loss together.

The same nonlinear flux, Faraday, electrical pickup and output-decimator path
processes the fields. Static loading is assumed initial storage rather than a
newly measured pick actuator. Its finite unloading does not qualify attacks,
same-note repicks, same-string note changes or fret-contact behavior.

## Known-Neck late body: all 46 E/A notes

Both fixed fresh Pick .18/.5 profiles retain all 138 original 128 ms source
windows centered .5/.8/1.1 s after labelled MIDI-on and their original
source-only metric guards. Each of seven complete 92 s tracks has one constant
RMS gain to −24 dBFS; their common headroom cap is 1. No per-note level, acoustic
alignment, denoising, EQ, fade or source admission is adjusted.

Mean absolute errors against the source; H1 share is a power fraction and
other rows are dB:

| Pick | Metric | N | A | oldB | C |
| --- | --- | ---: | ---: | ---: | ---: |
| .18 | H1/(H1+H2+H3) | 106 | .393941 | .088099 | .218335 |
| .18 | H2/H1 | 128 | 14.573022 | 6.638169 | 10.325847 |
| .18 | H3/H1 | 108 | 22.434268 | 10.418345 | 16.525618 |
| .18 | H1 level | 135 | 4.025201 | 1.473831 | 1.801249 |
| .18 | 80–250 Hz | 105 | 5.283791 | 3.208905 | 3.409345 |
| .18 | 250–1000 Hz | 112 | 8.398922 | 4.621964 | 8.097384 |
| .5 | H1/(H1+H2+H3) | 106 | .172669 | .082834 | .131978 |
| .5 | H2/H1 | 128 | 8.636964 | 7.426778 | 8.749084 |
| .5 | H3/H1 | 108 | 14.630252 | 11.278844 | 14.963545 |
| .5 | H1 level | 135 | 2.373512 | 3.579078 | 3.721115 |
| .5 | 80–250 Hz | 105 | 3.614322 | 4.134198 | 4.291520 |
| .5 | 250–1000 Hz | 112 | 5.782309 | 4.083770 | 6.644772 |

At Pick .18, C's median signed H1 error moves −3.291→−1.247 dB
and 80–250 Hz moves −3.618→−.327 dB. H2/H1 and H3/H1 still exceed
the recordings by median +8.227/+17.263 dB. The 250–1000 Hz signed
median worsens +6.107→+7.307 dB even though its MAE improves slightly.

At Pick .5, H1 share improves while H2/H1, H3/H1, H1-level and body MAEs
worsen. Smaller signed medians do not erase these regressions. The A-string
subset is especially weak: H1-level MAE 1.596→4.347 dB and 80–250 Hz
5.092→7.050 dB. Neither a favorable note nor a pick setting is selected as
a new default. The older material field is generally closer on these body
measures, but its independent upper-sustain failures prevent promotion.

## Bridge upper sustain: both players and all four profiles

The fixed open-E2 profiles are HB/SC endpoints × Pick .18/.5, Tone1,
fresh/light/Scale0; body, noise, hand, sympathy and FX are off. Rate48k,
block64, seed0x160c2026, velocity.85, NoteOn40 at frame2400, no NoteOff.
Each player's 13 complete two-second tracks has one RMS gain per track and
one shared headroom cap, again 1. The original source+A prefixes, guards and
windows remain unchanged. Slopes use complete-prefix broadband OLS dB/s;
they are distinct from absolute upper-band level and modal T60.

All eight C 1.5–4 kHz prefixes qualify and improve absolute slope error against
A. The reference slopes are −14.005/−14.456 dB/s for players1/2:

| Fixed profile | A | oldB | C |
| --- | ---: | ---: | ---: |
| HB .18 | −16.806 | −53.618 | −15.961 |
| HB .5 | −16.344 | −49.865 | −12.741 |
| SC .18 | −21.683 | −54.077 | −16.502 |
| SC .5 | −19.911 | −49.811 | −12.595 |

Only four oldB 1.5–4 kHz prefixes qualify; its other four full-prefix guards
fail. All four eligible player1 C 4–8 kHz prefixes also fail their guards.
Their full slopes remain descriptive in the scalar record. Player2 never had
an admitted 4–8 kHz source prefix, so none is created. No prefix is shortened
to obtain a favorable result.

Body outcomes remain mixed across the eight player/profile contexts.
SC .18 H1-share, H2/H1 and H3/H1 errors all worsen against A in both
players. Its matched H1 deficiency also grows. Player2 HB .5 H3/H1
signed error moves +.092→−1.801 dB. C retains +3.23 to +9.78 dB of
250–1000 Hz median excess. Broad 80–250 Hz includes E2's first three
partials: closeness in that band can hide an inadequate fundamental and
excess H2/H3.

## Integrity and remaining work

Before any C PCM, independent blind review checked all 96 cases and 34,120
mode-axis coordinates, force/work/dissipation ledgers, state evolution and
field observation. All 96 render metadata then reproduced the old geometry,
controls and timeline except the expected output RMS/peak. Independent result
checks subsequently reproduced all 828 Neck windows, old baseline observers,
matching/PCM copies, all 64 Bridge windows and all 16 prefixes. HTTP and Node
transport mocks verify delivery, with no live browser or listening claim.

Two original Neck narrative claims confused signed medians with MAE or omitted
a comparator. Original text is preserved; the corrected v2 report/key and
correction receipt change no audio, metric, mask or gain. A separate one-ulp
old-observer representation difference is explicitly recorded; source facts
and classifications remain exact and all seven matched WAVs are byte-identical
between attempts.

The complete C manifest is
`d28c5fd2e85a4a6ca70399ff378929a89d98bdf7707afc6c083f56737f9ceef1`;
its frozen receipt is
`9495503ccf3ae9707ddce07fa174bd17076a56ff818f31759de87dbcf9ed586a`.
It binds 780 artifacts, 964 inputs, all current/published Source files,
the blind receipt and both final independent tone reviews. Source, Tests and
CMake remain identical to shipping 6c83. Local full studies and audio remain
outside this portable scalar documentation; the JSON binds their exact paths
and hashes.

The next loss formulation needs causal storage/dispersion and independently
owned physical parameters, while preserving low-partial cooling and upper
sustain together. A guessed knee or blend chosen from these recordings would
not establish the mechanism. A playable coherent field also needs a continuous
contact actuator, retained state through repicks/refrets, complete realtime and
alias qualification. These studies do not establish 95% realism or actual
eight-string hardware fidelity.

## Reference attribution

[Guitar-TECHS](https://zenodo.org/records/14963133), by Hegel Emmanuel Pedroza
Villalobos, Termeh Taheri, Wallace Abreu, Ryan Corey and Iran R. Roman,
[DOI10.1109/ICASSP49660.2025.10887996](https://doi.org/10.1109/ICASSP49660.2025.10887996),
and [LongiGuitarDataset](https://zenodo.org/records/19823590), credited by its
record to Wright, Alec; McKenzie, Thomas; Matthew, Hamilton (names as listed),
are [CC BY4.0](https://creativecommons.org/licenses/by/4.0/).
The review copies use fixed channel/crop selection, one whole-track gain and
mono PCM24 conversion; no original audio is included in this scalar record.
Guitar-TECHS supplies the labelled Neck comparison. Longi hardware pickup,
force, pick point and tone settings remain unknown. These conventional-guitar
references do not certify a premium eight-string instrument. No endorsement
is implied.
