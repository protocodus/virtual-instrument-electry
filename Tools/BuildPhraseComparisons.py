#!/usr/bin/env python3
"""Actual isolated-string DI phrases beside annotated production-model scores.

The real phrase and model are different performances/instruments. Source MIDI
timing is retained after one constant first-onset offset; no note-by-note timing
fit. Model A/B use identical external score bytes, seed, controls and crop.
MIDI velocities are annotation dynamics, not measured pick forces. No EQ,
resampling, pitch shift, time warp or per-window gain. See source limits.
"""
import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import shutil

_spec = importlib.util.spec_from_file_location("dry_packager", Path(__file__).with_name("BuildDryReferenceComparisons.py"))
dry = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(dry)


def score_events(row, rate, destination):
    events = row["events"]
    if not events or events[0]["type"] != "on":
        raise ValueError("Phrase must start with a selected note-on")
    start = events[0]["time_s"]
    tuples = []
    for event in events:
        if event["type"] not in ("on", "off"):
            raise ValueError("Unknown phrase event")
        note = event["note"]
        if type(note) is not int or not 28 <= note <= 86:
            raise ValueError("Phrase sounding MIDI outside model range")
        if "crop_start_frame" in row and not (row["crop_start_frame"] / rate <= event["time_s"]
                < (row["crop_start_frame"] + row["crop_frames"]) / rate):
            raise ValueError("Selected source event is outside the frozen phrase crop")
        frame = round((event["time_s"] - start) * rate) + rate // 4
        velocity = event["velocity"] / 127 if event["type"] == "on" else 0
        if not 0 <= velocity <= 1 or (event["type"] == "on" and velocity == 0):
            raise ValueError("Invalid source annotation velocity")
        if frame < 0 or (tuples and frame < tuples[-1][0]):
            raise ValueError("Unsorted phrase events")
        tuples.append((frame, 0 if event["type"] == "on" else 1, note + 12, velocity))
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text("# frame kind written-MIDI value; source annotation dynamics, not measured forces\n" +
                           "".join(f"{f}\t{k}\t{n}\t{v:.12g}\n" for f, k, n, v in tuples))
    return tuples


def validation_row(row):
    # The generic crop/provenance validator's note is only a range guard; a
    # phrase has many notes and the frozen source plan is never rewritten.
    return {**row, "sounding_midi": row["events"][0]["note"]}


