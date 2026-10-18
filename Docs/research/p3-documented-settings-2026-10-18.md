# Documented Sire settings and lower body — 2026-10-18

Documented Both selection and maximum tone explain part of the earlier Sire body deficit. Every held-window H1 share and H2/H1/H3/H1 absolute ratio error improves relative to the old heavy Bridge context, but large residual errors remain. Both original upper-band slopes worsen in every changed context. This is a comparator-context study, with no DSP mechanism/default change or percent-authenticity score.

A is the original heavy Bridge/type.32/tone.7 context. B changes only documented Both/max-tone; C adds the manufacturer-supported stock single-coil hypothesis; D adds25.5-inch conventional scale and gauge.5 (.046667-inch lowE versus recorded .046), an approximate .010-like hardware hypothesis. Pick.18 and unknown age.30 stay fixed. Type1 is not a measured Sire transfer, and exact installed pickup/geometry/loading, pickpoint and force remain unknown.

| Context | H1-share mean absolute error | H2/H1 MAE, dB | H3/H1 MAE, dB |
|---|---:|---:|---:|
| A | 0.611749 | 15.085020 | 21.242171 |
| B | 0.420666 | 11.515654 | 15.755206 |
| C | 0.396128 | 11.060218 | 14.343220 |
| D | 0.393826 | 10.639861 | 14.334746 |

All ten H1 shares (power fraction, not physical body resonance or voltage calibration):

| Source window | Real | A | B | C | D |
|---|---:|---:|---:|---:|---:|
| 06-e-transition / 47 fresh 80ms | 0.77023 | 0.09735 | 0.29494 | 0.33090 | 0.33840 |
| 06-e-transition / 49 repick 80ms | 0.94628 | 0.11070 | 0.34108 | 0.37705 | 0.36483 |
| 06-e-repeat / 49 repick 80ms | 0.97054 | 0.11184 | 0.34267 | 0.37857 | 0.36601 |
| 12-d-descent / 55 fresh 80ms | 0.79883 | 0.11083 | 0.26700 | 0.30041 | 0.33466 |
| 12-d-descent / 55 fresh 200ms | 0.77300 | 0.12138 | 0.27888 | 0.31200 | 0.34728 |
| 12-d-descent / 53 changed-fret 80ms | 0.46773 | 0.10552 | 0.20763 | 0.23198 | 0.26837 |
| 12-d-descent / 52 changed-fret 80ms | 0.85466 | 0.10058 | 0.17769 | 0.19806 | 0.22858 |
| 12-high-e-descent / 79 fresh 80ms | 0.97253 | 0.38081 | 0.67336 | 0.73585 | 0.64991 |
| 12-high-e-descent / 79 fresh 200ms | 0.99130 | 0.59659 | 0.80707 | 0.85584 | 0.79917 |
| 12-high-e-descent / 77 changed-fret 80ms | 0.63311 | 0.32509 | 0.68499 | 0.76995 | 0.72348 |

F5 changed-fret overshoots the source H1 share in B/C/D despite being closer than A. SC/conventional contexts are not automatically better in every individual metric or absolute partial level; [portable numeric record](p3-documented-settings-2026-10-18.json) preserves all rows.

After the single whole-crop RMS trim, median H1 level errors are A−9.197, B−4.989, C−4.432, D−4.187 dB. Mean absolute H1 level errors are8.925/5.706/5.313/5.243dB; mean signed H3 errors stay+14.341/+12.714/+11.809/+11.808dB. All40 per-window signed partial levels, raw RMS/gains and errors are retained. These mixed signed distributions caution against interpreting H1 share as a uniform bass-amplitude correction.

| Original fixed upper prefix | Real slope dB/s | A | B | C | D |
|---|---:|---:|---:|---:|---:|
| G3 /1.5–4kHz | −17.968 | −54.872 | −57.705 | −59.368 | −58.516 |
| G5 /4–8kHz | −68.815 | −83.005 | −86.522 | −95.471 | −88.848 |

All8 admitted-prefix/context rows pass the unchanged old observability guards. Every band/frame from the original retained prefixes is preserved, including the two originally nonadmitted prefixes/context; no prefix is extended, shortened, newly admitted or remasked. The table reports band mixtures rather than identified material loss/T60. Absolute upper levels and guards are shown in the upper-level/decay figure in the listening room and full JSON.

