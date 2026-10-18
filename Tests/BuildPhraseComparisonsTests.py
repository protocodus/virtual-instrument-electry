"""Preserve annotated overlaps and a development-only timing anchor."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

import numpy as np
from scipy.io import wavfile

spec = importlib.util.spec_from_file_location("phrases", Path(__file__).resolve().parents[1] / "Tools/BuildPhraseComparisons.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class PhraseComparisonTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.folder = Path(tmp.name)

    def test_known_overlap_frames_written_octave_and_final_held_note(self):
        row = {"events": [{"time_s": t, "type": kind, "note": note, "velocity": vel} for t, kind, note, vel in [
            (10.2, "on", 47, 127), (10.3, "on", 48, 127), (10.304, "off", 47, 0),
            (10.4, "off", 48, 0), (10.5, "on", 49, 105), (10.55, "off", 49, 0), (10.8, "on", 49, 121)]]}
        events = tool.score_events(row, 48000, self.folder / "score.tsv")
        self.assertEqual([e[:3] for e in events], [(12000, 0, 59), (16800, 0, 60), (16992, 1, 59),
                                                  (21600, 1, 60), (26400, 0, 61), (28800, 1, 61), (40800, 0, 61)])
        self.assertAlmostEqual(events[4][3], 105 / 127)
        self.assertEqual(events[-1][1], 0)  # No fabricated end-of-crop NoteOff.

    def test_repeat_uses_shared_development_latency_and_preserves_source_residual(self):
        rate = 48000
        burst = (.2 * np.cos(2 * np.pi * 100 * np.arange(4000) / rate)).astype(np.float32)
        source = np.zeros(96000, np.float32)
        source[24000:28000] = burst
        source[72023:76023] = burst  # Known +23-frame repeat timing residual.
        path = self.folder / "source.wav"; wavfile.write(path, rate, source)
        base = {"id": "anchor", "title": "Synthetic", "path": str(path), "sha256": tool.dry.sha(path),
                "channel": 0, "crop_start_frame": 20000, "crop_frames": 20000, "onset_frame": 24000,
                "physical_string_index": 2, "pickup": "bridge", "style": "sustain",
                "alignment_group": "fixed", "alignment_role": "development-anchor", "midi_to_audio_offset_s": 0,
                "source": {"url": "https://example.test", "license": "test-only", "attribution": "Synthetic",
                           "role": "development", "limits": ["Synthetic, not a real guitar."]},
                "events": [{"time_s": .5, "type": "on", "note": 47, "velocity": 127},
                           {"time_s": .6, "type": "off", "note": 47, "velocity": 0}]}
        repeat = copy.deepcopy(base)
        repeat.update(id="repeat", crop_start_frame=68000, onset_frame=72023, alignment_role="repeat-check")
        repeat["events"][0]["time_s"] = 1.5; repeat["events"][1]["time_s"] = 1.6
        anchors = {}
        for row in (base, repeat):
            score = self.folder / (row["id"] + ".tsv")
            events = tool.score_events(row, rate, score)
            models = {}
            for letter, shift in (("A", 0), ("B", 20)):
                folder = self.folder / (row["id"] + letter); folder.mkdir()
                samples = np.zeros(68000, np.float32); samples[12000 + shift:16000 + shift] = burst
                wavfile.write(folder / "raw.wav", rate, np.column_stack([samples, samples]))
                receipt = {"rate": rate, "frames": 68000, "physical_string_index": 2, "initial_solo_key": 26,
                           "events": events, "render_seconds": 1}
                (folder / "render.json").write_text(json.dumps(receipt))
                models[letter] = folder
            result = tool.package(row, self.folder, models, self.folder / "listen" / row["id"], score, events, anchors)
            real = result["tracks"]["Reference"]["alignment"]["onset_frame_after_crop"]
            a = result["tracks"]["A"]["alignment"]
            b = result["tracks"]["B"]["alignment"]
            self.assertEqual(a["onset_frame_after_crop"] - real, 0 if row is base else -23)
            self.assertEqual(b["onset_frame_after_crop"] - a["onset_frame_after_crop"], 20)
            self.assertEqual(a["crop_start_frame"], b["crop_start_frame"])


if __name__ == "__main__":
    unittest.main()
