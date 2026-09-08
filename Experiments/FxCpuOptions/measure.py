#!/usr/bin/env python3
"""Serial FX CPU evaluation. Run while the machine is otherwise idle."""
import argparse
import csv
import hashlib
import json
import os
import platform
import statistics
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2] / "build-fx-options-repro"
VARIANTS = {
    "baseline": "baseline/evaluate",
    "diode_lut": "diode_lut/evaluate",
    "pi_math": "pi_math/evaluate",
    "oversampling": "oversampling/evaluate",
    "stereo": "stereo/evaluate",
    "solver_1e7": "solver/evaluate-1e-7",
    "solver_1e8": "solver/evaluate-1e-8",
    "combined": "combined/evaluate",
}
ORDER = list(VARIANTS)[1:]


def command_output(command):
    return subprocess.check_output(command, text=True).strip()


def metadata():
    return {
        "utc_created": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "processor": command_output(["sysctl", "-n", "machdep.cpu.brand_string"]) if platform.system() == "Darwin" else platform.processor(),
        "compiler": command_output(["c++", "--version"]),
        "executable_sha256": {
            name: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
            for name, path in VARIANTS.items()
        },
        "measurement": "CPU std::clock process time and steady_clock elapsed; stereo frames at requested rate, block128; process and per-block parameters included; construction/prepare/input/copy/reset/checksum excluded.",
        "cpu_limitations": "Process CPU excludes descheduling; frequency, core migration, cache/thermal contention remain possible. Serial bracketed alternating sweeps reduce drift sensitivity.",
    }


def run(out, phase, round_number, position, name, mode, repeats, rate, seconds=0.5):
    tag = f"{phase}-r{round_number}-{position:02d}-{name}-{mode}"
    command = [str(ROOT / VARIANTS[name]), "--benchmark", "--rate", str(rate),
               "--seconds", str(seconds), "--repeats", str(repeats), "--input", mode]
    print(f"START {tag}", flush=True)
    start = time.time()
    with (out / f"{tag}.csv").open("w") as stdout, (out / f"{tag}.stderr").open("w") as stderr:
        completed = subprocess.run(command, stdout=stdout, stderr=stderr)
    record = {"tag": tag, "phase": phase, "round": round_number, "position": position,
              "variant": name, "input": mode, "rate": rate, "seconds": seconds, "command": command,
              "start_epoch": start, "end_epoch": time.time(), "returncode": completed.returncode}
    with (out / "runs.jsonl").open("a") as log:
        log.write(json.dumps(record) + "\n")
    if completed.returncode:
        raise RuntimeError(f"benchmark failed: {tag}; inspect stderr")
    rows = list(csv.DictReader((out / f"{tag}.csv").open()))
    if len(rows) != 12 or any(int(row["frames"]) != round(rate * seconds) for row in rows):
        raise RuntimeError(f"unexpected/incomplete output: {tag}")
    print(f"DONE  {tag} ({time.time() - start:.1f}s)", flush=True)
    return record


def summarise(out):
    records = [json.loads(line) for line in (out / "runs.jsonl").read_text().splitlines()]
    data = {}
    for record in records:
        if record["returncode"]:
            continue
        rows = list(csv.DictReader((out / (record["tag"] + ".csv")).open()))
        data[record["tag"]] = {row["scenario"]: row for row in rows}
    fields = ["phase", "input", "variant", "scenario", "rate", "outer_runs", "baseline_cpu_ns", "candidate_cpu_ns",
              "cpu_saving_percent", "cpu_saving_min_percent", "cpu_saving_max_percent",
              "baseline_elapsed_ns", "candidate_elapsed_ns", "elapsed_saving_percent"]
    summaries = []
    groups = {}
    for record in records:
        if record["variant"] != "baseline" and record["returncode"] == 0:
            key = (record["phase"], record["input"], record["variant"], record["rate"])
            groups.setdefault(key, []).append(record)
    for (phase, mode, variant, rate), candidates in groups.items():
        for scenario in data[candidates[0]["tag"]]:
            cpus, walls, bases_cpu, bases_wall, ratios_cpu, ratios_wall = [], [], [], [], [], []
            for candidate in candidates:
                baselines = [r for r in records if r["variant"] == "baseline" and r["phase"] == phase
                             and r["input"] == mode and r["round"] == candidate["round"] and r["rate"] == rate and r["returncode"] == 0]
                if not baselines:
                    continue
                base_cpu = statistics.mean(float(data[r["tag"]][scenario]["median_cpu_ns_per_frame"]) for r in baselines)
                base_wall = statistics.mean(float(data[r["tag"]][scenario]["median_ns_per_frame"]) for r in baselines)
                current = data[candidate["tag"]][scenario]
                cpu = float(current["median_cpu_ns_per_frame"])
                wall = float(current["median_ns_per_frame"])
                cpus.append(cpu); walls.append(wall); bases_cpu.append(base_cpu); bases_wall.append(base_wall)
                ratios_cpu.append(100 * (1 - cpu / base_cpu)); ratios_wall.append(100 * (1 - wall / base_wall))
            if not cpus:
                continue
            summaries.append(dict(zip(fields, [phase, mode, variant, scenario, rate, len(cpus),
                statistics.median(bases_cpu), statistics.median(cpus), statistics.median(ratios_cpu),
                min(ratios_cpu), max(ratios_cpu), statistics.median(bases_wall),
                statistics.median(walls), statistics.median(ratios_wall)])))
    with (out / "summary.csv").open("w") as file:
        writer = csv.DictWriter(file, fieldnames=fields, lineterminator="\n")
        writer.writeheader(); writer.writerows(summaries)
    return summaries


def main():
    global ROOT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("phase", choices=["quick", "robust", "mono", "guitar", "summarise"])
    parser.add_argument("--rate", type=int, choices=[44100,48000,96000], default=48000)
    parser.add_argument("--build", type=Path, default=ROOT)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    ROOT = args.build.resolve()
    out = args.output.resolve() if args.output else ROOT / "central-timing"
    if args.phase == "summarise":
        summarise(out); return
    out.mkdir(parents=True, exist_ok=True)
    metadata_path = out / f"metadata-{args.phase}.json"
    if metadata_path.exists():
        parser.error("phase already started in output directory; choose a new output path")
    metadata_path.write_text(json.dumps(metadata(), indent=2) + "\n")
    if args.phase in ("quick", "robust"):
        rounds = 1 if args.phase == "quick" else 4
        repeats = 3 if args.phase == "quick" else 5
        for round_number in range(1, rounds + 1):
            order = ORDER if round_number % 2 else list(reversed(ORDER))
            for position, name in enumerate(["baseline", *order, "baseline"]):
                run(out, args.phase, round_number, position, name, "stereo", repeats, args.rate)
            summarise(out)
    elif args.phase == "mono":
        for position, name in enumerate(["baseline", "stereo", "stereo", "baseline"]):
            run(out, "mono", 1, position, name, "mono", 5, args.rate)
        summarise(out)
    else:
        for position, name in enumerate(["baseline", "combined", "combined", "baseline"]):
            run(out, "guitar", 1, position, name, "guitar", 3, args.rate, 2.0)
        summarise(out)
    print(f"COMPLETE {args.phase}; summary: {out / 'summary.csv'}", flush=True)


if __name__ == "__main__":
    main()
