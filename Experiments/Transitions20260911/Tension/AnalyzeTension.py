#!/usr/bin/env python3
"""Summarize an isolated tension-continuity run using only the standard library."""
from argparse import ArgumentParser
from array import array
import csv
import json
import math
from pathlib import Path
import sys


def read_audio(path):
    raw = path.read_bytes()
    if not raw or len(raw) % 4:
        raise ValueError(f"Invalid float32 mono file: {path}")
    values = array("f")
    values.frombytes(raw)
    if sys.byteorder != "little":
        values.byteswap()
    if not all(math.isfinite(value) for value in values):
        raise ValueError(f"Non-finite audio: {path}")
    return raw, values


def rms(values):
    return math.sqrt(sum(float(value) * value for value in values) / len(values))


def read_boundaries(path):
    with path.open(newline="") as source:
        rows = list(csv.DictReader(source))
    if len(rows) != 48:
        raise ValueError(f"Expected 48 release-boundary cases: {path}")
    for row in rows:
        if not all(math.isfinite(float(value)) for value in row.values()):
            raise ValueError(f"Non-finite boundary result: {path}")
    return rows


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("run_directory", type=Path)
    args = parser.parse_args()
    root = args.run_directory
    result = {}
    for scenario, name in enumerate(("open_sustain", "stationary_palm",
                                      "held_repicks", "sibling_chord")):
        baseline_raw, baseline = read_audio(root / "baseline-audio" / f"{scenario}-256.f32")
        candidate_raw, candidate = read_audio(root / "candidate-audio" / f"{scenario}-256.f32")
        alternate_raw, alternate = read_audio(root / "candidate-audio" / f"{scenario}-17.f32")
        if len(baseline) != len(candidate) or len(candidate) != len(alternate):
            raise ValueError(f"Frame mismatch: {name}")
        difference = [a - b for a, b in zip(baseline, candidate)]
        baseline_rms = rms(baseline)
        if baseline_rms <= 0:
            raise ValueError(f"Silent baseline: {name}")
        result[name] = {
            "same_pcm": baseline_raw == candidate_raw,
            "callback_same_pcm": candidate_raw == alternate_raw,
            # An exact null is recorded at the display floor, not -infinity.
            "delta_db_relative_to_before": 20 * math.log10(max(rms(difference) / baseline_rms, 1e-15)),
            "candidate_peak": max(map(abs, candidate)),
            "candidate_rms": rms(candidate),
        }
    before = read_boundaries(root / "before.csv")
    after = read_boundaries(root / "after.csv")
    case_fields = ("rate", "note", "style", "hold_ms")
    if [tuple(row[key] for key in case_fields) for row in before] != [
            tuple(row[key] for key in case_fields) for row in after]:
        raise ValueError("Boundary case order or settings changed")
    result["release_boundary"] = {
        "cases": len(before),
        "before_target_max_cents": max(abs(float(row["target_jump_c"])) for row in before),
        "before_current_max_cents": max(abs(float(row["current_jump_c"])) for row in before),
        "after_target_max_cents": max(abs(float(row["target_jump_c"])) for row in after),
        "after_current_max_cents": max(abs(float(row["current_jump_c"])) for row in after),
    }
    encoded = json.dumps(result, indent=2) + "\n"
    (root / "results.json").write_text(encoded)
    print(encoded, end="")


if __name__ == "__main__":
    main()
