#!/usr/bin/env python3
"""Compare matched RenderRealism.cpp runs; requires Python 3 and NumPy.

Example:
  python3 Experiments/Realism20260908/AnalyzeRealism.py \
      build-realism-20260908/baseline build-realism-20260908/candidate \
      build-realism-20260908/comparison

These are signal measurements, never perceptual quality or realism scores.
Both runs must contain equivalent musical/control manifests. Input float WAVs
and measurements remain unnormalised. Separate PCM listening copies use one
constant gain per complete file to match paired RMS; no dynamics processing.
"""

import argparse
import csv
import hashlib
import html
import json
import math
from pathlib import Path
import struct
import wave

import numpy as np


def read_float_wav(path, sample_rate, expected_frames):
    data = path.read_bytes()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"{path}: not RIFF WAVE")
    position = 12
    format_chunk = None
    payload = None
    while position + 8 <= len(data):
        kind, count = struct.unpack_from("<4sI", data, position)
        position += 8
        if position + count > len(data):
            raise ValueError(f"{path}: truncated chunk")
        chunk = data[position : position + count]
        if kind == b"fmt ":
            format_chunk = struct.unpack_from("<HHIIHH", chunk)
        elif kind == b"data":
            payload = chunk
        position += count + (count & 1)
    if format_chunk != (3, 1, sample_rate, sample_rate * 4, 4, 32):
        raise ValueError(f"{path}: expected mono float32 at {sample_rate} Hz")
    if payload is None:
        raise ValueError(f"{path}: no samples")
    samples = np.frombuffer(payload, dtype="<f4").astype(np.float64)
    if samples.size != expected_frames or not np.isfinite(samples).all():
        raise ValueError(f"{path}: invalid frame count or non-finite samples")
    return samples, hashlib.sha256(data).hexdigest()


def db(value):
    return float(20.0 * math.log10(max(float(value), 1.0e-12)))


def rms(samples):
    return float(np.sqrt(np.mean(samples * samples))) if samples.size else 0.0


def segment(samples, frame, milliseconds, sample_rate):
    stop = frame + round(milliseconds * sample_rate / 1000)
    return samples[max(0, frame) : max(0, stop)]


def spectral_measures(samples, sample_rate):
    if samples.size < 8 or not np.any(samples):
        return 0.0, 0.0
    spectrum = np.abs(np.fft.rfft(samples * np.hanning(samples.size))) ** 2
    frequencies = np.fft.rfftfreq(samples.size, 1.0 / sample_rate)
    energy = float(spectrum.sum())
    if energy <= 1.0e-30:
        return 0.0, 0.0
    return (float(np.sum(spectrum * frequencies) / energy),
            float(np.sum(spectrum[(frequencies >= 2000)
                                  & (frequencies <= 8000)]) / energy))


def measure(samples, events, sample_rate):
    attacks = []
    for event in events:
        if event["kind"] not in ("note_on", "repick"):
            continue
        frame = event["frame"]
        attack = segment(samples, frame, 20, sample_rate)
        centroid, high_fraction = spectral_measures(attack, sample_rate)
        attacks.append({
            "frame": frame,
            "kind": event["kind"],
            "engine_note": event["engine_note"],
            "velocity": event["value"],
            "rms_0_5_ms_dbfs": db(rms(segment(samples, frame, 5, sample_rate))),
            "rms_0_20_ms_dbfs": db(rms(attack)),
            "peak_0_20_ms_dbfs": db(np.max(np.abs(attack))),
            "spectral_centroid_0_20_ms_hz": centroid,
            "spectral_energy_fraction_2_8_khz_0_20_ms": high_fraction,
        })
    releases = [event for event in events if event["kind"] == "note_off"]
    release = None
    if releases:
        frame = releases[-1]["frame"]
        pre = samples[max(0, frame - round(0.020 * sample_rate)) : frame]
        after = segment(samples, frame, 80, sample_rate)
        late = segment(samples, frame + round(0.20 * sample_rate), 200, sample_rate)
        release = {
            "frame": frame,
            "engine_note": releases[-1]["engine_note"],
            "pre_20_ms_rms_dbfs": db(rms(pre)),
            "post_80_ms_rms_dbfs": db(rms(after)),
            "post_80_ms_peak_dbfs": db(np.max(np.abs(after))),
            "post_200_400_ms_rms_dbfs": db(rms(late)),
            "post_pre_rms_db": db(rms(after)) - db(rms(pre)),
        }
    attack_rms = [event["rms_0_20_ms_dbfs"] for event in attacks]
    return {
        "peak": float(np.max(np.abs(samples))),
        "peak_dbfs": db(np.max(np.abs(samples))),
        "rms_dbfs": db(rms(samples)),
        "dc": float(np.mean(samples)),
        "crest_db": db(np.max(np.abs(samples))) - db(rms(samples)),
        "peak_sample_step": float(np.max(np.abs(np.diff(samples)))),
        "samples_exceeding_unity": int(np.count_nonzero(np.abs(samples) > 1)),
        "mean_attack_20_ms_rms_dbfs": float(np.mean(attack_rms)),
        "attack_20_ms_rms_std_db": float(np.std(attack_rms)),
        "mean_attack_centroid_hz": float(np.mean([
            event["spectral_centroid_0_20_ms_hz"] for event in attacks])),
        "mean_attack_energy_fraction_2_8_khz": float(np.mean([
            event["spectral_energy_fraction_2_8_khz_0_20_ms"] for event in attacks])),
        "attacks": attacks,
        "final_release": release,
    }


