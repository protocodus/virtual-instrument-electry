#!/usr/bin/env python3
"""Audit static low-string modal frequencies without trusting the fit's score.

The C++ observer below reads coefficients from the shipping engine; it does
not implement a second string fitter. This Python oracle solves the isolated
vertical loop's unwrapped phase for each real-axis phase resonance, including the actual
cubic fractional read and loss-filter phase. These are not complex damped
pole roots or measured spectral peaks. The horizontal loop's detuning and
nonzero two-polarisation coupling are omitted; these are not the complete
static string's modes. It separately solves the dispersion-only
loop, with its fundamental phase compensated, to expose the fitter's error
apart from loss-filter curvature. The comparison law is
f_n = n f_1 sqrt((1 + B n^2)/(1 + B)). Neither this law nor a coefficient audit
establishes similarity to a particular recorded guitar.

All observations are static Sustain notes with attack tension reset to zero,
the bridge/body/noise paths disabled and no parameter smoothing in flight.
The observer deliberately accesses named private state through the same friend
seam as engine tests. It never changes production files or shipped DSP.
"""

import argparse
import cmath
import hashlib
import json
import math
import pathlib
import re
import statistics
import subprocess
import tempfile


OBSERVER = r'''
#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
namespace electry {
struct ElectryEngineTestAccess {
    static void benchmark(double rate, bool moving) {
        auto engine = std::make_unique<ElectryEngine>();
        EngineParameters p;
        p.bodyResonance = p.sympatheticAmount = p.artifactAmount = 0;
        p.pickNoise = p.fingerNoise = p.releaseNoise = 0;
        p.bendTimeSeconds = .04f;
        engine->setParameters(p); engine->prepare(rate, 256);
        std::array<ElectryEngine::NoteOnEvent, 8> notes {};
        for (int s = 0; s < 8; ++s)
            notes[s] = {engine->stringSpecs()[s].openMidiNote + 12, .8f};
        std::array<float, 256> left {}, right {};
        engine->noteOnChord(notes.data(), notes.size());
        engine->process(left.data(), right.data(), 256);
        std::vector<double> times;
        for (int repeat = -32; repeat < 512; ++repeat) {
            if (!moving) engine->reset();
            const auto start = std::chrono::steady_clock::now();
            if (moving) {
                engine->setPitchBendSemitones(
                    2.f * static_cast<float>((repeat + 32) % 17 - 8) / 8.f);
                engine->process(left.data(), right.data(), 256);
            } else engine->noteOnChord(notes.data(), notes.size());
            const auto end = std::chrono::steady_clock::now();
            if (repeat >= 0) times.push_back(
                std::chrono::duration<double, std::micro>(end-start).count());
        }
        std::sort(times.begin(), times.end());
        std::cout << "{\"host_rate\":" << rate << ",\"mode\":\""
                  << (moving ? "eight_string_bend_256_frames" : "cold_eight_string_chord_event")
                  << "\",\"samples\":" << times.size()
                  << ",\"p50_microseconds\":" << times[times.size()/2]
                  << ",\"p95_microseconds\":" << times[486]
                  << ",\"maximum_microseconds\":" << times.back() << '}';
    }
    static void emit(double hostRate, float gauge, float scale,
                     int stringIndex, int fret, float bend) {
        auto engine = std::make_unique<ElectryEngine>();
        EngineParameters p;
        p.stringGauge = gauge; p.scaleLength = scale;
        p.bodyResonance = p.sympatheticAmount = p.artifactAmount = 0;
        p.pickNoise = p.fingerNoise = p.releaseNoise = 0;
        engine->setParameters(p); engine->prepare(hostRate, 512);
        engine->setSoloStringMask(static_cast<unsigned char>(1u << stringIndex));
        const int note = engine->stringSpecs()[stringIndex].openMidiNote + fret;
        engine->noteOn(note, .8f);
        auto& v = engine->voices_[stringIndex];
        engine->pitchBendSemitones_ = bend;
        engine->configureVoicePitch(v, true);
        const auto& l = v.vertical;
        const auto biquad = [] (const auto& b) {
            std::cout << '[' << b.b0 << ',' << b.b1 << ',' << b.b2
                      << ',' << b.a1 << ',' << b.a2 << ']';
        };
        std::cout << "{\"host_rate\":" << hostRate
                  << ",\"rate\":" << engine->sampleRate_
                  << ",\"gauge\":" << gauge << ",\"scale\":" << scale
                  << ",\"string\":" << stringIndex << ",\"fret\":" << fret
                  << ",\"bend\":" << bend << ",\"f0\":" << v.lastConfiguredFrequency
                  << ",\"B\":" << v.inharmonicity
                  << ",\"delay\":" << l.currentDelay
                  << ",\"ap\":[" << l.dispersionLowCoefficient << ','
                  << l.dispersionHighCoefficient << ']'
                  << ",\"pole\":"
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
                  << l.loopDampingCoefficient
#else
                  << l.loopDampingCoefficient
#endif
                  << ",\"gain\":" << l.loopGain
                  << ",\"material\":";
        if (l.materialLossDepth > 0) biquad(l.materialLossDip);
        else std::cout << "[1,0,0,0,0]";
        std::cout << ",\"loss\":";
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
        if (l.lossShelfActive) biquad(l.lossShelf);
        else
#endif
            std::cout << "[1,0,0,0,0]";
        std::cout << ",\"loss_gain\":";
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
        std::cout << (l.lossShelfActive ? l.lossShelfGain : 1.0);
        std::cout << ",\"loss_zero\":" << (l.lossShelfActive ? l.lossShelfZeroCoefficient : 0.f);
#else
        std::cout << "1,\"loss_zero\":0";
#endif
        std::cout << ",\"hand\":";
        if (l.handDipActive) biquad(l.handDip);
        else std::cout << "[1,0,0,0,0]";
        std::cout << '}';
    }
};
}
int main(int argc, char**) {
    std::cout << std::setprecision(17) << '[';
    bool first = true;
    if (argc > 1) {
        for (double rate : {48000., 96000., 384000.})
            for (bool moving : {false, true}) {
                if (!first) std::cout << ',';
                first = false;
                electry::ElectryEngineTestAccess::benchmark(rate, moving);
            }
        std::cout << "]\n"; return 0;
    }
    for (double rate : {44100., 48000., 96000., 96001., 192000., 384000.})
        for (float gauge : {0.f, 1.f})
            for (float scale : {0.f, .85f})
                for (int stringIndex : {0, 1})
                    for (int fret : {0, 2, 7, 12, 17, 22})
                        for (float bend : {-2.f, 0.f, 2.f}) {
                            if (!first) std::cout << ',';
                            first = false;
                            electry::ElectryEngineTestAccess::emit(
                                rate, gauge, scale, stringIndex, fret, bend);
                        }
    std::cout << "]\n";
}
'''

