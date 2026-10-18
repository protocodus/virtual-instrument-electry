# Complex-pole dispersion feasibility — 2026-10-17

An offline conjugate-pole allpass experiment fits Electry's declared stiff-string
law substantially more accurately than the searched real-pole topology at the
same total order. It is numerical feasibility evidence. No complex-pole runtime
path, coefficient table or production sound change is implemented here.

## Reference and comparison conditions

The target, normalized to the sounding fundamental, is

```text
omega_n = n omega_1 sqrt((1 + B n^2) / (1 + B)).
```

This is the uniform-string relation in [Kemp, SN Applied Sciences 2, 636
(2020), equations 3–5](https://doi.org/10.1007/s42452-020-2391-2), divided by
its first mode. The experiment retains the engine's empirical wound-core `B`;
it does not measure a guitar's stiffness, mass or termination.

**The raw delay is the ideal fractional transfer `exp(-j omega D)`.** The
experiment excludes the actual cubic fractional-delay interpolator, loss-filter
phase/magnitude, horizontal polarization, bridge coupling, excitation, pickup,
body and amplifier. Its roots are exact phase roots of this isolated lossless
mathematical system. Compare them with `AuditDispersion.py`'s **isolated
dispersion-only** column, which uses the same ideal raw delay. The complete
lossy-loop phase column is a different condition; these values are neither
complete-engine damped poles nor recorded-guitar measurements.

Each conjugate pole pair `p = r exp(+-j theta)`, `0 < r < 1`, realizes

```text
a1 = -2 r cos(theta), a2 = r^2
H(z) = (a2 + a1 z^-1 + z^-2) / (1 + a1 z^-1 + a2 z^-2).
```

[Abel and Smith, DAFx 2006, sections 2–3](https://www.dafx.de/paper-archive/2006/papers/p_013.pdf)
give the allpass pole/zero and group-delay identities. This experiment uses a
separate constrained modal optimizer, rather than their design algorithm or an
overlap coefficient. Fundamental compensation sets
`D = 2 pi / omega_1 + phase(A(omega_1)) / omega_1`, where `A` is the cascade.
Safeguarded Newton iteration solves the globally indexed condition
`D omega - phase(A(omega)) = 2 pi n`. Every retained fit requires `D >= 4`,
stable poles and correct root indices. Positive delay plus positive allpass
group delay makes accumulated phase strictly increasing; the validation also
checks this across the full unit-circle half-band, preventing hidden extra
intra-target phase crossings or a wrapped root being mislabeled as Hn.

## Finite probe

The 27 cells are physical E1, the default 27.625-inch scale, Gauge 0/0.5/1,
frets 0/12/22 and host rates 44.1/96/384 kHz. One through four biquads are
fitted separately. The objective is maximum exact H2–H16 root error in cents.
A separate 121-point fractional-order grid over 1–16 checks inter-mode phase
ripple; those fractional indices are mathematical guards, not additional
guitar modes.

| Biquads | Total order | Maximum integer-mode error over 27 cells | Median cell maximum | Maximum fractional-order guard |
|---:|---:|---:|---:|---:|
| 1 | 2 | 78.095 cents | 11.647 cents | 86.426 cents |
| 2 | 4 | 35.855 cents | 2.143 cents | 36.317 cents |
| 3 | 6 | 13.957 cents | 0.280 cents | 17.582 cents |
| 4 | 8 | 4.286 cents | 0.022 cents | 7.562 cents |

All 108 retained fits pass their stability, positive-delay, unit-magnitude,
global root-index and phase-monotonicity checks. For four biquads, the smallest
raw delay is 373.716 samples and the smallest pole-radius stability margin is
0.003590. This establishes these sampled static filters only.

An earlier continuous real-pole search examined ten selected cells. Its best
two-groups-of-four results collapsed to nearly equal coefficients. At the
default heavy E1, its isolated maximum errors at frets 0/12/22 were
1.380/10.008/40.507 cents, compared with approximately
0.001/0.060/4.286 cents for four conjugate pairs here. Twelve additional
asymmetric eight-real-pole starts at the fret-22 cell returned the same
40.507-cent plateau. This is a numerical search result, **not a proof of a
global real-pole lower bound**. It identifies the restricted pole geometry as
a more promising variable than another coarse-grid adjustment.

The 27 cells exclude the maximum `B = 0.003` short-scale construction. A
separate heavy E1/fret-22/25.5-inch/-2-semitone probe at 44.1 and 384 kHz gives
4-pair integer maxima **10.490 cents**, with fractional-order maxima
**27.102 cents**. A five-pair library-API probe gives **2.818 cents** and
**15.099 cents** respectively. Every retained extreme fit passes the same
static stability/root checks. Four pairs therefore do not establish uniformly
few-cent accuracy over the full engine domain, and integer-mode minimax can
hide meaningful inter-mode phase ripple.

## Reproduction and provenance

The optional research dependency versions used were NumPy 1.26.4 and SciPy
1.14.1 under Python 3.11. The mandatory instrument build does not require them.
The optimizer is deterministic multistart SLSQP with seed 174271, `maxiter=300`
and `ftol=1e-8`; its search coordinates are `log(sigma)` and `theta/omega_1`,
with `r=exp(-sigma omega_1)`. The numerical search bounds are recorded in the
report and are not guitar-calibrated constants. There is no global-optimum
certificate. Optimizer statuses and every retained pole/residual are preserved.

```sh
python3 Tools/ExploreDispersion.py --output /tmp/complex-27-cells.json
python3 Tests/ExploreDispersionTests.py
```

Working evidence is retained under `build-legato-arrival-review/evidence/`:
`complex-27-cells.json`, `complex-extremes.json` and the `complex-provenance/`
directory. The original temporary archive is
`/tmp/electry-dispersion-optimizer/provenance/`: it contains exact as-run source,
observer C++, observations, reports, a manifest and `reproduce-extremes.py`.
The observation source was working-tree content based on `93f2341`, including
the separate legato-endpoint correction. Relevant SHA-256 values are:

```text
Engine.cpp: fe516ddabfed819d81e16ecd668683e6025c3259f465c39d2a3794c7ca416604
Engine.h:   861df324dcecd4ce5e82e21953e825ebe8b063c917dc5e6d85a7e3834b35043c
as-run explorer: a705d01d7843242f78f79a6c3810821a159516d4d08034e97985b108ccecff7d
27-cell report: 6e1120a97ad375379e76524d9037d6817841c9ce0caadb06789f2def31f3e8d2
extreme report: b5849e12317aa68f06aa42583351fc45ba93a4653dc5cc9a560a392897daf329
```

The retained as-run explorer differs from the current script only in its
corrected bibliographic article number. Re-run its frozen observations without
compiling or consulting the current checkout:

```sh
python3 build-legato-arrival-review/evidence/complex-provenance/Tools/ExploreDispersion.py \
  --observations build-legato-arrival-review/evidence/complex-provenance/observations.json \
  --output /tmp/complex-reproduced.json
python3 build-legato-arrival-review/evidence/complex-provenance/reproduce-extremes.py
```

Four-pair fitting took 52.3 seconds over 27 cells; all four orders took 102.7
seconds. These are offline wall times, not audio-callback measurements. A
128-by-64 table storing eight float parameters per node would occupy 256 KiB,
but no table resolution, interpolation accuracy, fitting duration or memory
budget has been validated. Runtime promotion still needs a defined full domain,
held-out interpolation/quantization tests, actual cubic-delay/loss/coupling
roots, a stable state handoff during playing, tuning and articulation rails,
audio comparisons and measured realtime cost. Instantaneously stable poles do
not establish safety of a time-varying filter or a preference by ear.
