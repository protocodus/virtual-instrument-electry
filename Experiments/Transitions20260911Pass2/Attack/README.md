# Reopened pick: preserve the relaxing heel

This isolated candidate fixes a small continuity error in an already-ringing
string's Palm → Sustain transition. `startVoice()` used to reset both hand
followers and their peaks on every repick. The control update interprets an
empty peak as full dip depth, so an open repick briefly tightened the previous
Palm filter despite the heel already lifting.

The candidate retains those four follower coordinates only for a ringing
Sustain repick while style contact remains positive or pressure is falling.
Fresh plucks, stationary Palm, steady-pressure repicks, and other articulations
keep their established per-stroke follower reset. It adds no gain, noise,
excitation, coefficient interpolation, or string-state reset. Existing passive
loss fitting, finite heel motion and delay compensation remain in control.

The 48 kHz dry probe starts with a 400 ms Palm ring and repicks the same held
string at equal velocity. Before the change, the live vertical hand-dip depth
increased by 19–35% within the first millisecond across E1, E2, G3, D4 and G4.
Preserving the follower removes the artificial reset. The remaining largest
increase is 7.6%; stroke-force variation and the existing loss refit are still
present. This does not impose an artificial monotonically decreasing filter.

This is a subtle repair: the 12–35 ms body-window level changes by less than
0.08 dB, and the complete short-probe null is −31 to −43 dB relative to its RMS.
The preceding Palm wave, stationary open/Palm repicks and sibling-driven
unpicked lifts are sample-identical. `results.json` records the measured
coordinates, source hashes and controls.

The frozen eight-bar score was also rendered through this candidate. Its dry
rolling-chord style take changes by +0.026 dB RMS (null −34.87 dB), and the
Drop-E take by +0.022 dB (null −34.51 dB). Pressure-riff lifts precede the next
pick far enough to settle, so they and the no-pick controls stay sample-identical.
See `riff-results.json`. These are descriptors of the change, not a listening
preference score or a new recorded-performer calibration.

Reproduce from the repository root (NumPy and a C++20 clang++ are required):

```sh
python3 Experiments/Transitions20260911Pass2/Attack/RunProbe.py \
  build-transitions-20260911-pass2/baseline-src \
  build-transitions-20260911-pass2/attack-src \
  build-transitions-20260911-pass2/attack-evidence
```

`Probe.cpp` writes raw float32 mono files at 48 kHz; cases 0–3 are reopened
repick, sibling-driven unpicked lift, stationary open repick, and stationary
Palm repick. The tool verifies that the controls and all samples preceding the
transition are identical. `FocusedTests.cpp` reuses the engine regression's
24 new continuity cases plus its finite/shared-hand tests. The baseline fails
all 24 new continuity assertions; the narrow candidate passes. The complete
engine suite also passed on the earlier prototype with a broader articulation
gate; final integration should rerun the complete suite on the narrowed
Sustain-only gate. No fixture tolerances were changed.
