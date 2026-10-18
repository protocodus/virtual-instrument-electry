#!/usr/bin/env python3
"""Offline topology experiment; never installs coefficients in the instrument.

Reference: the existing normalized stiff-string law
  omega_n = n omega_1 sqrt((1+B n^2)/(1+B)).
Kemp, SN Applied Sciences 2, 636 (2020), equations 3--5:
https://doi.org/10.1007/s42452-020-2391-2

For a stable conjugate pair p=r exp(+-i theta), use the real allpass
  H(z)=(r^2-2r cos(theta)z^-1+z^-2)
       /(1-2r cos(theta)z^-1+r^2 z^-2).
Its phase is -2 omega -2 arg(denominator), and its group delay is the sum of
the two Poisson kernels (1-r^2)/(1+r^2-2r cos(omega+-theta)). These are the
pole/zero and group-delay identities in Abel & Smith, DAFx2006, sections 2--3:
https://www.dafx.de/paper-archive/2006/papers/p_013.pdf
This tool uses its own constrained modal optimizer, not that paper's design
algorithm or its user-selected overlap coefficient.

The raw delay D=P+sum(arg H(omega_1))/omega_1 pins the fundamental. Solve the
global phase condition D omega-sum(arg H(omega))=2 pi n by safeguarded Newton
iteration. D>=4 and stable poles make its derivative strictly positive, so
the root index cannot skip a mode or hide additional intra-target crossings.
The optimizer minimizes maximum exact lossless H2--H16 root error in cents.
It is deterministic multistart SLSQP, not a global-optimum certificate.

Pole coordinates are log(sigma), theta/omega_1, with
r=exp(-sigma omega_1). Search bounds are numerical exploration limits, not
calibrated guitar constants. Every retained result includes poles, residuals,
stability/root-index checks, source hashes and optimizer settings. No result
establishes recorded-guitar similarity, moving-filter safety or runtime cost.
Requires NumPy/SciPy. Examples:
  python3 Tools/ExploreDispersion.py --output /tmp/complex-dispersion.json
  python3 Tools/ExploreDispersion.py --observations /tmp/observations.json \
      --pairs 4 --output /tmp/complex-dispersion.json
"""

import argparse
import hashlib
import importlib.util
import json
import math
import pathlib
import time

import numpy as np
import scipy
from scipy.optimize import minimize