def write_audition_pair(directory, name, waveforms, sample_rate):
    """Use one gain per complete file; equalise RMS, then protect pair peaks."""
    levels = {version: rms(samples) for version, samples in waveforms.items()}
    target = min(levels.values())
    gains = {version: target / max(level, 1.0e-15)
             for version, level in levels.items()}
    peak = max(float(np.max(np.abs(samples))) * gains[version]
               for version, samples in waveforms.items())
    safety = min(1.0, 0.98 / max(peak, 1.0e-15))
    gains = {version: gain * safety for version, gain in gains.items()}
    directory.mkdir(exist_ok=True)
    for version, samples in waveforms.items():
        pcm = np.rint(samples * gains[version] * 32767).astype("<i2")
        with wave.open(str(directory / f"{name}-{version}.wav"), "wb") as out:
            out.setnchannels(1)
            out.setsampwidth(2)
            out.setframerate(sample_rate)
            out.writeframes(pcm.tobytes())
    return gains


def write_listening_page(directory, takes):
    cards = []
    for take in takes:
        title = html.escape(take["id"].replace("-", " ").replace("fs1", "F♯1"))
        panels = []
        for tap, label in (("dry", "Dry guitar"), ("modern", "Modern high gain")):
            players = []
            for version, version_label in (("baseline", "Before"), ("candidate", "After")):
                filename = html.escape(f"audio/{take['id']}-{tap}-{version}.wav", quote=True)
                players.append(f'<label>{version_label}<audio controls preload="none" '
                               f'src="{filename}"></audio></label>')
            panels.append(f'<div class="tap"><h3>{label}</h3>{"".join(players)}</div>')
        cards.append(f'<section><h2>{title}</h2><div class="pair">{"".join(panels)}</div></section>')
    page = """<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Electry realism · before and after</title>
<style>
:root{color-scheme:dark;font-family:system-ui,sans-serif;background:#11151b;color:#e4e9ee}
body{max-width:1050px;margin:0 auto;padding:32px 20px}h1{font-size:32px;margin-bottom:12px}
p{line-height:1.6;color:#b9c6d2;max-width:80ch}a{color:#8dc6ff}
section{background:#1b222b;border:1px solid #36424d;border-radius:12px;margin:20px 0;padding:20px}
h2{font-size:19px;margin:0 0 18px}h3{font-size:15px;color:#aec3d6;margin:0 0 12px}
.pair{display:grid;grid-template-columns:1fr 1fr;gap:26px}label{display:flex;align-items:center;gap:16px;margin:8px 0;font-size:14px}
audio{width:100%;height:36px}label{min-width:0}label audio{min-width:0}
@media(max-width:650px){.pair{grid-template-columns:1fr}body{padding:20px 12px}}
</style><body><h1>Electry realism · before and after</h1>
<p>The same performance and settings in every pair. Listen to dry guitar attacks and releases,
then hear those details through the fixed Modern high-gain chain. E1 is the open low string;
F♯1 is fret 2 of the Drop-E build. Legato probes stay on that same physical string.</p>
<p>Listening copies are volume matched using one constant gain across each complete file.
Both files in a pair have equal RMS, with shared extra attenuation only when needed for peak headroom.
The original float WAVs and all measurements remain unnormalised.
These comparisons have not been assigned listening or realism scores.</p>
<p><a href="report.md">Signal measurements</a> · <a href="comparison.json">Detailed measurements and audition gains</a></p>
""" + "\n".join(cards) + """
<script>document.addEventListener('play',e=>{if(e.target.tagName==='AUDIO'){
document.querySelectorAll('audio').forEach(a=>{if(a!==e.target)a.pause()})}},true)</script>
</body></html>
"""
    (directory / "listening.html").write_text(page)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    baseline = json.loads((args.baseline / "manifest.json").read_text())
    candidate = json.loads((args.candidate / "manifest.json").read_text())
    if baseline != candidate:
        raise ValueError("Input manifests differ; musical/control comparisons are not matched")
    args.output.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "electry-realism-comparison/20260908-v1",
        "baseline_directory": str(args.baseline.resolve()),
        "candidate_directory": str(args.candidate.resolve()),
        "sample_rate": baseline["sample_rate"],
        "matched_manifest": True,
        "normalisation": "none",
        "interpretation": "Signal changes only; these measurements do not establish realism.",
        "takes": [],
    }
    rows = []
    for take in baseline["takes"]:
        for tap in ("dry", "modern"):
            versions = {}
            checksums = {}
            waveforms = {}
            for version, directory in (("baseline", args.baseline),
                                       ("candidate", args.candidate)):
                samples, checksum = read_float_wav(
                    directory / take[tap], baseline["sample_rate"], take["frames"])
                versions[version] = measure(samples, take["events"], baseline["sample_rate"])
                checksums[version] = checksum
                waveforms[version] = samples
            before, after = versions["baseline"], versions["candidate"]
            difference = waveforms["candidate"] - waveforms["baseline"]
            entry = {"id": take["id"], "tap": tap,
                     "sha256": checksums, "measurements": versions,
                     "sample_identical": checksums["baseline"] == checksums["candidate"],
                     "difference_rms_dbfs": db(rms(difference)),
                     "rms_change_db": after["rms_dbfs"] - before["rms_dbfs"],
                     "peak_change_db": after["peak_dbfs"] - before["peak_dbfs"],
                     "attack_20_ms_rms_change_db": after["mean_attack_20_ms_rms_dbfs"]
                                                    - before["mean_attack_20_ms_rms_dbfs"]}
            report["takes"].append(entry)
            entry["audition_constant_gains"] = write_audition_pair(
                args.output / "audio", f"{take['id']}-{tap}", waveforms, baseline["sample_rate"])
            row = {"id": take["id"], "tap": tap,
                   "rms_change_db": entry["rms_change_db"],
                   "attack_20_ms_rms_change_db": entry["attack_20_ms_rms_change_db"]}
            for version, metrics in versions.items():
                for key in ("peak_dbfs", "rms_dbfs", "peak_sample_step",
                            "samples_exceeding_unity", "mean_attack_centroid_hz",
                            "mean_attack_energy_fraction_2_8_khz"):
                    row[f"{version}_{key}"] = metrics[key]
                if metrics["final_release"]:
                    row[f"{version}_release_post_80_ms_rms_dbfs"] = \
                        metrics["final_release"]["post_80_ms_rms_dbfs"]
            rows.append(row)
    (args.output / "comparison.json").write_text(json.dumps(report, indent=2) + "\n")
    with (args.output / "comparison.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    lines = ["# Matched realism render comparison", "",
             "Raw mono float WAVs, 44.1 kHz; identical MIDI/control events and fixed gain.",
             "Dry DI is tapped immediately before the same Modern high-gain chain.",
             "Signal measurements below describe changes; they are not realism ratings.", "",
             "Attack RMS is the mean dBFS of each 20 ms window starting at a note-on or repick.",
             "Release is the first 80 ms after the final note-off. Windows are not pitch- or latency-aligned.",
             "Zero values have a -240 dBFS reporting floor. Source WAVs are unnormalised.", "",
             "| Take | Tap | Peak before / after dBFS | RMS change dB | Attack change dB | Release before / after dBFS |",
             "| --- | --- | ---: | ---: | ---: | ---: |"]
    for row in rows:
        lines.append(f"| {row['id']} | {row['tap']} | "
                     f"{row['baseline_peak_dbfs']:.2f} / {row['candidate_peak_dbfs']:.2f} | "
                     f"{row['rms_change_db']:+.2f} | "
                     f"{row['attack_20_ms_rms_change_db']:+.2f} | "
                     f"{row['baseline_release_post_80_ms_rms_dbfs']:.2f} / "
                     f"{row['candidate_release_post_80_ms_rms_dbfs']:.2f} |")
    (args.output / "report.md").write_text("\n".join(lines) + "\n")
    write_listening_page(args.output, baseline["takes"])
    print(f"Validated {len(rows)} paired mono WAVs; report: {args.output / 'report.md'}")
    print(f"Peak before: {max(row['baseline_peak_dbfs'] for row in rows):.2f} dBFS; "
          f"after: {max(row['candidate_peak_dbfs'] for row in rows):.2f} dBFS")


if __name__ == "__main__":
    main()
