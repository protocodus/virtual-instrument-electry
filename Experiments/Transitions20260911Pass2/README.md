# Second transition pass

The baseline is the first transition candidate, reconstructed from commit
`aa1f7e0963ae488a4d0c3c7eeb3c6cb81fb749f8` plus
`baseline-from-aa1f7e0.patch`. The final combined candidate fixes idle-string
hand-loss time units and a reopened pick's hand-follower reset.

See [the implementation and validation record](../../Docs/transition-refinement-2026-09-11.md)
and [the final receipt](validation-results.json). `Attack/`, `Ring/` and
`Review/` contain the independent probes and measurements. The isolated
source changes are preserved as `Attack/source.patch` and `Ring/source.patch`.
The combined final source passes all 23 core tests, all four native tests,
and the focused sanitizer suite; no fixture tolerances changed.

`RenderFollowup.cpp` reuses the complete previous renderer and appends two
low-string rhythm controls. The original renderer now returns zero explicitly
so its entry point can also be called as an ordinary C++ function; all 27
original baseline WAVs remain byte-identical to the previous pass.

From the repository root, reconstruct the baseline in a fresh directory:

```sh
mkdir -p build-transitions-20260911-pass2/baseline-src
git archive aa1f7e0963ae488a4d0c3c7eeb3c6cb81fb749f8 \
  | tar -x -C build-transitions-20260911-pass2/baseline-src
git apply --directory=build-transitions-20260911-pass2/baseline-src \
  Experiments/Transitions20260911Pass2/baseline-from-aa1f7e0.patch
```

For an isolated candidate, copy that reconstructed baseline into `attack-src`
or `ring-src` and apply the corresponding isolated patch there. The probe
READMEs accept those directories as inputs. The root source includes both.

Compile and render the matched comparison (Clang C++20 and Python/NumPy):

```sh
clang++ -std=c++20 -O2 -DNDEBUG \
  -DELECTRY_DECOUPLED_PICK_RELEASE=1 -DELECTRY_MEASURED_BODY_RESPONSE=1 \
  -DELECTRY_ENERGY_ATTACK_PITCH=1 \
  -Ibuild-transitions-20260911-pass2/baseline-src/Source \
  Experiments/Transitions20260911Pass2/RenderFollowup.cpp \
  build-transitions-20260911-pass2/baseline-src/Source/DSP/ElectryEngine.cpp \
  build-transitions-20260911-pass2/baseline-src/Source/DSP/ElectryFx.cpp \
  build-transitions-20260911-pass2/baseline-src/Source/DSP/ElectryVisuals.cpp \
  -o build-transitions-20260911-pass2/render-before
clang++ -std=c++20 -O2 -DNDEBUG \
  -DELECTRY_DECOUPLED_PICK_RELEASE=1 -DELECTRY_MEASURED_BODY_RESPONSE=1 \
  -DELECTRY_ENERGY_ATTACK_PITCH=1 -ISource \
  Experiments/Transitions20260911Pass2/RenderFollowup.cpp \
  Source/DSP/ElectryEngine.cpp Source/DSP/ElectryFx.cpp Source/DSP/ElectryVisuals.cpp \
  -o build-transitions-20260911-pass2/render-after
build-transitions-20260911-pass2/render-before build-transitions-20260911-pass2/before
build-transitions-20260911-pass2/render-after build-transitions-20260911-pass2/after
python3 Experiments/Transitions20260911Pass2/AnalyzeFollowup.py \
  build-transitions-20260911-pass2/before build-transitions-20260911-pass2/after \
  build-transitions-20260911-pass2/comparison
```

For the bounded combined sanitizer run, compile `SanitizeFollowup.cpp` with
the same definitions and DSP sources, replacing `-O2 -DNDEBUG` with
`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`, then run it.
The final receipt identifies the exact source, score, compiler, native
artifacts, raw audio and validation logs. Tests establish the model and
delivery invariants; the final musical preference remains an audition.
