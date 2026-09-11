#!/usr/bin/env python3
"""Validate/render comparisons for the continuous palm-transition benchmark.

Needs Python 3 and NumPy. Float WAV measurements remain unnormalised. Listening
copies use one constant gain per entire file, never per-note gain or compression.
"""
import argparse
import hashlib
import html
import importlib.util
import json
from html.parser import HTMLParser
from pathlib import Path
import shutil
import subprocess
import wave

import numpy as np

SHARED = Path(__file__).resolve().parents[1] / "Realism20260908/AnalyzeRealism.py"
spec = importlib.util.spec_from_file_location("realism_shared", SHARED)
shared = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shared)
TAPS = (("dry", "Dry guitar"), ("crunch", "British crunch"), ("modern", "Modern high gain"))


def verify_score(take):
    held = {}
    last = -1
    counts = {"note_on": 0, "note_off": 0, "repick": 0, "articulation_changes": 0}
    for event in take["events"]:
        frame, kind, string = event["frame"], event["kind"], event["string_index"]
        if frame < last or not 0 <= frame < take["frames"]:
            raise ValueError(f"{take['id']}: unordered or out-of-range event")
        last = frame
        if kind == "note_on":
            if string in held:
                raise ValueError(f"{take['id']}: repeated Note On hides a second owner")
            held[string] = event["engine_note"]
            counts[kind] += 1
        elif kind == "note_off":
            if held.pop(string, None) != event["engine_note"]:
                raise ValueError(f"{take['id']}: unbalanced fretting owner")
            counts[kind] += 1
        elif kind == "repick":
            if string not in held or event["engine_note"] != 88 + string:
                raise ValueError(f"{take['id']}: repick does not address a held physical string")
            counts[kind] += 1
        elif kind in ("play_style", "palm_pressure"):
            counts["articulation_changes"] += 1
    if held:
        raise ValueError(f"{take['id']}: fretting owners remain at the end")
    # The scheduled transitions inside every bar must leave fretting ownership
    # intact. Chord changes at bar starts are separately permitted and explicit.
    boundary_frames = [marker["frame"] for marker in take["markers"]
                       if marker["label"] in ("Palm down", "Palm lifts")]
    bad = [event for event in take["events"] if event["frame"] in boundary_frames
           and event["kind"] in ("note_on", "note_off")]
    if bad:
        raise ValueError(f"{take['id']}: note ownership changes at a hand boundary")
    counts["continuous_hand_boundaries"] = len(boundary_frames)
    return counts


def transition_measurements(samples, take, rate):
    result = []
    for marker in take["markers"]:
        if not any(word in marker["label"].lower() for word in ("palm", "repick", "contact", "control")):
            continue
        frame = marker["frame"]
        def window(start, duration):
            return samples[max(0, frame + round(start * rate)):
                           min(samples.size, frame + round((start + duration) * rate))]
        before, after = window(-.04, .04), window(.02, .06)
        neighborhood = window(-.008, .016)
        centroid, fraction = shared.spectral_measures(after, rate)
        result.append({
            "frame": frame, "time_seconds": frame / rate, "label": marker["label"],
            "pre_40_ms_rms_dbfs": shared.db(shared.rms(before)),
            "post_20_80_ms_rms_dbfs": shared.db(shared.rms(after)),
            "post_20_80_ms_centroid_hz": centroid,
            "post_20_80_ms_energy_fraction_2_8_khz": fraction,
            "max_sample_step_near_command": float(np.max(np.abs(np.diff(neighborhood))))
                if neighborhood.size > 1 else 0.0,
        })
    return result


