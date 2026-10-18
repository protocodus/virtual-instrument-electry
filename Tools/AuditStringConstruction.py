#!/usr/bin/env python3
"""Audit Electry's light-gauge mass proxy against current PL/XL NW singles.

This is an offline arithmetic report, not a shipping-model calibration. Source:
D'Addario's current String Tension Pro page embeds its calculator at
https://embed.cartfulsolutions.com/daddario-string-tension-finder/ . Its
data/strings_sample.json contains the Mass_UL literals below; its bundled
JavaScript uses UW*(2*scaleInches*frequencyHz)^2/386.088 to produce pounds-force,
establishing lb/in as the mass units. Snapshot hashes are in the report.

The eight unit weights below are transcribed lb/in values, not estimates from
diameter. Selected PL/NW singles match Electry's light nominal gauges; they do
not assert that this exact Drop-E combination is a packaged D'Addario set. NW
means this named XL nickel-plated-steel round-wound family, not NYXL or a
generic wound string. No audio, web requests, downloads or engine edits occur.

At fixed L and f, T = mu*(2*L*f)^2 and transverse Z = mu*(2*L*f). With flexural
rigidity EI unchanged, B is proportional to 1/(T*L^2), so B_new/B_old =
mu_old/mu_new. That ratio is a sensitivity, not an identified bending/axial
core or an absolute measured B. No inference extends to the heavy .098 set.
Historical tension_chart_13934.pdf data differ from this snapshot and are not
mixed into it; that legacy manufacturer URL now redirects to the calculator.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

INCH_METRES = 0.0254  # Exact international inch.
POUND_KILOGRAMS = 0.45359237  # Exact international avoirdupois pound.
STEEL_DENSITY_KG_M3 = 7850.0  # Current Electry construction proxy.
SCALE_LENGTHS_INCHES = (25.5, 27.625, 28.0)
SOURCE_URL = "https://embed.cartfulsolutions.com/daddario-string-tension-finder/data/strings_sample.json"
SOURCE_PAGE_URL = "https://www.daddario.com/pages/string-tension-pro-string-tension-calculator"
SOURCE_SHA256 = "855d638563bb07bcb156bb359a75ff3fdaf83a15d27cfa840b7950c675c10ee3"
CALCULATOR_JS_URL = "https://embed.cartfulsolutions.com/daddario-string-tension-finder/recommendation.min.js"
CALCULATOR_JS_SHA256 = "3df58f74c9c2f3609f271050baeab3a6d4529431133a5d42f67aa20a60f17442"

# string number, sounding note, sounding MIDI, nominal inches, manufacturer SKU,
# published unit weight in lb/in, wound. These are factual source literals;
# keep their provenance beside them and do not fit an interpolation to .098.
STRINGS = (
    (8, "E1", 28, 0.080, "NW080", 0.0011055562555720800, True),
    (7, "B1", 35, 0.060, "NW060", 0.0006414349593603170, True),
    (6, "E2", 40, 0.042, "NW042", 0.0003140532008355160, True),
    (5, "A2", 45, 0.032, "NW032", 0.0001902095534263840, True),
    (4, "D3", 50, 0.024, "NW024", 0.0001082688980358360, True),
    (3, "G3", 55, 0.016, "PL016", 0.0000567396765979545, False),
    (2, "B3", 59, 0.011, "PL011", 0.0000268183627670019, False),
    (1, "E4", 64, 0.009, "PL009", 0.0000179527882985715, False),
)


def _positive(value: float, name: str) -> float:
    if not math.isfinite(value) or value <= 0:
        raise ValueError(f"{name} must be finite and positive")
    return value


def unit_weight_to_kg_per_m(lb_per_inch: float) -> float:
    """Convert catalog unit mass (lb/in) to SI mass per length (kg/m)."""
    return _positive(lb_per_inch, "unit weight") * POUND_KILOGRAMS / INCH_METRES


def proxy_mass_kg_per_m(diameter_inches: float, wound: bool) -> float:
    diameter_metres = _positive(diameter_inches, "diameter") * INCH_METRES
    return (0.85 if wound else 1.0) * STEEL_DENSITY_KG_M3 * math.pi \
        * diameter_metres * diameter_metres / 4.0


def string_mechanics(mu: float, length_metres: float, frequency_hz: float) -> dict:
    """Ideal transverse-string identities; Z is force/transverse velocity."""
    _positive(mu, "linear mass")
    speed = 2 * _positive(length_metres, "length") * _positive(frequency_hz, "frequency")
    return {"linear_mass_kg_per_m": mu,
            "tension_newtons": mu * speed * speed,
            "transverse_impedance_kg_per_s": mu * speed}


def fixed_core_b_ratio(old_mass: float, new_mass: float) -> float:
    return _positive(old_mass, "old mass") / _positive(new_mass, "new mass")


def build_report() -> dict:
    rows = []
    for scale_inches in SCALE_LENGTHS_INCHES:
        for number, note, midi, gauge, sku, unit_weight, wound in STRINGS:
            length = scale_inches * INCH_METRES
            frequency = 440.0 * 2.0 ** ((midi - 69) / 12.0)
            proxy_mu = proxy_mass_kg_per_m(gauge, wound)
            published_mu = unit_weight_to_kg_per_m(unit_weight)
            mass_ratio = published_mu / proxy_mu
            b_ratio = fixed_core_b_ratio(proxy_mu, published_mu)
            rows.append({
                "scale_inches": scale_inches, "scale_metres": length,
                "string_number": number, "sounding_note": note,
                "sounding_midi": midi, "frequency_hz": frequency,
                "gauge_inches": gauge, "manufacturer_sku": sku,
                "source_subcomponent": "U1AGFNP" + sku[2:] if wound else "U1AGF" + sku,
                "construction": "XL nickel-plated steel round wound" if wound
                                else "plain tin-coated steel",
                "source_unit_weight_lb_per_inch": unit_weight,
                "proxy": string_mechanics(proxy_mu, length, frequency),
                "published_mass": string_mechanics(published_mu, length, frequency),
                "published_over_proxy_mass_tension_impedance_ratio": mass_ratio,
                "published_mass_tension_impedance_change_percent": 100 * (mass_ratio - 1),
                "fixed_core_B_published_over_proxy_ratio": b_ratio,
                "fixed_core_B_change_percent": 100 * (b_ratio - 1),
            })
    return {
        "schema": "electry-string-construction-audit/v1",
        "purpose": "Offline sensitivity audit; shipping engine unchanged",
        "source": {
            "publisher": "D'Addario",
            "title": "String Tension Pro calculator: strings_sample.json",
            "url": SOURCE_URL, "manufacturer_page": SOURCE_PAGE_URL,
            "snapshot_sha256": SOURCE_SHA256,
            "verified_date": "2026-10-17",
            "calculator_javascript_url": CALCULATOR_JS_URL,
            "calculator_javascript_sha256": CALCULATOR_JS_SHA256,
            "row_selection": "PackagedSingle PL/NW SKU plus the exact U1AGF subcomponent in each row",
            "unit_evidence": "Calculator JavaScript: UW*(2*scaleInches*frequencyHz)^2/386.088 returns lbf",
            "data_kind": "Source-controlled transcription of published unit weights in lb/in",
            "historical_pdf": "https://www.daddario.com/globalassets/pdfs/accessories/tension_chart_13934.pdf",
            "historical_pdf_note": "Legacy URL redirects; its indexed values differ and are not used",
            "access_note": "This tool uses no network and retains the verified snapshot literals",
        },
        "model": {
            "tuning": "Drop-E: E1 B1 E2 A2 D3 G3 B3 E4 (sounding)",
            "gauge_coordinate": "Light endpoint only; .009 .011 .016 .024 .032 .042 .060 .080",
            "proxy_density_kg_per_m3": STEEL_DENSITY_KG_M3,
            "proxy_wound_cylinder_factor": 0.85,
            "proxy_plain_cylinder_factor": 1.0,
            "mass_formula": "mu_proxy = factor * rho * pi * diameter^2 / 4",
            "tension_formula": "T = mu * (2 * L * f)^2",
            "impedance_formula": "Z = mu * 2 * L * f = sqrt(T * mu)",
            "B_ratio_assumption": "At unchanged EI, L and f: B_published/B_proxy = mu_proxy/mu_published",
            "percentage_denominator": "Current proxy; positive means published-based quantity is greater",
            "constants": {"inch_metres": INCH_METRES, "pound_kilograms": POUND_KILOGRAMS,
                          "concert_A4_hz": 440.0},
        },
        "limits": [
            "Named D'Addario PL/XL NW singles, not generic wound or NYXL construction",
            "Selected singles matching nominal light gauges, not a claimed packaged eight-string set",
            "Published total linear mass does not identify bending core, axial core, EI, winding slip or compliance",
            "Fixed-core B is a relative sensitivity only; no absolute B is fitted",
            "No heavy .098 construction, gauge interpolation or measured audio-realism improvement is identified",
            "Ideal open-string mechanics omit stiffness correction to frequency and changing speaking length",
        ],
        "rows": rows,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Write JSON to this file; default is stdout")
    args = parser.parse_args()
    payload = json.dumps(build_report(), indent=2, allow_nan=False) + "\n"
    if args.output:
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload, end="")


if __name__ == "__main__":
    main()
