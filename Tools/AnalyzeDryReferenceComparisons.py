#!/usr/bin/env python3
"""Describe isolated notes in a frozen real-DI/model pack; never fit controls.

Input is BuildDryReferenceComparisons.py's measurements.json. Each consumed
mono WAV must match its recorded SHA-256. Analysis stays at native rate and
uses the recorded audio onset, without EQ, resampling, time warping or another
level adjustment. Constant listening trims cancel from normalized envelopes
and decay slopes; pickup/DI chain, force and construction remain confounds.
This single-note protocol must not be applied to changing-note phrase packs.

Protocol: centred 2 ms RMS onset (25% of the first second's peak); RMS windows
relative to 0--50 ms; mean-removed Hann spectral descriptors over 20--8000 Hz.
For partials 1--12, a 128 ms Hann/20 ms-hop FFT tracks a local spectral maximum
within +/-0.30 fundamental spacings of n*f0, then uses log-parabolic sub-bin
interpolation. This tracks stretched/moving partials instead of measuring fixed
equal-temperament bins. Local median non-peak amplitude estimates a *spectral*
floor, not calibrated recording noise. A descriptive straight-line dB fit uses
0.12--2.0 s points at least 12 dB above that floor, >=8 points and >=0.14 s
span. T60 is -60/slope only for negative slopes; R2<0.9 is marked non-linear.
No reported threshold is an engine acceptance rail or a calibration target.

Requires research-only NumPy/SciPy. Example:
  python3 Tools/AnalyzeDryReferenceComparisons.py --input PACK/measurements.json \
      --output build-real-dry-review/reference-analysis/comparison
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import platform
import warnings

import numpy as np
import scipy
from scipy.io import wavfile

SCHEMA = "electry-dry-reference-analysis/v1"
FIT_START = 0.12
FIT_END = 2.0
MIN_SNR_DB = 12.0
MIN_FIT_POINTS = 8
MIN_FIT_SPAN = 0.14
RMS_WINDOWS = ((0, 50), (50, 150), (150, 500), (500, 1000), (1000, 2000))


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rms_onset(x: np.ndarray, rate: int) -> int:
    """First 25%-of-peak crossing, with clipped centred-window boundaries."""
    n = max(1, round(0.002 * rate))
    ends = np.arange(min(len(x), rate))
    lo = np.maximum(0, ends - n // 2)
    hi = np.minimum(len(x), ends + (n + 1) // 2)
    integral = np.r_[0.0, np.cumsum(x * x)]
    env = np.sqrt(np.maximum(0.0, (integral[hi] - integral[lo]) / (hi - lo)))
    if not len(env) or np.max(env) <= 1e-12:
        raise ValueError("cannot locate onset in silent audio")
    return int(np.flatnonzero(env >= 0.25 * np.max(env))[0])


def _db(value: float) -> float:
    return 20.0 * math.log10(max(value, 1e-30))


def _linear_fit(times: np.ndarray, levels: np.ndarray) -> dict:
    if len(times) < MIN_FIT_POINTS or np.ptp(times) < MIN_FIT_SPAN:
        return {"status": "insufficient", "points": len(times),
                "slope_db_per_s": None, "t60_s": None, "r_squared": None}
    slope, intercept = np.polyfit(times, levels, 1)
    residual = levels - (intercept + slope * times)
    variance = float(np.sum((levels - np.mean(levels)) ** 2))
    r2 = 1.0 - float(np.sum(residual ** 2)) / variance if variance > 1e-20 else None
    return {"status": "linear" if slope < 0 and r2 is not None and r2 >= 0.9
                       else "nonlinear_or_nondecaying",
            "points": len(times), "first_time_s": float(times[0]),
            "last_time_s": float(times[-1]), "slope_db_per_s": float(slope),
            "t60_s": float(-60.0 / slope) if slope < 0 else None,
            "r_squared": r2, "residual_rms_db": float(np.sqrt(np.mean(residual ** 2)))}


def tracked_partials(x: np.ndarray, rate: int, onset: int, nominal_hz: float) -> list[dict]:
    n = round(0.128 * rate)
    hop = max(1, round(0.020 * rate))
    nfft = 1 << math.ceil(math.log2(4 * n))
    window = np.hanning(n)
    frequencies = np.fft.rfftfreq(nfft, 1.0 / rate)
    step = rate / nfft
    tracks = []
    indices = []
    for harmonic in range(1, 13):
        centre = nominal_hz * harmonic
        if centre + 0.5 * nominal_hz >= min(8000.0, rate / 2):
            break
        indices.append(np.flatnonzero(np.abs(frequencies - centre) <= 0.30 * nominal_hz))
        tracks.append({"partial": harmonic, "time_s": [], "frequency_hz": [],
                       "amplitude": [], "local_floor_amplitude": [],
                       "snr_db": [], "search_edge": []})
    first = max(n // 2, onset + round(0.08 * rate))
    last = min(len(x) - (n - n // 2), onset + round(2.25 * rate))
    for centre in range(first, last + 1, hop):
        frame = x[centre - n // 2:centre - n // 2 + n]
        spectrum = np.abs(np.fft.rfft(frame * window, nfft)) * (2.0 / np.sum(window))
        for bins, track in zip(indices, tracks):
            k = int(bins[np.argmax(spectrum[bins])])
            edge = k == bins[0] or k == bins[-1]
            values = np.log(np.maximum(spectrum[k - 1:k + 2], 1e-30))
            denominator = values[0] - 2 * values[1] + values[2]
            delta = float(0.5 * (values[0] - values[2]) / denominator) if denominator < -1e-20 else 0.0
            delta = float(np.clip(delta, -0.5, 0.5))
            hz = (k + delta) * step
            amplitude = float(np.exp(values[1] - 0.25 * (values[0] - values[2]) * delta))
            distance = np.abs(frequencies - hz)
            floor_bins = (distance > 2.5 * rate / n) & (distance < 0.5 * nominal_hz)
            floor = max(float(np.median(spectrum[floor_bins])), 1e-15)
            track["time_s"].append((centre - onset) / rate)
            track["frequency_hz"].append(float(hz))
            track["amplitude"].append(amplitude)
            track["local_floor_amplitude"].append(floor)
            track["snr_db"].append(_db(amplitude / floor))
            track["search_edge"].append(bool(edge))
    for track in tracks:
        times = np.asarray(track["time_s"])
        amplitudes = np.asarray(track["amplitude"])
        snr = np.asarray(track["snr_db"])
        valid = ((times >= FIT_START) & (times <= FIT_END) & (snr >= MIN_SNR_DB)
                 & ~np.asarray(track["search_edge"], dtype=bool))
        track["fit"] = _linear_fit(times[valid], 20 * np.log10(np.maximum(amplitudes[valid], 1e-30)))
        track["fit"]["rejected_points_in_window"] = int(np.sum((times >= FIT_START)
                                                                & (times <= FIT_END) & ~valid))
        track["fit"]["median_snr_db"] = float(np.median(snr[valid])) if np.any(valid) else None
        track["median_frequency_hz"] = float(np.median(np.asarray(track["frequency_hz"])[valid])) if np.any(valid) else None
    return tracks


def analyze_audio(x: np.ndarray, rate: int, onset: int, nominal_hz: float) -> dict:
    if x.ndim != 1 or not np.isfinite(x).all() or not 0 <= onset < len(x):
        raise ValueError("analysis requires finite mono audio and an in-range onset")
    if onset + round(2.0 * rate) > len(x):
        raise ValueError("analysis requires two seconds after audio onset")
    windows = []
    baseline = None
    for start, end in RMS_WINDOWS:
        section = x[onset + round(start * rate / 1000):onset + round(end * rate / 1000)]
        level = float(np.sqrt(np.mean(section * section)))
        if baseline is None:
            baseline = level
            if baseline <= 1e-12:
                raise ValueError("silent normalization window")
        windows.append({"start_ms": start, "end_ms": end, "rms": level,
                        "relative_db": _db(level / baseline)})
    descriptors = []
    for start, end in ((0, 30), (30, 80), (80, 150), (150, 500)):
        section = x[onset + round(start * rate / 1000):onset + round(end * rate / 1000)]
        section = (section - np.mean(section)) * np.hanning(len(section))
        size = max(4096, 1 << math.ceil(math.log2(4 * len(section))))
        power = np.abs(np.fft.rfft(section, size)) ** 2
        frequencies = np.fft.rfftfreq(size, 1.0 / rate)
        band = (frequencies >= 20) & (frequencies <= min(8000, rate / 2))
        total = float(np.sum(power[band]))
        descriptors.append({"start_ms": start, "end_ms": end,
                            "centroid_hz": float(np.sum(frequencies[band] * power[band]) / total) if total else None,
                            "power_above_500_share": float(np.sum(power[band & (frequencies > 500)]) / total) if total else None})
    n = max(1, round(0.002 * rate))
    region = x[onset:min(len(x), onset + rate)]
    energy = np.convolve(region * region, np.ones(n) / n, mode="same")
    peak_frame = int(np.argmax(energy))
    return {"frames": len(x), "rate": rate, "recorded_onset_frame": onset,
            "independent_onset_frame": rms_onset(x, rate),
            "onset_to_first_second_rms_peak_ms": 1000.0 * peak_frame / rate,
            "peak": float(np.max(np.abs(x))), "normalized_rms": windows,
            "spectral_windows": descriptors,
            "partials": tracked_partials(x, rate, onset, nominal_hz)}


def read_mono(path: Path) -> tuple[int, np.ndarray]:
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", wavfile.WavFileWarning)
        rate, audio = wavfile.read(path)
    if audio.ndim != 1:
        raise ValueError(f"expected explicitly selected mono channel: {path}")
    if np.issubdtype(audio.dtype, np.signedinteger):
        audio = audio.astype(np.float64) / float(2 ** (np.iinfo(audio.dtype).bits - 1))
    elif np.issubdtype(audio.dtype, np.floating):
        audio = audio.astype(np.float64)
    else:
        raise ValueError(f"unsupported WAV encoding: {audio.dtype}")
    return int(rate), audio


def analyze_pack(manifest: Path, destination: Path) -> dict:
    pack = json.loads(manifest.read_text())
    if pack.get("schema") != "electry-dry-reference-comparisons/v1":
        raise ValueError("unknown listening-pack schema")
    result = {"schema": SCHEMA, "manifest_sha256": sha(manifest),
              "analyzer_sha256": sha(Path(__file__)),
              "runtime": {"python": platform.python_version(), "numpy": np.__version__, "scipy": scipy.__version__},
              "protocol": {"fit_start_s": FIT_START, "fit_end_s": FIT_END,
                           "minimum_local_snr_db": MIN_SNR_DB, "minimum_points": MIN_FIT_POINTS,
                           "minimum_fit_span_s": MIN_FIT_SPAN, "window_ms": 128, "hop_ms": 20},
              "limitations": ["Descriptive development comparison; no coefficients fitted and no engine acceptance score.",
                              "Separate players, guitars, pickups, strings and DI chains are not calibrated.",
                              "Local spectral-floor and linear-decay fits can be limited by beating, curved loss and recording noise."],
              "comparisons": []}
    rows = []
    for comparison in pack["comparisons"]:
        if "events" in comparison["reference"] or "sounding_midi" not in comparison["reference"]:
            raise ValueError("isolated-note analysis cannot describe a changing-note phrase")
        midi = comparison["reference"]["sounding_midi"]
        nominal = 440.0 * 2 ** ((midi - 69) / 12)
        entry = {"id": comparison["id"], "nominal_midi": midi,
                 "nominal_frequency_hz": nominal, "tracks": {}}
        for name, track in comparison["tracks"].items():
            path = manifest.parent / track["path"]
            if sha(path) != track["sha256"]:
                raise ValueError(f"audio hash mismatch: {path}")
            rate, audio = read_mono(path)
            if rate != comparison["rate"] or len(audio) != comparison["frames"]:
                raise ValueError(f"audio format differs from frozen receipt: {path}")
            analysis = analyze_audio(audio, rate, comparison["onset_frame"], nominal)
            analysis["audio_sha256"] = track["sha256"]
            entry["tracks"][name] = analysis
            for partial in analysis["partials"]:
                rows.append({"comparison": comparison["id"], "track": name,
                             "partial": partial["partial"], "frequency_hz": partial["median_frequency_hz"],
                             **partial["fit"]})
        result["comparisons"].append(entry)
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "analysis.json").write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    columns = sorted({key for row in rows for key in row})
    with (destination / "partial-decays.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, columns)
        writer.writeheader()
        writer.writerows(rows)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = analyze_pack(args.input.resolve(), args.output.resolve())
    print(f"Described {len(result['comparisons'])} comparisons; no engine controls fitted")


if __name__ == "__main__":
    main()
