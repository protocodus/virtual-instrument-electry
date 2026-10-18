#!/usr/bin/env python3
"""Protocol guards for the asset-independent refret phrase observer."""
import importlib.util
import copy
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
        count, source_start, model_start = 38400, 4800, 7200
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
        # Frozen rows are explicit fixture data, independent of the analyzer's
        # derivation. Receipts use float32 velocity as the production renderer
        # does, while the text retains the source velocity ratio's precision.
        velocity = float(np.float32(100 / 127))
        score = [[12000, 0, 59, velocity], [16800, 0, 60, velocity],
                 [17280, 1, 59, 0], [26400, 0, 60, velocity]]
        (self.root / 'score.tsv').write_text(
            '# frame kind written-MIDI value\n'
            '12000\t0\t59\t0.787401574803\n'
            '16800\t0\t60\t0.787401574803\n'
            '17280\t1\t59\t0\n'
            '26400\t0\t60\t0.787401574803\n')
        receipt = {'rate': self.rate, 'frames': model_start + count + 1000,
                   'physical_string_index': 2, 'initial_solo_key': 26,
                   'seed': 123, 'events': score}
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
            receipts[label] = copy.deepcopy(receipt)
        row = {'id': 'phrase', 'rate': self.rate, 'frames': count,
               'reference': {'path': 'source.wav',
                             'sha256': analysis.sha(self.root / 'source.wav'),
                             'midi_path': 'source.mid',
                             'midi_sha256': analysis.sha(self.root / 'source.mid'),
                             'crop_start_frame': source_start, 'crop_frames': count,
                             'channel': 0, 'events': events},
               'tracks': tracks, 'model_render': receipts,
               'score_path': 'score.tsv',
               'score_sha256': analysis.sha(self.root / 'score.tsv'),
               'alignment_anchor': {'latency_frames': 0, 'rate': self.rate},
               'source_onset_prediction': {'expected_frame_after_crop': 4800}}
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

    def test_source_release_velocity_is_not_a_rendered_noteoff_value(self):
        self.manifest['comparisons'][0]['reference']['events'][2]['velocity'] = 64
        self.write_manifest()
        result = analysis.analyze(self.path)['comparisons'][0]
        self.assertEqual(len(result['tracks']['B']['all_onsets']), 3)

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
        row['score_path'] = '../score.tsv'
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

    def assert_manifest_annotation_rejected(self, transform, message):
        row = self.manifest['comparisons'][0]
        frozen = {key: copy.deepcopy(row[key])
                  for key in ['tracks', 'model_render', 'score_path', 'score_sha256',
                              'alignment_anchor', 'source_onset_prediction']}
        wav_sha = analysis.sha(self.root / 'source.wav')
        midi_sha = analysis.sha(self.root / 'source.mid')
        transform(row['reference']['events'])
        self.write_manifest()
        self.assertEqual({key: row[key] for key in frozen}, frozen)
        self.assertEqual(analysis.sha(self.root / 'source.wav'), wav_sha)
        self.assertEqual(analysis.sha(self.root / 'source.mid'), midi_sha)
        with self.assertRaisesRegex(ValueError, message):
            analysis.analyze(self.path)

    def test_changed_note_time_cannot_move_the_registered_window(self):
        self.assert_manifest_annotation_rejected(
            lambda events: events[1].update(time_s=.305), 'frozen score TSV')

    def test_changed_note_pitch_cannot_relabel_the_recording(self):
        self.assert_manifest_annotation_rejected(
            lambda events: events[1].update(note=49), 'frozen score TSV')

    def test_changed_note_velocity_cannot_relabel_the_recording(self):
        self.assert_manifest_annotation_rejected(
            lambda events: events[1].update(velocity=101), 'frozen score TSV')

    def test_changed_noteoff_is_bound_as_well_as_noteons(self):
        self.assert_manifest_annotation_rejected(
            lambda events: events[2].update(time_s=.315), 'frozen score TSV')

    def test_changed_first_onset_is_rejected(self):
        self.assert_manifest_annotation_rejected(
            lambda events: events[0].update(time_s=.205), 'frozen score TSV')

    def test_global_annotation_shift_cannot_change_the_absolute_crop_origin(self):
        def shift(events):
            for event in events:
                event['time_s'] += .001
        self.assert_manifest_annotation_rejected(shift, 'frozen alignment anchor')

    def test_source_score_bytes_must_match_the_frozen_hash(self):
        with (self.root / 'score.tsv').open('a') as stream:
            stream.write('# changed\n')
        with self.assertRaisesRegex(ValueError, 'score TSV hash mismatch'):
            analysis.analyze(self.path)

    def test_source_score_hash_alone_cannot_bind_different_numeric_rows(self):
        path = self.root / 'score.tsv'
        path.write_text(path.read_text().replace('16800\t0\t60', '16801\t0\t60'))
        self.manifest['comparisons'][0]['score_sha256'] = analysis.sha(path)
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, 'frozen score TSV'):
            analysis.analyze(self.path)

    def test_source_score_must_include_all_numeric_rows_without_extras(self):
        path = self.root / 'score.tsv'
        original = path.read_text()
        for changed in (original.replace('17280\t1\t59\t0\n', ''),
                        original + '26400\t0\t60\t0.787401574803\n'):
            with self.subTest(score=changed):
                path.write_text(changed)
                self.manifest['comparisons'][0]['score_sha256'] = analysis.sha(path)
                self.write_manifest()
                with self.assertRaisesRegex(ValueError, 'frozen score TSV'):
                    analysis.analyze(self.path)

    def test_identical_model_receipts_cannot_override_the_frozen_source_score(self):
        for label in ('A', 'B'):
            receipt = self.manifest['comparisons'][0]['model_render'][label]
            receipt['events'][1][2] += 1
            (self.root / label / 'render.json').write_text(json.dumps(receipt))
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, 'model render score events'):
            analysis.analyze(self.path)

    def test_each_receipt_must_bind_every_source_event(self):
        for label in ('A', 'B'):
            with self.subTest(label=label):
                original = copy.deepcopy(self.manifest['comparisons'][0]['model_render'][label])
                receipt = copy.deepcopy(original)
                receipt['events'][2][0] += 1
                path = self.root / label / 'render.json'
                path.write_text(json.dumps(receipt))
                self.manifest['comparisons'][0]['model_render'][label] = receipt
                self.write_manifest()
                with self.assertRaisesRegex(ValueError, 'model render score events'):
                    analysis.analyze(self.path)
                path.write_text(json.dumps(original))
                self.manifest['comparisons'][0]['model_render'][label] = original
                self.write_manifest()

    def test_extra_receipt_events_are_also_independently_bound(self):
        folder = self.root / 'extra/phrase'
        folder.mkdir(parents=True)
        (folder / 'raw.wav').write_bytes((self.root / 'B/raw.wav').read_bytes())
        receipt = copy.deepcopy(self.manifest['comparisons'][0]['model_render']['B'])
        receipt['events'][1][2] += 1
        (folder / 'render.json').write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, 'model render score events'):
            analysis.analyze(self.path, {'counterexample': self.root / 'extra'})

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
