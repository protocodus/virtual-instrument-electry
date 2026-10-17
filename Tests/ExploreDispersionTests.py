#!/usr/bin/env python3
"""Analytical guards for the offline conjugate-pole feasibility experiment."""
import cmath
import importlib.util
import math
import pathlib
import unittest

import numpy as np

path = pathlib.Path(__file__).resolve().parents[1] / "Tools/ExploreDispersion.py"
spec = importlib.util.spec_from_file_location("explore_dispersion", path)
explore = importlib.util.module_from_spec(spec)
spec.loader.exec_module(explore)


class ComplexDispersionTests(unittest.TestCase):
    def test_zero_radius_pairs_are_pure_delays_and_preserve_harmonic_roots(self):
        row = {"rate": 48000, "f0": 1000, "B": 0}
        model = explore.ConjugateAllpassModel(row, 2)
        coordinates = np.array([6, 6, 2, 4])
        self.assertTrue(np.allclose(model.roots(coordinates), model.omega0 * model.orders,
                                    atol=1e-12, rtol=0))

    def test_conjugate_transfer_is_allpass_and_roots_have_correct_global_index(self):
        row = {"rate": 96000, "f0": 82.4069, "B": .00065}
        model = explore.ConjugateAllpassModel(row, 4)
        coordinates = np.array([1.6, 1.5, 1.4, 1.4, 1.4, 4.6, 8.2, 12.6])
        radius, theta = model.parameters(coordinates)
        for n, omega in zip(model.orders, model.roots(coordinates)):
            z = cmath.exp(-1j * omega)
            transfer = cmath.exp(-1j * omega * model.raw_delay(coordinates))
            for r, t in zip(radius, theta):
                transfer *= (r*r - 2*r*math.cos(t)*z + z*z) / (
                    1 - 2*r*math.cos(t)*z + r*r*z*z)
            self.assertAlmostEqual(abs(transfer), 1, places=9)
            self.assertAlmostEqual(cmath.phase(transfer), 0, places=9)
            self.assertAlmostEqual(model.accumulated_phase(omega, coordinates) / (2 * math.pi), n, places=9)
        self.assertTrue(model.validate(coordinates)["all"])

    def test_group_delay_matches_phase_derivative_and_stays_positive(self):
        model = explore.ConjugateAllpassModel({"rate": 48000, "f0": 500, "B": .001}, 2)
        coordinates = np.array([1.4, 1.9, 2, 6])
        for omega in (.01, .1, .5, 1, 2.5):
            h = 1e-6
            numerical = (model.accumulated_phase(omega + h, coordinates)
                - model.accumulated_phase(omega - h, coordinates)) / (2 * h)
            self.assertAlmostEqual(numerical, model.group_delay(omega, coordinates), places=6)
            self.assertGreater(numerical, 0)

    def test_noncausal_compensated_delay_is_rejected(self):
        model = explore.ConjugateAllpassModel({"rate": 48000, "f0": 1000, "B": .001}, 4)
        coordinates = np.array([-3] * 4 + [0] * 4)
        with self.assertRaisesRegex(ValueError, "four raw delay"):
            model.roots(coordinates)

    def test_missing_high_roots_are_rejected_instead_of_reindexed(self):
        model = explore.ConjugateAllpassModel({"rate": 48000, "f0": 1000, "B": 0}, 2)
        with self.assertRaisesRegex(ValueError, "above Nyquist"):
            model.roots(np.array([6, 6, 2, 4]), np.array([100.0]))


if __name__ == "__main__":
    unittest.main()
