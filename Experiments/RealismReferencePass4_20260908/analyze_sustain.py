#!/usr/bin/env python3
"""Exploratory pass-four sustain comparisons, with no fitting or perceptual score.
Run from repository root after the isolated baseline/loss065/loss100 renders
and integrated candidate render exist. Requires NumPy and SciPy. References
are the public CC0 previews documented in the first-pass reference report.
"""

from pathlib import Path
import argparse, hashlib, importlib.util, json, sys
import numpy as np
import scipy
from scipy.io import wavfile
from scipy.signal import welch

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument(
    "--build-root", type=Path, default=Path("build-realism-20260908-pass4")
)
parser.add_argument(
    "--reference-root", type=Path, default=Path("/tmp/electry-realism-reference")
)
parser.add_argument(
    "--output", type=Path, default=Path(__file__).with_name("sustain-summary.json")
)
args = parser.parse_args()
ROOT = args.build_root
CONDITIONS = {
    key: ROOT / "research-sustain" / key for key in ["baseline", "loss065", "loss100"]
}
CONDITIONS["integrated"] = ROOT / "candidate"
sys.dont_write_bytecode = True
base_analyzer = (
    Path(__file__).resolve().parents[1] / "RealismReference20260908" / "analyze.py"
)
spec = importlib.util.spec_from_file_location("reference_analyzer", base_analyzer)
analyzer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analyzer)
WINDOWS = {
    "body_150_350ms": (0.15, 0.35),
    "late_500_1000ms": (0.5, 1.0),
    "late_1000_1400ms": (1.0, 1.4),
}
BANDS = {
    "20_150hz": (20, 150),
    "150_500hz": (150, 500),
    "500_2000hz": (500, 2000),
    "2000_6000hz": (2000, 6000),
}


def rms(x):
    return float(np.sqrt(np.mean(x * x)))


def db(x):
    return float(20 * np.log10(max(x, 1e-15)))


def segment(x, fs, on, a, b):
    return x[on + round(a * fs) : on + round(b * fs)]


def shape(path, onset):
    fs, x = wavfile.read(path)
    x = x.astype(float)
    on = round(onset * fs)
    body = rms(segment(x, fs, on, 0.15, 0.35))
    result = {}
    for name, (a, b) in WINDOWS.items():
        z = segment(x, fs, on, a, b)
        frequency, power = welch(
            z,
            fs=fs,
            window="hann",
            nperseg=min(len(z), 8192),
            noverlap=None,
            nfft=65536,
            scaling="density",
            detrend=False,
        )
        df = frequency[1] - frequency[0]
        keep = (frequency >= 20) & (frequency <= 6000)
        bands = {
            key: db(
                np.sqrt(np.sum(power[(frequency >= low) & (frequency < high)]) * df)
                / body
            )
            for key, (low, high) in BANDS.items()
        }
        result[name] = {
            "rms_relative_to_body_db": db(rms(z) / body),
            "band_rms_relative_to_body_db": bands,
            "centroid_20_6000hz": float(
                np.sum(frequency[keep] * power[keep]) / np.sum(power[keep])
            ),
        }
    return {
        "path": str(path),
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "body_rms_dbfs": db(body),
        "windows": result,
    }


