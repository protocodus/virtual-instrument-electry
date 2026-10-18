"""Closed-form poles and finite-difference derivative witnesses for the oracle."""
import cmath
import importlib.util
import math
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("decay_audit", Path(__file__).resolve().parents[1] / "Tools/AuditModalDecay.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def fixture(delay=100, ap=(0, 0), pole=0, gain=.99):
    return {"rate": 48000, "f0": 48000 / (delay + 8), "delay": delay,
            "ap": ap, "pole": pole, "gain": gain,
            "material": [1, 0, 0, 0, 0], "hand": [1, 0, 0, 0, 0]}


class ModalDecayTests(unittest.TestCase):
    def test_pure_integer_delay_poles_and_decay_are_exact(self):
        for gain in (.5, .99, .99999):
            row = fixture(gain=gain)
            for partial in (1, 4, 16):
                pole = audit.solve_pole(row, partial)
                self.assertAlmostEqual(pole["log_radius"], math.log(gain) / 108, places=14)
                self.assertAlmostEqual(pole["pole_frequency_hz"], partial * 48000 / 108, places=9)
                expected = -3 * 108 / (48000 * math.log10(gain))
                self.assertLess(abs(pole["t60_seconds"] / expected - 1), 1e-9)

    def test_complex_derivative_matches_symmetric_finite_difference(self):
        row = fixture(delay=100.375, ap=(-.4, -.93), pole=.7)
        row["material"] = [0.8, -0.2, .04, -.6, .12]
        for s in (complex(-.0001, .02), complex(-.001, .2), complex(-.0005, .7)):
            h, derivative = audit.transfer_and_log_derivative(row, s)
            eps = 1e-7
            plus = audit.transfer_and_log_derivative(row, s + eps)[0]
            minus = audit.transfer_and_log_derivative(row, s - eps)[0]
            self.assertLess(abs((plus - minus) / (2 * eps) - h * derivative) / abs(h * derivative), 1e-8)

    def test_unit_circle_transfer_matches_independent_phase_oracle(self):
        row = fixture(delay=100.375, ap=(-.4, -.93), pole=.7)
        for omega in (.01, .1, .4, 1.0):
            h, _ = audit.transfer_and_log_derivative(row, complex(0, omega))
            phase, magnitude = audit.dispersion.loop_phase_and_magnitude(row, omega)
            self.assertLess(abs(h - cmath.rect(magnitude, phase)), 1e-12)

    def test_nondecaying_loop_is_rejected(self):
        for gain in (1.0, 1.1):
            with self.assertRaises(ValueError):
                audit.solve_pole(fixture(gain=gain), 1)


if __name__ == "__main__":
    unittest.main()
