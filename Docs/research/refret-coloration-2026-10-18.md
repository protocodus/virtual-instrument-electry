# Reducing refret handoff coloration: research options

18 October 2026. Baseline main `d5fc768412b0d6441f34a70c4e98658e0d1bf5b4`
contains the selected `634fd33` DSP. The selected source is `7d29d0e7be7f2e2afb7f5b8b4bb35285671fe8bd`
(version 1.2.5). Rendered validation and selection limits are recorded separately
in [the validation](../validation/refret-coloration-2026-10-18.md).

## What the present mechanism can explain

The existing picked refret reads old and new stationary periods from the same
ring and mixes them inside feedback during the existing pick Contact interval.
It preserves state and completes before the new modal pick Release. Small fret
changes use this path; ordinary large jumps outside an active handoff retain
the legacy choke and instantaneous retarget. Same-note repicks and fresh notes
are controls, not fresh instances of the crossfade mechanism.

For ideal taps, write `v = w(1-w)` and `delta = D_new-D_old`:

```text
H2(omega,w) = (1-w) exp(-j omega D_old) + w exp(-j omega D_new)
|H2|^2 = 1 - 4 v sin^2(omega delta/2).
```

At half weight, complete notches occur at `omega*delta = (2k+1)*pi`.
No finite scalar gain can recover a zero. The loss acts on the old ringing
state; the new modal release is subsequently superimposed. A persistent
DI/model spectral mismatch in fresh notes or same-note repicks therefore
cannot be assigned wholly to the brief refret blend.

