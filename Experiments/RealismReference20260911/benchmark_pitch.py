#!/usr/bin/env python3
"""Run warmed, balanced pairs of two separately compiled pitch ablations."""
import argparse
import hashlib
import json
import statistics
import subprocess
from pathlib import Path


def measure(executable):
    run = subprocess.run([str(executable)], capture_output=True, text=True, check=True)
    return json.loads(run.stdout)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('off', type=Path)
    parser.add_argument('on', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    executables = {'off': args.off.resolve(), 'on': args.on.resolve()}
    for executable in executables.values():
        measure(executable)
    rounds = []
    for index in range(9):
        order = ['off', 'on', 'on', 'off'] if index % 2 == 0 else ['on', 'off', 'off', 'on']
        runs = [{'state': state, 'measurements': measure(executables[state])} for state in order]
        comparison = {}
        for score in ('held', 'repicked'):
            off = statistics.mean(x['measurements'][score]['seconds'] for x in runs if x['state'] == 'off')
            on = statistics.mean(x['measurements'][score]['seconds'] for x in runs if x['state'] == 'on')
            comparison[score] = {'off_seconds': off, 'on_seconds': on,
                                 'overhead_percent': 100 * (on / off - 1)}
        rounds.append({'order': order, 'runs': runs, 'comparison': comparison})
    result = {
        'scope': 'Isolated energy pitch flag; eight strings Both/Stereo at 96 kHz, four-second hold or 4 Hz chord repicks; no FX',
        'executable_sha256': {key: hashlib.sha256(path.read_bytes()).hexdigest() for key, path in executables.items()},
        'rounds': rounds,
        'median_paired_overhead_percent': {
            score: statistics.median(row['comparison'][score]['overhead_percent'] for row in rounds)
            for score in ('held', 'repicked')
        },
        'host_limitations': 'Concurrent work can add timing noise; warmed balanced order reduces drift. No worst-case scheduling guarantee.',
    }
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result['median_paired_overhead_percent'], indent=2))
