#!/usr/bin/env python3
"""Export ordinary DI partial trajectories alongside the isolated ablation."""
import argparse
import json
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def trace(channel):
    parts = [p for p in channel['partials'] if p.get('sufficient')]
    times = np.array([x['seconds'] for x in parts[0]['trace']])
    values = np.array([[x['cents'] for x in p['trace']] for p in parts])
    return times, np.median(values, axis=0)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('reference', type=Path)
    parser.add_argument('model', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    real = json.loads(args.reference.read_text())['recordings']
    models = {r['label']: r['channels'][0] for r in json.loads(args.model.read_text())['recordings']}
    fig, axes = plt.subplots(1, 3, figsize=(11, 3.5), sharex=True, sharey=True)
    for axis, (pickup, index) in zip(axes, [('bridge', 2), ('couple', 1), ('neck', 0)]):
        source = next(r['channels'][0] for r in real if r['label'].startswith(f'HB-{pickup}_'))
        for channel, label, color, style in [
            (source, 'Real ordinary DI', '#246e96', '-'),
            (models[f'off-e2-pickup{index}'], 'Current baseline', '#777777', '--'),
            (models[f'on-e2-pickup{index}'], 'Bounded pitch candidate', '#c05724', '-'),
        ]:
            t, c = trace(channel)
            axis.plot(t * 1000, c, label=label, color=color, linewidth=1.9, linestyle=style)
        axis.axhline(0, color='#aaaaaa', linewidth=.7)
        axis.axvspan(160, 240, color='#555555', alpha=.08)
        axis.axvspan(600, 800, color='#555555', alpha=.08)
        axis.set_title({'bridge': 'Bridge', 'couple': 'Both pickups', 'neck': 'Neck'}[pickup])
        axis.set_xlabel('Time after detected onset (ms)')
        axis.grid(alpha=.15)
        axis.spines[['top', 'right']].set_visible(False)
    axes[0].set_ylabel('Median partial frequency offset (cents)')
    axes[0].set_ylim(-4, 12)
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc='lower center', ncol=3, frameon=False)
    fig.suptitle('Ordinary E2: each partial referred to its own 600–800 ms frequency', fontsize=12)
    fig.tight_layout(rect=(0, .08, 1, .94))
    fig.savefig(args.output, dpi=180, facecolor='white')
    print(args.output)