DEFAULT_DEFINES = ("ELECTRY_MEASURED_BODY_RESPONSE=1", "ELECTRY_DECOUPLED_PICK_RELEASE=1",
                   "ELECTRY_ENERGY_ATTACK_PITCH=1")


def compile_defines(extra):
    definitions = dict(define.split("=") for define in DEFAULT_DEFINES)
    definitions.update(define.split("=") for define in extra)
    return [name + "=" + value for name, value in definitions.items()]


def allpass_phase(coefficient, omega):
    """One passive first-order allpass, continuously unwrapped on [0, pi]."""
    z = cmath.exp(-1j * omega)
    return cmath.phase((coefficient + z) / (1 + coefficient * z))


def cubic_read_response(delay, omega):
    """Actual four-point read response, factored into integer and local FIR."""
    ceiling = math.ceil(delay)
    t = ceiling - delay
    coefficients = (
        -t * (t - 1) * (t - 2) / 6,
        (t + 1) * (t - 1) * (t - 2) / 2,
        -(t + 1) * t * (t - 2) / 2,
        (t + 1) * t * (t - 1) / 6,
    )
    local = sum(c * cmath.exp(1j * omega * offset)
                for c, offset in zip(coefficients, (-1, 0, 1, 2)))
    return -ceiling * omega + cmath.phase(local), abs(local)


