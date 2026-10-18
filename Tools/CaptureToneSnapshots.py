#!/usr/bin/env python3
"""Observe existing complete-source fixtures at an immutable source revision.

This preserves the original render calls, windows and assertions. It adds
numeric output, calls only the three relevant test functions, and records
their original failures; it does not turn the observation into a passing test.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--define", action="append", default=[],
                        metavar="ELECTRY_NAME=0|1",
                        help="Feature state, validated by the immutable builder")
    args = parser.parse_args()
    defines = {}
    for item in args.define:
        name, separator, value = item.partition("=")
        if not separator or value not in ("0", "1") or name in defines:
            parser.error("Each --define must name one unique ELECTRY_NAME=0|1")
        defines[name] = int(value)
    spec = importlib.util.spec_from_file_location(
        "immutable_build", args.repo / "Tools/BuildRealismComparisons.py")
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    revision = helper.run(["git", "rev-parse", args.revision + "^{commit}"], args.repo).decode().strip()
    original = helper.run(["git", "show", revision + ":Tests/ElectryEngineTests.cpp"], args.repo)
    source = original.decode()
    replacements = {
        "            worst = std::max(worst, std::abs(measured - partial.decibels));":
        '            std::cout << "SNAPSHOT partial " << partial.index << " " << measured << "\\n";\n'
        "            worst = std::max(worst, std::abs(measured - partial.decibels));",
        "    const double atFull = peakDbAt(127, shippingResponse);":
        "    const double atFull = peakDbAt(127, shippingResponse);\n"
        '    std::cout << "SNAPSHOT velocity " << atOne << " " << atSixtyFour << " " << atFull << "\\n";',
        "int main(int argc, char** argv)": "int originalEngineSuiteMain(int argc, char** argv)",
    }
    for old, new in replacements.items():
        if source.count(old) != 1:
            raise ValueError("Snapshot seam is absent or ambiguous: " + old)
        source = source.replace(old, new)
    source = '#include <iomanip>\n' + source + '''
int main() {
    std::cout << std::setprecision(17);
    testHumbuckerTwoCoilNotch();
    testVelocityDynamicRange();
    testDeadNote();
    std::cout << "SNAPSHOT original_failures " << failures << "\\n";
}
'''
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    driver = output / "snapshot-observer.cpp"
    driver.write_text(source)
    binary = helper.build(args.repo, output / "build", revision, [], driver, defines)
    import subprocess
    run = subprocess.run([str(binary)], capture_output=True, text=True, check=True)
    (output / "stdout.log").write_text(run.stdout)
    (output / "stderr.log").write_text(run.stderr)
    receipt = {
        "schema": "electry-source-snapshot-observation/v1", "revision": revision,
        "defines": defines,
        "original_test_sha256": hashlib.sha256(original).hexdigest(),
        "observer_sha256": helper.sha(driver), "script_sha256": helper.sha(Path(__file__)),
        "build_receipt_sha256": helper.sha(output / "build/build.json"),
        "stdout_sha256": helper.sha(output / "stdout.log"),
        "stderr_sha256": helper.sha(output / "stderr.log"),
        "observations": [line for line in run.stdout.splitlines()
                         if line.startswith(("SNAPSHOT ", "PROBE note ", "PROBE humbucker chord ",
                                             "PROBE stateful two-pass "))],
        "limits": "Original assertions are retained; reported failures require classification. "
                  "This is an independent exact-source observation, not a test waiver or physical fit.",
    }
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
