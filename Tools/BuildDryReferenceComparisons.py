#!/usr/bin/env python3
"""Package actual real recordings beside immutable production Electry renders.

This is a descriptive audition, not an exact-eight calibration or a blinded
study. Reference is a separately played instrument. A is an immutable shipping
snapshot; optional B uses identical controls/events and a candidate snapshot.
Input plans freeze original hashes, channel, integer crop and source limits.
No source download, EQ, denoising, time stretch, pitch shift, envelope fit or
per-window gain is performed. Source and model retain their native sample rate.
The common onset oracle is first crossing of 25% of the first second's peak
2 ms centred RMS. One integer crop aligns A's crossing to the real clip; the
same crop offset applies to B so candidate attack/latency changes remain audible.
Raw files and offsets remain available. Whole-file RMS uses one constant
gain per track to -24 dBFS, followed by one shared peak-headroom factor <= 1.
"""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import shutil

import numpy as np
from scipy.io import wavfile

_helper_path = Path(__file__).with_name("BuildRealismComparisons.py")
_spec = importlib.util.spec_from_file_location("realism_build", _helper_path)
_helper = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_helper)
sha = _helper.sha


def read_audio(path):
    rate, samples = wavfile.read(path)
    if samples.ndim == 1:
        samples = samples[:, None]
    if samples.ndim != 2 or not len(samples):
        raise ValueError("Audio must contain frames and channels")
    if samples.dtype.kind == "i":
        samples = samples.astype(np.float64) / (2 ** (samples.dtype.itemsize * 8 - 1))
    elif samples.dtype == np.uint8:
        samples = (samples.astype(np.float64) - 128) / 128
    elif samples.dtype.kind == "f":
        samples = samples.astype(np.float64)
    else:
        raise ValueError("Unsupported WAV sample format")
    if not np.isfinite(samples).all():
        raise ValueError("Nonfinite audio")
    return int(rate), samples