[Carpentier, SMC2024, sections 3–4](https://smcnetwork.org/smc2024/papers/SMC2024_paper_id118.pdf)
documents two-tap comb coloration and an auxiliary-tap extension. His signed
extra taps narrow the notches but introduce magnitude ripple and extra cost.
The paper is a reason to test alternatives, not proof that its method is safe
inside a near-lossless guitar feedback loop.

## Option 1: a canonical C2 contact envelope

Keep the two taps and exact existing duration. Replace only the cubic
`w3(p)=3p^2-2p^3` with the unique minimum-degree polynomial satisfying
`w(0)=0`, `w(1)=1`, and zero first and second derivatives at both endpoints:
`w5(p)=10p^3-15p^4+6p^5`. Its derivative is `30p^2(1-p)^2 >= 0`, so
the weights remain convex. There is no new fitted timing, damping or gain.

Own exact symbolic integration, not a result claimed by the cited papers:

| Clock | Integral of w(1-w), 0..1 | Maximum w' | Integral of w'^2 |
| --- | ---: | ---: | ---: |
| Cubic C1 | 9/70 | 3/2 | 6/5 |
| Quintic C2, tested alternative | 25/231 | 15/8 | 10/7 |
| C2 velocity shoulders, selected | 449/3780 | 3/2 | 183/140 |
| Septic C3, research control | 245/2574 | 35/16 | 700/429 |

For ideal taps held at a fixed separation, the quintic reduces the unweighted
time-integrated frozen comb deficit by the factor `250/297`, about 15.8%.
It does not remove the midpoint zero or uniformly improve every moment. It
increases the maximum weight slope by 25% and the integrated squared slope by
about 19%. A smoother endpoint can coexist with stronger central modulation.
Septic C3 has still steeper central motion; higher degree is not free quality.

[Smith, Relation of Smoothness to Roll-Off Rate](https://dsprelated.com/freebooks/SASP/Relation_Smoothness_Roll_Off_Rate.html)
derives the relationship between derivative discontinuities and asymptotic
Fourier rolloff, including discrete-time limits. Applying that principle to
the contact-control envelope is an engineering inference. It does not establish
lower measured high-band energy in every finite guitar attack window.

The quintic prototype `c6dae7218720bd748f2a61aae99fe3867bb4050d` uses mirrored
evaluation around the midpoint to avoid float cancellation near one. It is a
defensible simple curve, but the expanded actual-engine rapid-change tests
show a larger upper-spectrum change than the selected alternative below.
Its separately frozen room remains at `http://127.0.0.1:8724/`.

### Selected alternative: C2 shoulders with the legacy peak speed

Retain the old cubic's continuous peak weight speed `M=3/2`. Join constant
speed to rest with the minimum-degree C1 velocity shoulder
`M(3u^2-2u^3)`, where `u=t/L`. Symmetric shoulders each have half their
rectangular area, so unit total weight requires `M(1-L)=1`: `L=1/3`.
Integrating the shoulder gives this parameter-free fade:

```text
0 <= t < 1/3:    w = u^3(2-u)/4, u=3t
1/3 <= t <= 2/3: w = 3t/2 - 1/4
2/3 < t <= 1:    w = 1 - w_left(1-t)
```

Both endpoints and both joins have zero second derivative; endpoint velocity
is zero and the maximum velocity remains `3/2`. Production mirrors the left
half for stable float arithmetic. This is an engineering control-envelope
construction, not an identified physical finger trajectory. The shoulder
duration follows from the existing peak speed and total unit weight; no
timing or gain constant is fitted to the recordings.

Exact integration gives `integral(w(1-w))=449/3780`, a ratio `449/486` against
cubic: 7.6132% less frozen cancellation exposure. The squared-speed integral
is `183/140`, 8.93% above cubic rather than quintic's 19.05%. The midpoint
notch is unchanged. Neither local slope nor finite-window HF is uniformly
lower at every time; the derivative profile differs near the shoulders.

Select this conservative alternative because the user prioritized natural
note changes without added attack artifacts. It gives up some ideal deficit
reduction but stays closer to shipping in actual rapid-play stress, including
20 Hz. Real phrase changes remain tiny; their wider-window HF is slightly
higher than quintic in some cases, so this is an explicit tradeoff rather
than a universal spectral win. The original slope prototype
`6b08b9aed8ab6299ce94f6a99d5e81b56f57c264` and selected `7d29d0e` have
identical complete `Source/` bytes. See the source-bound validation for the
72-case stress, absolute levels and final room.
Reject it if registered attacks, pitch settling, envelopes or rapid interrupts
worsen beyond existing evidence-based tolerances. Do not lower a gate to select
the candidate or claim that 15.8% analytical deficit is 15.8% more realistic.

## Option 2: three convex fixed taps at equal subdivisions

The independently proposed kernel uses old, midpoint and new periods, with
Bernstein weights `(1-w)^2`, `2w(1-w)`, `w^2`. The
[MIT author-hosted textbook](https://web.mit.edu/hyperbook/Patrikalakis-Maekawa-Cho/node9.html)
states the nonnegativity and partition-of-unity identities. Applying them to
in-loop delay taps is our extension, not a cited physical fret model.

For ideal delays, its frozen factorization is

```text
H3 = exp(-j omega D_old) [(1-w) + w exp(-j omega delta/2)]^2
|H3|^2 = [1 - 4v sin^2(omega delta/4)]^2.
```

The first zero moves to twice the previous frequency. At half weight, the old
first zero becomes magnitude 1/2 (-6.02 dB), but the old unity point at
`omega*delta=2*pi` becomes a new zero. This trades losses between modes.
The uniform full-phase mean of squared magnitude at half weight falls from
1/2 to 3/8. In general those means are `1-2v` and `1-4v+6v^2`, whose
difference is `-2v+6v^2 <= 0`. Under the cubic clock their time integrals are
`26/35` and `632/1001`. These are ideal full-phase averages, not measured
audio-band brightness or a guitar spectrum average.

All weights are nonnegative and sum to one, so a convex frozen-magnitude bound
survives when every cubic tap is bounded. The ideal factorization is not exact
for three independently interpolated fractional taps; numerical observers must
evaluate all actual FIRs. More subdivisions approach a moving read head and can
reintroduce the swept-pitch behavior previously rejected. Test this separately
from C2, before considering a combination. Reject if better low modes merely
hide stronger high-mode loss, attack coloration or expression discontinuities.

## Option 3: sequential convex intermediate handoffs

Instead of simultaneous Bernstein weights, crossfade old to midpoint during
the first half of Contact, then midpoint to new during the second half. Each
local tap separation halves, pushing its first notch higher; only two reads are
active at once. Exact stage endpoints can meet continuously at the midpoint.
The cost is a twice-faster weight clock and a temporary intermediate loop
resonance. More stages tend toward a staircase approximation to the rejected
moving head. This is another own extension of the fixed-pointer method in
[Smith, Large Delay Changes](https://www.dsprelated.com/freebooks/pasp/Large_Delay_Changes.html).
It is less minimal than C2 and needs the same attack and interval-lag gates.

## Option 4: phase matching or correlation compensation

A constant delay can phase-align one partial, but cannot align all partials
at two different physical periods while preserving both endpoint delays.
An allpass phase corrector can align selected frequencies only by changing
their phase/group delay; that enters the modal tuning and decay solve. An
instantaneous sign or phase rotation can itself produce a pickup velocity
impulse. A zero-crossing switch only matches value, not derivative, and cannot
guarantee a suitable point within the unchanged Contact deadline.

Equal-power or correlation-normalized gains are particularly unsuitable as a
drop-in fix inside feedback. At the midpoint, equal-power coefficients sum to
sqrt(2), so coherent low-frequency loop gain can exceed one. A normalization
based on `1/sqrt(a^2+b^2+2*rho*a*b)` diverges near negative correlation and
cannot restore a true comb zero. A recent or measured correlation does not
certify future loop stability. Reject such gain compensation without an actual
power-state model and a complete bound; existing contact damping is not a
blanket allowance to amplify the loop.

## Option 5: an energy-preserving allpass/scattering transition

[Werner, DAFx2020](https://www.dafx.de/paper-archive/2020/proceedings/papers/DAFx2020_paper_59.pdf)
constructs time-varying allpasses from an orthogonal power-normalized junction
and a delay. A representative junction is
`[[g,sqrt(1-g^2)],[sqrt(1-g^2),-g]]`. Its two outputs retain the joint
instantaneous squared power of its two inputs; retaining the auxiliary port
as state yields an energy-preserving time-varying structure. That is stronger
than a frozen response bound for a changing scalar crossfade.

This could avoid discarding the difference component of old and new waves,
but its phase trajectory is not a pure variable delay. A large auxiliary delay
introduces dispersive resonances; fractional stages, initialization and removal
must be included in the energy proof. Copying old history into a new auxiliary
delay duplicates energy unless the original state is transformed consistently.
The endpoint at an uncoupled port can trap state; blindly reconnecting it on a
later note creates a new artifact. Do not add an arbitrary epsilon near a
singular coefficient and call the physical problem solved. This is a larger
architecture experiment, not the preferred small follow-up.

## Option 6: physical state remapping or a local moving fret

A physical handoff should preserve displacement and velocity on retained
physical string positions, impose a finite contact at the new boundary, and
carry both reflected rails and fixed pickup observations coherently. Uniform
resampling of the ring preserves normalized phase but changes the displacement
at fixed pickups. Preserving only their first flux sample conceals one
derivative impulse while leaving the state discontinuity. Preserving
displacement alone also fails to preserve the independent velocity state.

[Evangelista, DAFx2011](https://www.dafx.de/paper-archive/2011/Papers/96_e.pdf)
models the fret as a local junction and distinguishes vertical clamping from
horizontal slip/friction. A fuller fixed-open-length two-rail model could place
that junction at the chosen fret while retaining the inactive segment. It
would eliminate the artificial two-length superposition, but requires a new
fractional junction, boundary motion, inactive-string loss and pickup/dispersion
state treatment. Existing fitted loss/phase behavior must be revalidated.

The related [Pakarinen et al., ICASSP2005 author-institution paper](https://aaltodoc.aalto.fi/server/api/core/bitstreams/c4d811b1-05e4-4949-91dc-3c06ee3b9e9e/content)
analyzes energy errors in variable-delay strings and offers corrected sliding
points and power-normalized wave-digital alternatives. Its distinction between
varying length and varying tension matters here: generic energy conservation
does not prove that a finger changing frets supplies the modeled work.

## Selection gates

Compare the immutable main against each independent prototype first. Keep the
four actual DI phrase crops, event bytes, source offsets and one whole-file
gain protocol; do not retime or fit source-specific EQ. Retain source12's
4.45 ms transfer-onset residual and source06's repeat residual. The source
guitar/pickup/force mismatch remains, so use fresh-note and same-note controls
to distinguish general timbre gaps from this specific handoff.

Keep the prior pitch/attack/transient gates and all ownership, strum, expression,
queued repick, reset, no-allocation and sample-rate contracts. Add actual
tap-response measurements across frequency and the Contact clock. A frozen
convex bound does not prove passivity of time-varying reads: lengthening can
observe the same wave packet at more than one time. Descending transitions and
rapid interruption stress remain necessary. Document improved and worsened
bands/windows; neither a relocated zero nor one lower scalar metric establishes
uniformly better sound color.
