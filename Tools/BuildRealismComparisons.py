#!/usr/bin/env python3
"""Build isolated production-DSP listening pairs from immutable git inputs.

A is always --base. Each variant starts at that same base and applies only its
listed commits' Source/DSP diffs. The same RenderRealismComparison driver runs
both. One constant whole-file RMS trim per take matches each pair; common peak
headroom is applied to both. No spectral shaping, time warping, or tail-only
normalisation. Metadata retains source hashes, events, raw levels, trims, and
sample differences. key.md is deliberately separate from the listening page.
Requires NumPy/SciPy for WAV analysis; no third-party audio is downloaded.
"""
import argparse
import concurrent.futures
import hashlib
import io
import json
import math
import re
from pathlib import Path
import subprocess
import tarfile
import wave

def run(args, cwd=None, data=None):
    result = subprocess.run(args, cwd=cwd, input=data,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode:
        raise RuntimeError(result.stderr.decode(errors="replace"))
    return result.stdout


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def apply_dsp_patch(source, patch):
    # A review output may live under the canonical Git worktree. Without its
    # own repository, `git apply` inherits that outer root and can silently
    # skip every Source/DSP path as outside its current subdirectory. Give the
    # archived source an independent root before applying the immutable diff.
    if not patch.strip():
        raise ValueError("Candidate commit contains no Source/DSP change")
    run(["git", "init", "-q"], source)
    run(["git", "apply", "-"], source, patch)


def build(repo, out, base, commits, driver, defines=None):
    defines = defines or {}
    for name, value in defines.items():
        if not re.fullmatch(r"ELECTRY_[A-Z0-9_]+", name) or type(value) is not int or value not in (0, 1):
            raise ValueError("Experiment defines must be ELECTRY_ names with integer 0/1 values")
    receipt = out / "build.json"
    exe = out / "renderer"
    request = {"base": base, "commits": commits, "defines": defines, "driver_sha256": sha(driver),
               "builder_sha256": sha(Path(__file__)),
               "compiler": run(["clang++", "--version"]).decode()}
    if receipt.exists() and exe.exists():
        prior = json.loads(receipt.read_text())
        if prior.get("request") == request and prior.get("binary_sha256") == sha(exe):
            return exe
    source = out / "source"
    source.mkdir(parents=True, exist_ok=True)
    archive = run(["git", "archive", base, "Source"], repo)
    with tarfile.open(fileobj=io.BytesIO(archive)) as tf:
        members = tf.getmembers()
        for member in members:
            name = Path(member.name)
            if name.is_absolute() or ".." in name.parts or not (member.isfile() or member.isdir()):
                raise ValueError("Unexpected source archive entry")
        tf.extractall(source, members=members)
    patches = []
    for commit in commits:
        patch = run(["git", "diff", commit + "^", commit, "--", "Source/DSP"], repo)
        apply_dsp_patch(source, patch)
        patches.append({"commit": commit, "patch_sha256": hashlib.sha256(patch).hexdigest()})
    sources = [source / "Source/DSP" / (name + ".cpp") for name in
               ["ElectryEngine", "ElectryFx", "ElectryPerformer", "ElectryVisuals"]]
    common = ["-O3", "-DNDEBUG", "-mmacosx-version-min=11.0",
              "-DELECTRY_MEASURED_BODY_RESPONSE=1", "-DELECTRY_DECOUPLED_PICK_RELEASE=1",
              "-DELECTRY_ENERGY_ATTACK_PITCH=1", "-I", str(source / "Source")]
    common += [f"-D{name}={value}" for name, value in sorted(defines.items())]
    def compile_one(path):
        obj = out / (path.stem + ".o")
        run(["clang++", "-std=c++17", *common, "-c", str(path), "-o", str(obj)])
        return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        objects = list(pool.map(compile_one, sources))
    run(["clang++", "-std=c++20", *common, str(driver), *map(str, objects), "-o", str(exe)])
    facts = {"request": request, "base": base, "patches": patches, "flags": common,
             "source_hashes": {str(p.relative_to(source)): sha(p)
                               for p in sorted((source / "Source").rglob("*")) if p.is_file()},
             "driver_sha256": sha(driver), "binary_sha256": sha(exe)}
    (out / "build.json").write_text(json.dumps(facts, indent=2) + "\n")
    return exe


def write_pcm24(path, rate, audio):
    import numpy as np
    integers = np.rint(audio * 8388607).astype(np.int32).reshape(-1)
    packed = np.empty((integers.size, 3), dtype=np.uint8)
    for byte in range(3):
        packed[:, byte] = (integers >> (8 * byte)) & 255
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(audio.shape[1]); wav.setsampwidth(3)
        wav.setframerate(rate); wav.writeframes(packed.tobytes())


def package_pair(a, b, dest, facts):
    import numpy as np
    from scipy.io import wavfile
    ra, x = wavfile.read(a / "raw.wav"); rb, y = wavfile.read(b / "raw.wav")
    ma = json.loads((a / "render.json").read_text())
    mb = json.loads((b / "render.json").read_text())
    if ra != rb or x.shape != y.shape or ma["events"] != mb["events"]:
        raise ValueError("A/B sample rates, shape or events differ")
    x = x.astype(np.float64); y = y.astype(np.float64)
    if np.array_equal(x, y):
        raise ValueError("Intended-change fixture produced identical A/B audio")
    rms = [float(np.sqrt(np.mean(z*z))) for z in [x, y]]
    if min(rms) <= 1e-12 or not np.isfinite(x).all() or not np.isfinite(y).all():
        raise ValueError("Silent/nonfinite listening fixture")
    gains = [0.063095734448 / r for r in rms]  # -24 dBFS RMS
    peak = max(float(np.max(np.abs(z))) * g for z, g in zip([x, y], gains))
    headroom = min(1.0, 0.94 / peak)
    gains = [g * headroom for g in gains]
    dest.mkdir(parents=True, exist_ok=True)
    for letter, z, g in zip("AB", [x, y], gains):
        write_pcm24(dest / (letter + ".wav"), ra, z * g)
    residual = float(np.sqrt(np.mean((gains[1]*y - gains[0]*x)**2)))
    result = {**facts, "rate": ra, "frames": len(x), "channels": x.shape[1],
              "raw_rms": rms, "trim_db": [20*math.log10(g) for g in gains],
              "matched_rms_dbfs": 20*math.log10(rms[0]*gains[0]),
              "matched_difference_db_relative": 20*math.log10(max(residual, 1e-30)/(rms[0]*gains[0])),
              "peak": [float(np.max(np.abs(z)))*g for z,g in zip([x,y],gains)],
              "events_sha256": hashlib.sha256(json.dumps(ma["events"]).encode()).hexdigest(),
              "files": {letter: sha(dest/(letter+".wav")) for letter in "AB"}}
    (dest / "measurements.json").write_text(json.dumps(result, indent=2)+"\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--spec", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=True)
    driver = repo / "Tools/RenderRealismComparison.cpp"
    spec = json.loads(args.spec.read_text())
    base = run(["git", "rev-parse", args.base], repo).decode().strip()
    baseline = build(repo, output / "builds/baseline", base, [], driver)
    results = []
    for variant in spec:
        print("Building", variant["id"], flush=True)
        defines = variant.get("defines", {})
        exe = build(repo, output / "builds" / variant["id"], base, variant["commits"], driver, defines)
        for sound in variant.get("sounds", ["dry"]):
            name = variant["id"] + ("-amp" if sound == "amp" else "")
            locations = [output / "raw" / name / letter for letter in "AB"]
            for renderer, dest in zip([baseline, exe], locations):
                print(run([str(renderer), str(dest), variant["probe"]]
                          + (["amp"] if sound == "amp" else [])).decode().strip(), flush=True)
            results.append(package_pair(*locations, output / "listen" / name,
                {"id": name, "title": variant["title"], "probe": variant["probe"],
                 "sound": sound, "commits": variant["commits"], "defines": defines,
                 "base": base}))
    (output / "listen/measurements.json").write_text(json.dumps(results, indent=2)+"\n")
    display = [{key: r[key] for key in ["id", "title", "probe", "sound"]} for r in results]
    template = (repo / "Tools/RealismListening.html").read_text()
    (output / "listen/index.html").write_text(template.replace("/*MANIFEST*/[]", json.dumps(display)))
    key = ["# Listening key — read after listening", "",
           "A: unchanged upstream " + base + ". B: only the candidate listed below.",
           "", "All takes use identical events, seed, rate and buffers. Whole-file RMS matched; trims below are constant dB gains.", ""]
    for r in results:
        key.append(f"- {r['id']}: {r['title']}. A {r['trim_db'][0]:+.3f} dB; B {r['trim_db'][1]:+.3f} dB. Commits: {', '.join(r['commits'])}. Defines: {r['defines']}.")
    (output / "listen/key.md").write_text("\n".join(key)+"\n")
    print("Listening files:", output / "listen", flush=True)


if __name__ == "__main__":
    main()
