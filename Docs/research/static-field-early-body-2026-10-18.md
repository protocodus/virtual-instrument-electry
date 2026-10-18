# Static field and finite unloading: early body — 2026-10-18

A coherent static string field does not, by itself, restore the missing lower
body. Two new production-output diagnostics test instantaneous release and
finite force unloading. Some HB harmonic ratios improve, while matched bass
power and SC ratios have significant regressions. Neither formulation is
promoted to the instrument.

The [complete numeric record](static-field-early-body-2026-10-18.json) retains
both prospective protocols, all 128 window rows, physical fields, exact gains,
raw levels, source identities, independent checks and a terminology correction.
Published source remains `6c83ff453ff7ac346598e8e73367ced128a90838`, version1.2.6.

## One physical field with its own source and observation

The diagnostic defines a simply supported stiff string from the same physical
length, tension and linear mass as the production voice. Existing empirical B
defines `EI=T*L²*B/pi²`; it is a model coordinate, not measured EI. At contact,
hold its complete static point-force field with zero velocity:

```
m_n = mu*L/2
omega_n² = (T/mu)*(n*pi/L)²*(1+B*n²)
q_n = F*sin(n*pi*p)/(m_n*omega_n²), v_n=0.
```

F uses the existing4.8N force coordinate and the actual velocity/stroke draw,
giving4.077886N here. Neither force nor displacement is fitted to recordings.
All304 modes below48kHz, the Nyquist limit of the96kHz internal clock, are retained. The two axes
reuse the existing style weights as force-component factors and share this
uncoupled dynamics. This omits the production polarization split and glide.

The pickup observes that same field at one physical time, integrating its sine
shapes over the existing coil apertures, positions, spacing and balance. The
existing0.040m/unit conversion feeds a verified hook immediately before actual
production nonlinear magnetics, Faraday, loaded-coil and output processing.
This replaces source **and spatial decoding** coherently; it does not isolate
excitation alone. No separate offline approximation of the electrical chain,
output EQ or fitted gain is used.

The first voice callback precedes the scheduled physical contact. Actual
geometry is captured at Contact index317, followed by Release at467, with
native MIDI timing retained. The held field's flux is primed once at Contact;
there is no flux reset at Release. This is an initial-condition assumption,
not a measured or simulated pick-loading trajectory. The initial diagnostic
metadata error, caught before any candidate render, is preserved separately.

## Finite unloading has an exact mechanical interpretation

The first experiment removes the force immediately. A separate prospective
experiment reuses the existing cubic load/slip pulse and string-release
one-pole as a nonnegative, unit-discrete-area force-release kernel r. The
Bessel modal projection is omitted because the explicit static field supplies
the spatial spectrum it approximates. There is no H1 magnitude compensation.

At sample i, the remaining held force is `F0*(1-sum(r_j,j<=i))`, applied over
interval `[i,i+1]`. Its displacement equals the held field plus the convolution
of r with free displacement minus held displacement. Superposition commutes
with this linear spatial observation before the nonlinear pickup. Independent
constant-force modal stepping verifies that interpretation and timing.

The frozen protocol incorrectly calls the inherited pulse C2. Its clamped
cubic smoothstep extension is C1; the bound addendum corrects the terminology
without changing any data. Sampled force is interval-constant. Exact harmonic
flow makes displacement and velocity continuous across its changes; this does
not establish an artifact-free attack or a passive plectrum contact law.

## Fixed early-body comparison

Both studies use the first2s of each already frozen day1 open-E2 score from
[Wright, McKenzie and Hamilton's dataset](https://zenodo.org/records/19823590),
CC BY4.0. They retain each source's eight original first-note windows. The
primary scope is the three128ms windows centered at .10/.20/.35s. No known
zero-state source, matched hand force or pickup selector is asserted.

A is unchanged1.2.6 at clean, fresh/light conventional controls, Bridge/Tone1,
with body, noise, artifacts and sympathy zero. Four fixed profiles combine
HB/SC endpoints with public Pick Position.18/.5. These are hardware/control
hypotheses; all are shown. Models have identical MIDI, seed, rate, block,
length and controls within each pair. There is no new alignment or NoteOff.

One whole2s RMS trim to−24dBFS and common peak headroom match each letter and
reference. These gains differ from the earlier60s score convention. No EQ,
per-window gain or best setting per note is applied. The new field is explicitly
lossless, so differences cannot all be assigned to initial source alone.

Finite-unloading median **model minus reference** errors over the three primary
windows; each cell is A → candidate, in dB:

| Recording/profile | H2/H1 | H3/H1 | Matched80–250Hz power |
|---|---:|---:|---:|
| Strat/HB.18 | 8.951 → 7.942 | 8.336 → 7.352 | −.407 → −2.149 |
| Strat/HB.5 | 6.688 → 5.464 | 1.249 → −.638 | 3.193 → .751 |
| Strat/SC.18 | 7.715 → 8.077 | 6.682 → 7.715 | −.732 → −3.374 |
| Strat/SC.5 | 5.449 → 5.588 | −.395 → −.326 | 2.884 → −.511 |
| LesPaul/HB.18 | 2.723 → 1.714 | 7.178 → 6.195 | −4.524 → −6.266 |
| LesPaul/HB.5 | .460 → −.763 | .092 → −1.796 | −.924 → −3.366 |
| LesPaul/SC.18 | 1.488 → 1.849 | 5.524 → 6.558 | −4.849 → −7.491 |
| LesPaul/SC.5 | −.779 → −.640 | −1.552 → −1.484 | −1.233 → −4.628 |

HB near-bridge ratios move closer, but fundamental power still falls after
matching: Strat H1 error−7.597→−8.195dB and LesPaul−8.543→−9.463dB.
The instantaneous formulation is similarly mixed; its complete rows remain
separate. Finite candidate raw full-file RMS is1.385–1.602 times A. Normalizing
the output does not erase that physical-level change.

## Validation and limits

Four A fixtures are byte-identical between separately linked original and
hook builds; four parameter-export passes reproduce them again. The earlier
bridge proof covers24 routed fixtures and six zero-sensor controls. All builds
use the production flags and C++17. All24 current Source files equal6c83.

Independent review reconstructs all304 finite-release modes over4096 steps:
maximum pickup-field difference7.60e−15, relative work-ledger error4.16e−14.
It also reproduces all128 analysis windows and36 packaged WAVs. Independent
coil quadrature, static storage and scalar cosine witnesses pass. Doubling the
field cutoff changes early harmonic ratios by less than1.14e−5dB; that sampled
truncation test is not an antialiasing certificate.

Finite modal bandwidth does not qualify attack propagation. In particular,
the earlier flexible-string hard-front test cannot be applied directly to a
stiff PDE whose high-frequency group speed exceeds the flexible wave speed.
The separate distributed-string numerical work makes that distinction explicit.

The original engine still owns scheduling, pitch metadata and retirement.
Loss, real loading, body, polarization, repicks, moving frets, aliases and
eight-string realtime remain unqualified for the replacement field. Production
tests were not represented as qualifying this diagnostic. No Source, defaults,
test fixtures, thresholds or version changed, and no authenticity percentage
or new natural-performance claim follows.

Completed136-artifact evidence manifest:
`89e0ac7126ab17e415a133ead1caed4306f8d6f65d8831f10ef1c5dc0ce6dbea`.
Both protocols, failed preliminary metadata assumption, C1 addendum and
independent review are retained. Research continues from the corrected
reference settings before adopting a new source law.
