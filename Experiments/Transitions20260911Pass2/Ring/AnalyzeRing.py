#!/usr/bin/env python3
"""Compare the isolated idle-hand fix against matching original riff takes."""
import argparse
import json
from pathlib import Path
import numpy as np
from scipy.io import wavfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('baseline', type=Path)
parser.add_argument('candidate', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
before = json.loads((args.baseline / 'manifest.json').read_text())
after = json.loads((args.candidate / 'manifest.json').read_text())
base_takes = {take['id']: take for take in before['takes']}
rows = []
for take in after['takes']:
    assert take == base_takes[take['id']], take['id']
    for tap in ('dry', 'crunch', 'modern'):
        name = f"{take['id']}-{tap}.wav"
        before_rate, x = wavfile.read(args.baseline / name)
        after_rate, y = wavfile.read(args.candidate / name)
        assert before_rate == after_rate and x.shape == y.shape
        x, y = x.astype(np.float64), y.astype(np.float64)
        assert np.all(np.isfinite(x)) and np.all(np.isfinite(y))
        identical = (args.baseline / name).read_bytes() == (args.candidate / name).read_bytes()
        rows.append({'take': take['id'], 'tap': tap, 'byte_identical': identical,
                     'rms_change_db': float(20 * np.log10(np.linalg.norm(y) / np.linalg.norm(x))),
                     'null_db_relative_baseline': None if identical else float(
                         20 * np.log10(np.linalg.norm(y - x) / np.linalg.norm(x))),
                     'candidate_peak': float(np.max(np.abs(y)))})
args.output.write_text(json.dumps({'matching_first_nine_take_manifests': True,
                                  'rows': rows}, indent=2) + '\n')
