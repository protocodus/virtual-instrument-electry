# Upper-loss ablation against two dry guitars — 2026-10-18

Reject this candidate as a universal tone fix. Removing one inherited upper-loss
multiplier slows the modeled upper decay, but substantially worsens the Les
Paul recording's existing 1.5–4 kHz level excess. The lower-body deficit barely
changes. The experiment remains default off, with its original 25 Engine
failures retained; no production setting, test or source pin changes.

This rejects one empirical trial under fixed control hypotheses. It does not
prove that every lower-loss formulation is physically wrong: the recordings'
pickup selection, electrical settings, pick location and applied force remain
unknown. A future mechanism must distinguish excitation/readout weighting from
loss through an explicitly declared physical law. These results do not justify
fitting an intermediate multiplier or excluding the second performer.

## Comparison and integrity

A is immutable shipping commit `6c83ff453ff7ac346598e8e73367ced128a90838`.
B is `95e5c1b2db1d393d9550c2c5e910d189169b0db0`, with
`ELECTRY_STRING_ONLY_UPPER_LOSS=1`. B omits only `0.86 * 0.58` from the
measured-body branch's upper-decay ratio. This is an ablation of inherited
structural voicing, not a measured material-loss law or proof of double counting.

The [Longitudinal Guitar String Ageing dataset](https://zenodo.org/records/19823590)
supplies both complete day-1 open-E2 sequences: 30 plucks on a Fender
Stratocaster and 30 on an Epiphone Les Paul. Its
[author manuscript](https://www.pure.ed.ac.uk/ws/portalfiles/portal/656673718/A_Longitudinal_Dataset_for_Guitar_String_Ageing.pdf)
documents the instruments and string sets. The two verified originals and all
derived audio remain in ignored research storage under the dataset's CC BY 4.0
license; no audio or samples enter Git or the product.

Each player has two fixed whole-score profiles, public Pick Position `.18` and
`.5`. All use fresh/light Age `0`, Scale `0`, Gauge `0`, Tone `1`, common pickup
type `0` and Bridge selection. The `.5` control is a sensitivity hypothesis,
not an identified source hand position or the string's geometric midpoint.
The resolved parameters, all 30 native-frame events, raw levels and exact
whole-file gains are in the [numeric export](upper-loss-ablation-2026-10-18.json).

Every A/B pair uses native 48 kHz, block 64, seed `0x160c2026`, fixed velocity
`.85`, mono and FX off. The original crop and frame count remain fixed. No
NoteOffs, alignment correction, force fitting, EQ, per-note gains or profile
selection are introduced. Each track receives one whole-score gain under the
original RMS-matching rule, to −24 dBFS; common headroom remains one. Raw RMS
and peak values are retained because whole-score matching can conceal differences in the
distribution of energy over time.

All four newly built A raw WAVs match the complete bytes and SHA256 of their
previously frozen takes. Every recomputed A metric scalar and observability
guard dictionary matches its previous export exactly. All A/B resolved
controls, event records and frame counts match. The 96 archived source files
across four builds match their immutable commits.

## Upper decay and absolute level

The original 240 source-defined windows per player remain unchanged. Each is
128 ms, centered at `.10, .20, .35, .55, .80, 1.10, 1.45, 1.75` seconds after
its source-derived onset. The first window therefore spans `.036–.164` seconds.
Source-only hold censoring retains 236 P1 and 239 P2 windows; observed
preparations/stops are not treated as identified physical pick-contact durations.

A later, separate joint observability mask was frozen before B: entire
Reference + A(`.18`) + A(`.5`) prefixes passing the unchanged Blackman–Harris/
Hann, background and low-partial leakage guards. It admits 30 P1 and 27 P2
prefixes from 1.5–4 kHz, eight P1 prefixes from 4–8 kHz, and no P2 prefixes
from 4–8 kHz, at each pick setting. No original mask is rewritten, prefix
shortened or candidate-dependent exclusion added. All 130 compared
Reference/A/B prefixes pass those same guards.

Band-mixture decay slopes, dB/s; negative absolute-error change means closer
to the reference. These slopes are not identified modal T60 or material constants.

| Player / Pick Position / band | Prefixes | Reference / A / B median slope | Improved / worsened | Median absolute-error change |
| --- | ---: | ---: | ---: | ---: |
| P1 / .18 / 1.5–4 kHz | 30 | −13.830 / −18.611 / −10.873 | 21 / 9 | −2.185 |
| P1 / .5 / 1.5–4 kHz | 30 | −13.830 / −17.193 / −10.181 | 16 / 14 | −0.099 |
| P2 / .18 / 1.5–4 kHz | 27 | −12.851 / −18.512 / −10.907 | 22 / 5 | −3.648 |
| P2 / .5 / 1.5–4 kHz | 27 | −12.851 / −17.355 / −10.259 | 21 / 6 | −1.447 |
| P1 / .18 / 4–8 kHz | 8 | −38.127 / −103.659 / −75.382 | 8 / 0 | −29.718 |
| P1 / .5 / 4–8 kHz | 8 | −38.127 / −105.023 / −75.123 | 8 / 0 | −30.929 |

B's median 1.5–4 kHz decay is slower than the reference in every profile;
34 of the 114 paired slope errors worsen. All 16 P1 4–8 kHz comparisons
improve, but their decay remains too fast. P2 4–8 kHz remains unscored.

Absolute band level is a separate outcome. The following signed errors use
the same qualified prefix windows and one whole-score gain: positive means
more modeled power than reference power. Improvements are counted by paired
absolute error, before reducing to medians.

| Player / Pick Position / band | Windows | A → B median error, dB | Improved / worsened windows | Median absolute-error change, dB |
| --- | ---: | ---: | ---: | ---: |
| P1 / .18 / 1.5–4 kHz | 227 | −2.710 → +2.386 | 113 / 114 | +0.043 |
| P1 / .5 / 1.5–4 kHz | 227 | −1.310 → +3.610 | 88 / 139 | +1.223 |
| P2 / .18 / 1.5–4 kHz | 216 | +5.216 → +10.173 | 1 / 215 | +3.804 |
| P2 / .5 / 1.5–4 kHz | 216 | +6.392 → +11.065 | 1 / 215 | +3.944 |
| P1 / .18 / 4–8 kHz | 36 | −37.441 → −16.986 | 36 / 0 | −16.858 |
| P1 / .5 / 4–8 kHz | 36 | −38.196 → −17.788 | 36 / 0 | −17.009 |

P2's 1.5–4 kHz level worsens in 215 of 216 windows at each pick setting.
P1's 1.5–4 kHz outcome is mixed; its qualified 4–8 kHz level becomes closer
but remains deficient. Slower decay alone is insufficient evidence of a
tone improvement across these references.

## Lower body and partial balance

Signed model-minus-reference errors over the fixed uncensored hold windows,
dB; each cell is A → B. Absolute H1/H2/H3 levels, ratios, paired absolute-error
distributions and window RMS are retained separately in the numeric record.

| Player / Pick Position | H1 | H2 | H3 | H3/H1 | 80–250 Hz | 250 Hz–1 kHz |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1 / .18 | −8.434 → −8.415 | −5.738 → −5.798 | +0.328 → +0.257 | +8.339 → +8.195 | −3.937 → −3.995 | +7.659 → +7.618 |
| P1 / .5 | −1.382 → −1.364 | −0.970 → −0.992 | −0.277 → −0.310 | +1.007 → +0.890 | −0.956 → −0.946 | +4.362 → +4.421 |
| P2 / .18 | −8.038 → −8.092 | −4.957 → −4.971 | −0.861 → −0.908 | +7.513 → +7.479 | −4.449 → −4.431 | +6.076 → +6.110 |
| P2 / .5 | −1.174 → −1.187 | −0.424 → −0.464 | −1.829 → −1.860 | −0.194 → −0.166 | −0.642 → −0.714 | +2.065 → +2.044 |

The ablation barely changes lower-body balance or the 250 Hz–1 kHz excess.
It does not resolve P1's nonmonotonic H1/H2 trough. The earlier fixed pick
position sensitivity is much larger than this loss change; it still does
not identify the actual hand position or justify a global default.

## Evidence receipt and limits

The [compact JSON](upper-loss-ablation-2026-10-18.json) retains all 130
qualified prefix rows, including every regression, all three slopes/R²
values, window IDs, guards and matched-level distributions. It also contains
all fixed-offset and all 30 per-pluck distributions for every profile,
the whole-study distributions, gains, controls, events, crop frames and
track hashes. Summary arrays use explicitly named columns and six decimal
places; the exact cached records retain full precision and quartiles.
Unqualified broad upper-band summaries remain explicitly descriptive.

The immutable full aggregate is 2,305,908 bytes, SHA256
`ef886d4f42480cb6679e2302884fcea758d02a7ae19ccb96e3afc9fdc803baaa`.
Its 181-artifact manifest is
`069d0ae9248961ecbd4fced49bc347b11899c8fba3ad056fc504c91456917f13`.
The prospective protocol is
`afb0a670d02aa7172bf2e29e15f53186bd995d1f38265c65aaecb18d01b59c4f`;
the joint mask is
`53d3ab583b40e50e885c1d2c663d3edc350ebf9c673673f2bc17b4957f08d659`.
The original annotations retain SHA256
`98188fd8f57b8394f77b1b9ab1abcfed96b8be9748f6a46d1781143eb6726786`.
All paths in the numeric export are portable ignored-cache references.

In the candidate research worktree, the unchanged evidence can be checked with:

```sh
python3 build-structural-evidence/whole-score-upper-loss/final-export/verify.py
```

The original 24 source-pin failures and one
[ultrasonic-floor contract failure](ultrasonic-floor-diagnosis-2026-10-18.md)
remain. Both flag states compile under C++17 and pass Fx and Refret; that does
not make B green. SDK 5 authority was verified before native builds; no new
SDK-host or realtime qualification is claimed.

Both references are six-string instruments with incomplete pickup/electrical
and hand metadata. P1's .038 low E is below the model's .042 minimum; a common
24.75-inch Les Paul scale is also outside the model's 25.5-inch minimum, while
the actual source scale remains unmeasured. Correlated repeated plucks are
not independent population evidence. These tone windows do not validate
attack artifacts, fret changes, repick realism, premium eight-string identity
or a percentage of authenticity. A research audition cannot waive the failed
contract or reverse the measured level tradeoff.

The local [four-pair listening room](http://127.0.0.1:8728/) contains exact
copies of all twelve already matched Reference/A/B whole-take files, with a
separate mechanism key. No new listener vote is claimed under the standing
autonomous-evaluation instruction. A prospective six-score, 111-onset
performance protocol was preserved but left unexecuted after this tone
rejection. These results make no new attack, repick or refret qualification.
