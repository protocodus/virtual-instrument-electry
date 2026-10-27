#!/usr/bin/env python3
"""Write the committed demo phrases in Docs/audio/midi/ and Docs/audio/midi/more/.

One phrase per factory rig (README G3, G3a; a bank of four, so every rig),
written for what the rig is for: clean arpeggios, harmonics and a slide on
Factory Default; palm-muted chugs, power chords, a gallop and a pinch
harmonic on Drop-E Metal; Sustain, Mute and Dead notes and a mute-pressure
sweep on Mute / Dead DI; bends, hammer-ons, a slide and a resonance swell on
Blues Rock Lead. The four More demos (G3d) are played through the rigs named
in Tools/RenderPresetDemos.cpp. File names follow the rig names in
Source/PluginProcessor.cpp and the showcase names in the renderer, so a
rename fails here.

The MIDI map the phrases use (README, Keyswitches):
- 12-14 pick stroke (Down, Up, Alternate) and 15-21 play style (Sustain,
  Mute, Hammer, Harmonic, Pinch, Slide, Dead) are latching keyswitches. A
  phrase presses one 10 ticks before the note it sets and holds it 24 ticks
  (14 in the More demos), and sets its stroke and style at tick 48.
- 22 is finger vibrato and 23 the tremolo-picking wrist, held as gestures.
- 40-98 is the playable range, written an octave above sounding pitch.
- 100-107 repick the note held on strings 8..1.
- CC 1 is the performance resonance, CC 2 the bridge-hand mute pressure, and
  the bender spans +/-2 semitones. Every phrase centres the bender and zeroes
  both controllers at tick 0 (the order of those resets does not matter) and
  leaves them at rest by its end.

Each file is format 0 at 480 PPQ with a track name, tempo and 4/4 time
signature. Events keep the order they are written in here within a tick, so
a note-off written before a note-on at the same tick stays before it. Every
phrase's last event falls at 5.5-9.5 s, so the renderer's 2.5 s tail keeps
each demo within 8-12 s (G3c).

The files are sources, not build outputs: rerun this only to change a phrase
on purpose, and commit the result in its own commit. --check fails unless the
committed files match this script byte for byte.
"""
import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "Docs/audio/midi"
PPQ = 480

DOWN, UP, ALTERNATE = 12, 13, 14
SUSTAIN, MUTE, HAMMER, HARMONIC, PINCH, SLIDE, DEAD = range(15, 22)
VIBRATO, TREMOLO = 22, 23
LOWEST, HIGHEST = 40, 98
STRING_4_TRIGGER = 104  # 100..107 repick strings 8..1
RESONANCE, MUTE_PRESSURE = 1, 2
# Written open strings 8..1: E1 B1 E2 A2 D3 G3 B3 E4, sounding an octave lower.
OPEN = [40, 47, 52, 57, 62, 67, 71, 76]
LOW_E = OPEN[0]


class Phrase:
    """One demo's events, kept in the order they are written."""

    def __init__(self, press_ticks):
        self.press_ticks = press_ticks
        self.events = []

    def note(self, at, until, keys, velocity):
        for key in keys if isinstance(keys, list) else [keys]:
            if not (DOWN <= key <= TREMOLO or LOWEST <= key <= HIGHEST or 100 <= key <= 107):
                raise ValueError(f"key {key} is outside the instrument's MIDI map")
            self.events += [(at, bytes([0x90, key, velocity])), (until, bytes([0x80, key, 0]))]

    def press(self, at, *keys):
        """Press keyswitches for the note at `at`."""
        self.note(at - 10, at - 10 + self.press_ticks, list(keys), 100)

    def cc(self, at, controller, value):
        self.events.append((at, bytes([0xB0, controller, value])))

    def cc_ramp(self, at, step, controller, start, end, steps):
        for i in range(steps + 1):
            self.cc(at + step * i, controller, round(start + (end - start) * i / steps))

    def bend(self, at, semitones):
        value = 8192 + round(semitones / 2 * (8191 if semitones > 0 else 8192))
        self.events.append((at, bytes([0xE0, value & 127, value >> 7])))

    def bend_ramp(self, at, step, start, end, steps):
        for i in range(steps + 1):
            self.bend(at + step * i, start + (end - start) * i / steps)

    def rest(self, *controllers):
        self.bend(0, 0)
        for controller in controllers:
            self.cc(0, controller, 0)