def biquad_response(coefficients, omega):
    b0, b1, b2, a1, a2 = coefficients
    z = cmath.exp(-1j * omega)
    return (b0 + b1 * z + b2 * z * z) / (1 + a1 * z + a2 * z * z)


def loop_phase_and_magnitude(observation, omega, dispersion_only=False):
    ap_phase = 4 * sum(allpass_phase(c, omega) for c in observation["ap"])
    if dispersion_only:
        omega0 = 2 * math.pi * observation["f0"] / observation["rate"]
        base_phase = 4 * sum(allpass_phase(c, omega0) for c in observation["ap"])
        delay = observation["rate"] / observation["f0"] + base_phase / omega0
        return -delay * omega + ap_phase, 1.0
    phase, magnitude = cubic_read_response(observation["delay"], omega)
    z = cmath.exp(-1j * omega)
    pole = (1 - observation["pole"]) / (1 - observation["pole"] * z)
    loss = (pole * biquad_response(observation["material"], omega)
            * biquad_response(observation["hand"], omega)
            * biquad_response(observation.get("loss", [1,0,0,0,0]), omega)
            * observation.get("loss_gain", 1)
            * (1 - observation.get("loss_zero", 0) * z))
    return phase + ap_phase + cmath.phase(loss), magnitude * abs(loss) * observation["gain"]


def modal_frequency(observation, partial, dispersion_only=False):
    """Real-axis phase resonance; exact mode for the lossless isolated loop."""
    target = -2 * math.pi * partial
    lo, hi = 0.0, math.pi * (1 - 1e-9)
    if loop_phase_and_magnitude(observation, hi, dispersion_only)[0] > target:
        return None
    for _ in range(64):
        middle = (lo + hi) / 2
        if loop_phase_and_magnitude(observation, middle, dispersion_only)[0] > target:
            lo = middle
        else:
            hi = middle
    omega = (lo + hi) / 2
    frequency = omega * observation["rate"] / (2 * math.pi)
    return frequency


def analyze(observation, partials=16):
    fundamental = modal_frequency(observation, 1)
    result = dict(observation)
    result["fundamental_error_cents"] = 1200 * math.log2(fundamental / observation["f0"])
    result["modes"] = []
    for n in range(1, partials + 1):
        stretch = math.sqrt((1 + observation["B"] * n * n) / (1 + observation["B"]))
        wanted = observation["f0"] * n * stretch
        actual = modal_frequency(observation, n)
        isolated = modal_frequency(observation, n, dispersion_only=True)
        if actual is None or isolated is None:
            break
        omega = 2 * math.pi * actual / observation["rate"]
        magnitude = loop_phase_and_magnitude(observation, omega)[1]
        # This is per-traversal attenuation. Group delay would be needed to
        # turn it into a modal T60, so do not mislabel it as a decay-time fit.
        read_magnitude = cubic_read_response(observation["delay"], omega)[1]
        result["modes"].append({
            "partial": n, "target_hz": wanted, "actual_hz": actual,
            "dispersion_only_hz": isolated,
            "full_error_cents": 1200 * math.log2(actual / wanted),
            "dispersion_only_error_cents": 1200 * math.log2(isolated / wanted),
            "round_trip_gain_db": 20 * math.log10(max(magnitude, 1e-300)),
            "interpolation_gain_db": 20 * math.log10(max(read_magnitude, 1e-300)),
        })
    return result


def observe(source_root, compiler="c++", defines=(), benchmark=False):
    with tempfile.TemporaryDirectory(prefix="electry-dispersion-audit-") as temporary:
        temporary = pathlib.Path(temporary)
        source = temporary / "observe.cpp"
        binary = temporary / "observe"
        source.write_text(OBSERVER)
        command = [compiler, "-std=c++17", "-O2",
                   "-I", str(source_root / "Source"), str(source),
                   str(source_root / "Source/DSP/ElectryEngine.cpp"), "-o", str(binary)]
        command[1:1] = ["-D" + define for define in compile_defines(defines)]
        subprocess.run(command, check=True)
        return json.loads(subprocess.check_output([str(binary)] + (["benchmark"] if benchmark else []), text=True))


