#!/usr/bin/env python3
"""Solve static string-loop damped poles independently of the loss fitter.

The production loss design maps a requested T60 to one traversal per
fundamental period. In a dispersive loop, the envelope sees group delay, not
phase delay: J. O. Smith, Physical Audio Signal Processing, Digital Waveguide
Theory, https://www.dsprelated.com/freebooks/pasp/Digital_Waveguide_Theory.html .
This tool evaluates the actual cubic read, eight allpasses and loss filters
in the complex z plane. Newton iteration in s=log(z) solves H(exp(s))=1.
It reports actual pole decay separately from the fundamental-period mapping
and its group-delay approximation. No calibration constant or engine source
is changed. Static Sustain observations exclude time-varying attack/contact,
bridge coupling and output pickup/body/effect coloration. Observations contain
only the vertical loop, omitting the horizontal loop's detuning and nonzero
two-axis coupling: these are isolated vertical-polarisation loop poles, not
the poles of the complete static string or full performed instrument.
"""
import argparse
import cmath
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import statistics

_spec = importlib.util.spec_from_file_location("dispersion_audit", Path(__file__).with_name("AuditDispersion.py"))
dispersion = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(dispersion)


def transfer_and_log_derivative(row, s):
    """H(exp(s)), d log(H(exp(s)))/ds; no finite differences or fitted decay."""
    q = cmath.exp(-s)
    ceiling = math.ceil(row["delay"])
    t = ceiling - row["delay"]
    taps = (-t * (t - 1) * (t - 2) / 6,
            (t + 1) * (t - 1) * (t - 2) / 2,
            -(t + 1) * t * (t - 2) / 2,
            (t + 1) * t * (t - 1) / 6)
    terms = [c * cmath.exp(s * offset) for c, offset in zip(taps, (-1, 0, 1, 2))]
    local = sum(terms)
    h = cmath.exp(-ceiling * s) * local
    derivative = -ceiling + sum(v * offset for v, offset in zip(terms, (-1, 0, 1, 2))) / local
    for a in row["ap"]:
        h *= ((a + q) / (1 + a * q)) ** 4
        derivative += 4 * (-q / (a + q) + a * q / (1 + a * q))
    a = row["pole"]
    h *= row["gain"] * (1 - a) / (1 - a * q)
    derivative -= a * q / (1 - a * q)
    for coefficients in (row["material"], row["hand"], row.get("loss", [1,0,0,0,0])):
        b0, b1, b2, a1, a2 = coefficients
        numerator = b0 + b1 * q + b2 * q * q
        denominator = 1 + a1 * q + a2 * q * q
        h *= numerator / denominator
        derivative += (-b1 * q - 2 * b2 * q * q) / numerator
        derivative -= (-a1 * q - 2 * a2 * q * q) / denominator
    zero = row.get("loss_zero", 0)
    h *= row.get("loss_gain", 1) * (1 - zero * q)
    derivative += zero * q / (1 - zero * q)
    return h, derivative


def solve_pole(row, partial):
    phase_frequency = dispersion.modal_frequency(row, partial)
    if phase_frequency is None:
        return None
    omega = 2 * math.pi * phase_frequency / row["rate"]
    h, derivative = transfer_and_log_derivative(row, complex(0, omega))
    group_delay = -derivative.real
    gain_log = math.log(abs(h))
    # A few unit-circle operations can leave a unity-magnitude loop a handful
    # of ULPs below one. Such an unresolved loss cannot support a finite T60.
    if group_delay <= 0 or gain_log >= -1e-14:
        raise ValueError("Expected a passive decaying loop with positive group delay")
    s = complex(gain_log / group_delay, omega)
    for iteration in range(32):
        h, derivative = transfer_and_log_derivative(row, s)
        delta = (h - 1) / (h * derivative)
        s -= delta
        if abs(delta) < 1e-14:
            break
    h, _ = transfer_and_log_derivative(row, s)
    residual = abs(h - 1)
    # A root must remain in its original mode's half-spacing neighbourhood.
    # Otherwise a converged Newton solve could silently identify a neighbour.
    if residual > 1e-9 or s.real >= 0 or abs(s.imag - omega) > math.pi * row["f0"] / row["rate"]:
        raise ValueError(f"Pole did not converge to decaying mode {partial}: residual={residual}")
    pole_t60 = -3 * math.log(10) / (s.real * row["rate"])
    nominal_t60 = -3 * math.log(10) / (row["f0"] * gain_log)
    group_t60 = -3 * math.log(10) * group_delay / (row["rate"] * gain_log)
    return {"partial": partial, "phase_frequency_hz": phase_frequency,
            "pole_frequency_hz": s.imag * row["rate"] / (2 * math.pi),
            "log_radius": s.real, "t60_seconds": pole_t60,
            "fundamental_period_mapping_t60_seconds": nominal_t60,
            "group_delay_approximation_t60_seconds": group_t60,
            "group_delay_samples": group_delay,
            "t60_over_period_mapping": pole_t60 / nominal_t60,
            "group_approximation_relative_error": group_t60 / pole_t60 - 1,
            "residual": residual, "iterations": iteration + 1}


def analyze(rows):
    result = []
    for row in rows:
        modes = [mode for n in range(1, 17) if (mode := solve_pole(row, n)) is not None]
        result.append({**{key: row[key] for key in
            ("host_rate", "gauge", "scale", "string", "fret", "bend", "B")}, "modes": modes})
    modes = [(row, mode) for row in result for mode in row["modes"]]
    worst_row, worst_mode = min(modes, key=lambda pair: pair[1]["t60_over_period_mapping"])
    return {"schema": "electry-modal-decay-audit/v1",
        "topology": "isolated vertical loop; horizontal detuning and polarisation coupling omitted",
        "summary": {
        "observations": len(result), "poles": len(modes),
        "maximum_root_residual": max(mode["residual"] for _, mode in modes),
        "minimum_t60_over_period_mapping": worst_mode["t60_over_period_mapping"],
        "median_t60_over_period_mapping": statistics.median(mode["t60_over_period_mapping"] for _, mode in modes),
        "maximum_group_approximation_relative_error": max(abs(mode["group_approximation_relative_error"]) for _, mode in modes),
        "worst": {**{k: v for k, v in worst_row.items() if k != "modes"}, **worst_mode}},
        "observations": result}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--observations", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = json.loads(args.observations.read_text())
    rows = source["observations"] if isinstance(source, dict) else source
    result = analyze(rows)
    result["source_provenance"] = (
        source.get("provenance", {"status": "unverified: observations have no provenance"})
        if isinstance(source, dict) else {"status": "unverified bare-list input"})
    result["audit_provenance"] = {
        "input_sha256": hashlib.sha256(args.observations.read_bytes()).hexdigest(),
        "input_path": str(args.observations),
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "dispersion_script_sha256": hashlib.sha256(Path(dispersion.__file__).read_bytes()).hexdigest()}
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False) + "\n")
    print(json.dumps(result["summary"], indent=2))


if __name__ == "__main__":
    main()
