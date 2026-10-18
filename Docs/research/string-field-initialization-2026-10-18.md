# String state, pickup time and the attack — 2026-10-18

Matching the running loop filters to an installed string state matters greatly
to its first returns and subsequent tone. It does not, by itself, establish
that the state represents a physical released string. A separate common-time
pickup prototype passes steady-field checks but introduces an attack boost
and fails existing behavior constraints. Neither experiment is enabled in the
published instrument.

This investigation continues the
[tone and control audit](tone-identifiability-2026-10-18.md), using published
`6c83ff453ff7ac346598e8e73367ced128a90838`. It separates four questions that
cannot be answered by a favorable harmonic ratio alone: consistency of the
loop state, its spatial interpretation, observation time, and local forcing.
The accompanying [numeric record](string-field-initialization-2026-10-18.json)
retains measured rows, source hashes and the frozen evidence receipts.

## Complete state versus an empty filter bank

The existing default-off analytic initializer installs a folded triangular
displacement in the delay line. Its feedback-filter histories start at zero.
The line samples and those histories together form the string's state;
initializing one component does not initialize the whole system.

An independent characteristic-equation calculation includes the actual cubic
fractional read, eight dispersion allpasses, damping, active loss stages and
loop gain. For E1, E2, B2, G3 and G5, finite sets of 16, 32, 32, 20 and 24
known complex modes with matching feedback histories follow the production linear loop for
0.5 seconds to relative RMS error below 7.4e−7. Clearing only the filter
histories, while retaining exactly the same line samples, increases error
to 15.6–44.9% without the additional release low-pass. This is a causal
initialization witness, not a comparison with a guitar recording.

On E2, during 200–500 ms, that chosen state's pickup-output spectral centroid
changes from approximately 365 to 683 Hz when its histories are cleared;
the 500–2000 Hz power share changes from .136 to .765. The test holds the
initial line samples fixed. A missing source low-pass alone therefore does
not explain this particular bright-start result.

A second construction propagates the feedback state over piecewise affine
incoming triangle segments. Binary matrix powers avoid a long sample-by-sample
pre-roll. Ten raw/physical-period cases agree with independent 32,768-sample
SciPy pre-rolls within 4.1e−13 maximum absolute state error. These five
particular banks need 256–2048 past samples and 5–17 affine segments to reach
the recorded residuals. They do not establish a universal coefficient-domain
bound or a realtime implementation budget.

## A consistent state still needs a spatial model

For an ideal, nondispersive string, the two current traveling rails give
`y(x,t) = q(t + x/c) - q(t - x/c)`. A folded ring can represent both current
rails without reading future audio. In an independent 1024-cell ideal-ring
witness, the appropriate current-rail difference reconstructs a known
511-mode triangle at 257 positions to floating precision, with zero initial
velocity. The finite-mode target differs slightly from an unbandlimited
point triangle; those are separate errors.

Electry's raw line delay is shorter than the physical fundamental period:
the rational feedback bank supplies the remaining frequency-dependent phase.
Its internal states are not uniformly spaced samples of that missing string
segment. Reading a reflected point from the raw line interprets the bank as
a localized termination. Distributing its phase along the string is a
different model, although both can have the same temporal loop poles.

An independent calculation includes both coupled polarization branches.
Initialized production trajectories agree with those actual coupled poles to
3.44e−7–1.19e−6 relative RMS over 0.25 seconds. Choosing a simply supported
distributed sine basis additionally permits a finite-mode static point-force
shape and zero velocity to be imposed in both polarizations. That spatial
basis is an explicit assumption; it is not identified by the lumped bank.

Against that chosen field, raw-current-rail reads have 19.3–30.6% relative
displacement error. Constant physical-period time delays are closer,
.060–.432%, but retain nonzero velocity and endpoint residuals. Thus neither
warming histories nor replacing the raw period with the physical period
proves a correct initial physical field.

Stiffness also changes propagation phase with frequency. Paiva et al.,
Section 4, Eqs. 25–26, warp each spatial delay. The current loop's temporal
dispersion fit does not supply that spatial propagation operator. Smith's
digital-waveguide derivation similarly places dispersive allpasses in the
propagating medium. A compact approximation or segmented two-rail model
must specify how loss and dispersion are distributed.

## Pickup time and the failed startup candidate

The legacy local comb `q(t) - beta*q(t - 2*x/c)` has an anchor-dependent
phase relative to the physical standing field. Mixing independently delayed
Neck and Bridge anchors drops different phase factors. Paiva et al.,
Section 2, Eqs. 12–15, retain those factors under one common source delay.

Prototype `875f58b58fb154ac2188a86d90a1c806067c2afe` instead reads
`q(t - T + x/c) - beta*q(t - T - x/c)`, with one causal `T` for both anchors.
It retains the old guarded geometry, beta .60, coil pair, aperture, flux and
Faraday circuit. Unsupported complete ray pairs fall back together to the
legacy reader. The flag defaults off; a later test-only commit verifies reset.

