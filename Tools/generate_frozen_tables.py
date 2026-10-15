#!/usr/bin/env python3
"""Generate or verify Electry's immutable embedded amplifier tables.

The exporter includes the canonical DSP builders directly. Native plug-ins keep
those original runtime caches; ELECTRY_EMBEDDED_TARGET consumes the frozen data
without writable globals, static guards or startup solves. --check compares both
the regenerated values and the embedded accessors byte for byte.

Generation environment: GCC 14.2, glibc 2.41, Linux. Exact transcendental results
can differ by a last bit on another platform; review such changes explicitly.
"""
import argparse
import pathlib
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--cxx", default="g++")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    target = root / "Source/DSP/FrozenTables"
    with tempfile.TemporaryDirectory(prefix="electry-frozen-") as name:
        temporary = pathlib.Path(name)

        def generate(output, embedded=False):
            executable = temporary / ("embedded" if embedded else "native")
            subprocess.run([
                args.cxx, "-std=c++17", "-O2", "-I", str(root / "Source"),
                *(["-DELECTRY_EMBEDDED_TARGET=1"] if embedded else []),
                str(root / "Tools/GenerateFrozenFxTables.cpp"),
                "-o", str(executable),
            ], check=True)
            subprocess.run([str(executable), str(output)], check=True)

        def contents(directory):
            return {p.name: p.read_bytes() for p in directory.glob("*.inc")}

        generated = temporary / "native-data"
        generate(generated)
        actual = contents(generated)
        if args.check:
            expected = contents(target)
            mismatches = sorted(name for name in expected.keys() | actual.keys()
                                if expected.get(name) != actual.get(name))
            if mismatches:
                raise SystemExit("Frozen data differ: " + ", ".join(mismatches))
            embedded = temporary / "embedded-data"
            generate(embedded, embedded=True)
            if contents(embedded) != actual:
                raise SystemExit("Embedded accessors differ from native builders")
            print(f"Verified {len(actual)} frozen files and embedded accessors "
                  "bit for bit against native builders.")
        else:
            target.mkdir(exist_ok=True)
            for path in generated.glob("*.inc"):
                shutil.copyfile(path, target / path.name)
            print(f"Generated {len(actual)} frozen files.")


if __name__ == "__main__":
    main()
