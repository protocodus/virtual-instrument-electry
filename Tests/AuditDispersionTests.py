#!/usr/bin/env python3
"""Independent analytical checks for the modal phase oracle."""

import cmath
import importlib.util
import math
import pathlib
import unittest

path = pathlib.Path(__file__).resolve().parents[1] / "Tools/AuditDispersion.py"
spec = importlib.util.spec_from_file_location("audit_dispersion", path)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def observation(delay=100, ap=(0, 0), pole=0):
    # Each of the eight c=0 allpasses is exactly one sample of delay.
    return {"rate": 48000, "f0": 48000 / (delay + 8), "B": 0,
            "delay": delay, "ap": ap, "pole": pole, "gain": .99,
            "material": [1, 0, 0, 0, 0], "hand": [1, 0, 0, 0, 0]}


class DispersionAuditTests(unittest.TestCase):
    def test_integer_delay_modes_are_exact_and_attenuation_cannot_detune(self):
        row = observation()
        for gain in (.001, .5, .99999):
            row["gain"] = gain
            for n in (1, 2, 8, 16):
                self.assertAlmostEqual(audit.modal_frequency(row, n), n * row["f0"], places=8)

    def test_cubic_response_matches_interpolation_of_a_complex_sinusoid(self):
        # Read a synthetic travelling sinusoid using independent Lagrange
        # basis products, at several arbitrary fractional delays/frequencies.
        for delay in (10, 10.125, 10.5, 10.875):
            for omega in (.01, .4, 1.2):
                x = -delay
                integer = math.floor(x)
                points = [integer - 1, integer, integer + 1, integer + 2]
                direct = 0j
                for j, position in enumerate(points):
                    basis = math.prod((x - other) / (position - other)
                                      for k, other in enumerate(points) if j != k)
                    direct += basis * cmath.exp(1j * omega * position)
                phase, magnitude = audit.cubic_read_response(delay, omega)
                self.assertAlmostEqual(abs(direct - cmath.rect(magnitude, phase)), 0, places=12)

    def test_complete_filter_phase_matches_complex_transfer_at_solved_mode(self):
        row = observation(delay=100.25, ap=(-.4, -.85), pole=.7)
        for n in (1, 2, 4, 8, 16):
            frequency = audit.modal_frequency(row, n)
            omega = 2 * math.pi * frequency / row["rate"]
            delay_phase, delay_magnitude = audit.cubic_read_response(row["delay"], omega)
            z = cmath.exp(-1j * omega)
            transfer = cmath.rect(delay_magnitude, delay_phase)
            for coefficient in row["ap"]:
                transfer *= ((coefficient + z) / (1 + coefficient * z)) ** 4
            transfer *= (1 - row["pole"]) / (1 - row["pole"] * z)
            transfer *= row["gain"]
            self.assertAlmostEqual(cmath.phase(transfer), 0, places=10)
            phase, magnitude = audit.loop_phase_and_magnitude(row, omega)
            self.assertAlmostEqual(phase, -2 * math.pi * n, places=10)
            self.assertAlmostEqual(magnitude, abs(transfer), places=12)

    def test_dispersion_only_fundamental_is_independently_phase_compensated(self):
        row = observation(delay=80.25, ap=(-.3, -.9), pole=.85)
        row["f0"] = 251.25
        self.assertAlmostEqual(audit.modal_frequency(row, 1, True), row["f0"], places=8)

    def test_modes_above_nyquist_are_not_fabricated(self):
        self.assertIsNone(audit.modal_frequency(observation(delay=4), 7))


if __name__ == "__main__":
    unittest.main()
