#!/usr/bin/env python3
"""Paired transition-heavy engine timing; generated binaries stay in build/."""
from __future__ import annotations
import argparse
import hashlib
import json
import platform
import statistics
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCES = ('Source/DSP/ElectryEngine.cpp', 'Source/DSP/ElectryEngine.h',
           'Source/DSP/ElectryFx.cpp', 'Source/DSP/ElectryFx.h',
           'Source/DSP/ElectryVisuals.cpp', 'Source/DSP/ElectryVisuals.h')
FLAGS = ['-std=c++20', '-O3', '-DNDEBUG', '-DELECTRY_DECOUPLED_PICK_RELEASE=1',
         '-DELECTRY_ENERGY_ATTACK_PITCH=1', '-DELECTRY_MEASURED_BODY_RESPONSE=1']

def hashes(root):
    return {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in SOURCES}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', type=Path, default=ROOT / 'build-transitions-20260911/baseline-src')
    parser.add_argument('--candidate', type=Path, default=ROOT)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build-transitions-20260911/performance')
    parser.add_argument('--rounds', type=int, default=7)
    parser.add_argument('--block-size', type=int, choices=(64, 128, 256, 512), default=256)
    parser.add_argument('--output', type=Path, default=HERE / 'results.json')
    args = parser.parse_args()
    args.build_dir.mkdir(parents=True, exist_ok=True)
    if args.rounds < 3:
        parser.error('At least three paired rounds are required')
    roots = {'baseline': args.baseline.resolve(), 'candidate': args.candidate.resolve()}
    sources_before = {label: hashes(root) for label, root in roots.items()}
    binaries = {}
    for label, root in roots.items():
        binary = args.build_dir / label
        command = ['clang++', *FLAGS, '-DELECTRY_PERFORMANCE_BLOCK=' + str(args.block_size), '-I' + str(root / 'Source'), str(HERE / 'Benchmark.cpp'),
                   *(str(root / name) for name in SOURCES if name.endswith('.cpp')),
                   '-o', str(binary)]
        subprocess.run(command, check=True)
        binaries[label] = binary
        if hashes(root) != sources_before[label]:
            raise RuntimeError(label + ' source changed during compilation')
    # Discard an entire paired warmup, including all scenario paths.
    for binary in binaries.values():
        subprocess.run([str(binary)], check=True, stdout=subprocess.DEVNULL)
    rounds = []
    for number in range(args.rounds):
        order = ('baseline', 'candidate') if number % 2 == 0 else ('candidate', 'baseline')
        pair = {'round': number, 'order': order, 'renders': {}}
        for label in order:
            pair['renders'][label] = json.loads(subprocess.check_output([str(binaries[label])]))
        rounds.append(pair)
        print('Finished paired round', number + 1, flush=True)
    summaries = []
    for scenario in [result['scenario'] for result in rounds[0]['renders']['baseline']]:
        records = {label: [next(item for item in pair['renders'][label] if item['scenario'] == scenario)
                           for pair in rounds] for label in binaries}
        fields = ('cpu_realtime_ratio', 'wall_seconds', 'render_cpu_seconds', 'event_cpu_seconds',
                  'moving_render_ratio', 'settled_render_ratio', 'callback_p50_ratio',
                  'callback_p95_ratio', 'callback_p99_ratio', 'callback_max_ratio')
        medians = {label: {field: statistics.median(item[field] for item in values) for field in fields}
                   for label, values in records.items()}
        paired_delta = [100 * (c['cpu_realtime_ratio'] / b['cpu_realtime_ratio'] - 1)
                        for b, c in zip(records['baseline'], records['candidate'])]
        summaries.append({'scenario': scenario, 'median': medians,
                          'paired_cpu_percent_change_median': statistics.median(paired_delta),
                          'paired_cpu_percent_change_range': [min(paired_delta), max(paired_delta)],
                          'all_eight_strings_active': all(item['minimum_voices'] == 8 for values in records.values() for item in values)})
    payload = {'format_version': 1, 'compiler': subprocess.check_output(['clang++', '--version'], text=True).splitlines()[0],
               'platform': platform.platform(), 'architecture': platform.machine(),
               'flags': FLAGS, 'sample_rate': 96000, 'block_size': args.block_size,
               'scenario_audio_seconds': 2, 'excluded_warmup_seconds': .2,
               'sources_sha256': sources_before, 'summary': summaries, 'rounds': rounds,
               'note': 'Engine-only thread CPU time includes event dispatch and inexpensive measurement overhead. Wall time reflects scheduling contention. Changes/second counts individual hand transitions, not complete open/mute cycles. First 80 ms after a transition is the moving interval. Callback percentiles omit only partial blocks split by an event; aggregate totals retain all samples.'}
    args.output.write_text(json.dumps(payload, indent=2) + '\n')
    for result in summaries:
        print(result['scenario'], result['paired_cpu_percent_change_median'], result['median']['candidate']['cpu_realtime_ratio'])

if __name__ == '__main__':
    main()
