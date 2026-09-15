"""Original showroom performances, expressed in beats and sounding MIDI pitch.

The renderer consumes the shipping DSP directly. MIDI companions transpose only
pitched notes by +12 for Electry's written-guitar host layout. All instruments,
amplifiers, doubling and ambience come from Electry; mastering is constant gain.
"""
from __future__ import annotations

import json
import math
import struct
from pathlib import Path

SUSTAIN, MUTE, HAMMER, HARMONIC, PINCH, SLIDE, DEAD = range(7)
DOWN, UP, ALT = range(3)

ENGINE_DEFAULTS = dict(
    guitarBuild=.8, pickupSelector=2, pickupType=.32, toneKnob=.70,
    bodyResonance=.35, stringAge=.30, pickPosition=.18, pickHardness=.58,
    pickNoise=.5, fingerNoise=.4, releaseNoise=.4, muteDamping=.55,
    bendTimeSeconds=.28, velocityAmount=.85, outputDb=-6.0,
    artifactAmount=.18, sympatheticAmount=.20, palmMute=0.0,
    strumSpreadSeconds=0.0, tremoloRateHz=12.0, resonanceDepth=.35,
    mode=1,
)
FX_DEFAULTS = dict(distortion=0.0, amp=0.0, ampModel=0,
                   compressor=0.0, delay=0.0, room=0.0)


