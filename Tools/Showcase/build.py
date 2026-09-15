#!/usr/bin/env python3
"""Render ten original Electry showroom performances and verify the delivery.

Requires Python 3.10+, numpy, scipy, ffmpeg, and ElectryRenderShowcase (Release).
The final WAVs contain only Electry plus constant gain, a tail fade and 24-bit
TPDF dither. Loudness is targeted at -16 LUFS, capped at -1 dBTP: clean dynamics
take precedence over forcing every guitar to the same integrated loudness.
The 8x polyphase peak check is independent of ffmpeg's true-peak measurement.
"""
from __future__ import annotations

import argparse
import concurrent.futures
from collections import Counter
import hashlib
import html
import json
import math
import re
import subprocess
import wave
import zipfile
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly

from clean import build_scores as clean_scores
from heavy import build_scores as heavy_scores


def run(args):
    return subprocess.run([str(a) for a in args], check=True, capture_output=True, text=True)


def meter(path):
    result = run(["ffmpeg", "-hide_banner", "-nostats", "-i", path, "-af",
                  "loudnorm=I=-16:TP=-1:LRA=11:print_format=json", "-f", "null", "-"])
    matches = re.findall(r'\{\s*"input_i".*?\}', result.stderr, flags=re.S)
    if not matches:
        raise RuntimeError(f"No loudness measurement for {path}: {result.stderr[-2000:]}")
    return {k: float(v) for k, v in json.loads(matches[-1]).items() if k.startswith("input_")}


def db(value):
    return 20 * math.log10(max(float(value), 1e-15))


def peak8(audio):
    # Overlap each 2-second window so filter boundaries never hide a peak.
    peak = 0.0
    step, margin = 192000, 256
    for start in range(0, len(audio), step):
        begin, end = max(0, start-margin), min(len(audio), start+step+margin)
        oversampled = resample_poly(audio[begin:end], 8, 1, axis=0,
                                    window=("kaiser", 8.6))
        first = (start-begin)*8
        last = min(len(audio)-start, step)*8 + first
        peak = max(peak, float(np.max(np.abs(oversampled[first:last]))))
    return peak


def validate_score(score):
    active = Counter()
    note_times = set()
    for beat, op, args in score.ordered():
        if op == "N":
            if beat in note_times:
                raise ValueError(f"{score.slug}: merge simultaneous notes into one chord at beat {beat}")
            note_times.add(beat)
            for pitch in args[3:]:
                if active[pitch] and not score.allow_repeated_pitches:
                    raise ValueError(f"{score.slug}: overlapping ownership of pitch {pitch} at beat {beat}")
                active[pitch] += 1
        elif op == "O":
            if not active[args[0]]:
                raise ValueError(f"{score.slug}: unmatched note-off at beat {beat}")
            active[args[0]] -= 1
    if any(active.values()):
        raise ValueError(f"{score.slug}: unreleased pitches {+active}")


def write_pcm24(path, audio, gain_db, seed):
    # Dither at the ACTUAL target precision, not the s32 transport format used
    # by many encoders. Independent U-U noise has a triangular [-1,+1] LSB
    # distribution; rounding gives unbiased, deterministic 24-bit PCM.
    rng = np.random.default_rng(seed)
    scale = 8388608.0 * 10**(gain_db/20)
    with wave.open(str(path), "wb") as output:
        output.setnchannels(2)
        output.setsampwidth(3)
        output.setframerate(96000)
        for start in range(0, len(audio), 65536):
            samples = audio[start:start+65536].astype(np.float64)*scale
            samples += rng.random(samples.shape)-rng.random(samples.shape)
            integers = np.rint(samples).astype(np.int32).reshape(-1)
            if np.max(integers) > 8388607 or np.min(integers) < -8388608:
                raise RuntimeError("24-bit mastering overflow")
            packed = np.empty((len(integers),3),dtype=np.uint8)
            for byte in range(3):
                packed[:,byte] = (integers >> (byte*8)) & 255
            output.writeframesraw(packed.tobytes())


