#!/usr/bin/env python3
"""Describe refret attacks in a frozen real-DI phrase comparison manifest.

Uses each original reference crop and the manifest's A-derived integer model
crop for A, B and optional counterexamples. Every source NoteOn is inspected
at its original annotated time: no candidate onset search, note-wise warp,
EQ, filter restart, or per-window gain. One constant full-crop gain reaches
-24 dBFS RMS; raw metrics, gain and following-window RMS expose attenuation.

The 8 kHz observation is a causal fourth-order Butterworth highpass run over
the full crop before windows are extracted. These descriptive step/energy
observations can expose a failed attack, but do not establish a calibrated
perceptual click threshold, subjective preference or acoustic parity with
different guitar hardware. Short-window audio pitch is deliberately omitted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import warnings

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt


WINDOWS = {"attack_-5_10ms": (-.005, .010), "onset_0_30ms": (0, .030),
           "following_20_80ms": (.020, .080)}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def locate(value, parent):
    path = Path(value)
    return path if path.is_absolute() else parent / path


def audio(path):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", wavfile.WavFileWarning)
        rate, samples = wavfile.read(path)
    if samples.dtype.kind == 'i':
        samples = samples.astype(np.float64) / 2 ** (samples.dtype.itemsize * 8 - 1)
    elif samples.dtype.kind not in 'f':
        raise ValueError("Expected signed PCM or float audio")
    if samples.ndim == 1:
        samples = samples[:, None]
    if samples.ndim != 2 or not np.isfinite(samples).all():
        raise ValueError("Expected finite audio channels")
    return int(rate), samples.astype(np.float64)


def window_metrics(samples, high, lo, hi):
    if not 1 <= lo < hi <= len(samples):
        raise ValueError("Registered analysis window exceeds frozen crop")
    return {"max_sample_step": float(np.max(np.abs(np.diff(samples[lo - 1:hi])))),
            "rms": float(np.sqrt(np.mean(samples[lo:hi] ** 2))),
            "peak": float(np.max(np.abs(samples[lo:hi]))),
            "high8k_rms": float(np.sqrt(np.mean(high[lo:hi] ** 2)))}


def configuration(receipt):
    # Extra counterexamples may have shorter unused post-crop padding. Every
    # selected event, physical string, parameter and seed remains identical.
    return {key: value for key, value in receipt.items()
            if key not in {'render_seconds', 'frames'}}


def analyze(manifest_path, extras=None):
    manifest_path = Path(manifest_path).resolve()
    digest = sha(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    if manifest.get('schema') != 'electry-dry-phrase-comparisons/v1':
        raise ValueError("Expected a real-DI phrase comparison manifest")
    comparisons = manifest.get('comparisons', [])
    if not comparisons or len({row['id'] for row in comparisons}) != len(comparisons):
        raise ValueError("Expected unique nonempty phrase comparisons")
    extras = extras or {}
    if any(not name or name in {'Reference', 'A', 'B'} for name in extras):
        raise ValueError("Extra model labels must be nonempty and distinct from Reference/A/B")
    results = []
    for comparison in comparisons:
        row = comparison['reference']
        identifier = comparison['id']
        rate, count = comparison['rate'], comparison['frames']
        if rate <= 16000 or count <= 0:
            raise ValueError("Expected positive crop and native rate above 16 kHz")
        # The builder preserves source paths from the input plan, while model
        # paths belong to the output manifest. They can live in different
        # directories. Older manifests used absolute source paths and remain
        # compatible with the historical manifest-directory fallback.
        source_base = locate(comparison.get('reference_plan_directory', '.'),
                             manifest_path.parent)
        reference_path = locate(row['path'], source_base)
        if sha(reference_path) != row['sha256']:
            raise ValueError("Real source WAV hash mismatch")
        midi_path = locate(row['midi_path'], source_base)
        if sha(midi_path) != row['midi_sha256']:
            raise ValueError("Source MIDI hash mismatch")
        actual_rate, reference = audio(reference_path)
        start = row['crop_start_frame']
        channel = row['channel']
        if (actual_rate != rate or row['crop_frames'] != count or start < 0
                or channel < 0 or channel >= reference.shape[1]
                or start + count > len(reference)):
            raise ValueError("Reference crop/rate/channel differs")
        clips = {'Reference': reference[start:start + count, channel]}
        sources = {'Reference': {'path': str(reference_path), 'sha256': sha(reference_path),
                                 'crop_start_frame': start, 'channel': channel}}
        model_start = comparison['tracks']['A']['alignment']['crop_start_frame']
        expected = comparison['model_render']['A']
        folders = {}
        for label in ('A', 'B'):
            if label not in comparison['tracks']:
                continue
            track = comparison['tracks'][label]
            if track['alignment']['crop_start_frame'] != model_start:
                raise ValueError("A/B crops differ; candidate-specific alignment is forbidden")
            raw_path = locate(track['raw_path'], manifest_path.parent)
            if sha(raw_path) != track['raw_sha256']:
                raise ValueError("Model raw WAV hash mismatch")
            folders[label] = (raw_path, comparison['model_render'][label])
        folders.update({label: (Path(folder) / identifier / 'raw.wav', None)
                        for label, folder in extras.items()})
        for label, (path, stored_receipt) in folders.items():
            actual_rate, samples = audio(path)
            receipt_path = path.with_name('render.json')
            receipt = json.loads(receipt_path.read_text())
            if stored_receipt is not None and receipt != stored_receipt:
                raise ValueError("Model render receipt differs from frozen manifest")
            if (configuration(receipt) != configuration(expected)
                    or actual_rate != rate or len(samples) != receipt['frames']
                    or samples.shape[1] != 2
                    or not np.array_equal(samples[:, 0], samples[:, 1])
                    or model_start < 0 or model_start + count > len(samples)):
                raise ValueError("Model score/configuration/rate/crop or mono output differs")
            clips[label] = samples[model_start:model_start + count, 0]
            sources[label] = {'path': str(path), 'sha256': sha(path),
                              'render_receipt_sha256': sha(receipt_path),
                              'crop_start_frame': model_start}
        facts = {}
        for label, samples in clips.items():
            rms = float(np.sqrt(np.mean(samples ** 2)))
            if not rms > 0:
                raise ValueError("Silent crop")
            gain = 10 ** (-24 / 20) / rms
            high = sosfilt(butter(4, 8000, fs=rate, btype='highpass', output='sos'), samples)
            onsets = []
            previous_time = -float('inf')
            for index, event in enumerate(row['events']):
                if event['time_s'] < previous_time:
                    raise ValueError("Source events are not sorted")
                previous_time = event['time_s']
                if event['type'] != 'on':
                    if event['type'] != 'off':
                        raise ValueError("Unknown source event")
                    continue
                boundary = round(event['time_s'] * rate) - start
                windows = {}
                for name, (begin, end) in WINDOWS.items():
                    lo, hi = boundary + round(begin * rate), boundary + round(end * rate)
                    raw = window_metrics(samples, high, lo, hi)
                    windows[name] = {'frames': [lo, hi], 'raw': raw,
                                     'matched': {key: value * gain for key, value in raw.items()}}
                onsets.append({'source_event_index': index, 'sounding_midi': event['note'],
                               'registered_frame': boundary, 'windows': windows})
            if not onsets:
                raise ValueError("Phrase has no source NoteOn")
            facts[label] = {'source': sources[label], 'whole_crop_raw_rms': rms,
                            'whole_crop_gain': gain,
                            'whole_crop_raw_peak': float(np.max(np.abs(samples))),
                            'all_onsets': onsets}
        for label, track in facts.items():
            if label == 'Reference':
                continue
            for index, event in enumerate(track['all_onsets']):
                for name, window in event['windows'].items():
                    baseline = facts['A']['all_onsets'][index]['windows'][name]['matched']
                    window['ratio_to_A'] = {key: value / baseline[key] if baseline[key] > 0 else None
                                           for key, value in window['matched'].items()}
        results.append({'id': identifier, 'rate': rate, 'frames': count,
                        'midi_sha256': sha(midi_path), 'tracks': facts})
    if sha(manifest_path) != digest:
        raise ValueError("Phrase manifest changed during analysis")
    return {'schema': 'electry-refret-phrase-analysis/v1', 'method': __doc__,
            'observer_sha256': sha(__file__), 'manifest_sha256': digest,
            'revisions': manifest.get('revisions', {}),
            'limits': ['No subjective listening judgment or calibrated perceptual click threshold.',
                       'Six-string hardware/force differs from the extended-range model.',
                       'MIDI timing/velocity are annotations, not measured physical contacts/forces.',
                       'Preserve source-specific development/repeat/holdout limits in the source plan.',
                       'A frozen full-loop phase observer diagnoses pitch-state convergence separately.'],
            'comparisons': results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--extra', action='append', default=[], metavar='LABEL=DIR',
                        help='Optional counterexample root with PHRASE_ID/raw.wav and render.json')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    extras = {}
    for value in args.extra:
        label, path = value.split('=', 1)
        if label in extras:
            parser.error('Duplicate extra model label')
        extras[label] = Path(path).resolve()
    result = analyze(args.manifest, extras)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False) + '\n')
    print(f"Analyzed {len(result['comparisons'])} real-DI phrase comparisons: {args.output}")


if __name__ == '__main__':
    main()