def factory_default(p):
    p.rest(RESONANCE, MUTE_PRESSURE)
    p.press(48, DOWN, SUSTAIN)
    # An E arpeggio across all eight strings, left ringing.
    for i, key in enumerate([40, 47, 52, 59, 64, 66, 71, 76]):
        p.note(240 + 240 * i, 2150, key, [70, 78, 86][i % 3])
    p.press(2160, HARMONIC)
    for i, key in enumerate([64, 69, 74, 79]):
        p.note(2160 + 240 * i, 3360, key, 92)
    p.press(3360, SUSTAIN)
    p.note(3360, 3840, 79, 88)
    p.press(3840, SLIDE)
    p.note(3840, 4320, 83, 80)
    p.press(4320, SUSTAIN)
    p.note(4320, 4560, 81, 84)
    p.note(4560, 4800, 79, 78)
    p.note(4800, 6480, [40, 47, 52, 55, 59, 64], 86)


def drop_e_metal(p):
    def riff(at, chugs, chord):
        # Muted sixteenths on the low E, accented on the beat, then a ringing
        # power chord with one more chug under it, and back to Mute.
        for i in list(range(chugs)) + [chugs + 1]:
            p.note(at + 120 * i, at + 120 * i + 96, LOW_E, 112 if i % 4 == 0 else 96)
        p.press(at + 120 * chugs, SUSTAIN)
        p.note(at + 120 * chugs, at + 120 * chugs + 216, chord, 120)
        p.press(at + 120 * (chugs + 2), MUTE)

    p.rest(RESONANCE, MUTE_PRESSURE)
    p.press(48, DOWN, SUSTAIN, ALTERNATE, MUTE)
    riff(240, 6, [46, 53, 58])
    riff(1200, 6, [45, 52, 57])
    riff(2160, 6, [46, 53, 58])
    riff(3120, 2, [48, 55, 60])
    riff(3600, 2, [45, 52, 57])
    for i in range(12):  # the gallop
        p.note(4080 + 120 * i, 4176 + 120 * i, LOW_E, 104 if i % 3 == 0 else 90)
    p.press(5520, PINCH)
    p.note(5520, 6000, 62, 124)
    p.note(5640, 6000, VIBRATO, 110)
    p.press(6000, SUSTAIN)
    p.note(6000, 7440, [40, 47, 52, 55], 122)
    p.press(7440, MUTE)
    for i in range(4):
        p.note(7440 + 120 * i, 7536 + 120 * i, LOW_E, 116)
    p.press(7920, SUSTAIN)
    p.note(7920, 9840, [41, 48, 53], 124)


def mute_dead_di(p):
    styles = {"S": SUSTAIN, "M": MUTE, "D": DEAD}

    def bar(at, line, stop=None):
        # Sixteenths, accented on the beat. A style key is pressed for the
        # first note and at each change; a Sustain note rings to 5 ticks
        # before the next, Mute and Dead notes stop 14 ticks short. `stop`
        # cuts off a note still sounding there.
        previous = None
        for i, word in enumerate(line.split()):
            t, style, key = at + 120 * i, word[0], int(word[1:])
            if style != previous:
                p.press(t, styles[style])
            until = t + (115 if style == "S" else 106)
            p.note(t, until if stop is None else min(until, stop), key, 100 if i % 4 == 0 else 84)
            previous = style

    p.rest(MUTE_PRESSURE, RESONANCE)
    p.press(48, DOWN, SUSTAIN, ALTERNATE)
    bar(240, "S62 D62 D62 S64 M52 M52 D62 S67 M52 D62 S60 D60 M52 M52 D57 S62", stop=2150)
    bar(2160, "S62 D62 D62 S64 M52 M52 D62 S67 M52 D62 S60 D60 M54 M54 D59 S64")
    # Low chugs while the mute pressure rises.
    p.press(4080, MUTE)
    p.cc_ramp(4080, 120, MUTE_PRESSURE, 0, 120, 15)
    for i in range(16):
        p.note(4080 + 120 * i, 4176 + 120 * i, LOW_E, 104)
    p.press(6000, DEAD)
    p.cc(6000, MUTE_PRESSURE, 0)
    p.note(6000, 6096, [52, 57, 62], 110)
    p.press(6240, SUSTAIN)
    p.note(6240, 7440, [52, 59, 64], 106)