def render_one(score, renderer, output, work, remaster=False):
    validate_score(score)
    metadata = score.write(output / "Presets-and-MIDI")
    raw = work / (score.slug + ".raw.wav")
    render_record = work / (score.slug + ".render.json")
    if remaster:
        render_info = json.loads(render_record.read_text())
    else:
        result = run([renderer, output / "Presets-and-MIDI" / (score.slug + ".score"), raw])
        render_info = json.loads(result.stdout)
        render_record.write_text(json.dumps(render_info,indent=2)+"\n")
    sample_rate, audio = wavfile.read(raw)
    if sample_rate != 96000 or audio.ndim != 2 or audio.shape[1] != 2 or not np.isfinite(audio).all():
        raise RuntimeError(f"Invalid renderer output for {score.slug}")
    # Retain the complete 5-second release; only the final 900ms taper to zero.
    fade_frames = round(.9 * sample_rate)
    x = np.linspace(0, 1, fade_frames, dtype=np.float32)
    fade = 1 - x*x*(3-2*x)
    tail_before_fade = db(np.sqrt(np.mean(audio[-fade_frames:]**2)))
    audio[-fade_frames:] *= fade[:, None]
    prepared = work / (score.slug + ".prepared.wav")
    wavfile.write(prepared, sample_rate, audio)
    before = meter(prepared)
    input_peak8 = db(peak8(audio))
    ceiling = -1.05  # quantization/filter-estimator margin
    gain_db = min(-16.0-before["input_i"], ceiling-max(before["input_tp"], input_peak8))
    wav_path = output / "WAV-96k-24bit" / (score.slug + ".wav")
    seed = int.from_bytes(hashlib.sha256(score.slug.encode()).digest()[:8], "little")
    write_pcm24(wav_path,audio,gain_db,seed)
    after = meter(wav_path)
    # scipy returns 24-bit PCM left-justified in int32.
    delivered_rate, pcm = wavfile.read(wav_path)
    delivered = pcm.astype(np.float64) / 2147483648.0
    true_peak = db(peak8(delivered))
    correlation = float(np.corrcoef(delivered.T)[0, 1])
    rms = float(np.sqrt(np.mean(delivered**2)))
    mono_rms = float(np.sqrt(np.mean(np.mean(delivered, axis=1)**2)))
    probe = json.loads(run(["ffprobe", "-v", "error", "-show_entries",
                           "stream=codec_name,sample_rate,channels,bits_per_sample", "-of", "json", wav_path]).stdout)["streams"][0]
    assert probe == {"codec_name": "pcm_s24le", "sample_rate": "96000", "channels": 2, "bits_per_sample": 24}, probe
    assert delivered_rate == 96000 and np.isfinite(delivered).all()
    assert true_peak <= -1.0 and after["input_tp"] <= -1.0, (score.slug, true_peak, after)
    assert np.max(np.abs(delivered)) < 1.0
    assert -30 < after["input_i"] < -10
    assert db(np.max(np.abs(delivered[-round(.005*sample_rate):]))) < -65
    assert mono_rms > rms * .35, (score.slug, "Excessive mono cancellation")
    report = metadata | dict(
        file="WAV-96k-24bit/" + wav_path.name,
        render=render_info, duration_seconds=round(len(audio)/sample_rate, 3),
        integrated_lufs=after["input_i"], loudness_range_lu=after["input_lra"],
        true_peak_dbtp=after["input_tp"], independent_8x_peak_dbfs=round(true_peak, 3),
        sample_peak_dbfs=round(db(np.max(np.abs(delivered))), 3),
        constant_master_gain_db=round(gain_db, 3),
        tail_before_fade_dbfs=round(tail_before_fade, 3),
        stereo_correlation=round(correlation, 4), mono_rms_change_db=round(db(mono_rms/rms), 3),
        sha256=hashlib.sha256(wav_path.read_bytes()).hexdigest(), bytes=wav_path.stat().st_size,
    )
    print(f"READY {score.slug}: {report['duration_seconds']:.1f}s, {after['input_i']:.1f} LUFS, {true_peak:.2f} dBTP", flush=True)
    return report


def player(output, reports):
    cards = []
    for report in reports:
        title, desc = html.escape(report["name"]), html.escape(report["description"])
        chips = " · ".join(html.escape(f) for f in report["features"])
        sections = " · ".join(f"{s['seconds']:.0f}s {html.escape(s['label'])}" for s in report["sections"])
        cards.append(f'''<article><div class="num">{report['slug'][:2]}</div><div class="take">
<h2>{title}</h2><p>{desc}</p><div class="chips">{chips}</div>
<audio controls preload="metadata" aria-label="Listen to {title}" src="{report['file']}?v={report['sha256'][:12]}"></audio>
<div class="details">{report['duration_seconds']:.1f} sec · {report['bpm']:g} BPM · {report['render']['output_mode']} · {report['integrated_lufs']:.1f} LUFS</div>
<div class="sections">{sections}</div><a download href="{report['file']}">Download WAV ↗</a> · <a download href="Presets-and-MIDI/{report['slug']}.mid">MIDI ↗</a> · <a href="Presets-and-MIDI/{report['slug']}.preset.json">Preset settings ↗</a></div></article>''')
    page = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Electry — Showroom Sessions</title><style>