Independent field, geometry and production-observation checks pass more than
1.1 million assertions. They establish the narrower steady-observation
identity, including interpolation error, rather than causal source equivalence.
Existing Idle, Refret and Fx suites pass, but Engine fails five behavior rails
and five source snapshot/table pins. Four E1/B1 peak-to-sustain limits and
the velocity-span minimum are actual behavior failures. No gain, threshold
or snapshot adjustment was attempted.

With artifact, contact-noise, body and sympathetic paths removed, fresh E1/E2
raw-field RMS rises 4.60/3.67 dB during 0–80 ms and dry attack peaks rise
2.62/3.17 dB. At 1–2 seconds, dry RMS differs only .022/.014 dB. This
separates a forced-startup interaction from a steady-state gain difference.

The compact source writes its opposite image into older ring cells *now*.
Those cells can represent current spatial state; during forcing they are
not immutable records of earlier seam output. A sinusoidal pickup proof
cannot justify using them as such. An independent ideal impulse witness
demonstrates this distinction without noise, a filter or a fitted gain.

The next experiment needs a defined source/observer coordinate mapping or a
causal reconstruction of the seam history. Advancing initial modes to cancel
an observation delay merely changes which physical event time the output
describes. It does not restore the original zero-velocity condition at the
same release boundary.

## Actual history removes the boost but worsens real tone

Follow-up `2f34c9ce71e5cde8be083b8fc16b9ea065001e9b` records actual head writes
in a separate fixed history before the source edits spatial cells. Chokes do
not rewrite that past. Common-time taps read this history; the source and
all gain settings remain fixed. Both experiment flags still default off.

Clean E1/E2 attack pickup-centre field peaks now differ from baseline by less than .0001 dB,
and dry attack peaks by −.0032/−.0167 dB. All five previous behavior failures
clear. C++17, Idle, Refret, Fx and the independent oracle pass, but one Engine
Dead evaluation-table pin remains a failure. It was not refreshed. The extra
history increases engine size by 1,049,216 bytes, including geometry; reset
clear cost was outside the native timing probe and remains unmeasured.

The original six SG body windows and three admitted upper prefixes provide
decisive counterevidence. Both-selector H1 share falls .14660 → .12432 against
.17285 real in the early window, and .14758 → .12622 against .17192 late.
Early H3/H1 rises 3.644 → 4.997 dB against 3.860 real. Its 1.5–4 kHz decay
slope moves −43.893 → −52.724 dB/s against −24.402 real. No observability
guard fails. Bridge differences are small and Neck is nearly unchanged.
Removing the attack boost is useful causal evidence, but this candidate is
rejected as a standalone tone improvement. Full-corpus qualification was not
started after this result.

## A proven ideal source mapping has stricter limits

In a lossless integer ring, the compact opposite-image source is an exact
signed rotation of equal physical source rays. A decoder with the matching
rotation works during arbitrary ongoing forcing, not merely for free modes.
When the pick position changes, the old state must be re-gauged before using
that new coordinate. The earlier unshifted impulse witness demonstrates a
coordinate mismatch; it does not prove that an ideal transform is impossible.

Two current production features prevent promoting that shortcut directly:
the image has different filtering and width from the direct ray, and a
ring-only rotation does not commute with a fixed feedback bank. Independent
counterexamples preserve both limitations. An exact causal correction can
instead reconstruct past head writes from the source-deposit history, but
that is a history identity rather than a physical point-force proof.

A fixed physical seam, equal source rays and current two-rail reads do have
an exact compact realization. An independent unfolded network agrees with
the folded ring over 4096 samples at all 511 interior points, including two
selected complete Electry feedback banks. These are integer reference-network
tests, not production renders at the instrument's fractional delays.

Using the raw propagation period as the physical length still changes the
instrument hypothesis. For the same length and mass, the five tested notes
across both polarizations would imply 12.2–30.0% higher tension than the declared material coordinates.
The earlier distributed-field discrepancy remains. Fractional reads,
source-force integration, initial conditions, dispersion distribution and
live performance therefore need explicit treatment before this mathematical
network becomes a guitar candidate.

## Qualification boundary

These experiments identify mechanisms and reject incomplete fixes. They do
not qualify a new source, claim artifact-free note changes, or establish a
percentage of guitar authenticity. Fresh notes, ringing repicks, changed
frets, hand contact, coupled strings, automation, realtime cost and whole-score
dry-reference comparisons remain required before enabling a joint candidate.
The published DSP, its defaults, existing test limits and frozen listening
rooms remain unchanged by this research.

Primary sources:

- [Smith, ideal plucked string and traveling initial conditions](https://www.dsprelated.com/freebooks/pasp/Ideal_Plucked_String.html).
- [Smith, lossy and dispersive digital-waveguide theory](https://www.dsprelated.com/freebooks/pasp/Digital_Waveguide_Theory.html).
- [Paiva, Pakarinen and Välimäki, Acoustics and Modeling of Pickups](https://research.aalto.fi/en/publications/acoustics-and-modeling-of-pickups/), JAES 60(10), 2012, pp. 768–782; [author manuscript](https://www.researchgate.net/publication/234034228_Acoustics_and_Modeling_of_Pickups).