def overview_svg(before, after, rate):
    """Shared raw-amplitude scale: display must not disguise local dynamics."""
    width, height, bins = 900, 110, 450
    peak = max(float(np.max(np.abs(before))), float(np.max(np.abs(after))), 1e-10)
    pieces = []
    for index, (samples, colour) in enumerate(((before, "#9ca9b6"), (after, "#e7b05c"))):
        centre = 27 + index * 55
        cuts = np.linspace(0, samples.size, bins + 1, dtype=int)
        for i in range(bins):
            part = samples[cuts[i]:cuts[i + 1]]
            amplitude = float(np.max(np.abs(part))) / peak * 23 if part.size else 0
            pieces.append(f'<path d="M{i*2+1} {centre-amplitude:.2f}v{amplitude*2:.2f}" '
                          f'stroke="{colour}" stroke-width="1.3"/>')
    return (f'<svg class="wave" viewBox="0 0 {width} {height}" role="img" '
            'aria-label="Raw peak envelope, before above and after below, on a shared scale">'
            + ''.join(pieces) + '</svg>')


def listening_page(directory, manifest, waveforms):
    cards = []
    rate = manifest["sample_rate"]
    for index, take in enumerate(manifest["takes"]):
        panels = []
        for tap, name in TAPS:
            audio = []
            for version, label in (("baseline", "Before"), ("candidate", "After")):
                path = html.escape(f"audio/{take['id']}-{tap}-{version}.wav", quote=True)
                audio.append(f'<div class="player"><span>{label}</span><audio controls preload="metadata" '
                             f'data-version="{version}" src="{path}"></audio></div>')
            svg = overview_svg(*waveforms[(take["id"], tap)], rate)
            panels.append(f'<div class="tap-panel" data-tap="{tap}" {"" if tap == "crunch" else "hidden"}>'
                          f'{svg}<div class="envelope-label">Before / After · raw peak envelope, shared scale</div>'
                          + ''.join(audio) + '</div>')
        markers = []
        for marker in take["markers"]:
            seconds = marker["frame"] / rate
            markers.append(f'<button class="marker" data-seek="{max(0,seconds-.1):.4f}">'
                           f'<time>{seconds:05.2f}s</time> {html.escape(marker["label"])}</button>')
        # Show all transition markers without letting the main riff consume a
        # wall of controls. Full timestamps remain one click away.
        overview = [m for m in markers if "Bar " in m] if index < 3 else markers
        details = ('<details><summary>All palm transitions and chord changes</summary><div class="markers">'
                   + ''.join(markers) + '</div></details>') if index < 3 else ''
        cards.append(f'<section id="take-{index}"><div class="eyebrow">'
                     f'{"MUSICAL BENCHMARK" if index < 3 else "ISOLATED CONTROL"} · '
                     f'{take["frames"]/rate:.2f} SECONDS</div><h2>{html.escape(take["title"])}</h2>'
                     f'<p>{html.escape(take["description"])}</p>'
                     '<div class="switches"><button data-version-switch="baseline">Switch to before</button>'
                     '<button data-version-switch="candidate">Switch to after</button>'
                     '<button data-restart>Restart phrase</button></div>'
                     + ''.join(panels) + '<div class="markers">' + ''.join(overview)
                     + '</div>' + details + '</section>')
    page = """<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Electry · Open / mute / open</title><style>
:root{color-scheme:dark;font:15px/1.55 system-ui,sans-serif;background:#101316;color:#e9e8e4;--gold:#e7b05c}
*{box-sizing:border-box}body{max-width:1060px;margin:auto;padding:44px 24px 80px}
.eyebrow{font-size:11px;letter-spacing:.13em;color:var(--gold);font-weight:700}h1{font-size:clamp(30px,5vw,49px);line-height:1.12;margin:14px 0}
h2{font-size:23px;line-height:1.3;margin:9px 0 12px}p{color:#b8c0c5;max-width:84ch;margin:12px 0 18px}
a{color:var(--gold)}.toolbar{position:sticky;top:0;z-index:2;padding:15px 0;background:#101316f5;border-bottom:1px solid #3b3d3f;display:flex;align-items:center;gap:12px;flex-wrap:wrap}
.toolbar strong{margin-right:5px;font-size:13px}.toolbar button[aria-pressed=true]{background:#e7b05c;color:#171717;border-color:#e7b05c}
button{font:inherit;font-size:13px;background:#252b30;color:#e9e8e4;border:1px solid #48515a;border-radius:7px;padding:10px 15px;cursor:pointer;min-height:40px}
button:hover{border-color:var(--gold)}button:focus-visible,a:focus-visible,summary:focus-visible{outline:2px solid var(--gold);outline-offset:3px}
section{background:#191e22;border:1px solid #353b40;border-radius:12px;padding:25px;margin:25px 0}
.switches{display:flex;gap:10px;flex-wrap:wrap;margin:20px 0 14px}.player{display:flex;gap:20px;align-items:center;margin:14px 0}.player span{width:48px;font-weight:650;font-size:13px}
audio{width:100%;min-width:0;height:40px}.wave{width:100%;display:block;background:#11171b;border-radius:7px}.envelope-label{color:#909da6;font-size:11px;margin:4px 0 10px}
.markers{display:flex;flex-wrap:wrap;gap:8px;margin-top:20px}.marker{font-size:11px;padding:7px 10px;color:#c4cbd0;min-height:34px}.marker time{font-variant-numeric:tabular-nums;color:var(--gold);margin-right:5px}
details{margin-top:18px}summary{font-size:13px;color:#b8c0c5;cursor:pointer;padding:8px 0}.fine{font-size:12px}.links{display:flex;gap:18px;flex-wrap:wrap;margin-top:20px}
@media(max-width:600px){body{padding:25px 12px}section{padding:18px 15px}.toolbar{gap:7px}.toolbar button{padding:9px 11px}.player{gap:8px}}
</style></head><body><div class="eyebrow">ELECTRY / CONTINUOUS HAND TRANSITIONS</div>
<h1>Open. Palm down. Open again.</h1>
<p>An original 105 BPM, eight-bar rolling chord riff, followed by a Drop-E companion and isolated controls.
The fretting hand keeps owning each note through the mute changes. Start with the first riff through British crunch, then use the dry tap to inspect the hand transitions.</p>
<p class="fine">Before: aa1f7e0. Every pair uses the same score, seed and settings. Listening copies use one constant whole-file gain to equalise RMS, with shared peak protection; no per-note normalisation. Lifting the palm should stop damping the remaining vibration. It should not recreate energy already absorbed by the hand. These are diagnostic comparisons, not measured realism scores.</p>
<div class="links"><a href="report.md">Measurements</a><a href="comparison.json">Detailed data and gains</a><a href="score.json">Exact score and settings</a></div>
<div class="toolbar"><strong>AMPLIFIER</strong><button data-tap-switch="dry" aria-pressed="false">Dry guitar</button><button data-tap-switch="crunch" aria-pressed="true">British crunch</button><button data-tap-switch="modern" aria-pressed="false">Modern high gain</button></div>
""" + '\n'.join(cards) + """
<script>
let tap='crunch';
function activePanel(section){return section.querySelector('.tap-panel:not([hidden])')}
function pauseAll(){document.querySelectorAll('audio').forEach(a=>a.pause())}
function running(section){return [...section.querySelectorAll('audio')].find(a=>!a.paused)}
document.addEventListener('play',event=>{if(event.target.tagName==='AUDIO'){document.querySelectorAll('audio').forEach(a=>{if(a!==event.target)a.pause()})}},true);
document.addEventListener('click',event=>{
const button=event.target.closest('button');if(!button)return;
if(button.dataset.tapSwitch){
 const playing=[...document.querySelectorAll('audio')].find(a=>!a.paused);
 const section=playing?.closest('section');const time=playing?.currentTime||0;const version=playing?.dataset.version;
 pauseAll();tap=button.dataset.tapSwitch;
 document.querySelectorAll('[data-tap-switch]').forEach(b=>b.setAttribute('aria-pressed',b.dataset.tapSwitch===tap));
 document.querySelectorAll('.tap-panel').forEach(p=>p.hidden=p.dataset.tap!==tap);
 if(section){const target=activePanel(section).querySelector(`[data-version="${version}"]`);target.currentTime=time;target.play().catch(()=>{})}
 return;
}
const section=button.closest('section');if(!section)return;const panel=activePanel(section);
if(button.dataset.versionSwitch){const from=running(section)||panel.querySelector('audio');const time=from.currentTime;pauseAll();const to=panel.querySelector(`[data-version="${button.dataset.versionSwitch}"]`);to.currentTime=time;to.play().catch(()=>{})}
if(button.dataset.seek){const was=running(section);const version=was?.dataset.version||'candidate';pauseAll();panel.querySelectorAll('audio').forEach(a=>a.currentTime=Number(button.dataset.seek));panel.querySelector(`[data-version="${version}"]`).play().catch(()=>{})}
if(button.hasAttribute('data-restart')){const was=running(section);const version=was?.dataset.version||'candidate';pauseAll();panel.querySelectorAll('audio').forEach(a=>a.currentTime=0);panel.querySelector(`[data-version="${version}"]`).play().catch(()=>{})}
});
</script></body></html>"""
    (directory / "listening.html").write_text(page)


