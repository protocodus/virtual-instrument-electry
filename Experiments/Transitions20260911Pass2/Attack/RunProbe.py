#!/usr/bin/env python3
"""Reproduce the small reopened-attack heel-follower reset independently."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument('baseline', type=Path)
parser.add_argument('candidate', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
source = Path(__file__).with_name('Probe.cpp').resolve()
traces = {}
flags = ['-std=c++20', '-O2', '-DNDEBUG', '-DELECTRY_DECOUPLED_PICK_RELEASE=1',
         '-DELECTRY_MEASURED_BODY_RESPONSE=1', '-DELECTRY_ENERGY_ATTACK_PITCH=1']
for label, tree in [('before', args.baseline), ('after', args.candidate)]:
    tree = tree.resolve()
    binary = (args.output / f'{label}-probe').resolve()
    subprocess.run(['clang++', *flags, '-I'+str(tree/'Source'), str(source),
                    str(tree/'Source/DSP/ElectryEngine.cpp'), '-o', str(binary)], check=True)
    with (args.output/f'{label}.csv').open('w') as log:
        subprocess.run([str(binary), str((args.output/label).resolve())], stdout=log, check=True)
    trace = {}; key = None
    for line in (args.output/f'{label}.csv').read_text().splitlines():
        if line.startswith('case,'):
            key = tuple(map(int, line.split(',')[1:])); trace[key] = []
        else:
            trace[key].append(list(map(float, line.split(','))))
    traces[label] = {key: np.array(value) for key, value in trace.items()}
results = []
for note in [28, 40, 55, 62, 67]:
    item = {'midi_note': note, 'controls_sample_identical': []}
    for scenario in range(4):
        a = np.fromfile(args.output/'before'/f'{note}_{scenario}.f32', np.float32)
        b = np.fromfile(args.output/'after'/f'{note}_{scenario}.f32', np.float32)
        assert np.isfinite(a).all() and np.isfinite(b).all()
        if scenario:
            item['controls_sample_identical'].append(bool(np.array_equal(a,b)))
            continue
        assert np.array_equal(a[:19200], b[:19200]), 'Candidate changed the preceding Palm'
        item['null_relative_to_whole_probe_rms_db'] = float(10*np.log10(np.mean((a-b)**2)/np.mean(a*a)))
        item['body_rms_change_12_35ms_db'] = float(10*np.log10(np.mean(b[19776:20880]**2)/np.mean(a[19776:20880]**2)))
        for label in ['before','after']:
            q = traces[label][note,0]
            maximum = q[(q[:,0]>0) & (q[:,0]<.001),5].max()
            item[label] = {'pre_repicking_depth': float(q[0,5]),
                           'maximum_depth_in_first_ms': float(maximum),
                           'depth_increase_ratio': float(maximum/q[0,5])}
    assert all(item['controls_sample_identical']), 'Stationary or no-repick controls changed'
    results.append(item)
receipt = {'method': '48 kHz mono DI; 400 ms Palm ring then same held-string Sustain repick; static controls are unpicked sibling lift, open repick, Palm repick. Trace sampled per host frame through first 3 ms.',
           'source_sha256': {label: {f: hashlib.sha256((tree/f).read_bytes()).hexdigest()
                               for f in ['Source/DSP/ElectryEngine.cpp','Source/DSP/ElectryEngine.h']}
                             for label, tree in [('before',args.baseline),('after',args.candidate)]},
           'flags': flags, 'probe_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
           'results': results}
(args.output/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
print(json.dumps(results, indent=2))