def blues_rock_lead(p):
    p.rest(MUTE_PRESSURE, RESONANCE)
    p.press(48, DOWN, SUSTAIN)
    p.note(240, 480, 72, 96)
    p.note(480, 1440, 74, 108)
    p.bend_ramp(528, 16, 0, 2, 12)  # a whole-tone bend, then vibrato on it
    p.note(768, 1392, VIBRATO, 90)
    p.bend(1488, 0)
    p.press(1560, SUSTAIN)
    p.note(1560, 1720, 69, 96)
    p.press(1720, HAMMER)
    p.note(1720, 1880, 72, 80)
    p.press(1880, HAMMER)
    p.note(1880, 2030, 69, 80)
    p.press(2040, SUSTAIN)
    p.note(2040, 2280, 64, 92)
    p.note(2280, 2520, 67, 96)
    p.press(2520, SLIDE)
    p.note(2520, 2760, 72, 90)
    p.press(2760, SUSTAIN)
    p.note(2760, 3000, 76, 104)
    p.note(3000, 3960, 79, 112)
    p.bend_ramp(3096, 12, 0, 1, 12)  # a half-step bend, vibrato, release
    p.note(3288, 3864, VIBRATO, 100)
    p.bend_ramp(3816, 20, 1, 0, 6)
    for i, key in enumerate([81, 79, 76, 74]):
        p.note(4080 + 120 * i, 4200 + 120 * i, key, 100 - 3 * i)
    # The resonance swell starts with the double-stop and rises under the
    # vibrato.
    p.cc(4560, RESONANCE, 0)
    p.note(4560, 6000, [69, 72], 104)
    p.note(4848, 5904, VIBRATO, 80)
    p.cc_ramp(4704, 144, RESONANCE, 10, 80, 7)
    p.cc(6096, RESONANCE, 0)


def full_range(p):
    p.rest(RESONANCE, MUTE_PRESSURE)
    p.press(48, DOWN, SUSTAIN)
    for i, key in enumerate(OPEN):
        p.note(240 + 480 * i, 4320, key, 84 + 3 * i)
    # Up the high E string to its 22nd fret.
    for i, key in enumerate([79, 81, 83, 86, 88, 91, 93, 95]):
        p.note(4320 + 240 * i, 4560 + 240 * i, key, 88)
    p.note(6240, 7440, 98, 100)
    p.note(6480, 7440, VIBRATO, 80)
    p.note(7440, 8640, [40, 47, 52], 104)


def picking_hand_triggers(p):
    p.rest(RESONANCE, MUTE_PRESSURE)
    p.press(48, ALTERNATE, SUSTAIN)
    # Eight-string shapes at the open position, the 3rd and the 5th fret,
    # cross-picked on strings 4..1, accented at each group's start.
    pattern = [(0, 92), (1, 80), (2, 80), (3, 80), (1, 92), (2, 80), (3, 80)]
    shapes = [(240, 2160, 0, 7), (2160, 4080, 3, 7), (4080, 6000, 5, 7), (6000, 8160, 0, 3)]
    for at, until, fret, picks in shapes:
        p.note(at, until, [key + fret for key in reversed(OPEN)], 72)
        for i, (string, velocity) in enumerate(pattern[:picks]):
            p.note(at + 240 + 240 * i, at + 360 + 240 * i, STRING_4_TRIGGER + string, velocity)


def tremolo_picking(p):
    p.rest(RESONANCE, MUTE_PRESSURE)
    p.press(48, ALTERNATE, SUSTAIN)
    for i, key in enumerate([76, 79, 83, 81, 79, 78]):
        p.note(240 + 960 * i, 1200 + 960 * i, key, 104)
    p.press(6000, MUTE)
    p.note(6000, 7680, [40, 47, 52], 112)
    p.note(235, 7680, TREMOLO, 96)  # the wrist, held under the whole phrase


