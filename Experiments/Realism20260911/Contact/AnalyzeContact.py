#!/usr/bin/env python3
"""Measure paired raw renders from RealismEverydayContactTests.

The paired baseline ablates one contact mechanism, retaining all other DSP,
settings and sample-accurate events. These descriptors are signal differences,
not a perceptual realism score. Inputs are mono little-endian float32 samples.
"""

import argparse
import json
from pathlib import Path

import numpy as np
from scipy.signal import butter, sosfilt

SAMPLE_RATES = (44100, 48000, 96000)
PITCHES = (28, 40, 64)
BANDS = {
    "low": (30, 400),
    "middle": (400, 1500),
    "high": (1500, 7000),
}


def relative_db(numerator, denominator):
    return float(20 * np.log10(max(numerator, 1e-30) / max(denominator, 1e-30)))


def window_rms(signal, sample_rate, window):
    first, last = (round(sample_rate * time) for time in window)
    samples = signal[first:last]
    if samples.size != last - first:
        raise ValueError("Render is too short for the registered analysis window")
    return np.sqrt(np.mean(samples * samples))


def analyze(input_directory):
    rows = []
    for sample_rate in SAMPLE_RATES:
        for pitch in PITCHES:
            for mechanism in ("release", "pick"):
                for tap in ("dry", "amp"):
                    prefix = f"{sample_rate}-{pitch}-{mechanism}"
                    baseline = np.fromfile(
                        input_directory / f"{prefix}-baseline-{tap}.f32", dtype="<f4"
                    ).astype(float)
                    candidate = np.fromfile(
                        input_directory / f"{prefix}-candidate-{tap}.f32", dtype="<f4"
                    ).astype(float)
                    if baseline.size != sample_rate // 2 or candidate.shape != baseline.shape:
                        raise ValueError(f"Unexpected render length for {prefix}-{tap}")
                    if not np.isfinite(baseline).all() or not np.isfinite(candidate).all():
                        raise ValueError(f"Non-finite samples in {prefix}-{tap}")
                    window = (0.09, 0.18) if mechanism == "release" else (0, 0.03)
                    baseline_rms = window_rms(baseline, sample_rate, window)
                    row = {
                        "rate": sample_rate,
                        "pitch": pitch,
                        "idea": mechanism,
                        "tap": tap,
                        "differenceDb": relative_db(
                            window_rms(candidate - baseline, sample_rate, window), baseline_rms
                        ),
                        "levelDb": relative_db(
                            window_rms(candidate, sample_rate, window), baseline_rms
                        ),
                    }
                    for band, limits in BANDS.items():
                        sections = butter(
                            3, limits, fs=sample_rate, btype="bandpass", output="sos"
                        )
                        filtered_baseline = sosfilt(sections, baseline)
                        filtered_candidate = sosfilt(sections, candidate)
                        row[f"{band}Db"] = relative_db(
                            window_rms(filtered_candidate, sample_rate, window),
                            window_rms(filtered_baseline, sample_rate, window),
                        )
                    rows.append(row)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_directory", type=Path)
    parser.add_argument("output_json", type=Path)
    args = parser.parse_args()
    rows = analyze(args.input_directory)
    args.output_json.parent.mkdir(parents=True, exist_ok=True)
    args.output_json.write_text(json.dumps(rows, indent=2) + "\n")
    for row in rows:
        if row["rate"] == 48000:
            print(
                f"{row['pitch']:2d} {row['idea']:7s} {row['tap']:3s}: "
                f"difference {row['differenceDb']:+.2f} dB, "
                f"level {row['levelDb']:+.3f} dB, "
                f"high band {row['highDb']:+.2f} dB"
            )


if __name__ == "__main__":
    main()
