# Wound-string loss hypothesis: numerical qualification — 2026-10-19

A positive fixed-string material-loss hypothesis can produce more H2/H3
cooling than the current loop. The existing voiced decay targets do not
identify it as one literal material law. This study selects no DSP candidate
and renders no audio. All five frozen open-string coefficient pairs are
positive; negative diagnostic per-note solves are a different result.

The [complete numeric record](wound-loss-hypothesis-2026-10-19.json) contains
the prospective protocol,115 parameter cells,230 diagnostic solves,690 actual
coupled poles, every flagged case and independent checks. Production remains
`6c83ff453ff7ac346598e8e73367ced128a90838`, version 1.2.6.

## Defined physical hypothesis

Use five wound strings E1/B1/E2/A2/D3, frets0–22, fresh/light conventional
controls:115 cells. The protocol predates parameter export and computation.
An observer-only shadow captures actual target locals and T/µ/L/diameter/B
from immutable production source. An independently compiled uninstrumented
control reproduces all 5,635 comparable fields exactly. No process() or PCM
output is involved, and no later sensitivity grid is added.

With `c=sqrt(T/mu)` and existing empirical `B=pi²*EI/(T*L²)`, define:

```
omega_n = (pi*c/L)*n*sqrt(1+B*n²)
RL = 2*pi*eta + 2*pi*d*sqrt(pi*eta*rho*f)
alpha_air = RL/(2*mu)
alpha_n = alpha_air + omega_n*(delta_W + delta_E*B*n²)/(2*(1+B*n²))
T60 = ln(1000)/alpha_n
```

Air uses outer diameter, density1.2kg/m³ and viscosity1.8e−5Pa·s, identically
on both axes. [Paté et al., III-A](https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A8_Pate_PredictingDecayTime.pdf)
describe air loss, constant-inverse-Q winding/dislocation contributions and
bending loss. The denominator here follows the declared stiff-PDE extension
with complex tension and EI; it is not a verbatim measured guitar equation.
Finite-band complex loss is a weak-loss assumption, not a complete causal
constitutive model. The constant-Q coordinate labelled delta_W can lump
other mechanisms, including dislocation loss; two anchors cannot identify
winding friction separately.

Solve the two effective coefficients once from each **open vertical** fresh
intrinsic fundamental/upper target after subtracting air. Keep those same
coefficients across all frets and both axes. Invert stiff dispersion at each
requested anchor frequency; do not erase the physical fundamental shift.
There is no negative clipping, record-driven fit, per-note replacement,
new mixture fraction or gain/EQ adjustment. The230 additional per-cell/axis
solves are diagnostic and never become a candidate coefficient table.

## Positive fixed-open model; unidentified literal material

| String | delta_W | delta_E | Anchor condition number |
|---|---:|---:|---:|
| E1 |.002062462|.000044627|106.58|
| B1 |.001392192|.000653687|267.65|
| E2 |.001019558|.007438037|798.73|
| A2 |.000769518|.056161856|2586.28|
| D3 |.000549767|.365256397|8559.17|

All five pairs are positive, and all 115 fixed-open H1–H3 predictions are
dissipative. Large delta_E interacts with very small empirical B: maximum
effective H1–H3 material loss factor is.002062. The coefficient alone does
not prove weak-loss instability. The cited steel-loss context1e−4–1e−3 is
far smaller, but is not a universal acceptance bound for wound strings.
The effective core EI is also unmeasured, so literal material identification
remains unsupported.

Open E2's physical H1/H2/H3 slopes are−2.76884/−5.22542/−7.64999dB/s,
versus actual configured production vertical poles
−2.79369/−2.79502/−2.80032dB/s. Thus this fixed-string hypothesis supplies
lower-partial cooling in principle. It is not a dry-recording result, an
evolving pickup-band prediction or a newly qualified instrument source.

Nineteen of 230 independent per-cell target decompositions require negative
delta_E: E1 vertical frets7–22 and horizontal frets20–22. These expose
incompatibility with exactly retaining every legacy voiced target in this
positive two-loss basis. They are **not** coefficients or instability of the
frozen-open model.184 diagnostic delta_E values fall outside the cited steel
context; the poorest-conditioned solve is open D3.

Fixed open coefficients depart from vertical fundamental targets by up to
+17.92% in T60 and from horizontal targets by−41.18% to−30.64%. Existing
dead-spot, speaking-length and horizontal×1.7 voicing contribute to these
differences. Retaining all those old targets is not itself a naturalness
criterion, and fitting new material properties at each fret would hide the
identifiability problem. No universal rejection of constant-Q loss follows.

## Verification and remaining implementation work

Independent coefficient reconstruction closes to1.67e−16, and all 690
configured coupled-pole determinants are below1.47e−15. Requested decay
targets, physical modal rates and actual rational-loop poles remain separate.
The fixed-bank poles exclude active Contact and pickup/output processing.
All original files and failed preliminary serialization are retained.

A separate fixed-corner first-order shelf fails before audio: matching the
same two gains requires scalar gains above unity. A fixed-bank removal of
the selected correction, with raw delay rephased at H1, increases fresh E2
vertical fixed-bank H2/H1 and H3/H1 cooling at1.1s only
from.001333/.006641 to.250990/.669352dB/s. That counterfactual change is
therefore much smaller than the [observed finite-envelope mismatch](p2-known-neck-body-2026-10-19.md).
This does not isolate an intrinsic recorded loss or qualify the removal.

## A unique unit-DC shelf also has a tradeoff

A separately registered [eight-case numerical followup](unit-dc-loss-shelf-2026-10-19.json)
solves a first-order shelf from both absolute fresh gains with DC fixed at1.
No corner or extra scalar is chosen. All eight sections are passive and match
the anchors to1.11e−16. Actual cached loss-fit frequency is distinguished from
the slightly moving live pitch; the corrected basis reproduces all48 old
correction coefficients exactly, before candidate computation.

This produces substantial lower-partial cooling, but fresh vertical coupled
T60 near1.5kHz falls5.744→.493s; current-default falls1.964→.372s. Modes
near4/8kHz last longer. These fixed-bank results expose a large middle-band
tradeoff, not a rendered improvement or measured guitar modal comparison.
The fully fresh loop also retains an undamped common DC pole at1, and every
combined response exceeds the existing original-scalar bound. A passive
section is insufficient to qualify the complete feedback loop. No audio,
gain clipping, DC blocker, scalar-bound waiver or production change follows.

## Production scope

Any production implementation still needs a causal passive approximation,
phase compensation and retained state in both refret banks, separate age and
hand losses, and current play-quality, alias and CPU qualification. No new
attack, natural repick, fret-change, upper-sustain or authenticity claim
follows. Source, defaults, tests, thresholds and frozen previews are unchanged.

Original 47-artifact manifest:
`09d36c86fa2cc5339f172dbc32b36155e38ace8b926815cf9a3a61a7e8e3ecd9`.
