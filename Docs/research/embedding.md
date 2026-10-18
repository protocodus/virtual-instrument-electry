# Embedding Electry

This repository owns Electry's instrument: the string/pickup engine, effects,
playing logic, and JUCE plug-in. Host adapters consume these sources at a pinned
revision. The Reason adapter lives separately in
[`reason-rack-extensions/Examples/Electry`](https://github.com/protocodus/reason-rack-extensions/tree/main/Examples/Electry);
its `Instrument` submodule points here. Do not copy or patch a second DSP engine
inside a host adapter.

`Source/DSP/ElectryPerformer.h` is the shared host-neutral entry point. It owns
written-to-sounding MIDI translation, articulation and solo keys, HOLD/latch
state, vibrato/tremolo gestures, chord dispatch, both Double performances, the
effect bypass ramp, and the causal acoustic feedback path. JUCE retains its
host-specific UI queue, state serialization, parameter access, and MPE zone
and channel ownership; it delegates the instrument behavior to this performer.

## Host contract

1. Construct a performer and call `prepare(sampleRate, maximumBlockSize)` outside
   the realtime callback. Time-effect storage is allocated at this boundary.
2. Map host values into `PerformerParameters`: physical `EngineParameters`,
   `FxParameters`, `outputMode` (0 Mono, 1 Stereo, 2 Double), and `fxEnabled`.
   Call `applyGuitarBuild()` for the single Guitar Build coordinate.
3. Call controller setters at their event positions. Pitch bend is bipolar
   -1..1 with the guitar's two-semitone range; master tuning adds -100..100 cents
   without consuming the wheel range.
4. Submit written MIDI numbers, including playable notes 40..98 and repick
   commands 100..107. Keyswitches 12..21 and momentary gestures 22..23 stay
   untransposed; written solo keys are 24..31, with 32 clearing solos.
5. At each sample position, process attack-conditioning events before playable
   attacks. `isAttackConditioningNote()` identifies performance keys.
   Submit each contiguous group of playable attacks through `noteOnChord()`;
   releases and intervening controllers flush the group. The API accepts at
   most 128 events per call and does not allocate.
6. Render the intervals between event positions with `render(left, right,
   samples)`. It fills both output buffers and handles feedback internally.
   `reset()` clears the complete instrument and effect tail; `allNotesOff()`
   releases keys with their natural tail; `allSoundOff()` immediately clears
   audio while preserving performance/controller state.

The engine API remains available for canonical DSP tests and JUCE MPE expression
routing. New wrappers should use the performer instead of rebuilding its signal
chain or note mapping.

## C++17 / embedded build

The DSP library compiles as C++17 without JUCE. The desktop plug-in and existing
test/oracle code continue to use C++20. For an SDK that does not allow runtime
global initialization, define `ELECTRY_EMBEDDED_TARGET=1`; amplifier lookup tables
then come from immutable data in `Source/DSP/FrozenTables/`. These data are
generated from the canonical native solvers, not separately tuned models.

Compile these translation units with `Source` on the include path:

- `Source/DSP/ElectryEngine.cpp`
- `Source/DSP/ElectryFx.cpp`
- `Source/DSP/ElectryPerformer.cpp`

`ElectryVisuals.cpp` is optional editor drawing support. Shipping builds also
define `ELECTRY_MEASURED_BODY_RESPONSE=1`, `ELECTRY_DECOUPLED_PICK_RELEASE=1`,
`ELECTRY_ENERGY_ATTACK_PITCH=1`, `ELECTRY_REFERENCE_STRING_MASS=1`, and
`ELECTRY_MODAL_DISPERSION_FIT=1`;
other experimental features default to zero. The mass reference is the named
light-gauge XL construction; its heavier gauge morph is a geometric-similarity
hypothesis, not a measured heavy eight-string set. CMake emits explicit 0/1
definitions for the mass and modal-fit options, so an explicit OFF also
overrides the source header's development defaults.
Use the same feature definitions in the adapter and every instrument source,
because some definitions affect class layouts.

To exercise the embedded profile without an SDK:

```sh
cmake -S . -B build-embedded -DCMAKE_BUILD_TYPE=Release \
  -DELECTRY_BUILD_PLUGIN=OFF -DELECTRY_BUILD_TOOLS=OFF \
  -DELECTRY_EMBEDDED_TARGET=ON
cmake --build build-embedded --target ElectryPerformerTests ElectryFxTests
ctest --test-dir build-embedded --output-on-failure -R 'Electry.(Performer|ElectryFx)$'
python3 Tools/generate_frozen_tables.py --check
```

This profile additionally compiles the core with `-fno-exceptions -fno-rtti
-fno-builtin` on GCC/Clang. It does not replace validation in the licensed SDK
and its host. Frozen tables were generated with GCC 14.2/glibc 2.41 on Linux;
the generator checks native builders and embedded accessors bit for bit in that
environment. Last-bit differences from another platform's math library require
review before regenerating the checked-in data.
