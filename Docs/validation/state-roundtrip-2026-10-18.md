# Canonical plug-in state — 18 October 2026

The existing macOS CI run on main `45e9a81` failed 152 parameter-matrix
save/restore checks. A current-source diagnostic using JUCE 8.0.14 isolated
one differing value in factory rig 0: output saved as
`-5.999999046325684` dB and restored as `-5.999999523162842` dB. The other 29
parameter values and root metadata were unchanged.

JUCE's parameter adapter constructs its default through normalized conversion
without snapping. Electry's validated restore already snaps every plain value
to its legal range, so the first restoration could change its floating-point
representation. The other three factory rigs already reproduced exact bytes.

`getStateInformation` now applies the same range snapping to the detached
tree returned by `copyState` before serializing it. The live parameter atomics,
audio state, restore validation, parameter definitions and metadata are
unchanged. Independent review confirmed that JUCE returns a deep copy and
that parameter ranges remain valid throughout the operation.

The new processor regression checks all four factory rigs and 15 automation
positions across all 30 parameters, including endpoints and values around
quantization boundaries. It requires exact state bytes through two restores
and bit-identical live raw values before and after saving. These focused
checks pass. The existing matrix's exact-byte assertion is unchanged.

This fixes an existing publication gate while retaining development version
1.2.2. It does not change the selected DSP or rerender the frozen auditions.
Hosted Linux/Windows/macOS and sanitizer validation remains the PR merge gate.
