#!/usr/bin/env python3
"""Exploratory low-string reference descriptors; no fitting or holdout claims.

Usage: analyze.py INPUT.wav MIDI_NOTE OUTPUT.json
Requires NumPy and SciPy. Reads one mono WAV (PCM or float), preserves level,
and tracks local spectral peaks rather than sampling equal-temperament bins.
The same analyzer is used for real previews and model renders.
"""

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
import scipy
from scipy.io import wavfile
from scipy.ndimage import uniform_filter1d


def db(amplitude):
    return 20.0 * np.log10(np.maximum(amplitude, 1e-15))


def read_mono(path):
    fs, signal = wavfile.read(path)
    if signal.ndim != 1:
        raise ValueError("Expected mono audio; select and document a channel first")
    if signal.dtype.kind in "iu":
        if signal.dtype.kind == "u":
            signal = (signal.astype(float) - 128.0) / 128.0
        else:
            signal = signal.astype(float) / (2 ** (8 * signal.dtype.itemsize - 1))
    else:
        signal = signal.astype(float)
    if not np.isfinite(signal).all() or len(signal) < 1.2 * fs:
        raise ValueError("Need at least 1.2 seconds of finite audio")
    return fs, signal


def spectrum(signal, fs, onset, begin, end):
    segment = signal[onset + round(begin * fs):onset + round(end * fs)]
    window = np.hanning(len(segment))
    nfft = 1 << (len(segment) * 8 - 1).bit_length()
    amplitude = 2 * np.abs(np.fft.rfft(segment * window, nfft)) / np.sum(window)
    return np.fft.rfftfreq(nfft, 1 / fs), amplitude


def analyze(path, midi):
    fs, signal = read_mono(path)
    envelope = np.sqrt(np.maximum(uniform_filter1d(
        signal ** 2, round(0.002 * fs), mode="constant"), 0))
    peak_index = np.argmax(envelope[:fs])
    threshold = 0.25 * envelope[peak_index]
    onset = int(np.flatnonzero(envelope >= threshold)[0])
    f0 = 440 * 2 ** ((midi - 69) / 12)
    starts = np.arange(0.05, 0.851, 0.10)
    tracked_frequency = []
    tracked_amplitude = []
    for start in starts:
        frequency, amplitude = spectrum(signal, fs, onset, start, start + 0.16)
        frequencies, amplitudes = [], []
        for harmonic in range(1, 13):
            # Non-overlapping +/-0.4*f0 brackets retain drift and modest
            # inharmonicity. A boundary flag exposes unsupported tracking.
            bins = np.flatnonzero(abs(frequency - f0 * harmonic) < 0.4 * f0)
            k = int(bins[np.argmax(amplitude[bins])])
            y0, y1, y2 = np.log(np.maximum(amplitude[k - 1:k + 2], 1e-15))
            shift = 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2)
            shift = float(np.clip(shift, -0.5, 0.5))
            frequencies.append(float((k + shift) * (frequency[1] - frequency[0])))
            amplitudes.append(float(np.exp(y1 - 0.25 * (y0 - y2) * shift)))
        tracked_frequency.append(frequencies)
        tracked_amplitude.append(amplitudes)
    amplitudes = np.array(tracked_amplitude)
    frequencies = np.array(tracked_frequency)
    partials = []
    for h in range(12):
        level = db(amplitudes[:, h])
        slope, intercept = np.polyfit(starts + 0.08, level, 1)
        residual = level - (slope * (starts + 0.08) + intercept)
        partials.append({
            "harmonic": h + 1,
            "frequency_hz": frequencies[:, h].tolist(),
            "level_dbfs": level.tolist(),
            "linear_decay_db_per_s": float(slope),
            "linear_fit_rms_db": float(np.sqrt(np.mean(residual ** 2))),
            "near_search_boundary": bool(np.any(
                abs(frequencies[:, h] - (h + 1) * f0) > 0.35 * f0)),
        })
    rms_windows = [(0, .05), (.05, .15), (.15, .5), (.5, 1.0)]
    rms_levels = [float(db(np.sqrt(np.mean(signal[
        onset + round(a * fs):onset + round(b * fs)] ** 2))))
        for a, b in rms_windows]
    timbre = []
    for begin, end in [(0, .03), (.03, .08), (.08, .18), (.5, .66)]:
        frequency, amplitude = spectrum(signal, fs, onset, begin, end)
        power = amplitude ** 2
        keep = (frequency >= 20) & (frequency <= 6500)
        high = (frequency >= 500) & (frequency <= 2000)
        timbre.append({
            "window_seconds": [begin, end],
            "power_centroid_20_6500_hz": float(np.sum(
                frequency[keep] * power[keep]) / np.sum(power[keep])),
            "power_share_500_2000": float(np.sum(power[high]) / np.sum(power[keep])),
        })
    ratio = db(np.sqrt(np.sum(amplitudes[:, 4:] ** 2, axis=1)
                       / np.sum(amplitudes[:, :3] ** 2, axis=1)))
    return {
        "status": "exploratory_direction_check_not_calibration",
        "path": str(path),
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "analyzer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "numpy": np.__version__, "scipy": scipy.__version__,
        "sample_rate": fs, "samples": len(signal), "midi": midi,
        "onset_seconds": onset / fs,
        "left_boundary_censored": bool(onset < round(0.002 * fs)),
        "global_peak_after_onset_ms": (int(peak_index) - onset) * 1000 / fs,
        "rms_window_seconds": rms_windows,
        "rms_dbfs": rms_levels,
        "rms_relative_to_0_50_ms_db": [v - rms_levels[0] for v in rms_levels],
        "timbre": timbre,
        "partial_window_starts_seconds": starts.tolist(),
        "partial_window_duration_seconds": .16,
        "partials": partials,
        "h5_12_over_h1_3_db": ratio.tolist(),
        "h5_12_over_h1_3_change_db": float(ratio[-1] - ratio[0]),
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("audio", type=Path)
    parser.add_argument("midi", type=int)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result = analyze(args.audio, args.midi)
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps({k: result[k] for k in ["midi", "left_boundary_censored",
        "rms_relative_to_0_50_ms_db", "h5_12_over_h1_3_change_db"]}))
