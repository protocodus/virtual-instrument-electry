#!/usr/bin/env python3
"""Protocol guards for the asset-independent refret phrase observer."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

import numpy as np
from scipy.io import wavfile


spec = importlib.util.spec_from_file_location(
    'refret_analysis', Path(__file__).resolve().parents[1] / 'Tools/AnalyzeRefretComparisons.py')
analysis = importlib.util.module_from_spec(spec)
spec.loader.exec_module(analysis)


class RefretProtocolTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.rate = 48000
        count, source_start, model_start = 38400, 4800, 5000
        time = np.arange(count) / self.rate
        signal = (.03 * np.sin(2 * np.pi * 123.47 * time)
                  + .005 * np.sin(2 * np.pi * 987.76 * time)).astype(np.float32)
        wavfile.write(self.root / 'source.wav', self.rate,
                      np.pad(signal, (source_start, 2000)))
        (self.root / 'source.mid').write_bytes(b'synthetic timing identity fixture')
        events = [{'time_s': .2, 'type': 'on', 'note': 47, 'velocity': 100},
                  {'time_s': .3, 'type': 'on', 'note': 48, 'velocity': 100},
                  {'time_s': .31, 'type': 'off', 'note': 47, 'velocity': 0},
                  {'time_s': .5, 'type': 'on', 'note': 48, 'velocity': 100}]
        receipt = {'rate': self.rate, 'frames': model_start + count + 1000,
                   'physical_string_index': 2, 'initial_solo_key': 26,
                   'seed': 123, 'events': [[4800, 0, 59, .8], [9600, 0, 60, .8]]}
        tracks = {}
        receipts = {}
        for label in ('A', 'B'):
            folder = self.root / label
            folder.mkdir()
            clip = signal.copy()
            if label == 'B':
                # A known discontinuity inside the fixed second attack window
                # must remain visible even though the full crop is level matched.
                clip[9600] += .2
            raw = np.pad(clip, (model_start, 1000))
            wavfile.write(folder / 'raw.wav', self.rate, np.column_stack((raw, raw)))
            (folder / 'render.json').write_text(json.dumps(receipt))
            tracks[label] = {'raw_path': f'{label}/raw.wav',
                             'raw_sha256': analysis.sha(folder / 'raw.wav'),
                             'alignment': {'crop_start_frame': model_start}}
            receipts[label] = receipt.copy()
        row = {'id': 'phrase', 'rate': self.rate, 'frames': count,
               'reference': {'path': 'source.wav',
                             'sha256': analysis.sha(self.root / 'source.wav'),
                             'midi_path': 'source.mid',
                             'midi_sha256': analysis.sha(self.root / 'source.mid'),
                             'crop_start_frame': source_start, 'crop_frames': count,
                             'channel': 0, 'events': events},
               'tracks': tracks, 'model_render': receipts}
        self.manifest = {'schema': 'electry-dry-phrase-comparisons/v1',
                         'revisions': {'A': 'base', 'B': 'candidate'}, 'comparisons': [row]}
        self.path = self.root / 'measurements.json'
        self.write_manifest()

    def tearDown(self):
        self.temporary.cleanup()

    def write_manifest(self):
        self.path.write_text(json.dumps(self.manifest))

    def test_fixed_windows_expose_a_discontinuity_after_whole_crop_matching(self):
        result = analysis.analyze(self.path)['comparisons'][0]
        a, b = result['tracks']['A'], result['tracks']['B']
        attack = b['all_onsets'][1]['windows']['attack_-5_10ms']
        self.assertGreater(attack['ratio_to_A']['max_sample_step'], 10)
        self.assertGreater(attack['ratio_to_A']['high8k_rms'], 10)
        self.assertEqual(a['all_onsets'][1]['registered_frame'], 9600)
        self.assertEqual(a['all_onsets'][1]['windows']['attack_-5_10ms']['frames'],
                         attack['frames'])
        self.assertEqual(len(b['all_onsets']), 3)
        for onset in b['all_onsets']:
            for window in onset['windows'].values():
                self.assertAlmostEqual(window['matched']['rms'],
                                       window['raw']['rms'] * b['whole_crop_gain'])
        self.assertAlmostEqual(b['whole_crop_raw_rms'] * b['whole_crop_gain'],
                               10 ** (-24 / 20))

    def test_candidate_specific_alignment_is_rejected(self):
        self.manifest['comparisons'][0]['tracks']['B']['alignment']['crop_start_frame'] += 1
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, 'crops differ'):
            analysis.analyze(self.path)

    def test_plan_relative_sources_are_separate_from_output_relative_models(self):
        output = self.root / 'output'
        output.mkdir()
        row = self.manifest['comparisons'][0]
        row['reference_plan_directory'] = str(self.root)
        for label in ('A', 'B'):
            row['tracks'][label]['raw_path'] = f'../{label}/raw.wav'
        path = output / 'measurements.json'
        path.write_text(json.dumps(self.manifest))
        result = analysis.analyze(path)['comparisons'][0]
        self.assertEqual(result['tracks']['Reference']['source']['sha256'],
                         row['reference']['sha256'])
        self.assertEqual(result['midi_sha256'], row['reference']['midi_sha256'])
        self.assertEqual(result['tracks']['A']['source']['sha256'],
                         row['tracks']['A']['raw_sha256'])

    def test_unbound_model_audio_is_rejected(self):
        with (self.root / 'B/raw.wav').open('ab') as stream:
            stream.write(b'changed')
        with self.assertRaisesRegex(ValueError, 'Model raw WAV hash mismatch'):
            analysis.analyze(self.path)

    def test_unbound_annotation_is_rejected(self):
        (self.root / 'source.mid').write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'Source MIDI hash mismatch'):
            analysis.analyze(self.path)

    def test_counterexample_configuration_must_match(self):
        folder = self.root / 'extra/phrase'
        folder.mkdir(parents=True)
        (folder / 'raw.wav').write_bytes((self.root / 'B/raw.wav').read_bytes())
        receipt = self.manifest['comparisons'][0]['model_render']['B'].copy()
        receipt['seed'] += 1
        (folder / 'render.json').write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, 'score/configuration'):
            analysis.analyze(self.path, {'counterexample': self.root / 'extra'})

    def test_counterexample_may_omit_only_unused_post_crop_padding(self):
        folder = self.root / 'extra/phrase'
        folder.mkdir(parents=True)
        rate, raw = wavfile.read(self.root / 'B/raw.wav')
        wavfile.write(folder / 'raw.wav', rate, raw[:-500])
        receipt = self.manifest['comparisons'][0]['model_render']['B'].copy()
        receipt['frames'] -= 500
        (folder / 'render.json').write_text(json.dumps(receipt))
        result = analysis.analyze(self.path, {'counterexample': self.root / 'extra'})
        track = result['comparisons'][0]['tracks']['counterexample']
        self.assertEqual(track['all_onsets'], result['comparisons'][0]['tracks']['B']['all_onsets'])


if __name__ == '__main__':
    unittest.main()
