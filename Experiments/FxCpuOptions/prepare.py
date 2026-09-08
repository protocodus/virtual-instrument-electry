#!/usr/bin/env python3
"""Build isolated FX candidates against the pinned baseline; never edit Source/."""
import argparse
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
BASELINE = "f2a5258b7f14b5dc72ef87250636f3021b3dc241"
VARIANTS = ("diode_lut", "pi_math", "oversampling", "stereo", "solver")
FLAGS = ["-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra", "-Wpedantic",
         "-DELECTRY_MEASURED_BODY_RESPONSE=1", "-DELECTRY_DECOUPLED_PICK_RELEASE=1"]


def run(*args):
    subprocess.run([str(arg) for arg in args], cwd=REPO, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=REPO / "build-fx-options-repro")
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    output = args.output.resolve()
    try:
        relative = output.relative_to(REPO)
    except ValueError:
        parser.error("output must be inside the repository")
    if output.exists():
        parser.error("choose a new output directory")
    source = output / "baseline" / "Source" / "DSP"
    source.mkdir(parents=True)
    for name in ("ElectryFx.cpp", "ElectryFx.h", "ElectryEngine.cpp",
                 "ElectryEngine.h", "DspMath.h", "ModernCabinetIR.h"):
        data = subprocess.check_output(
            ["git", "show", f"{BASELINE}:Source/DSP/{name}"], cwd=REPO)
        (source / name).write_bytes(data)
    for variant in (*VARIANTS, "combined"):
        shutil.copytree(source.parent, output / variant / "Source")
        patches = ("pi_math", "diode_lut") if variant == "combined" else (variant,)
        for patch in patches:
            run("git", "apply", f"--directory={relative / variant}",
                HERE / "patches" / f"{patch}.patch")
        probes = HERE / "probes" / variant
        if probes.exists():
            shutil.copytree(probes, output / variant / "Tools")

    # Keep the alias fixture and regression suite pinned to the same revision.
    tests = output / "Tests"
    tests.mkdir()
    (tests / "ElectryFxTests.cpp").write_bytes(subprocess.check_output(
        ["git", "show", f"{BASELINE}:Tests/ElectryFxTests.cpp"], cwd=REPO))

    # Every candidate uses the same unchanged guitar generator object. Guitar
    # synthesis is outside the effects timer; it is an additional audio probe.
    engine_object = output / "engine.o"
    run(args.cxx, *FLAGS, f"-I{source.parent}", "-c", source / "ElectryEngine.cpp",
        "-o", engine_object)
    for variant in ("baseline", *VARIANTS, "combined"):
        directory = output / variant
        tolerances = ("1e-7", "1e-8") if variant == "solver" else (None,)
        for tolerance in tolerances:
            extra = [f"-DELECTRY_PHASE_CONVERGENCE_TOLERANCE={tolerance}"] if tolerance else []
            filename = f"evaluate-{tolerance}" if tolerance else "evaluate"
            run(args.cxx, *FLAGS, *extra, f"-I{directory / 'Source'}",
                HERE / "EvaluateFx.cpp", directory / "Source/DSP/ElectryFx.cpp",
                engine_object, "-o", directory / filename)
    print(f"Built all candidates in {output}")


if __name__ == "__main__":
    main()