def package(row, plan_dir, folders, destination, score_path, expected_events, anchors):
    if "A" not in folders or set(folders) - {"A", "B"}:
        raise ValueError("Model tracks must include A and optional B")
    original, rate, reference, onset = dry.validate_reference(validation_row(row), plan_dir)
    clips = {"Reference": reference}
    alignments = {"Reference": {"source_start_frame": row["crop_start_frame"], "source_channel": row["channel"],
                                "oracle_source_onset_frame": row["crop_start_frame"] + onset,
                                "declared_source_onset_frame": row["onset_frame"], "onset_frame_after_crop": onset}}
    receipts = {}
    crop_start = None
    for letter in (x for x in ("A", "B") if x in folders):
        folder = folders[letter]
        current, audio = dry.read_audio(folder / "raw.wav")
        if current != rate or audio.shape[1] != 2 or not dry.np.array_equal(audio[:, 0], audio[:, 1]):
            raise ValueError("Expected same-rate production mono outputs")
        receipt = json.loads((folder / "render.json").read_text())
        expected = [[frame, kind, note, value] for frame, kind, note, value in expected_events]
        if (receipt.get("rate") != rate or receipt.get("frames") != len(audio)
            or receipt.get("physical_string_index") != row["physical_string_index"]
            or receipt.get("initial_solo_key") != 24 + row["physical_string_index"]
            or len(receipt.get("events", [])) != len(expected)
            or any(a[:3] != b[:3] or not math.isclose(a[3], b[3], abs_tol=1e-6, rel_tol=0)
                   for a, b in zip(receipt["events"], expected))):
            raise ValueError("Rendered phrase does not bind the selected source score/string")
        if receipts and {k: v for k, v in receipt.items() if k != "render_seconds"} != {k: v for k, v in next(iter(receipts.values())).items() if k != "render_seconds"}:
            raise ValueError("A/B score/configuration differs")
        receipts[letter] = receipt
        detected = dry.onset_frame(audio[:, 0], rate)
        if letter == "A":
            group = row["alignment_group"]
            nominal_onset = round(row["events"][0]["time_s"] * rate) - row["crop_start_frame"]
            if row["alignment_role"] == "development-anchor":
                if group in anchors:
                    raise ValueError("Duplicate development alignment anchor")
                anchors[group] = {"latency_frames": detected - rate // 4 + nominal_onset - onset,
                                  "reference_id": row["id"], "rate": rate,
                                  "midi_to_audio_offset_s": row["midi_to_audio_offset_s"]}
            elif row["alignment_role"] not in ("repeat-check", "transfer-check"):
                raise ValueError("Unknown phrase alignment role")
            if group not in anchors or anchors[group]["rate"] != rate or anchors[group]["midi_to_audio_offset_s"] != row["midi_to_audio_offset_s"]:
                raise ValueError("Phrase check needs an earlier same-rate frozen development anchor")
            crop_start = rate // 4 + anchors[group]["latency_frames"] - nominal_onset
        if crop_start < 0 or crop_start + len(reference) > len(audio):
            raise ValueError("Insufficient shared model crop padding")
        clips[letter] = audio[crop_start:crop_start + len(reference), 0]
        alignments[letter] = {"raw_onset_frame": detected, "crop_start_frame": crop_start,
                              "onset_frame_after_crop": detected - crop_start,
                              "model_crop_basis": "development A latency shared across group and A/B; later events unchanged"}
    facts, headroom = dry.match_and_write(clips, rate, destination)
    for name, detail in facts.items():
        raw = original if name == "Reference" else folders[name] / "raw.wav"
        detail.update(raw_path=str(raw), raw_sha256=dry.sha(raw), alignment=alignments[name])
    result = {"id": row["id"], "title": row["title"], "rate": rate, "frames": len(reference),
              "onset_frame": onset, "tracks": facts, "reference": row, "model_render": receipts,
              "reference_plan_directory": str(plan_dir.resolve()),
              "score_path": str(score_path), "score_sha256": dry.sha(score_path), "shared_headroom_gain": headroom,
              "alignment_anchor": anchors[row["alignment_group"]],
              "source_onset_prediction": {"expected_frame_after_crop": nominal_onset,
                                          "measured_frame_after_crop": onset,
                                          "residual_frames": onset - nominal_onset,
                                          "model_A_minus_real_frames": alignments["A"]["onset_frame_after_crop"] - onset}}
    (destination / "measurements.json").write_text(json.dumps(result, indent=2) + "\n")
    (destination / "attribution.txt").write_text(row["source"]["attribution"] + "\n" + row["source"]["url"] + "\n" + row["source"]["license"] + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, required=True)
    parser.add_argument("--base", required=True)
    parser.add_argument("--candidate")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    plan_path = args.plan.resolve()
    plan_bytes = plan_path.read_bytes()
    plan_digest = hashlib.sha256(plan_bytes).hexdigest()
    plan = json.loads(plan_bytes)
    if plan.get("schema") != "electry-dry-reference-phrase-plan/v1" or not plan.get("references"):
        raise ValueError("Unknown/empty phrase plan")
    ids = [row["id"] for row in plan["references"]]
    if len(ids) != len(set(ids)):
        raise ValueError("Duplicate phrase id")
    revisions = {"A": dry._helper.run(["git", "rev-parse", args.base + "^{commit}"], repo).decode().strip()}
    if args.candidate:
        revisions["B"] = dry._helper.run(["git", "rev-parse", args.candidate + "^{commit}"], repo).decode().strip()
    driver = repo / "Tools/RenderPerformanceComparison.cpp"
    binaries = {letter: dry._helper.build(repo, output / "builds/performance" / letter, revision, [], driver)
                for letter, revision in revisions.items()}
    results, anchors = [], {}
    for row in plan["references"]:
        _, rate, clip, _ = dry.validate_reference(validation_row(row), plan_path.parent)
        midi = Path(row["midi_path"])
        if not midi.is_absolute():
            midi = plan_path.parent / midi
        if dry.sha(midi) != row["midi_sha256"]:
            raise ValueError("Source MIDI hash mismatch")
        score_path = output / "phrase-scores" / (row["id"] + ".tsv")
        events = score_events(row, rate, score_path)
        frames = len(clip) + rate
        if events[-1][0] >= frames:
            raise ValueError("Phrase score exceeds render length")
        folders = {letter: output / "raw-phrases" / row["id"] / letter for letter in binaries}
        for letter, binary in binaries.items():
            print(dry._helper.run([str(binary), str(folders[letter]), "score:" + str(score_path), str(rate), str(frames), str(row["physical_string_index"])]).decode().strip(), flush=True)
        results.append(package(row, plan_path.parent, folders, output / "phrases" / row["id"], score_path, events, anchors))
    if dry.sha(plan_path) != plan_digest:
        raise ValueError("Phrase plan changed while packaging; rerun from the frozen input")
    receipt = {"schema": "electry-dry-phrase-comparisons/v1", "revisions": revisions,
               "plan_sha256": plan_digest, "builder_sha256": dry.sha(Path(__file__)),
               "packager_sha256": dry.sha(Path(dry.__file__)), "driver_sha256": dry.sha(driver),
               "build_helper_sha256": dry.sha(Path(dry._helper.__file__)), "comparisons": results,
               "builds": {k: json.loads((output / "builds/performance" / k / "build.json").read_text()) for k in binaries}}
    dest = output / "phrases"
    (dest / "measurements.json").write_text(json.dumps(receipt, indent=2) + "\n")
    display = [{"id": r["id"], "title": r["title"], "description": r["reference"].get("description", ""),
                "tracks": list(r["tracks"]), "limits": r["reference"]["source"]["limits"] + [r["reference"]["source"]["attribution"]]} for r in results]
    template = (repo / "Tools/DryReferenceListening.html").read_text()
    (dest / "index.html").write_text(template.replace("/*MANIFEST*/[]", json.dumps(display).replace("</", "<\\/")))
    (dest / "key.md").write_text("# Real phrase key\n\nReference: actual separately performed DI recording.\n\nA: shipping " + revisions["A"] + ".\n\nB: " + revisions.get("B", "not present") + ".\n\nOriginal MIDI timing and annotation velocities, one constant initial onset alignment only. A/B use identical events and crop. Unknown construction and human force are not matched.\n")
    shutil.copyfile(plan_path, output / "phrase-plan.json")
    shutil.copyfile(repo / "Tools/DryReviewLanding.html", output / "index.html")
    print("Real phrase room:", dest)


if __name__ == "__main__":
    main()
