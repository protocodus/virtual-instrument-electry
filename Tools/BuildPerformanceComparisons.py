#!/usr/bin/env python3
"""Render fixed retrigger/one-string scores through immutable production paths.

A is shipping; optional B is a candidate. Unlike the separate real-reference
room, model events, silence, seed, controls and sample offsets match exactly.
No onset realignment, EQ, editing or per-window gains. One whole-file RMS gain
per track plus shared .94 sample-peak headroom; identical outputs are reported.
"""
import argparse
import importlib.util
import json
from pathlib import Path

import numpy as np

_spec = importlib.util.spec_from_file_location("dry_packager", Path(__file__).with_name("BuildDryReferenceComparisons.py"))
dry = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(dry)

PROBES = {
    "aba-ownership": ("A → B → A on one string", "E1, G1, E1 overlap. The old E1 release at 0.65 s arrives while the new E1 should still ring to 0.95 s."),
    "repeat-8hz": ("Same note · 8 Hz", "24 E1 attacks. Gaps before the next pick cycle through 0, 2, 10 and 30 ms."),
    "repeat-12hz": ("Same note · 12 Hz", "24 E1 attacks. The same four gaps test faster contact and note termination."),
    "overlapping-string": ("Overlapping notes · one string", "E1, G1, E1 and B1 change every 125 ms while earlier notes remain held for 400 ms."),
    "alternating-string": ("Alternating frets · one string", "E1, E2, G1 and B2 alternate at 150 ms intervals on the same lowest string."),
    "rapid-palm": ("Ringing / palm · rapid changes", "24 E1 attacks alternate ringing and palm articulation, with a full bridge-hand pressure landing every fourth hit.")}


def package(folders, destination, probe):
    clips, receipts = {}, {}
    rate = None
    for letter, folder in folders.items():
        current, audio = dry.read_audio(folder / "raw.wav")
        receipt = json.loads((folder / "render.json").read_text())
        if audio.shape[1] != 2 or not np.array_equal(audio[:, 0], audio[:, 1]):
            raise ValueError("Expected production mono duplicated onto two output channels")
        if rate is not None and rate != current:
            raise ValueError("A/B rate differs")
        if receipts and {k: v for k, v in receipt.items() if k != "render_seconds"} != {k: v for k, v in next(iter(receipts.values())).items() if k != "render_seconds"}:
            raise ValueError("A/B event/configuration differs")
        rate = current; clips[letter] = audio[:, 0]; receipts[letter] = receipt
    facts, headroom = dry.match_and_write(clips, rate, destination)
    for letter, detail in facts.items():
        detail["raw_path"] = str(folders[letter] / "raw.wav")
        detail["raw_sha256"] = dry.sha(folders[letter] / "raw.wav")
    title, description = PROBES[probe]
    result = {"id": probe, "title": title, "description": description,
              "rate": rate, "frames": len(clips["A"]), "tracks": facts, "model_render": receipts,
              "shared_headroom_gain": headroom, "identical_raw_audio": "B" in clips and np.array_equal(clips["A"], clips["B"])}
    (destination / "measurements.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--candidate")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build-only", action="store_true")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    revisions = {"A": dry._helper.run(["git", "rev-parse", args.base + "^{commit}"], repo).decode().strip()}
    if args.candidate:
        revisions["B"] = dry._helper.run(["git", "rev-parse", args.candidate + "^{commit}"], repo).decode().strip()
    driver = repo / "Tools/RenderPerformanceComparison.cpp"
    binaries = {letter: dry._helper.build(repo, output / "builds" / "performance" / letter, revision, [], driver)
                for letter, revision in revisions.items()}
    if args.build_only:
        print(json.dumps({k: str(v) for k, v in binaries.items()}, indent=2)); return
    results = []
    for probe in PROBES:
        folders = {letter: output / "raw-performance" / probe / letter for letter in binaries}
        for letter, exe in binaries.items():
            print(dry._helper.run([str(exe), str(folders[letter]), probe, "48000"]).decode().strip(), flush=True)
        results.append(package(folders, output / "performance" / probe, probe))
    receipt = {"schema": "electry-performance-comparisons/v1", "revisions": revisions,
               "driver_sha256": dry.sha(driver), "builder_sha256": dry.sha(Path(__file__)),
               "packager_sha256": dry.sha(Path(dry.__file__)), "build_helper_sha256": dry.sha(Path(dry._helper.__file__)),
               "comparisons": results,
               "builds": {k: json.loads((output / "builds/performance" / k / "build.json").read_text()) for k in binaries}}
    dest = output / "performance"
    (dest / "measurements.json").write_text(json.dumps(receipt, indent=2) + "\n")
    display = [{"id": r["id"], "title": r["title"], "description": r["description"], "tracks": list(r["tracks"]),
                "limits": ["All notes use written MIDI; forced lowest physical E1 string. Same score, seed, sample rate, 64-frame block cap and controls across model letters.",
                           "No real recording is substituted for A. See the separate real-reference room for actual recorded guitars.",
                           "One constant whole-file RMS gain per track; shared sample-peak headroom. No onset realignment or editing."]} for r in results]
    template = (repo / "Tools/PerformanceListening.html").read_text()
    (dest / "index.html").write_text(template.replace("/*MANIFEST*/[]", json.dumps(display).replace("</", "<\\/")))
    (dest / "key.md").write_text("# Performance comparison key\n\nA: shipping " + revisions["A"] + ".\n\nB: " + revisions.get("B", "not present yet") + ".\n\nIdentical events and controls; constant whole-file RMS matching. No time realignment.\n")
    print("Performance room:", dest)


if __name__ == "__main__":
    main()