def load_audit():
    path = pathlib.Path(__file__).with_name("AuditDispersion.py")
    spec = importlib.util.spec_from_file_location("electry_dispersion_audit", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ConjugateAllpassModel:
    def __init__(self, observation, pairs):
        self.observation = observation
        self.pairs = pairs
        self.period = observation["rate"] / observation["f0"]
        self.omega0 = 2 * np.pi / self.period
        self.orders = np.arange(2.0, 17.0)
        self.target = self.target_frequencies(self.orders)

    def target_frequencies(self, orders):
        b = self.observation["B"]
        orders = np.asarray(orders)
        return self.omega0 * orders * np.sqrt((1 + b * orders**2) / (1 + b))

    def parameters(self, coordinates):
        sigma = np.exp(coordinates[:self.pairs])
        theta = np.asarray(coordinates[self.pairs:]) * self.omega0
        return np.exp(-sigma * self.omega0), theta

    def stage_phase(self, omega, coordinates):
        radius, theta = self.parameters(coordinates)
        omega = np.asarray(omega)
        z = np.exp(-1j * omega[..., None])
        denominator = 1 - 2 * radius * np.cos(theta) * z + radius**2 * z**2
        return -2 * omega[..., None] - 2 * np.angle(denominator)

    def raw_delay(self, coordinates):
        return self.period + np.sum(self.stage_phase(self.omega0, coordinates)) / self.omega0

    def accumulated_phase(self, omega, coordinates):
        return self.raw_delay(coordinates) * np.asarray(omega) - np.sum(
            self.stage_phase(omega, coordinates), axis=-1)

    def group_delay(self, omega, coordinates):
        radius, theta = self.parameters(coordinates)
        omega = np.asarray(omega)[..., None]
        # Positive forms avoid subtracting nearly equal numbers near DC.
        def kernel(angle):
            return (1 - radius**2) / ((1 - radius)**2
                + 4 * radius * np.sin(angle / 2)**2)
        return self.raw_delay(coordinates) + np.sum(
            kernel(omega - theta) + kernel(omega + theta), axis=-1)

    def roots(self, coordinates, orders=None):
        orders = self.orders if orders is None else np.asarray(orders)
        if not self.raw_delay(coordinates) >= 4:
            raise ValueError("fundamental compensation leaves less than four raw delay samples")
        target_phase = 2 * np.pi * orders
        low = np.zeros_like(orders)
        high = np.full_like(orders, np.pi * (1 - 1e-10))
        if np.any(self.accumulated_phase(high, coordinates) <= target_phase):
            raise ValueError("a requested root is above Nyquist")
        omega = np.minimum(self.target_frequencies(orders), high)
        for _ in range(48):
            residual = self.accumulated_phase(omega, coordinates) - target_phase
            if np.max(np.abs(residual)) < 1e-10:
                break
            low = np.where(residual < 0, omega, low)
            high = np.where(residual >= 0, omega, high)
            proposed = omega - residual / self.group_delay(omega, coordinates)
            omega = np.where((proposed > low) & (proposed < high), proposed, (low + high) / 2)
        if np.max(np.abs(self.accumulated_phase(omega, coordinates) - target_phase)) > 1e-8:
            raise ValueError("global modal root did not converge")
        return omega

    def errors(self, coordinates, orders=None):
        orders = self.orders if orders is None else np.asarray(orders)
        return 1200 / np.log(2) * np.log(self.roots(coordinates, orders)
            / self.target_frequencies(orders))

    def safe_errors(self, coordinates):
        try:
            return self.errors(coordinates)
        except (ValueError, FloatingPointError):
            return np.full(15, 1e6)

    def validate(self, coordinates):
        roots = self.roots(coordinates)
        radius, theta = self.parameters(coordinates)
        grid = np.linspace(0, np.pi, 2049)
        phase = self.accumulated_phase(grid, coordinates)
        index_residual = self.accumulated_phase(roots, coordinates) / (2 * np.pi) - self.orders
        z = np.exp(-1j * grid[:, None])
        transfer = (radius**2 - 2 * radius * np.cos(theta) * z + z**2) / (
            1 - 2 * radius * np.cos(theta) * z + radius**2 * z**2)
        dense_errors = self.errors(coordinates, np.linspace(1, 16, 121))
        checks = {
            "raw_delay_at_least_four": bool(self.raw_delay(coordinates) >= 4),
            "stable_conjugate_poles": bool(np.all((radius > 0) & (radius < 1))),
            "global_phase_strictly_increasing": bool(np.all(np.diff(phase) > 0)),
            "maximum_root_index_residual": float(np.max(np.abs(index_residual))),
            "maximum_allpass_magnitude_error": float(np.max(np.abs(np.abs(transfer) - 1))),
            "dense_fractional_order_maximum_cents": float(np.max(np.abs(dense_errors))),
            "minimum_pole_stability_margin": float(np.min(1 - radius)),
        }
        checks["all"] = bool(checks["raw_delay_at_least_four"] and checks["stable_conjugate_poles"]
            and checks["global_phase_strictly_increasing"]
            and checks["maximum_root_index_residual"] < 1e-8
            and checks["maximum_allpass_magnitude_error"] < 1e-8)
        return checks


def fit(model, seed, maxiter=300, warm_start=None):
    # Leading-order cubic phase expansion supplies a deterministic starting
    # time scale; it is initialization, never a replacement for root validation.
    sigma = (4 * model.pairs / (3 * np.pi * max(model.observation["B"], 1e-12)))**(1 / 3)
    cell_seed = int.from_bytes(hashlib.sha256(json.dumps(model.observation,
        sort_keys=True).encode()).digest()[:8], "big") ^ seed
    rng = np.random.default_rng(cell_seed)
    starts = [] if warm_start is None else [np.asarray(warm_start)]
    for ratio in (0.2, 0.5, 0.8):
        starts.append(np.r_[np.log(sigma) + rng.normal(0, 0.15, model.pairs),
            np.linspace(0.2, 1.6, model.pairs) * sigma * ratio])
    theta_bound = min(32.0, (np.pi - 1e-8) / model.omega0)
    bounds = [(-3, 5)] * model.pairs + [(0, theta_bound)] * model.pairs + [(0, None)]
    best = None
    attempts = []
    for start in starts:
        start = np.asarray([np.clip(x, lo, hi) for x, (lo, hi) in zip(start, bounds[:-1])])
        initial = np.r_[start, np.max(np.abs(model.safe_errors(start))) + 1e-7]
        def constraints(value):
            errors = model.safe_errors(value[:-1])
            return np.r_[value[-1] - errors, value[-1] + errors]
        result = minimize(lambda value: value[-1], initial, method="SLSQP", bounds=bounds,
            constraints=[{"type": "ineq", "fun": constraints}],
            options={"maxiter": maxiter, "ftol": 1e-8})
        coordinates = result.x[:-1]
        try:
            checks = model.validate(coordinates)
            errors = model.errors(coordinates)
            maximum = float(np.max(np.abs(errors)))
        except ValueError:
            checks, maximum = {"all": False}, 1e6
        attempts.append({"success": bool(result.success), "message": result.message,
            "iterations": result.nit, "maximum_cents": maximum, "validation": checks})
        if checks["all"] and (best is None or maximum < best[0]):
            best = maximum, coordinates, errors, checks
    if best is None:
        raise RuntimeError("no feasible stable candidate with correctly indexed roots")
    maximum, coordinates, errors, checks = best
    # Canonical pole ordering permits continuation warm starts. The transfer
    # function is unchanged because cascade multiplication commutes.
    order = np.argsort(coordinates[model.pairs:])
    coordinates = np.r_[coordinates[:model.pairs][order], coordinates[model.pairs:][order]]
    radius, theta = model.parameters(coordinates)
    return {"pairs": model.pairs, "maximum_cents": maximum,
        "coordinates": coordinates.tolist(), "radii": radius.tolist(),
        "angles_radians": theta.tolist(), "raw_delay": float(model.raw_delay(coordinates)),
        "errors_cents_H2_H16": errors.tolist(),
        "roots_hz_H2_H16": (model.roots(coordinates) * model.observation["rate"] / (2 * np.pi)).tolist(),
        "denominator_a1_a2": [[float(-2 * r * np.cos(t)), float(r*r)] for r, t in zip(radius, theta)],
        "validation": checks, "attempts": attempts}


def observations(source_root, audit):
    # Reuse the shipping observer, changing only its explicit inventory. Its
    # source is hashed below, so this alteration cannot masquerade as the
    # ordinary AuditDispersion sweep.
    replacements = {
        "{44100., 48000., 96000., 96001., 192000., 384000.}": "{44100., 96000., 384000.}",
        "for (float gauge : {0.f, 1.f})": "for (float gauge : {0.f, .5f, 1.f})",
        "for (float scale : {0.f, .85f})": "for (float scale : {.85f})",
        "for (int stringIndex : {0, 1})": "for (int stringIndex : {0})",
        "for (int fret : {0, 2, 7, 12, 17, 22})": "for (int fret : {0, 12, 22})",
        "for (float bend : {-2.f, 0.f, 2.f})": "for (float bend : {0.f})",
    }
    observer = audit.OBSERVER
    for before, after in replacements.items():
        if observer.count(before) != 1:
            raise ValueError("shipping observer inventory changed: " + before)
        observer = observer.replace(before, after)
    audit.OBSERVER = observer
    metadata = audit.provenance(source_root, "c++", [])
    metadata["observer_sha256"] = hashlib.sha256(observer.encode()).hexdigest()
    return audit.observe(source_root), metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--observations", type=pathlib.Path)
    parser.add_argument("--pairs", type=int, nargs="+", default=[1, 2, 3, 4])
    parser.add_argument("--seed", type=int, default=174271)
    parser.add_argument("--maxiter", type=int, default=300)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    if any(count < 1 or count > 4 for count in args.pairs):
        parser.error("this finite probe supports one through four biquads")
    audit = load_audit()
    if args.observations:
        saved = json.loads(args.observations.read_text())
        rows = saved["observations"] if isinstance(saved, dict) else saved
        metadata = saved.get("provenance", {}) if isinstance(saved, dict) else {}
        metadata = {**metadata, "observation_file_sha256": hashlib.sha256(args.observations.read_bytes()).hexdigest()}
    else:
        rows, metadata = observations(args.source_root, audit)
    metadata.update({"explorer_sha256": hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
        "numpy": np.__version__, "scipy": scipy.__version__, "seed": args.seed,
        "optimizer": "deterministic multistart SLSQP epigraph exact lossless root cents",
        "sigma_log_bounds": [-3, 5], "theta_over_omega0_bounds": [0, 32],
        "maxiter": args.maxiter, "ftol": 1e-8})
    output = {"schema": "electry-complex-dispersion-exploration/v1", "provenance": metadata, "cells": []}
    warm = {}
    for row in sorted(rows, key=lambda item: (item["host_rate"], item["gauge"], item["fret"])):
        entry = {"observation": row, "fits": []}
        for pairs in args.pairs:
            started = time.monotonic()
            result = fit(ConjugateAllpassModel(row, pairs), args.seed, args.maxiter, warm.get(pairs))
            result["fit_seconds"] = time.monotonic() - started
            warm[pairs] = result["coordinates"]
            entry["fits"].append(result)
            print(json.dumps({"rate": row["host_rate"], "gauge": row["gauge"], "fret": row["fret"],
                "pairs": pairs, "maximum_cents": result["maximum_cents"],
                "dense_guard_cents": result["validation"]["dense_fractional_order_maximum_cents"],
                "seconds": result["fit_seconds"]}), flush=True)
        output["cells"].append(entry)
        args.output.write_text(json.dumps(output, indent=2) + "\n")


if __name__ == "__main__":
    main()
