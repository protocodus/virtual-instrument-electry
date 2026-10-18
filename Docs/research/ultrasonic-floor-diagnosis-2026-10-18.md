# Upper-loss experiment: ultrasonic-floor diagnosis — 2026-10-18

The exact failed maximum is surviving modeled signal before the output
decimator. The evidence does not attribute it to output-decimator folding,
output-FIR arithmetic error or low-partial Hann leakage. This classification
does not certify the source/noise model, exclude every upstream artifact,
waive the failed test or approve a production change.

The experiment remains `95e5c1b2db1d393d9550c2c5e910d189169b0db0`, with its
single upper-loss flag default off. The [original failure](dry-guitar-tone-baseline-2026-10-18.md#rejected-upper-loss-experiment)
and all 39 original evidence artifacts remain unchanged. A separate 120-artifact
diagnostic freezes the new inspection, including an initial stopped driver
attempt. Its manifest SHA256 is
`3cd49644654292d9be718b55e83133e517c6cb065dffbf8810c430182a310ba7`.
The [numeric export](ultrasonic-floor-diagnosis-2026-10-18.json) is the exact
frozen aggregate, SHA256
`09be17346dfbe9a6784a6da5345c45ae8b5d9a4ef44fd8fe671d5db2761a2a5a`.

## Evidence at the failed maximum

The original eight-note, one-second fixture is reproduced at host 48 kHz,
internal 96 kHz, with the same seed, controls, Hann window, geometric frequency
grid and .2-second window start. Internal capture reconstructs public-process
PCM bit for bit in all eleven diagnostic runs.

| Quantity at 12424.018 Hz | Level |
| --- | ---: |
| Native candidate output | −168.871675 dBFS |
| Same input and FIR taps, double accumulation | −168.884359 dBFS |
| Native-minus-double arithmetic residual | −217.188668 dBFS |

At the failed maximum, an exact finite-window baseband/folding decomposition
reproduces the double-filtered complex DFT within `1.05e-15` absolute error.
The folding term there is near double-precision cancellation scale; it is
not a resolved explanation for the roughly `3.6e-9` native amplitude. Four
independent synthetic low/high-tone witnesses check the decomposition.

Hann nuisance reconstructions of H1–H3 and a full-file Fourier projection
onto bins below 1 kHz give upper maxima of −276.79 and −272.18 dBFS.
A Blackman–Harris observer still sees upper
energy at −175.79 dBFS, above its own corresponding nuisance estimates of
−194.61 and −190.11 dBFS. This is not a complete
decomposition of every 1–12 kHz component, and taper disagreement alone does
not identify leakage.

## Source path and clock controls

Turning off the contact-noise controls reduces the original upper maximum by
about 9.52 dB. Turning off body or artifact controls changes it much less.
These are causal control/path diagnostics, not isolated additive stems:
nonlinear source and numerical trajectories can also change.

At a shared internal 192 kHz clock, the host-96-kHz pre-decimator trace is bit
identical to native host-192-kHz PCM, both flag off and on. Candidate upper
energy persists at approximately −161.30/−161.32 dBFS in those two outputs.
This confirms survival without the output decimator. It does not establish
physical material correctness or rule out upstream nonlinear aliasing.

## Contract and comparison boundary

The old test caps the fixture's sampled Hann-windowed DFT maxima above 12 kHz
at −180 dBFS or 150 dB below its sampled spectral peak. Its fixed geometric
grid spans 40 to below 23500 Hz in one .2-second-start, 32768-frame window.
At a 48 kHz host, this band includes ordinary baseband signal.
Its failure is therefore not itself a measurement of aliasing. Replacing it
with an isolated output-error invariant would explicitly change the contract's
scope; retaining its numbers would not make the two meanings equivalent.
Such a test needs independent negative controls and leaves upstream alias
guards necessary. No production fixture or threshold is changed here.

The diagnosis permits a separately frozen, research-only whole-score comparison
against the two dry recordings. A must reproduce the prior shipping PCM
exactly; B changes only the disabled upper-loss mechanism. Source annotations,
controls, event times, whole-file gain rule and the previously frozen upper
prefix qualifications remain fixed. That comparison cannot erase this failure
or establish natural repicks, note changes or realtime readiness by itself.

A subsequent [output-decimator error-contract prototype](output-decimator-error-contract-2026-10-18.md)
retains this failure while testing aggregate output-stage error with independent
controls. It changes no fixture and does not approve the upper-loss candidate.
