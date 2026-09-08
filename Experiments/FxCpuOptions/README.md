These are isolated prototypes for the [five-option evaluation](../../Docs/fx-cpu-options.md), pinned to baseline `f2a5258b7f14b5dc72ef87250636f3021b3dc241`. They are not connected to the shipping CMake targets. The patches reproduce the measured DSP snapshots, including the combined PI math + diode candidate.

**Historical snapshot.** The commands below reproduce the original `f2a5258` prototypes. The later [mono-reuse integration patch](patches/mono_reuse_integration.patch) instead targets the frozen selector/table implementation identified in the [integration metadata](../../Docs/fx-cpu/mono-reuse/metadata.json); [CheckFirstDivergence.cpp](probes/stereo/CheckFirstDivergence.cpp) probes the integrated implementation. These two files are separate from the original eight-executable reproduction. See the [current implementation](../../README.md#amplifier-chain), [oversampling measurements](../../Docs/fx-cpu/selectable-oversampling/summary.csv), and [channel-reuse measurements](../../Docs/fx-cpu/mono-reuse/summary.csv) for the shipped changes.

Build from the repository root with Python 3 and a C++20 compiler:

```sh
python3 Experiments/FxCpuOptions/prepare.py
```

The default output is the ignored `build-fx-options-repro/` directory. Choose a new directory with `--output` for another build. `--cxx` selects the compiler. The script extracts the pinned sources, applies the candidate patches inside that directory and builds all eight executables. It also copies the circuit/transition probes and pinned FX regression suite for further investigation. No external DSP library is needed.

Run timings serially while the machine is otherwise idle. The robust phase takes several minutes; it runs four alternating candidate sweeps with baseline brackets and five repeats per scenario. Mono and guitar phases use baseline/candidate/candidate/baseline order. Each output directory permits a phase only once to prevent accidental duplicate samples.

```sh
python3 Experiments/FxCpuOptions/measure.py robust
python3 Experiments/FxCpuOptions/measure.py mono
python3 Experiments/FxCpuOptions/measure.py guitar
python3 Experiments/FxCpuOptions/measure.py quick --rate 96000 --output build-fx-options-repro/timing-96
```

Pass `--build` to use a different prepared directory. `summary.csv` reports baseline-relative process CPU savings and elapsed corroboration. `runs.jsonl`, raw run CSVs and metadata retain commands, machine/compiler details and executable hashes. These are FX-only timings; guitar input is synthesized before the timer starts.

Capture deterministic references and compare any candidate:

```sh
build-fx-options-repro/baseline/evaluate --capture build-fx-options-repro/reference --seconds 2
build-fx-options-repro/combined/evaluate --compare build-fx-options-repro/reference --seconds 2
build-fx-options-repro/baseline/evaluate --capture build-fx-options-repro/guitar-reference --input guitar --seconds 4 --rate 48000
build-fx-options-repro/combined/evaluate --compare build-fx-options-repro/guitar-reference --input guitar --seconds 4 --rate 48000
```

Default comparison limits are peak error `5e-5` FS and RMS error `1e-6` FS (−120 dBFS). A failing comparison returns nonzero. These numerical budgets are useful regression criteria, not a formal listening threshold. Oversampling changes latency, so its raw null needs alignment and a separate alias measurement. `--help` lists input modes, scenarios and limits.

The diode and solver probes compile against their candidate FX source; the PI scalar probe includes its source directly. `prepare.py` puts each under the corresponding candidate's `Tools/`. For example:

```sh
c++ -std=c++20 -O3 -DNDEBUG -Ibuild-fx-options-repro/diode_lut/Source build-fx-options-repro/diode_lut/Tools/ValidateDiode.cpp build-fx-options-repro/diode_lut/Source/DSP/ElectryFx.cpp -o build-fx-options-repro/validate-diode
build-fx-options-repro/validate-diode
c++ -std=c++20 -O3 -DNDEBUG -Ibuild-fx-options-repro/pi_math/Source build-fx-options-repro/pi_math/Tools/ValidatePiMath.cpp -o build-fx-options-repro/validate-pi
build-fx-options-repro/validate-pi
```

To run the full FX suite against the combined candidate:

```sh
c++ -std=c++20 -O3 -DNDEBUG -DELECTRY_MEASURED_BODY_RESPONSE=1 -DELECTRY_DECOUPLED_PICK_RELEASE=1 -Ibuild-fx-options-repro/combined/Source build-fx-options-repro/Tests/ElectryFxTests.cpp build-fx-options-repro/combined/Source/DSP/ElectryFx.cpp build-fx-options-repro/engine.o -o build-fx-options-repro/combined/fx-tests
build-fx-options-repro/combined/fx-tests
```