def validate_listening_artifacts(directory, manifest):
    """Check the actual deliverable, including every linked PCM playback file."""
    class Page(HTMLParser):
        def __init__(self):
            super().__init__(); self.links = []; self.audio = []; self.scripts = []; self.in_script = False

        def handle_starttag(self, tag, attributes):
            attributes = dict(attributes)
            if tag == "a" and "href" in attributes:
                self.links.append(attributes["href"])
            if tag == "audio":
                if "controls" not in attributes:
                    raise ValueError("Listening audio is missing playback controls")
                self.audio.append(attributes["src"])
            if tag == "script":
                self.in_script = True

        def handle_endtag(self, tag):
            if tag == "script":
                self.in_script = False

        def handle_data(self, data):
            if self.in_script:
                self.scripts.append(data)

    page = Page(); page.feed((directory / "listening.html").read_text())
    expected_audio = len(manifest["takes"]) * len(TAPS) * 2
    if len(page.audio) != expected_audio or len(set(page.audio)) != expected_audio:
        raise ValueError("Listening page has missing or duplicate audio assets")
    for link in page.links + page.audio:
        target = (directory / link).resolve()
        if not target.is_relative_to(directory.resolve()) or not target.is_file():
            raise ValueError(f"Listening page has an unresolved local link: {link}")
    peak = 0.0
    rms_errors = []
    for take in manifest["takes"]:
        for tap, _ in TAPS:
            levels = []
            for version in ("baseline", "candidate"):
                filename = directory / "audio" / f"{take['id']}-{tap}-{version}.wav"
                with wave.open(str(filename), "rb") as audio:
                    if (audio.getnchannels(), audio.getsampwidth(), audio.getframerate(), audio.getnframes()) != (
                            1, 2, manifest["sample_rate"], take["frames"]):
                        raise ValueError(f"Wrong listening WAV format: {filename}")
                    samples = np.frombuffer(audio.readframes(audio.getnframes()), dtype="<i2").astype(np.float64) / 32767
                if not np.any(samples) or not np.isfinite(samples).all():
                    raise ValueError(f"Silent or invalid listening WAV: {filename}")
                local_peak = float(np.max(np.abs(samples)))
                if local_peak > .9801:
                    raise ValueError(f"Listening WAV lost headroom: {filename}")
                peak = max(peak, local_peak); levels.append(shared.rms(samples))
            rms_errors.append(abs(shared.db(levels[0]) - shared.db(levels[1])))
    if max(rms_errors) > .02:
        raise ValueError("Quantised listening WAVs no longer have matched whole-file RMS")
    syntax = "not checked: Node.js unavailable"
    if shutil.which("node"):
        subprocess.run(["node", "--check", "-"], input='\n'.join(page.scripts),
                       text=True, check=True, capture_output=True)
        syntax = "passed: node --check"
    return {"resolved_local_links":len(page.links), "validated_pcm_audio_files":expected_audio,
            "audio_format":"mono PCM16, 44100 Hz; original frame counts",
            "all_audio_finite_nonzero":True,"largest_audition_peak":peak,
            "largest_quantised_pair_rms_error_db":max(rms_errors),
            "javascript_syntax":syntax,"visual_browser_review":"not performed"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path); parser.add_argument("candidate", type=Path)
    parser.add_argument("output", type=Path); args = parser.parse_args()
    manifest = json.loads((args.baseline / "manifest.json").read_text())
    if manifest != json.loads((args.candidate / "manifest.json").read_text()):
        raise ValueError("Musical/control manifests differ; this is not a matched comparison")
    args.output.mkdir(parents=True, exist_ok=True)
    rate = manifest["sample_rate"]
    report = {"schema":"electry-transition-comparison/20260911-v1", "sample_rate":rate,
              "baseline_directory":str(args.baseline.resolve()), "candidate_directory":str(args.candidate.resolve()),
              "matched_manifest":True, "normalisation":"none in measurements; constant whole-file RMS match for listening",
              "interpretation":"Signal descriptors, not perceptual quality or realism scores", "takes":[]}
    waveform_cache = {}
    for take in manifest["takes"]:
        ownership = verify_score(take)
        for tap, _ in TAPS:
            waves, checksums, measurements = {}, {}, {}
            for version, source in (("baseline",args.baseline),("candidate",args.candidate)):
                samples, checksum = shared.read_float_wav(source / take[tap], rate, take["frames"])
                waves[version] = samples; checksums[version] = checksum
                measurements[version] = shared.measure(samples, take["events"], rate)
                measurements[version]["transitions"] = transition_measurements(samples,take,rate)
            waveform_cache[(take["id"],tap)] = (waves["baseline"],waves["candidate"])
            before, after = measurements["baseline"],measurements["candidate"]
            report["takes"].append({"id":take["id"],"tap":tap,"ownership_check":ownership,
                "sha256":checksums,"sample_identical":checksums["baseline"]==checksums["candidate"],
                "measurements":measurements,
                "difference_rms_dbfs":shared.db(shared.rms(waves["candidate"]-waves["baseline"])),
                "rms_change_db":after["rms_dbfs"]-before["rms_dbfs"],
                "audition_constant_gains":shared.write_audition_pair(args.output/"audio", f"{take['id']}-{tap}",waves,rate)})
    (args.output/"comparison.json").write_text(json.dumps(report,indent=2)+"\n")
    (args.output/"score.json").write_text(json.dumps(manifest,indent=2)+"\n")
    lines = ["# Continuous palm-transition benchmark", "",
        "Original 105 BPM rolling D / Cadd9 / G chord score, low-register companion and isolated controls.",
        "Matched seeded events; all fretting-owner counters balance and repicks do not add owners.",
        "Raw float mono WAVs at 44.1 kHz. Crunch and Modern process the same DI; no acoustic feedback.",
        "Listening copies use constant whole-file gain. No per-note normalisation or dynamic gain is added.", "",
        "The continuous no-pick controls intentionally do not restore absorbed string energy after lifting the palm.",
        "Transition windows in comparison.json describe command-adjacent RMS, spectra and sample steps.",
        "Style selection is a latch: the following pick applies its hand contact. CC2 pressure is continuous.",
        "A maximum sample-step descriptor is not a click or realism score; normal attacks also create large steps.", "",
        "| Take | Tap | Peak before / after dBFS | RMS change dB | Raw identical |",
        "| --- | --- | ---: | ---: | --- |"]
    for entry in report["takes"]:
        m = entry["measurements"]
        lines.append(f"| {entry['id']} | {entry['tap']} | {m['baseline']['peak_dbfs']:.2f} / "
                     f"{m['candidate']['peak_dbfs']:.2f} | {entry['rms_change_db']:+.2f} | {entry['sample_identical']} |")
    (args.output/"report.md").write_text('\n'.join(lines)+'\n')
    listening_page(args.output,manifest,waveform_cache)
    delivery = validate_listening_artifacts(args.output,manifest)
    (args.output / "listening-validation.json").write_text(json.dumps(delivery,indent=2)+'\n')
    print(f"Validated {len(manifest['takes'])} balanced scores and {len(report['takes'])} paired WAVs.")
    print(f"Validated {delivery['validated_pcm_audio_files']} PCM playback assets and all local links; {delivery['javascript_syntax']}.")
    print(f"Listening page: {args.output / 'listening.html'}")


if __name__ == "__main__":
    main()
