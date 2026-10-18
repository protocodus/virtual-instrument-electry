#!/usr/bin/env python3
"""Independent signal and receipt controls for the descriptive DI analyzer."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

import numpy as np
from scipy.io import wavfile

SPEC = importlib.util.spec_from_file_location(
    "dry_analysis", Path(__file__).resolve().parents[1] / "Tools/AnalyzeDryReferenceComparisons.py")
TOOL = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TOOL)


class DryReferenceAnalysisTests(unittest.TestCase):
    def test_tracks_stretched_partials_and_known_independent_decays(self):
        rate, onset = 8000, 1200
        time = (np.arange(24000) - onset) / rate
        active = time >= 0
        nominal = 82.40688923
        stiffness = 0.006
        t60s = [3.0, 1.4, 0.8, 0.55]
        signal = np.random.default_rng(821).normal(0.0, 2e-7, len(time))
        expected_frequencies = []
        for harmonic, t60 in enumerate(t60s, 1):
            hz = nominal * harmonic * np.sqrt((1 + stiffness * harmonic**2)
                                               / (1 + stiffness))
            expected_frequencies.append(hz)
            # Independent continuous-time amplitude decay: -60 dB at t60.
            signal[active] += (0.12 / harmonic
                               * 10 ** (-3 * time[active] / t60)
                               * np.sin(2 * np.pi * hz * time[active] + 0.17 * harmonic))
        partials = TOOL.tracked_partials(signal, rate, onset, nominal)
        for partial, frequency, t60 in zip(partials, expected_frequencies, t60s):
            self.assertEqual(partial["fit"]["status"], "linear")
            self.assertAlmostEqual(partial["median_frequency_hz"], frequency, delta=0.15)
            self.assertAlmostEqual(partial["fit"]["slope_db_per_s"], -60 / t60,
                                   delta=0.02 * 60 / t60)
            self.assertGreater(partial["fit"]["r_squared"], 0.995)
        # H4 lies >13 Hz above the equal-temperament bin; a fixed-bin test
        # cannot satisfy the frequency assertion above by following n*f0.
        self.assertGreater(expected_frequencies[3] - 4 * nominal, 13.0)

    def test_noise_does_not_become_a_modal_decay(self):
        noise = np.random.default_rng(1910).normal(0.0, 0.02, 24000)
        for partial in TOOL.tracked_partials(noise, 8000, 1200, 82.40688923):
            self.assertEqual(partial["fit"]["status"], "insufficient")
            self.assertIsNone(partial["fit"]["t60_s"])
        with self.assertRaisesRegex(ValueError, "silent"):
            TOOL.rms_onset(np.zeros(24000), 8000)

    def test_rejects_modified_listening_audio(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            wavfile.write(root / "Reference.wav", 8000, np.zeros(24000, dtype=np.int16))
            manifest = {"schema": "electry-dry-reference-comparisons/v1", "comparisons": [{
                "id": "control", "rate": 8000, "frames": 24000, "onset_frame": 1200,
                "reference": {"sounding_midi": 40},
                "tracks": {"Reference": {"path": "Reference.wav", "sha256": "0" * 64}}
            }]}
            (root / "measurements.json").write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, "hash mismatch"):
                TOOL.analyze_pack(root / "measurements.json", root / "analysis")


if __name__ == "__main__":
    unittest.main()