def whammy_and_feedback(p):
    p.rest(RESONANCE, MUTE_PRESSURE)
    p.press(48, DOWN, SUSTAIN)
    p.note(240, 2640, [40, 47, 52], 120)
    p.bend_ramp(720, 60, 0, -2, 16)  # the dive, held, and back
    p.bend_ramp(2160, 60, -2, 0, 8)
    # A held note into feedback: resonance up, then a bend and vibrato.
    p.note(2880, 8160, 83, 116)
    p.cc_ramp(3120, 120, RESONANCE, 0, 127, 24)
    p.bend_ramp(6240, 60, 0, 1, 8)
    p.note(6720, 7920, VIBRATO, 96)
    p.bend_ramp(7680, 60, 1, 0, 4)
    p.cc(8160, RESONANCE, 0)


# Program order, as Source/PluginProcessor.cpp names the rigs: phrase, tempo.
RIGS = [
    (factory_default, 92),
    (drop_e_metal, 132),
    (mute_dead_di, 100),
    (blues_rock_lead, 84),
]
# File order, as the showcase table in Tools/RenderPresetDemos.cpp names them.
MORE = [
    (full_range, 120),
    (picking_hand_triggers, 120),
    (tremolo_picking, 120),
    (whammy_and_feedback, 120),
]


def slug(name):
    return re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")


def vlq(value):
    data = [value & 127]
    while value > 127:
        value >>= 7
        data.insert(0, 128 | (value & 127))
    return bytes(data)


def smf(name, bpm, phrase):
    def meta(kind, data):
        return b"\0\xff" + bytes([kind]) + vlq(len(data)) + data

    tempo = round(60_000_000 / bpm)
    track = (meta(0x03, f"Electry - {name}".encode()) + meta(0x51, tempo.to_bytes(3, "big"))
             + meta(0x58, bytes([4, 2, 24, 8])))
    last = 0
    for tick, message in sorted(phrase.events, key=lambda event: event[0]):
        track += vlq(tick - last) + message
        last = tick
    seconds = last / PPQ * 60 / bpm
    if not 5.5 <= seconds <= 9.5:
        raise ValueError(f"{name}: its last event at {seconds:.2f} s is outside 5.5-9.5 s")
    track += b"\0\xff\x2f\0"
    return b"MThd" + (6).to_bytes(4, "big") + bytes([0, 0, 0, 1]) + PPQ.to_bytes(2, "big") \
        + b"MTrk" + len(track).to_bytes(4, "big") + track


def files():
    processor = (ROOT / "Source/PluginProcessor.cpp").read_text()
    rigs = re.findall(r'"([^"]+)"', re.search(r"factoryProgramNames \{([^}]*)\}", processor).group(1))
    showcases = re.findall(r'\{ "([^"]+)", \d+ \}', (ROOT / "Tools/RenderPresetDemos.cpp").read_text())
    result = {}
    for folder, names, phrases, press_ticks in [("", rigs, RIGS, 24), ("more/", showcases, MORE, 14)]:
        if len(names) != len(phrases):
            raise ValueError(f"{len(names)} names for {len(phrases)} {folder or 'rig '}phrases")
        for number, (name, (write, bpm)) in enumerate(zip(names, phrases), start=1):
            phrase = Phrase(press_ticks)
            write(phrase)
            result[f"{folder}{number:02d}-{slug(name)}.mid"] = smf(name, bpm, phrase)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="fail unless the committed files match")
    args = parser.parse_args()
    expected = files()
    found = {path.relative_to(OUT).as_posix(): path.read_bytes()
             for path in [*OUT.glob("*.mid"), *OUT.glob("more/*.mid")]}
    if args.check:
        stale = sorted(set(found) ^ set(expected) | {name for name in expected if found.get(name) != expected[name]})
        if stale:
            sys.exit(f"error: Docs/audio/midi differs from {pathlib.Path(__file__).name} in: {', '.join(stale)}")
        print(f"{len(expected)} demo phrases match {pathlib.Path(__file__).name}")
        return
    (OUT / "more").mkdir(parents=True, exist_ok=True)
    for name in set(found) - set(expected):
        (OUT / name).unlink()
    for name, data in expected.items():
        (OUT / name).write_bytes(data)
    print(f"wrote {len(expected)} demo phrases to {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
