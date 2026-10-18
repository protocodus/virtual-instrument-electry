# Known-Neck reference: lower body and partial cooling — 2026-10-19

The lower-body deficit persists with a documented Neck pickup setting. Across
qualified held-note windows, released 1.2.6 has too little fundamental relative
to H2 and H3. Later envelope measurements also show too little relative cooling
of those harmonics. Changing the existing pick-position control helps some
notes but does not resolve the mismatch. No DSP or default is promoted here.

The [complete comparison record](p2-known-neck-body-2026-10-19.json) retains the
prospective protocol, all 138 source rows and guards, all 552 model comparisons,
events, controls, gains, distributions and source identities. The separate
[cooling record](p2-partial-cooling-2026-10-19.json) retains all 184 note/context
rows, including unqualified values, its post-hoc derivation plan, interpretation
clarification and independent review. Source remains
`6c83ff453ff7ac346598e8e73367ced128a90838`, version 1.2.6.

## Documented reference and fixed comparison

The [Guitar-TECHS paper, III-B/Table I](https://arxiv.org/pdf/2501.03720) specifies
P2's EVH Wolfgang Exotic, .009–.042 nickel strings, Neck pickup and maximum
tone/volume. The actual downloaded DI is PCM24/48k with duplicate stereo
channels; use channel0 without resampling. Exact installed pickups, loading,
force, picking point and string age remain unknown.

Keep all 46 original MIDI-labelled E/A chromatic notes, frets0–22 on each
string. The crop is0–183.85s. MIDI960TPQ at1,000,000µs/quarter gives exact
48k frame=`tick*50`. All46 Note Ons and46 Note Offs retain their original
times. Fixed modelvelocity.85 is an unknown-force controller hypothesis.
Performer solo-string keys26/27 and the public engine mask route every note
to physical string6/index2 or string5/index3, with46 verified note/fret/string
witnesses per context and no resets between notes.

The prospective protocol predates new windows and model builds. Three128ms
windows per note, centered on MIDI+.5/.8/1.1s, retain source-only hold,
activity, pitch, background, nuisance and taper checks. Every partial and band
has its own admission. Weak H3 does not exclude usable H1. All138 rows remain;
the first open E2 has insufficient pre-onset background and stays unscored.
The background high-endpoint consistency correction was completed before
model access, with its initial attempt preserved.

Four unchanged-engine contexts share Neck/HB0/Tone1/Scale0/Gauge0, FX off,
mono48k, block64, seed0x160c2026 and all other serialized controls. Sympathy
is0 because the reference does not identify the model's extra strings.

| Context | Age | Pick Position |
|---|---:|---:|
| A, primary | .3 | .18 |
| B | 0 | .18 |
| C | .3 | .5 |
| D | 0 | .5 |

One constant gain per whole183.85s crop matches−24dBFS RMS. The shared peak
cap is1, so no additional trim is needed. There is no EQ, per-note/window gain,
offset fit, timing warp or hardware optimization. Absolute errors below are
conditional matched DI levels, not calibrated pickup voltage.

## Lower-body result

Each cell is the median **paired model minus reference** error. H1 share is
fundamental power divided by H1+H2+H3 power; it is not total-spectrum share.
Each metric uses its own identical source-admitted IDs across all contexts.

| Metric | Windows | A | B | C | D |
|---|---:|---:|---:|---:|---:|
| H1 share |106|−.3187|−.3720|−.0812|−.1091|
| H2/H1, dB |128|+12.786|+13.522|+4.272|+5.012|
| H3/H1, dB |108|+19.607|+21.883|+10.356|+11.998|
| Matched H1, dB |135|−3.045|−3.143|−1.131|−1.229|
| Matched H2, dB |128|+8.635|+9.062|+4.048|+3.909|
| Matched H3, dB |108|+16.469|+18.747|+10.604|+12.411|
| Matched80–250Hz, dB |105|−2.941|−3.117|+.187|+.085|
| Matched250–1000Hz, dB |112|+5.849|+6.778|+2.865|+3.462|

Reference H1-share median is.96124; A's is.55141. A has lower H1 share in
106/106 qualified windows, higher H2/H1 in128/128 and higher H3/H1 in108/108.
Paired medians differ from subtracting two independent distribution medians.

C's near-zero aggregate80–250Hz error hides E/A errors−1.124/+1.552dB.
Its matched H1 errors are−1.784/+.580dB. Fret-dependent notches and overshoots
remain. No single setting is inferred as the reference player's hand position,
selected as a default, or reported as a new DSP improvement. Fixed Hz bands
are operational descriptors and do not contain every note's fundamental.

## Relative cooling is also missing

After observing the late-body aggregate, a separate derivation plan was frozen
before computing new finite rates. Use the existing+.5/.8/1.1s values only;
require all three source admissions for that metric and no frozen activity or
attenuation across their entire enclosing interval plus25ms. Retain all 46
notes and every failure for all four contexts. No new audio or observer is used.

Calculate endpoint difference divided by.6s, and retain midpoint departure
without excluding curved envelopes. Median rates in dB/s:

| Metric | Notes | Reference | A | B | C | D |
|---|---:|---:|---:|---:|---:|---:|
| H1 level |45|−4.552|−6.687|−5.429|−6.684|−5.424|
| H2 level |39|−13.193|−7.254|−5.479|−8.053|−6.131|
| H3 level |29|−15.362|−7.462|−4.791|−9.044|−7.451|
| H2/H1 |39|−7.829|−1.066|−.455|−1.930|−1.125|
| H3/H1 |29|−12.109|−2.426|−.886|−3.706|−2.079|

Different rows use different note sets; ratio medians are not obtained by
subtracting unrelated level medians. Results support a time-dependent colour
mismatch as well as a static offset. They do not identify intrinsic material
damping: source ratio midpoint departures reach−10.54 to+5.25dB for H2/H1
and−12.97 to+2.26dB for H3/H1, with one admitted positive H3/H1 rate.

A fixed unknown onset offset leaves the.6s separation unchanged, but can
change the measured rate by sampling different acoustic envelope ages.
Polarization, interference, nonlinear pickup response and undetected gestures
remain possible contributions. These are finite envelope rates, not modal T60.
The [material-loss hypothesis study](wound-loss-hypothesis-2026-10-19.md)
investigates that distinction without converting these measurements into a fit.

## Verification, attribution and scope

Independent review reconstructs all 138 source observations/admissions,
552 model windows and173,632 aggregate numeric checks, all 21 original report
table rows, all 44,124,000 matched PCM samples, and all 184 cooling rows.
Final local HTTP/HEAD byte checks cover all 11 room resources; all 46 seek
callbacks execute against media mocks. No live-browser or perceptual audition
is claimed. The [local comparison room](http://127.0.0.1:8741/room/) retains
Reference and A–D, with a separate context key and attribution.

Completed 120-artifact comparison receipt:
`70dbeb2ffddb735b8adf285f2e3cf5f320cdfba0ab35103eea888ea537580d68`.
The separate 15-artifact cooling manifest is
`92089e351680602a6976c88aabfb64a9c5c4a8ffd00d8e6b5eb22e4a520ee1d9`.

The recording is from [Guitar-TECHS record14963133](https://zenodo.org/records/14963133),
by Hegel Emmanuel Pedroza Villalobos, Termeh Taheri, Wallace Abreu, Ryan Corey
and Iran R. Roman, under [CC BY4.0](https://creativecommons.org/licenses/by/4.0/).
The record lists [DOI10.1109/ICASSP49660.2025.10887996](https://doi.org/10.1109/ICASSP49660.2025.10887996).
The review copy uses channel0, crop0–183.85s, constant gain5.680330753942426
and mono PCM24/48k encoding. No endorsement is implied. These dataset creator
and DOI fields supersede earlier four-paper-author or assumed Zenodo-DOI
attributions; frozen earlier records remain intact.

Source/defaults/tests/thresholds/version and every existing preview remain
unchanged. This study does not qualify attacks, note changes, repicks, upper
sustain, a new engine, listener preference or an authenticity percentage.