def onset_frame(samples, rate):
    """One explicit native-rate oracle, applied identically to each track."""
    x = np.asarray(samples, dtype=np.float64)
    if x.ndim != 1 or not len(x) or not np.isfinite(x).all():
        raise ValueError("Onset input must be finite nonempty mono")
    width = max(1, int(round(rate * .002)))
    padded = np.pad(x * x, (width // 2, width - 1 - width // 2))
    summed = np.r_[0., np.cumsum(padded)]
    envelope = np.sqrt(np.maximum(0., (summed[width:] - summed[:-width]) / width))
    peak = float(np.max(envelope[:min(len(envelope), rate)]))
    if peak <= 1e-10:
        raise ValueError("Silent onset search")
    return int(np.flatnonzero(envelope >= .25 * peak)[0])


def validate_reference(row, plan_dir):
    required = {"id", "title", "path", "sha256", "channel", "crop_start_frame", "crop_frames",
                "onset_frame", "sounding_midi", "physical_string_index", "pickup", "style", "source"}
    if not required <= row.keys():
        raise ValueError("Reference plan lacks required fields: " + str(sorted(required - row.keys())))
    if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", row["id"]):
        raise ValueError("Unsafe reference id")
    source = row["source"]
    if not isinstance(source, dict) or any(not source.get(k) for k in ("url", "license", "attribution", "role", "limits")):
        raise ValueError("Source URL, license, attribution, role and explicit limits are required")
    if (not all(isinstance(source[k], str) for k in ("url", "license", "attribution", "role"))
        or not isinstance(source["limits"], list) or not all(isinstance(x, str) and x for x in source["limits"])):
        raise ValueError("Source provenance must contain named strings and a nonempty limits list")
    if source["license"].strip().lower() in {"unknown", "unverified", "unspecified", "n/a"}:
        raise ValueError("An identified source license is required; unknown provenance is not inferred")
    if source["role"] not in ("development", "held-out"):
        raise ValueError("Reference role must be explicit development or held-out")
    path = Path(row["path"])
    if not path.is_absolute():
        path = plan_dir / path
    path = path.resolve()
    if sha(path) != row["sha256"]:
        raise ValueError("Original reference SHA-256 mismatch: " + row["id"])
    rate, audio = read_audio(path)
    channel, start, count, declared_onset = (row[k] for k in ("channel", "crop_start_frame", "crop_frames", "onset_frame"))
    if any(type(n) is not int for n in (channel, start, count, declared_onset)):
        raise ValueError("Channel and crop positions must be integer frames")
    if not 0 <= channel < audio.shape[1] or start < 0 or count < 1 or start + count > len(audio):
        raise ValueError("Reference crop/channel outside original WAV")
    if not start <= declared_onset < start + count:
        raise ValueError("Declared onset outside original crop")
    if not 44100 <= rate <= 192000 or count > 60 * rate:
        raise ValueError("Unsupported native rate or crop duration")
    clip = audio[start:start + count, channel].copy()
    onset = onset_frame(clip, rate)
    if onset < int(.01 * rate) or count - onset < int(.1 * rate):
        raise ValueError("Need >=10 ms actual pre-onset padding and >=100 ms post-onset")
    if abs(onset + start - declared_onset) > .1 * rate:
        raise ValueError("Declared and common-oracle onset differ by over 100 ms")
    if row["pickup"] not in ("bridge", "neck", "both") or row["style"] not in ("sustain", "palm"):
        raise ValueError("Unknown pickup/style")
    if type(row["sounding_midi"]) is not int or not 28 <= row["sounding_midi"] <= 86:
        raise ValueError("Sounding MIDI outside Electry range")
    if type(row["physical_string_index"]) is not int or not 0 <= row["physical_string_index"] <= 7:
        raise ValueError("Physical string index outside Electry range")
    return path, rate, clip, onset


def render_configuration(row, rate, count):
    model = row.get("model", {})
    config = {"rate": rate, "frames": count + rate, "note_on_frame": rate // 4,
              "sounding_midi": row["sounding_midi"], "physical_string_index": row["physical_string_index"],
              "pickup": row["pickup"], "style": row["style"],
              "gauge": model.get("gauge", 0), "scale": model.get("scale", 0),
              "velocity": model.get("velocity", .85), "palm_tightness": model.get("palm_tightness", .55)}
    for key in ("gauge", "scale", "velocity", "palm_tightness"):
        value = config[key]
        if not isinstance(value, (int, float)) or isinstance(value, bool) or not math.isfinite(value) or not 0 <= value <= 1:
            raise ValueError("Model controls must be finite unit coordinates")
    if config["velocity"] == 0:
        raise ValueError("Zero velocity does not produce an audition")
    return config


def render(renderer, destination, config):
    fields = ("rate", "frames", "note_on_frame", "sounding_midi", "physical_string_index",
              "pickup", "style", "gauge", "scale", "velocity", "palm_tightness")
    print(_helper.run([str(renderer), str(destination), *(str(config[k]) for k in fields)]).decode().strip(), flush=True)
    receipt = json.loads((destination / "render.json").read_text())
    return receipt


def match_and_write(clips, rate, destination):
    """Single RMS gains plus shared peak limit; never modify a track's shape."""
    shapes = {np.asarray(x).shape for x in clips.values()}
    if len(shapes) != 1 or len(next(iter(shapes))) != 1:
        raise ValueError("All listening tracks must be equal-length mono")
    rms = {}
    for name, x in clips.items():
        if not len(x) or not np.isfinite(x).all():
            raise ValueError("Nonfinite/empty listening track")
        rms[name] = float(np.sqrt(np.mean(x * x)))
        if rms[name] <= 1e-12:
            raise ValueError("Silent listening track")
    initial = {name: 10 ** (-24 / 20) / value for name, value in rms.items()}
    peak = max(float(np.max(np.abs(x))) * initial[name] for name, x in clips.items())
    headroom = min(1., .94 / peak)
    destination.mkdir(parents=True, exist_ok=True)
    facts = {}
    for name, x in clips.items():
        gain = initial[name] * headroom
        path = destination / (name + ".wav")
        _helper.write_pcm24(path, rate, (x * gain)[:, None])
        facts[name] = {"path": destination.name + "/" + path.name, "sha256": sha(path),
                       "raw_rms": rms[name], "trim_db": 20 * math.log10(gain),
                       "peak": float(np.max(np.abs(x))) * gain,
                       "matched_rms_dbfs": 20 * math.log10(rms[name] * gain)}
    return facts, headroom


def package_reference(row, plan_dir, render_dirs, destination):
    path, rate, reference, onset = validate_reference(row, plan_dir)
    clips = {"Reference": reference}
    alignment = {"Reference": {"source_start_frame": row["crop_start_frame"],
                               "source_channel": row["channel"], "declared_source_onset_frame": row["onset_frame"],
                               "oracle_source_onset_frame": row["crop_start_frame"] + onset,
                               "onset_frame_after_crop": onset}}
    receipts = {}
    if "A" not in render_dirs or set(render_dirs) - {"A", "B"}:
        raise ValueError("Model tracks must include A and optional B")
    model_start = None
    for letter in (x for x in ("A", "B") if x in render_dirs):
        folder = render_dirs[letter]
        model_rate, audio = read_audio(folder / "raw.wav")
        if model_rate != rate or audio.shape[1] != 1:
            raise ValueError("Model must be mono at the original reference rate")
        receipt = json.loads((folder / "render.json").read_text())
        expected = render_configuration(row, rate, len(reference))
        for key in ("rate", "frames", "sounding_midi", "physical_string_index", "pickup", "style"):
            if receipt.get(key) != expected[key]:
                raise ValueError("Render configuration differs from reference plan: " + key)
        for key in ("gauge", "scale", "velocity", "palm_tightness"):
            if not isinstance(receipt.get(key), (int, float)) or not math.isclose(receipt[key], expected[key], rel_tol=0, abs_tol=1e-6):
                raise ValueError("Render control differs from reference plan: " + key)
        if (receipt.get("written_midi") != row["sounding_midi"] + 12
            or receipt.get("fx_enabled") is not False or receipt.get("output_mode") != "mono"
            or receipt.get("frames") != len(audio)
            or receipt.get("events", [])[-1:] != [[expected["note_on_frame"], "note-on-written", row["sounding_midi"] + 12, receipt["velocity"]]]):
            raise ValueError("Model event, pitch convention or audio metadata do not bind this render")
        if receipts:
            first = next(iter(receipts.values()))
            if {k: v for k, v in receipt.items() if k != "render_seconds"} != {k: v for k, v in first.items() if k != "render_seconds"}:
                raise ValueError("A/B render events or configuration differ")
        receipts[letter] = receipt
        detected = onset_frame(audio[:, 0], rate)
        if letter == "A":
            model_start = detected - onset
        start = model_start
        if start < 0 or start + len(reference) > len(audio):
            raise ValueError("Model has insufficient onset padding or tail for the fixed crop")
        clips[letter] = audio[start:start + len(reference), 0].copy()
        alignment[letter] = {"raw_onset_frame": detected, "crop_start_frame": start,
                             "onset_frame_after_crop": detected - start,
                             "model_crop_basis": "A onset; identical model crop offset for A and B",
                             "note_on_frame": receipt["events"][-1][0]}
    facts, headroom = match_and_write(clips, rate, destination)
    for name, detail in facts.items():
        detail["alignment"] = alignment[name]
        raw = path if name == "Reference" else render_dirs[name] / "raw.wav"
        detail["raw_path"] = str(raw)
        detail["raw_sha256"] = sha(raw)
    result = {"id": row["id"], "title": row["title"], "rate": rate, "frames": len(reference),
              "onset_frame": onset, "shared_headroom_gain": headroom, "reference": row,
              "tracks": facts, "model_render": receipts, "limits": [
                  "Reference and models are independently performed; identical pluck force, release and guitar construction are not established.",
                  "One A-derived integer onset crop is shared by model A/B, retaining candidate timing changes; one constant gain per track only, no EQ, denoising, time/pitch warp or per-window fitting.",
                  "Source role and construction uncertainties are in reference.source; no calibrated quality score or promotion follows."]}
    (destination / "measurements.json").write_text(json.dumps(result, indent=2) + "\n")
    (destination / "attribution.txt").write_text(row["source"]["attribution"] + "\n" + row["source"]["url"] + "\n" + row["source"]["license"] + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path)
    parser.add_argument("--base", required=True)
    parser.add_argument("--candidate")
    parser.add_argument("--candidate-define", action="append", default=[])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build-only", action="store_true")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=True)
    driver = repo / "Tools/RenderDryReferenceComparison.cpp"
    revisions = {"A": _helper.run(["git", "rev-parse", args.base + "^{commit}"], repo).decode().strip()}
    defines = {}
    for item in args.candidate_define:
        name, value = item.split("=", 1)
        if not re.fullmatch(r"ELECTRY_[A-Z0-9_]+", name) or value not in ("0", "1"):
            raise ValueError("Candidate defines must be ELECTRY_NAME=0 or =1")
        defines[name] = int(value)
    if defines and not args.candidate:
        raise ValueError("Candidate defines require --candidate")
    if args.candidate:
        revisions["B"] = _helper.run(["git", "rev-parse", args.candidate + "^{commit}"], repo).decode().strip()
    binaries = {letter: _helper.build(repo, output / "builds" / letter, revision, [], driver,
                                     defines if letter == "B" else {}) for letter, revision in revisions.items()}
    if args.build_only:
        print(json.dumps({k: str(v) for k, v in binaries.items()}, indent=2)); return
    if args.plan is None:
        parser.error("--plan is required unless --build-only")
    plan_path = args.plan.resolve()
    plan_bytes = plan_path.read_bytes()
    plan_digest = hashlib.sha256(plan_bytes).hexdigest()
    plan = json.loads(plan_bytes)
    if plan.get("schema") != "electry-dry-reference-plan/v1" or not plan.get("references"):
        raise ValueError("Unknown/empty reference plan")
    ids = [row["id"] for row in plan["references"]]
    if len(ids) != len(set(ids)):
        raise ValueError("Duplicate reference id")
    results = []
    for row in plan["references"]:
        _, rate, clip, _ = validate_reference(row, plan_path.parent)
        config = render_configuration(row, rate, len(clip))
        folders = {letter: output / "raw" / row["id"] / letter for letter in binaries}
        for letter, binary in binaries.items():
            render(binary, folders[letter], config)
        results.append(package_reference(row, plan_path.parent, folders, output / "listen" / row["id"]))
    if sha(plan_path) != plan_digest:
        raise ValueError("Reference plan changed while packaging; rerun from the frozen input")
    receipt = {"schema": "electry-dry-reference-comparisons/v1", "base": revisions["A"],
               "candidate": revisions.get("B"), "candidate_defines": defines,
               "plan_sha256": plan_digest, "builder_sha256": sha(Path(__file__)),
               "build_helper_sha256": sha(_helper_path), "driver_sha256": sha(driver),
               "comparisons": results,
               "builds": {k: json.loads((output / "builds" / k / "build.json").read_text()) for k in binaries}}
    listen = output / "listen"
    (listen / "measurements.json").write_text(json.dumps(receipt, indent=2) + "\n")
    shutil.copyfile(plan_path, output / "reference-plan.json")
    display = [{"id": r["id"], "title": r["title"], "description": r["reference"].get("description", ""),
                "tracks": list(r["tracks"]), "limits": r["reference"]["source"]["limits"] + [r["reference"]["source"]["attribution"]]} for r in results]
    template = (repo / "Tools/DryReferenceListening.html").read_text()
    (listen / "index.html").write_text(template.replace("/*MANIFEST*/[]", json.dumps(display).replace("</", "<\\/")))
    key = ["# Dry-reference comparison key", "", "Reference: original separately performed real recording.",
           "A: unchanged shipping snapshot " + revisions["A"] + ".",
           "B: " + revisions.get("B", "not present") + "; defines: " + json.dumps(defines) + ".", "",
           "All model letters share events/controls. Real and model performances, construction and force differ.",
           "Whole-file RMS matched with one constant gain each plus common sample-peak headroom; one A-derived integer onset crop is shared by A/B, preserving candidate timing changes.", ""]
    for r in results:
        key.append("- " + r["id"] + ": " + "; ".join(k + f" {v['trim_db']:+.3f} dB" for k, v in r["tracks"].items()))
    (listen / "key.md").write_text("\n".join(key) + "\n")
    print("Listening files:", listen)


if __name__ == "__main__":
    main()
