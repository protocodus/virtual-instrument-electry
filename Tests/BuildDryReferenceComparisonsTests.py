"""Descriptive real/model pack integrity, independently of the DSP sound."""
import copy
import importlib.util
import json
import math
from pathlib import Path
import tempfile
import unittest

import numpy as np
from scipy.io import wavfile

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("dry_comparisons", root / "Tools/BuildDryReferenceComparisons.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


class DryComparisonTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.folder = Path(self.tmp.name)
        self.rate = 48000
        self.reference = np.zeros(48000, dtype=np.float32)
        t = np.arange(36000) / self.rate
        self.reference[9600:45600] = .2 * np.cos(2 * np.pi * 82.406889 * t) * np.exp(-3 * t)
        self.source = self.folder / "source.wav"
        wavfile.write(self.source, self.rate, self.reference)
        self.row = {"id": "test-e2", "title": "Independent synthetic test", "path": str(self.source),
                    "sha256": tool.sha(self.source), "channel": 0, "crop_start_frame": 4800,
                    "crop_frames": 40000, "onset_frame": 9600, "sounding_midi": 40,
                    "physical_string_index": 2, "pickup": "bridge", "style": "sustain",
                    "source": {"url": "https://example.test/fixture", "license": "test-only",
                               "attribution": "Synthetic unit fixture", "role": "development",
                               "limits": ["Synthetic data, not a real guitar or a rights receipt."]}}

    def model(self, name="A", **changes):
        folder = self.folder / name
        folder.mkdir()
        config = tool.render_configuration(self.row, self.rate, self.row["crop_frames"])
        audio = np.zeros(config["frames"], dtype=np.float32)
        t = np.arange(len(audio) - config["note_on_frame"]) / self.rate
        audio[config["note_on_frame"]:] = .12 * np.cos(2 * np.pi * 82.406889 * t) * np.exp(-2 * t)
        wavfile.write(folder / "raw.wav", self.rate, audio)
        receipt = {k: v for k, v in config.items() if k != "note_on_frame"}
        receipt.update(written_midi=52, fx_enabled=False, output_mode="mono",
                       events=[[0, "solo-string", 26], [0, "articulation", 3],
                               [config["note_on_frame"], "note-on-written", 52, config["velocity"]]],
                       render_seconds=1)
        receipt.update(changes)
        (folder / "render.json").write_text(json.dumps(receipt))
        return folder

    def test_original_channel_and_integer_crop_are_preserved(self):
        _, rate, clip, onset = tool.validate_reference(self.row, self.folder)
        self.assertEqual(rate, self.rate)
        np.testing.assert_array_equal(clip, self.reference[4800:44800])
        # A centred 2 ms energy window may lead the first nonzero frame by
        # at most half a window. It must preserve the original quiet prefix.
        self.assertGreaterEqual(onset, 4800 - 48)
        self.assertLessEqual(onset, 4800 + 48)
        self.assertTrue(np.all(clip[:4700] == 0))

    def test_bounds_hash_and_missing_source_provenance_fail(self):
        for key, value in [("crop_start_frame", -1), ("crop_frames", 100000), ("channel", 1),
                           ("onset_frame", 100000), ("sha256", "0" * 64)]:
            row = copy.deepcopy(self.row); row[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                tool.validate_reference(row, self.folder)
        for key in ("url", "license", "attribution", "role", "limits"):
            row = copy.deepcopy(self.row); del row["source"][key]
            with self.subTest(source=key), self.assertRaises(ValueError):
                tool.validate_reference(row, self.folder)
        row = copy.deepcopy(self.row); row["source"]["role"] = "unknown"
        with self.assertRaises(ValueError):
            tool.validate_reference(row, self.folder)
        row = copy.deepcopy(self.row); row["source"]["license"] = "unknown"
        with self.assertRaises(ValueError):
            tool.validate_reference(row, self.folder)

    def test_truncated_attack_and_insufficient_model_padding_fail(self):
        row = copy.deepcopy(self.row)
        row.update(crop_start_frame=9600, crop_frames=30000)
        with self.assertRaisesRegex(ValueError, "pre-onset"):
            tool.validate_reference(row, self.folder)
        model = self.model()
        audio = np.ones(self.row["crop_frames"] + self.rate, dtype=np.float32) * .1
        wavfile.write(model / "raw.wav", self.rate, audio)
        with self.assertRaisesRegex(ValueError, "padding"):
            tool.package_reference(self.row, self.folder, {"A": model}, self.folder / "listen/test-e2")

    def test_one_gain_and_shared_headroom_have_independent_known_answer(self):
        reference = np.zeros(1000); reference[200] = 1.
        model = np.full(1000, .25)
        dest = self.folder / "gain"
        facts, headroom = tool.match_and_write({"Reference": reference, "A": model}, self.rate, dest)
        expected_rms = .94 / math.sqrt(1000)
        self.assertLess(headroom, 1)
        self.assertAlmostEqual(10 ** (facts["Reference"]["trim_db"] / 20), .94, places=12)
        self.assertAlmostEqual(10 ** (facts["A"]["trim_db"] / 20), expected_rms / .25, places=12)
        for name, original in (("Reference", reference), ("A", model)):
            rate, decoded = tool.read_audio(dest / (name + ".wav"))
            expected = original * (.94 if name == "Reference" else expected_rms / .25)
            np.testing.assert_allclose(decoded[:, 0], expected, atol=2 / 8388607, rtol=0)
            self.assertEqual(rate, self.rate)
            self.assertAlmostEqual(facts[name]["matched_rms_dbfs"], 20 * math.log10(expected_rms), places=10)
            self.assertLessEqual(facts[name]["peak"], .94 + 1e-15)

    def test_real_A_B_share_shape_rate_onset_and_event_configuration(self):
        a, b = self.model("A"), self.model("B", render_seconds=99)
        result = tool.package_reference(self.row, self.folder, {"A": a, "B": b}, self.folder / "listen/test-e2")
        self.assertEqual(set(result["tracks"]), {"Reference", "A", "B"})
        for detail in result["tracks"].values():
            rate, audio = tool.read_audio(self.folder / "listen" / detail["path"])
            self.assertEqual((rate, audio.shape), (self.rate, (40000, 1)))
            self.assertTrue(np.isfinite(audio).all())
            self.assertEqual(detail["alignment"]["onset_frame_after_crop"], result["onset_frame"])
        receipt = json.loads((b / "render.json").read_text())
        receipt["events"][0][2] = 25
        (b / "render.json").write_text(json.dumps(receipt))
        with self.assertRaisesRegex(ValueError, "configuration differ"):
            tool.package_reference(self.row, self.folder, {"A": a, "B": b}, self.folder / "bad")

    def test_wrong_rate_stereo_nonfinite_and_unequal_tracks_fail(self):
        a = self.model()
        for rate, audio in [(44100, np.ones(88000, np.float32)),
                            (48000, np.ones((88000, 2), np.float32)),
                            (48000, np.full(88000, np.nan, np.float32))]:
            wavfile.write(a / "raw.wav", rate, audio)
            with self.subTest(rate=rate, shape=audio.shape), self.assertRaises(ValueError):
                tool.package_reference(self.row, self.folder, {"A": a}, self.folder / "bad")
        for b in (np.zeros(10), np.full(20, np.nan), np.ones(21)):
            with self.assertRaises(ValueError):
                tool.match_and_write({"Reference": np.ones(20), "A": b}, self.rate, self.folder / "bad")

    def test_candidate_attack_shift_is_preserved_by_shared_A_crop(self):
        a, b = self.model("A"), self.model("B")
        rate, audio = wavfile.read(b / "raw.wav")
        delayed = np.r_[np.zeros(20, np.float32), audio[:-20]]
        wavfile.write(b / "raw.wav", rate, delayed)
        result = tool.package_reference(self.row, self.folder, {"B": b, "A": a}, self.folder / "listen/test-e2")
        a_align = result["tracks"]["A"]["alignment"]
        b_align = result["tracks"]["B"]["alignment"]
        self.assertEqual(a_align["crop_start_frame"], b_align["crop_start_frame"])
        self.assertEqual(b_align["onset_frame_after_crop"] - a_align["onset_frame_after_crop"], 20)
        self.assertEqual(b_align["raw_onset_frame"] - a_align["raw_onset_frame"], 20)


if __name__ == "__main__":
    unittest.main()