All four new A WAV SHA256 values exactly equal the old frozen selected46f4/6c83 files before any B/C/D render. Source24 snapshot is byte-identical to immutable6c83; compilation uses productionONdefines and C++17 DSP. All sixteen event/frame records match. Every changed control is fixed across each phrase, with all other captured controls identical. Original source/crop/channel0, ten body windows, two admitted upper prefixes, score events/velocities/repicks, seed/block64/native48k/mono/FXoff, and old model crop starts remain unchanged. Whole-crop−24dBFS gains follow the old method; no per-note/window gain, EQ or onset adaptation.

Analysis initially compared A to the older3df6 Current object instead of selected6c83 B. This failed assertion is retained with its script/log. The explicit analysis-provenance-clarification.json binds the existing selectedB sanity oracle; no source, mask, gain or timing changed. The ten reobserved A and reference window facts match their correct frozen records.

Known-selector SG remains the independent contrast: its H3/H1 is already near the source under matched Both/Neck and public-pick midpoint worsens all six H3 windows. P3 context correction justifies a documented comparator, not a new universal source law. No new SG/Longitudinal renders or product Source, test, source-pin or default changes are part of this study. Earlier rooms remain frozen.

Reproduction: run.py builds/renders from the frozen protocol; analyze.py computes the unchanged observer; package.py writes figures and compact export. Verify final artifacts with verify.py. For a clean rerun use a separate directory and preserve the frozen receipt.


## Primary-source correction and evidence

The [Guitar-TECHS paper, p2 §III-B](https://arxiv.org/pdf/2501.03720)
specifies P3 Both selection and maximum tone/volume. Earlier claims that the
Sire selector was unidentified are superseded. Its force, hand position and
string age remain unknown. The original comparison audio and numeric masks
stay unchanged; they are context comparisons with a documented selector
mismatch. The official [T7 stock specification](https://www.sire-usa.com/products/t7)
supports the25.5-inch/two-single-coil hypothesis without identifying the
recorded instrument's installed electronics.

The paper gives P1 Bridge and P2 Neck, also at maximum tone/volume. Its
P2 interface table conflicts with the project website, and its declared
float32 format differs from actual cached PCM24 bytes; those differences
remain recorded. Analysis uses actual WAV encoding and existing source timing.
The paper mentions capo and finger plucking collectively across the twelve
excerpts. The inspected source inventory, original06/12 MIDI metadata and
project repository do not map those gestures to these excerpts. Pick versus
fingers and capo status therefore remain unknown, without a timbre inference.

The [four-phrase listening room](http://127.0.0.1:8729/) contains the real DI
and A/B/C/D at the same original fixed crops. Each whole clip has one RMS trim
to−24dBFS and common peak headroom; there is no EQ or new alignment. All
alternatives are shown, with the key separate. They use the released engine
under different fixed controls, not a new DSP candidate. The room's20 PCM24
files, gains and26 HTTP routes are checked; live browser interaction was not
tested because computer use is disabled.

Independent review reproduces all40 body rows,164 retained band-frame guards
and eight admitted Theil–Sen slopes, with maximum metric difference2.22e−16
and identical slopes. It verifies every changed control, all four old A raw
identities, crops/gains and unchanged Source/Tests. Original final artifacts
remain frozen; the separate review receipt does not alter their manifest.

Evidence receipts:

- Known-selector metadata: `727de7d95d122bd09139e39f2cf9e4b789991910cc3e3c37ded093fda7103592`.
- Prospective context protocol: `5299d901dd642e209b7bc31a0a91188788db07f2ce048dcd1a8f3c3b93eb67ec`.
- Completed98-artifact study: `f8da8e7acb2862f6b2a82528823187f1bffb9d3568ed76b65a07899e7bcd8675`.
- Independent numeric review: `287e5b4f5cba77d3194699c57981e4a2f33ca4ea81837839e900f43af328c00c`.
- Per-excerpt gesture audit: `4b6fb37d18417779b2c74da334c911b062d6700779cb4805ccc7b750666f9116`.

The separate [physical static-field study](static-field-early-body-2026-10-18.md)
also fails to establish a universal body correction. Neither favorable settings
nor a force-field idealization licenses a fitted global bass gain, source law,
factory default or authenticity percentage. Production remains1.2.6.