rows = []
for midi, sound in [(30, 525010), (36, 525008), (42, 525009)]:
    for condition in ["reference"] + list(CONDITIONS):
        p = (
            args.reference_root / f"{sound}.wav"
            if condition == "reference"
            else CONDITIONS[condition] / f"reference-midi{midi}-held-1500ms-dry.wav"
        )
        d = analyzer.analyze(p, midi)
        fs, x = wavfile.read(d["path"])
        x = x.astype(float)
        on = round(d["onset_seconds"] * fs)
        peak = np.max(abs(segment(x, fs, on, 0, 0.2)))
        body = rms(segment(x, fs, on, 0.2, 0.7))
        freq = np.array([h["frequency_hz"] for h in d["partials"][:5]])
        cents = 1200 * np.log2(freq / freq[:, -1:])
        median = np.median(cents, axis=0)
        r = {
            "midi": midi,
            "condition": condition,
            "onset_seconds": d["onset_seconds"],
            "late_500_1000_vs_0_50_rms_db": d["rms_relative_to_0_50_ms_db"][-1],
            "h5_12_over_h1_3_cooling_db": d["h5_12_over_h1_3_change_db"],
            "h5_12_over_h1_3_trace_db": d["h5_12_over_h1_3_db"],
            "peak_0_200_over_rms_200_700_db": db(peak / body),
            "left_boundary_censored": d["left_boundary_censored"],
            "h1_5_median_cents_relative_to_last_window": median.tolist(),
            "partial_decay_db_per_s": [
                h["linear_decay_db_per_s"] for h in d["partials"]
            ],
            "partial_fit_rms_db": [h["linear_fit_rms_db"] for h in d["partials"]],
            "partial_bracket_alarms": [
                h["harmonic"] for h in d["partials"] if h["near_search_boundary"]
            ],
            "dry_sustain": shape(Path(d["path"]), d["onset_seconds"]),
        }
        if condition != "reference":
            # Keep wet windows aligned to the dry audio onset; no latency changes
            # occur between candidates and no amplified real target exists.
            r["modern_sustain"] = shape(
                CONDITIONS[condition] / f"reference-midi{midi}-held-1500ms-modern.wav",
                d["onset_seconds"],
            )
        rows.append(r)
source_files = {
    "baseline": ROOT / "baseline-src" / "Source" / "DSP" / "ElectryEngine.cpp",
    "loss065": ROOT / "research-sustain" / "loss065.cpp",
    "loss100": ROOT / "research-sustain" / "loss100.cpp",
}
manifests = {
    key: {
        "path": str(path / "manifest.json"),
        "sha256": hashlib.sha256((path / "manifest.json").read_bytes()).hexdigest(),
    }
    for key, path in CONDITIONS.items()
}
sources = {
    key: {"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
    for key, path in source_files.items()
}
references = [
    {
        "sound": sid,
        "page": f"https://freesound.org/people/cabled_mess/sounds/{sid}/",
        "preview": f"https://cdn.freesound.org/previews/525/{sid}_5450487-hq.mp3",
        "license": "CC0-1.0",
        "representation": "public MP3 preview decoded to float WAV, not upstream original",
        "preview_sha256": hashlib.sha256(
            (args.reference_root / f"{sid}.mp3").read_bytes()
        ).hexdigest(),
    }
    for sid in [525010, 525008, 525009]
]
result = {
    "status": "exploratory_direction_check_not_calibration",
    "analyzer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    "base_analyzer_sha256": hashlib.sha256(base_analyzer.read_bytes()).hexdigest(),
    "numpy": np.__version__,
    "scipy": scipy.__version__,
    "sources": sources,
    "manifests": manifests,
    "references": references,
    "integrated_source_provenance": "See main pass-four frozen build receipt; only the rendered WAV and manifest bytes are attested here.",
    "modern_alignment": "dry audio onset, common identical chain",
    "modern_normalization": "one constant per complete file: RMS150-350ms; preserve later dynamics",
    "spectral_method": "Hann Welch up to8192 samples,50% overlap,nfft65536,power density integrated in specified bands",
    "partial_window_centers_seconds": [
        0.13,
        0.23,
        0.33,
        0.43,
        0.53,
        0.63,
        0.73,
        0.83,
        0.93,
    ],
    "rows": rows,
}
args.output.write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
for r in rows:
    if r["condition"] == "reference":
        continue
    w = r["modern_sustain"]["windows"]["late_500_1000ms"]
    print(
        r["midi"],
        r["condition"],
        "Modern500–1000 rms/body",
        round(w["rms_relative_to_body_db"], 2),
        "centroid",
        round(w["centroid_20_6000hz"], 1),
        "bands/body",
        [round(v, 2) for v in w["band_rms_relative_to_body_db"].values()],
    )
