#!/usr/bin/env python3
"""Independent unit, imperial-physics and dimensional checks for the audit."""

from __future__ import annotations

import importlib.util
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "Tools/AuditStringConstruction.py"
SPEC = importlib.util.spec_from_file_location("string_construction_audit", TOOL)
assert SPEC and SPEC.loader
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class ConstructionAuditTests(unittest.TestCase):
    def test_exact_international_unit_fixture(self):
        # Independently evaluated 0.1 lb / 1000 inches, expressed in kg/m.
        self.assertAlmostEqual(AUDIT.unit_weight_to_kg_per_m(0.0001),
                               0.0017857967322834646, places=17)
        self.assertAlmostEqual(AUDIT.unit_weight_to_kg_per_m(1.0),
                               17.857967322834646, places=13)

    def test_mechanics_with_round_number_fixture(self):
        # A 10 g/m string, half a metre long, at 100 Hz: c=100 m/s.
        result = AUDIT.string_mechanics(0.01, 0.5, 100)
        self.assertEqual(result["tension_newtons"], 100)
        self.assertEqual(result["transverse_impedance_kg_per_s"], 1)
        longer = AUDIT.string_mechanics(0.01, 1.0, 100)
        self.assertEqual(longer["tension_newtons"], 400)
        self.assertEqual(longer["transverse_impedance_kg_per_s"], 2)
        heavier = AUDIT.string_mechanics(0.02, 0.5, 100)
        self.assertEqual(heavier["tension_newtons"], 200)
        self.assertEqual(AUDIT.fixed_core_b_ratio(0.01, 0.02), 0.5)

    def test_independent_imperial_tension_witnesses(self):
        # Independently evaluated calculator's imperial law for the snapshot:
        # these fixtures never call the tool's SI conversion. The calculator's
        # 386.088 in/s^2 is rounded, so compare to 5 micro-pounds, not bitwise.
        rows = AUDIT.build_report()["rows"]
        expected_lbf = {"PL009": 13.141138168660058,
                        "NW024": 15.725391455744033,
                        "NW042": 14.367602805805692}
        for sku, pounds in expected_lbf.items():
            row = next(r for r in rows if r["scale_inches"] == 25.5
                       and r["manufacturer_sku"] == sku)
            with self.subTest(sku=sku):
                self.assertAlmostEqual(row["published_mass"]["tension_newtons"]
                                       / 4.4482216152605, pounds, delta=0.00003)

    def test_complete_scope_and_named_construction(self):
        report = AUDIT.build_report()
        self.assertEqual(len(report["rows"]), 24)
        for scale in (25.5, 27.625, 28.0):
            group = [r for r in report["rows"] if r["scale_inches"] == scale]
            self.assertEqual([r["sounding_note"] for r in group],
                             ["E1", "B1", "E2", "A2", "D3", "G3", "B3", "E4"])
            self.assertEqual([r["manufacturer_sku"] for r in group],
                             ["NW080", "NW060", "NW042", "NW032", "NW024", "PL016", "PL011", "PL009"])
        self.assertEqual(report["source"]["snapshot_sha256"],
                         "855d638563bb07bcb156bb359a75ff3fdaf83a15d27cfa840b7950c675c10ee3")
        self.assertEqual(report["rows"][0]["source_subcomponent"], "U1AGFNP080")
        self.assertIn("not generic", " ".join(report["limits"]))
        self.assertIn("No heavy .098", " ".join(report["limits"]))

    def test_length_scaling_and_fixed_core_direction(self):
        rows = AUDIT.build_report()["rows"]
        for sku in ("NW080", "NW024", "PL009"):
            a, b = [next(r for r in rows if r["manufacturer_sku"] == sku
                         and r["scale_inches"] == scale) for scale in (25.5, 28.0)]
            for branch in ("proxy", "published_mass"):
                self.assertAlmostEqual(b[branch]["tension_newtons"] / a[branch]["tension_newtons"],
                                       1.205690119184929, places=13)
                self.assertAlmostEqual(b[branch]["transverse_impedance_kg_per_s"]
                                       / a[branch]["transverse_impedance_kg_per_s"],
                                       1.0980392156862746, places=13)
            self.assertGreater(a["fixed_core_B_change_percent"], 0)
            self.assertLess(a["published_mass_tension_impedance_change_percent"], 0)
            self.assertEqual(a["fixed_core_B_published_over_proxy_ratio"],
                             b["fixed_core_B_published_over_proxy_ratio"])

    def test_plain_controls_and_lowest_string_factual_mass(self):
        rows = AUDIT.build_report()["rows"][:8]
        # A distinct non-formula fixture for the .080 catalog conversion.
        self.assertAlmostEqual(rows[0]["published_mass"]["linear_mass_kg_per_m"],
                               0.019742987485561636, places=16)
        for row in rows[-3:]:
            self.assertAlmostEqual(row["published_over_proxy_mass_tension_impedance_ratio"],
                                   1.0, delta=0.006)
        self.assertAlmostEqual(rows[0]["published_mass_tension_impedance_change_percent"],
                               -8.759638106026413, places=11)

    def test_invalid_physical_inputs_fail(self):
        for bad in (0, -1, math.nan, math.inf):
            with self.subTest(value=bad):
                with self.assertRaises(ValueError):
                    AUDIT.unit_weight_to_kg_per_m(bad)
                with self.assertRaises(ValueError):
                    AUDIT.string_mechanics(0.01, bad, 100)
                with self.assertRaises(ValueError):
                    AUDIT.fixed_core_b_ratio(0.01, bad)

    def test_cli_stdout_and_explicit_file_are_identical(self):
        stdout = subprocess.run([sys.executable, str(TOOL)], check=True,
                                capture_output=True, text=True).stdout
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "audit.json"
            run = subprocess.run([sys.executable, str(TOOL), "--output", str(output)],
                                 check=True, capture_output=True, text=True)
            self.assertEqual(run.stdout, "")
            self.assertEqual(output.read_text(), stdout)
        self.assertEqual(json.loads(stdout), AUDIT.build_report())


if __name__ == "__main__":
    unittest.main()