*{box-sizing:border-box}body{margin:0;background:#101214;color:#ecebe7;font:16px/1.55 -apple-system,BlinkMacSystemFont,sans-serif}
main{max-width:980px;margin:auto;padding:64px 28px}header{padding-bottom:36px;border-bottom:1px solid #34373a}
.eyebrow{color:#f7a869;font-size:12px;font-weight:700;letter-spacing:.24em}h1{font-size:clamp(40px,7vw,72px);letter-spacing:-.06em;line-height:1.05;margin:18px 0}
header p{color:#adb1b4;max-width:600px;font-size:18px}.spec{color:#edc6a5;font-size:13px}article{display:flex;gap:25px;padding:30px 0;border-bottom:1px solid #34373a}
.num{font-size:22px;color:#b68056;padding-top:2px;font-variant-numeric:tabular-nums}.take{flex:1;min-width:0}h2{font-size:25px;margin:0 0 8px;letter-spacing:-.02em}
p{color:#bbbfc1;margin:0 0 12px}.chips{font-size:12px;color:#e0b18e;margin-bottom:12px}audio{width:100%;height:40px}.details,.sections{font-size:12px;color:#999fa3;margin:7px 0}
a{color:#f7a869;font-size:13px;text-decoration:none}footer{color:#909699;font-size:13px;margin-top:30px}button{border:1px solid #555;background:#202326;color:#eee;padding:10px 17px;border-radius:5px;cursor:pointer}
</style><main><header><div class="eyebrow">ELECTRY · SHOWROOM SESSIONS</div><h1>Eight strings.<br>Ten personalities.</h1>
<p>Original performances across clean, blues, rock and modern metal. Every guitar, amplifier and room comes from Electry.</p>
<div class="spec">96 kHz · 24-bit stereo WAV · High quality · Original MIDI included</div><br><button id="playall">Play all ten</button></header>'''
    page += "\n".join(cards)
    page += '''<footer>Ten curated showcase presets using the shipping instrument controls. These are new showcase settings, not ten existing factory programs. Dynamics are preserved with constant gain; only the final tail is faded. No external instrument or effect is used. For MIDI playback, enable FX, choose High quality and LATCH mode, and apply the accompanying preset settings.</footer></main>
<script>const players=[...document.querySelectorAll('audio')];let continuous=false;players.forEach((p,i)=>{p.addEventListener('play',()=>players.forEach(q=>{if(q!==p)q.pause()}));p.addEventListener('ended',()=>{if(continuous&&i+1<players.length){players[i+1].play();players[i+1].closest('article').scrollIntoView({behavior:'smooth',block:'center'})}})});document.querySelector('#playall').onclick=()=>{continuous=true;players.forEach(p=>{p.pause();p.currentTime=0});players[0].play()};</script></html>'''
    (output / "Listen.html").write_text(page)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--renderer", type=Path, default=Path("build-showcase/ElectryRenderShowcase"))
    parser.add_argument("--output", type=Path, default=Path("out/Electry-Showroom-Sessions"))
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--only", help="Render matching slug substring (for audition iteration)")
    parser.add_argument("--remaster", action="store_true", help="Reuse existing raw renders after a mastering-only change")
    args = parser.parse_args()
    if not 1 <= args.jobs <= 10:
        parser.error("--jobs must be 1 through 10")
    output, renderer = args.output.resolve(), args.renderer.resolve()
    work = output.parent / (output.name + "-work")
    (output / "WAV-96k-24bit").mkdir(parents=True, exist_ok=True)
    work.mkdir(parents=True, exist_ok=True)
    scores = sorted(clean_scores()+heavy_scores(), key=lambda s: s.slug)
    assert len(scores) == 10 and len({s.slug for s in scores}) == 10
    if args.only:
        scores = [s for s in scores if args.only in s.slug]
        if not scores:
            parser.error("--only matches no score")
    for score in scores:
        validate_score(score)
    print(f"Rendering {len(scores)} performances at native 96 kHz / High quality with {args.jobs} workers", flush=True)
    reports = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(render_one,s,renderer,output,work,args.remaster): s for s in scores}
        for future in concurrent.futures.as_completed(futures):
            report = future.result()
            (work / (report["slug"]+".report.json")).write_text(json.dumps(report,indent=2)+"\n")
            reports.append(report)
    # Partial rerenders keep the other completed, verified takes in the pack.
    reports = [json.loads(p.read_text()) for p in sorted(work.glob("*.report.json"))
               if (output/"WAV-96k-24bit"/(p.name.removesuffix(".report.json")+".wav")).exists()]
    revision = run(["git", "rev-parse", "HEAD"]).stdout.strip()
    source_hashes = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in
        [Path("Source/DSP/ElectryEngine.cpp"),Path("Source/DSP/ElectryFx.cpp"),
         Path("Tools/RenderShowcase.cpp"),*sorted(Path("Tools/Showcase").glob("*.py"))]}
    manifest = dict(title="Electry — Showroom Sessions", source_revision=revision,
        source_sha256=source_hashes, count=len(reports),
        quality="Native 96 kHz; High FX oversampling; 24-bit PCM stereo WAV",
        mastering="Constant gain, -16 LUFS target constrained by -1 dBTP ceiling; 900ms final tail fade; TPDF dither",
        tracks=reports)
    (output / "manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    (output / "SHA256SUMS.txt").write_text("".join(f"{r['sha256']}  {r['file']}\n" for r in reports))
    player(output,reports)
    archive = output.with_suffix(".zip")
    with zipfile.ZipFile(archive,"w",compression=zipfile.ZIP_DEFLATED,compresslevel=1) as pack:
        for path in sorted(output.rglob("*")):
            if path.is_file():
                pack.write(path,Path(output.name)/path.relative_to(output))
    print(f"DELIVERED {len(reports)} WAVs: {output}\nZIP: {archive}",flush=True)


if __name__ == "__main__":
    main()