class Score:
    def __init__(self, slug, title, bpm, description, engine=None, fx=None,
                 features=None):
        self.slug, self.title, self.bpm = slug, title, bpm
        self.description = description
        self.engine = ENGINE_DEFAULTS | (engine or {})
        self.fx = FX_DEFAULTS | (fx or {})
        self.features = features or []
        self.events = []
        self.allow_repeated_pitches = False
        self.sections = []
        self.end = 0.0
        self.tail = 5.0

    def event(self, beat, op, *args):
        assert math.isfinite(beat) and beat >= 0
        self.events.append((float(beat), op, args))
        self.end = max(self.end, float(beat))

    def note(self, beat, pitch, duration, velocity=.8, style=SUSTAIN, pick=ALT):
        self.chord(beat, [pitch], duration, velocity, style, pick)

    def chord(self, beat, pitches, duration, velocity=.8, style=SUSTAIN, pick=DOWN):
        pitches = list(pitches)
        assert duration > 0 and pitches and len(pitches) <= 8
        assert len(set(pitches)) == len(pitches)
        assert all(28 <= p <= 86 for p in pitches)
        assert 0 < velocity <= 1 and 0 <= style <= 6 and 0 <= pick <= 2
        # MIDI and direct render share the same 7-bit performance resolution.
        velocity = max(1, round(127 * velocity)) / 127
        self.event(beat, "N", style, pick, velocity, *pitches)
        for pitch in pitches:
            self.event(beat + duration, "O", pitch)

    def control(self, beat, op, value):
        assert op in ("bend", "vibrato", "palm", "resonance", "sustain", "tremolo", "solo")
        if op == "bend":
            value = round(value * (8191 if value >= 0 else 8192)) / (8191 if value >= 0 else 8192)
        elif op not in ("sustain", "solo"):
            value = round(127 * value) / 127
        self.event(beat, op, value)

    def ramp(self, op, start, end, first, last, steps=20):
        for i in range(steps + 1):
            u = i / steps
            self.control(start + (end - start) * u, op, first + (last - first) * u)

    def section(self, beat, label):
        self.sections.append(dict(beat=beat, seconds=round(.12 + beat*60/self.bpm, 3), label=label))

    def ordered(self):
        # Release old ownership before changing hands and starting a new note.
        return sorted(self.events, key=lambda x: (x[0], 0 if x[1] == "O" else 2 if x[1] == "N" else 1))

    def write(self, directory):
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        score_path = directory / (self.slug + ".score")
        with score_path.open("w") as out:
            out.write("# Electry showroom score v1; absolute seconds, sounding pitch\n")
            for k, v in self.engine.items():
                out.write(f"E {k} {v}\n")
            for k, v in self.fx.items():
                out.write(f"F {k} {v}\n")
            out.write(f"END {.12 + self.end*60/self.bpm + self.tail:.9f}\n")
            for beat, op, args in self.ordered():
                out.write(f"{.12 + beat*60/self.bpm:.9f} {op} " + " ".join(str(a) for a in args) + "\n")
        metadata = dict(name=self.title, slug=self.slug, bpm=self.bpm,
            description=self.description, features=self.features,
            origin="Curated showcase preset using shipping Electry controls",
            engine=self.engine, fx=self.fx | dict(enabled=True, oversampling="High"),
            sample_rate=96000, sections=self.sections,
            performance_seconds=round(self.end*60/self.bpm, 3), tail_seconds=self.tail,
            playStyleKeysHold=False,
            midi_layout="Electry written pitch: sounding notes +12; keyswitches unchanged; LATCH mode")
        renamed = dict(toneKnob="tone", bendTimeSeconds="bendTime",
            velocityAmount="velocity", outputDb="output", artifactAmount="artifacts",
            sympatheticAmount="sympathetic", strumSpreadSeconds="strumSpread",
            tremoloRateHz="tremoloRate", mode="outputMode")
        host = {renamed.get(k,k):v for k,v in self.engine.items()}
        host["strumSpread"] = round(host["strumSpread"]*1000, 6)
        host["resonanceDepth"] = round(host["resonanceDepth"]*100, 6)
        metadata["host_parameter_values"] = host | self.fx | dict(fxEnabled=1, fxOversampling=1)
        metadata["host_parameter_units"] = "Plain host parameter values; output dB, bendTime seconds, strumSpread ms, resonanceDepth percent; other amounts 0..1; choices zero-indexed"
        (directory / (self.slug + ".preset.json")).write_text(json.dumps(metadata, indent=2) + "\n")
        self.write_midi(directory / (self.slug + ".mid"))
        return metadata

    def write_midi(self, path):
        def vlq(value):
            data = [value & 127]
            while value >> 7:
                value >>= 7
                data.insert(0, (value & 127) | 128)
            return bytes(data)

        ppq = 9600
        lead = round(.12 * self.bpm/60 * ppq)
        tempo = round(60000000 / self.bpm)
        title = self.title.encode("utf-8")
        midi = [(0, b"\xff\x51\x03" + tempo.to_bytes(3, "big")),
                (0, b"\xff\x03" + vlq(len(title)) + title)]
        for section in self.sections:
            label = section["label"].encode("utf-8")
            midi.append((lead + round(section["beat"] * ppq), b"\xff\x06" + vlq(len(label)) + label))
        gesture_active = {"vibrato": False, "tremolo": False}
        current_pick = current_style = None
        for beat, op, args in self.ordered():
            tick = lead + round(beat * ppq)
            def add(data, at=tick):
                midi.append((at, bytes(data)))
            if op == "N":
                style, pick, velocity, *notes = args
                keys = []
                if pick != current_pick:
                    keys.append(12 + pick)
                    current_pick = pick
                if style != current_style:
                    keys.append(15 + style)
                    current_style = style
                for key in keys:
                    add([0x90, key, 127])
                    add([0x80, key, 0], tick + 1)
                for n in notes:
                    add([0x90, n + 12, round(velocity * 127)])
            elif op == "O":
                add([0x80, args[0] + 12, 0])
            elif op == "bend":
                value = args[0]
                raw = min(16383, max(0, 8192 + round(value * (8191 if value >= 0 else 8192))))
                add([0xe0, raw & 127, raw >> 7])
            elif op in ("palm", "resonance", "sustain"):
                add([0xb0, {"palm": 2, "resonance": 1, "sustain": 64}[op], round(args[0]*127)])
            elif op in gesture_active:
                key = 22 if op == "vibrato" else 23
                if gesture_active[op]:
                    add([0x80, key, 0])
                gesture_active[op] = args[0] > 0
                if gesture_active[op]:
                    add([0x90, key, max(1, round(args[0]*127))])
            elif op == "solo":
                # The host's solo-string keys are latched; clear before replace.
                add([0x90, 32, 127]); add([0x80, 32, 0], tick + 1)
                mask = int(args[0])
                for string in range(8):
                    if mask & (1 << string):
                        add([0x90, 24+string, 127]); add([0x80, 24+string, 0], tick+1)
        data, previous = bytearray(), 0
        for tick, payload in sorted(midi, key=lambda e: e[0]):
            data += vlq(tick - previous) + payload
            previous = tick
        finish = lead + round((self.end + self.tail*self.bpm/60) * ppq)
        data += vlq(max(0, finish-previous)) + b"\xff\x2f\x00"
        Path(path).write_bytes(b"MThd" + struct.pack(">IHHH", 6, 0, 1, ppq)
                               + b"MTrk" + struct.pack(">I", len(data)) + data)