def provenance(source_root, compiler, defines):
    source_root = source_root.resolve()
    files = ["Source/DSP/ElectryEngine.cpp", "Source/DSP/ElectryEngine.h", "Source/DSP/DspMath.h"]
    hashes = {name: hashlib.sha256((source_root / name).read_bytes()).hexdigest() for name in files}
    return {"source_root": str(source_root), "source_git_revision": subprocess.check_output(
                ["git", "-C", str(source_root), "rev-parse", "HEAD"], text=True).strip(),
            "source_sha256": hashes,
            "oracle_sha256": hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
            "compiler": subprocess.check_output([compiler, "--version"], text=True).splitlines()[0],
            "compile_defines": compile_defines(defines), "optimization": "-O2", "language": "c++17"}


def summarize(rows):
    modes = [(row, mode) for row in rows for mode in row["modes"] if mode["partial"] > 1]
    full = [abs(mode["full_error_cents"]) for _, mode in modes]
    isolated = [abs(mode["dispersion_only_error_cents"]) for _, mode in modes]
    worst_row, worst_mode = max(modes, key=lambda x: abs(x[1]["full_error_cents"]))
    return {
        "observations": len(rows), "resolved_upper_modes": len(modes),
        "full_median_absolute_cents": statistics.median(full), "full_maximum_absolute_cents": max(full),
        "dispersion_only_median_absolute_cents": statistics.median(isolated),
        "dispersion_only_maximum_absolute_cents": max(isolated),
        "worst_full_loop": {**{key: worst_row[key] for key in
            ("host_rate", "gauge", "scale", "string", "fret", "bend", "B")}, **worst_mode},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--observations", type=pathlib.Path, help="Analyze an existing observer JSON instead of compiling")
    parser.add_argument("--save-observations", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--compiler", default="c++")
    parser.add_argument("--modal-fit", action="store_true", help="Explicitly enable the selected modal fitter (also the development default)")
    parser.add_argument("--define", action="append", default=[], help="Extra experimental ELECTRY_* macro, NAME=0 or NAME=1")
    parser.add_argument("--benchmark-only", action="store_true", help="Time cold chord events and moving-bend callbacks instead of modes")
    args = parser.parse_args()
    defines = list(args.define)
    if args.modal_fit:
        defines.append("ELECTRY_MODAL_DISPERSION_FIT=1")
    if any(not re.fullmatch(r"ELECTRY_[A-Z0-9_]+=[01]", define) for define in defines):
        parser.error("--define accepts only ELECTRY_NAME=0 or ELECTRY_NAME=1")
    if len({define.split("=")[0] for define in defines}) != len(defines):
        parser.error("a macro may be specified only once")
    if args.observations:
        saved = json.loads(args.observations.read_text())
        if isinstance(saved, dict):
            observations = saved["observations"]
            metadata = saved["provenance"]
        else:
            observations = saved
            metadata = {"kind": "legacy_unverified_observations",
                        "observation_file_sha256": hashlib.sha256(args.observations.read_bytes()).hexdigest()}
    else:
        # Hash before compiling so a later edit cannot be mislabeled as the
        # observed implementation. The caller should avoid concurrent edits.
        metadata = provenance(args.source_root, args.compiler, defines)
        observations = observe(args.source_root, args.compiler, defines, args.benchmark_only)
    if args.benchmark_only:
        result = {"schema": "electry-dispersion-benchmark/v1", "provenance": metadata, "timings": observations}
        args.output.write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps(result, indent=2))
        return
    if args.save_observations:
        args.save_observations.write_text(json.dumps({"provenance": metadata,
            "observations": observations}, indent=2) + "\n")
    rows = [analyze(row) for row in observations]
    summary = summarize(rows)
    args.output.write_text(json.dumps({"schema": "electry-dispersion-audit/v1", "provenance": metadata, "summary": summary,
                                      "observations": rows}, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
