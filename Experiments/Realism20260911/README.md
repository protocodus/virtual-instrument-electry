# Everyday playing comparison, September 11

The frozen baseline is commit `3baafa69adf1c6b80455ed6e58dcde009873c359`.
The candidate source hashes and build definitions are in
[source-provenance.json](source-provenance.json). The same renderer supplies 19
phrases with simultaneous dry and Modern taps, an identical seeded score and
explicit event/parameter manifest. Measurements use raw mono float32. Listening
copies have constant whole-file gains for paired RMS matching and safe peaks.

Run from the repository root (Clang C++20, Python 3 with NumPy):

```sh
mkdir -p build-realism-20260911/baseline-src
git archive 3baafa69adf1c6b80455ed6e58dcde009873c359 Source \
  | tar -x -C build-realism-20260911/baseline-src
clang++ -std=c++20 -O2 -DNDEBUG \
  -DELECTRY_DECOUPLED_PICK_RELEASE=1 -DELECTRY_MEASURED_BODY_RESPONSE=1 \
  -DELECTRY_ENERGY_ATTACK_PITCH=0 \
  -Ibuild-realism-20260911/baseline-src/Source \
  Experiments/Realism20260911/RenderRealism.cpp \
  build-realism-20260911/baseline-src/Source/DSP/ElectryEngine.cpp \
  build-realism-20260911/baseline-src/Source/DSP/ElectryFx.cpp \
  build-realism-20260911/baseline-src/Source/DSP/ElectryVisuals.cpp \
  -o build-realism-20260911/render-before
clang++ -std=c++20 -O2 -DNDEBUG \
  -DELECTRY_DECOUPLED_PICK_RELEASE=1 -DELECTRY_MEASURED_BODY_RESPONSE=1 \
  -DELECTRY_ENERGY_ATTACK_PITCH=1 -ISource \
  Experiments/Realism20260911/RenderRealism.cpp \
  Source/DSP/ElectryEngine.cpp Source/DSP/ElectryFx.cpp Source/DSP/ElectryVisuals.cpp \
  -o build-realism-20260911/render-after
build-realism-20260911/render-before build-realism-20260911/before
build-realism-20260911/render-after build-realism-20260911/after
python3 Experiments/Realism20260911/AnalyzeRealism.py \
  build-realism-20260911/before build-realism-20260911/after \
  build-realism-20260911/comparison
```

Open `build-realism-20260911/comparison/listening.html`. All 38 dry/Modern
comparisons have raw measurements and checksums in `comparison.json`.

[Contact](Contact/README.md) and [Rhythm](Rhythm/README.md) retain the independent
ablation/prototype comparisons. The [real DI study](../../Docs/realism-reference-pass5-2026-09-11.md)
keeps recording evidence separate from the model's voicing choices. No real
performance audio is bundled in the instrument.
