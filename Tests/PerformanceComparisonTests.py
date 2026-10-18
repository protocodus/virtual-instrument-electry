"""The production performance renderer must emit the promised public events.

These check score/timing integrity rather than pinning a sound snapshot or
deciding which engine sounds better. The DSP ownership tests own that contract.
"""
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

RENDERER = Path(sys.argv.pop(1)).resolve()


class PerformanceComparisonTests(unittest.TestCase):
    def render(self, probe):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        out = Path(temporary.name)
        subprocess.run([str(RENDERER), str(out), probe, "48000"], check=True, capture_output=True)
        receipt = json.loads((out / "render.json").read_text())
        data = (out / "raw.wav").read_bytes()
        self.assertEqual(data[:4], b"RIFF")
        self.assertEqual(data[8:12], b"WAVE")
        self.assertEqual(struct.unpack_from("<HHI", data, 20), (3, 2, 48000))
        self.assertEqual(len(data) - 44, receipt["frames"] * 8)
        self.assertEqual(receipt["block"], 64)
        self.assertEqual(receipt["initial_solo_key"], 24)
        self.assertFalse(receipt["fx_enabled"])
        samples = [value[0] for value in struct.iter_unpack("<f", data[44:])]
        self.assertTrue(all(math.isfinite(x) for x in samples))
        self.assertTrue(all(x == 0 for x in samples[:int(.17 * 48000) * 2]))
        self.assertGreater(max(abs(x) for x in samples), 1e-6)
        return receipt

    def test_exact_ABA_late_note_off_score_is_not_accidentally_simplified(self):
        receipt = self.render("aba-ownership")
        self.assertEqual(receipt["events"], [
            [9600, 0, 40, receipt["events"][0][3]],
            [19200, 0, 43, receipt["events"][0][3]],
            [28800, 0, 40, receipt["events"][0][3]],
            [31200, 1, 40, 0], [33600, 1, 43, 0], [45600, 1, 40, 0]])
        self.assertAlmostEqual(receipt["events"][0][3], .85, places=6)
        self.assertEqual(receipt["frames"], 96000)

    def test_repeats_have_all_four_termination_gaps_at_both_rates(self):
        for probe, interval in (("repeat-8hz", 6000), ("repeat-12hz", 4000)):
            with self.subTest(probe=probe):
                events = self.render(probe)["events"]
                ons = [e for e in events if e[1] == 0]
                offs = [e for e in events if e[1] == 1]
                self.assertEqual(len(ons), 24)
                self.assertEqual(len(offs), 24)
                self.assertEqual([e[0] for e in ons], [9600 + interval * n for n in range(24)])
                for n, (on, off) in enumerate(zip(ons, offs)):
                    self.assertEqual(off[0] - on[0], interval - (0, 96, 480, 1440)[n % 4])
                    self.assertEqual((on[2], off[2]), (40, 40))
                # At a zero gap the old off happens before the new on. The
                # separate ABA probe supplies the opposite ownership stress.
                at = [e[1] for e in events if e[0] == 9600 + interval]
                self.assertEqual(at, [1, 0])

    def test_alternating_overlap_and_palm_keep_public_one_string_setup(self):
        for probe in ("overlapping-string", "alternating-string", "rapid-palm"):
            with self.subTest(probe=probe):
                receipt = self.render(probe)
                self.assertEqual(receipt["physical_string_index"], 0)
                self.assertEqual(receipt["events"], sorted(receipt["events"], key=lambda e: e[0]))
                if probe == "rapid-palm":
                    self.assertEqual(len([e for e in receipt["events"] if e[1] == 2]), 24)
                    self.assertEqual(len([e for e in receipt["events"] if e[1] == 3 and e[3] == 1]), 6)

    def test_external_annotation_score_uses_given_frames_and_public_sixth_string(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        folder = Path(temporary.name)
        score = folder / "source.tsv"
        score.write_text("9600 0 59 1\n16800 0 60 0.8\n16992 1 59 0\n21600 1 60 0\n")
        args = [str(RENDERER), str(folder / "render"), "score:" + str(score), "48000", "48000", "2"]
        subprocess.run(args, check=True, capture_output=True)
        receipt = json.loads((folder / "render/render.json").read_text())
        self.assertEqual(receipt["initial_solo_key"], 26)
        self.assertEqual(receipt["physical_string_index"], 2)
        self.assertEqual([e[:3] for e in receipt["events"]], [[9600, 0, 59], [16800, 0, 60], [16992, 1, 59], [21600, 1, 60]])
        score.write_text("9600 0 59 1\n9000 1 59 0\n")
        self.assertNotEqual(subprocess.run(args, capture_output=True).returncode, 0)


if __name__ == "__main__":
    unittest.main()
