#!/usr/bin/env python3
"""Report same-settings pitch ablation against the recorded ordinary DI cohort."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np

FIELDS = (
    'aggregate_attack_cents',
    'aggregate_slope_cents_per_second',
    'fundamental_attack_cents',
    'fundamental_slope_cents_per_second',
)
PICKUPS = {'bridge': 2, 'couple': 1, 'neck': 0}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def compare(reference, models):
    model_index = {row['label']: row['channels'][0] for row in models['recordings']}
    cells = []
    for row in reference['recordings']:
        if not row['label'].startswith('HB-'):
            continue
        pickup = row['label'].split('_')[0].removeprefix('HB-')
        real = row['channels'][0]
        cell = {'pickup': pickup, 'reference_file': row['file'], 'metrics': {}}
        for key in FIELDS:
            off = model_index[f'off-e2-pickup{PICKUPS[pickup]}']
            on = model_index[f'on-e2-pickup{PICKUPS[pickup]}']
            if not all(x['sufficient'] for x in (real, off, on)):
                raise ValueError('A required comparison cell is insufficient')
            baseline_error = abs(real[key] - off[key])
            candidate_error = abs(real[key] - on[key])
            cell['metrics'][key] = {
                'reference': real[key], 'baseline': off[key], 'candidate': on[key],
                'baseline_absolute_error': baseline_error,
                'candidate_absolute_error': candidate_error,
                'error_reduction_percent': 100 * (1 - candidate_error / baseline_error),
            }
        cells.append(cell)
    aggregate = {}
    for key in FIELDS:
        errors = [cell['metrics'][key] for cell in cells]
        aggregate[key] = {
            'median_baseline_absolute_error': float(np.median([x['baseline_absolute_error'] for x in errors])),
            'median_candidate_absolute_error': float(np.median([x['candidate_absolute_error'] for x in errors])),
            'maximum_baseline_absolute_error': max(x['baseline_absolute_error'] for x in errors),
            'maximum_candidate_absolute_error': max(x['candidate_absolute_error'] for x in errors),
            'every_cell_improves': all(x['candidate_absolute_error'] < x['baseline_absolute_error'] for x in errors),
        }
    return {'cells': cells, 'aggregate': aggregate,
            'scope': 'Descriptive unmatched ordinary E2 DI; no coefficient fit or holdout promotion claim'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('reference', type=Path)
    parser.add_argument('models', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = compare(json.loads(args.reference.read_text()), json.loads(args.models.read_text()))
    result['input_sha256'] = {str(p): digest(p) for p in (args.reference, args.models)}
    result['script_sha256'] = digest(__file__)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result['aggregate'], indent=2))
