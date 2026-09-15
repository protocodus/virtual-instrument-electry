"""Five original, physically playable solo-guitar showroom performances.

The little scheduler keeps a new pick from inheriting an older MIDI note-off.
Pitches are the shipping Drop-E guitar's sounding fundamentals; Harmonic style
naturally exposes the midpoint partial above that fundamental.
"""
from score import Score, SUSTAIN, MUTE, HARMONIC, PINCH, DEAD, DOWN, UP, ALT


class _Guitar:
    def __init__(self, score):
        self.score, self.hits = score, []

    def hit(self, beat, pitches, duration, velocity=.8, style=SUSTAIN, pick=DOWN):
        if isinstance(pitches, int):
            pitches = [pitches]
        self.hits.append((beat, list(pitches), duration, velocity, style, pick))

    def finish(self):
        s = self.score
        releases = {}
        for beat, pitches, duration, velocity, style, pick in sorted(self.hits, key=lambda h: h[0]):
            for pitch in pitches:
                if pitch in releases:
                    index = releases[pitch]
                    old_beat, op, args = s.events[index]
                    if old_beat >= beat - .012:
                        s.events[index] = (max(0., beat - .012), op, args)
            s.chord(beat, pitches, duration, velocity, style, pick)
            for index, pitch in enumerate(pitches, len(s.events) - len(pitches)):
                releases[pitch] = index
        s.end = max(event[0] for event in s.events)
        return s


def _glasshouse():
    s = Score('02-glasshouse-clean', 'Glasshouse Clean', 92,
        'A chiming E-add-nine signature unfolds into wide open-string chords, '
        'a quieter answering phrase and a generous final resolution.',
        engine=dict(pickupSelector=1, pickupType=.19, toneKnob=.81,
                    bodyResonance=.43, stringAge=.19, pickPosition=.22,
                    pickHardness=.49, pickNoise=.34, fingerNoise=.28,
                    releaseNoise=.26, artifactAmount=.12, sympatheticAmount=.31,
                    strumSpreadSeconds=.013, velocityAmount=.92, outputDb=1.5),
        fx=dict(ampModel=0, amp=.46, compressor=.18, delay=.095, room=.22),
        features=['Open-string add9 voicings', 'Ringing alternate-picked melody',
                  'Velocity-shaped repeats', 'Natural stereo string placement'])
    g = _Guitar(s)
    # Strings 6..1: Eadd9 024102; Aadd9 x02200; C#m7 x4645x;
    # Bsus4 x24452. Non-open fretted notes span at most three frets.
    shapes = [[40, 47, 54, 56, 59, 66], [45, 52, 57, 59, 64],
              [49, 56, 59, 64], [47, 54, 59, 64, 66]]
    s.section(0, 'The glass hook')
    for bar in range(4):
        t, ch = 4*bar, shapes[bar]
        g.hit(t, ch, 3.72, .76 if bar != 3 else .81)
        top = [66, 64, 64, 66][bar]
        # The same lyrical 3+3+2 answer, reharmonized each bar.
        g.hit(t+.754, top, .66, .79, pick=UP)
        g.hit(t+1.506, 64 if bar in (0, 3) else 59, .68, .68, pick=DOWN)
        g.hit(t+2.258, 59 if bar != 1 else 57, .46, .64, pick=UP)
        g.hit(t+2.758, top, .84, .84, pick=DOWN)
        if bar == 3:
            g.hit(t+3.503, [59, 64], .36, .62, pick=UP)
    s.section(16, 'An open-window answer')
    # A fingerboard-consistent, less dense contrast: C#m7 to Aadd9.
    for bar, ch in enumerate([shapes[2], shapes[1], shapes[0], shapes[3]], 4):
        t = 4*bar
        pattern = [(0, ch[0], 2.82, .68), (.505, ch[1], 2.55, .55),
                   (1.001, ch[2], 2.48, .62), (1.758, ch[-1], 1.12, .76),
                   (2.505, ch[-2], .68, .58), (3.006, ch[-1], .86, .70)]
        for at, pitch, dur, vel in pattern:
            g.hit(t+at, pitch, dur, vel, pick=ALT)
    s.section(32, 'The hook opens up')
    for bar in range(8, 12):
        t, ch = 4*bar, shapes[bar-8]
        g.hit(t, ch, 3.74, .86 if bar in (8, 11) else .80)
        top = [66, 64, 64, 66][bar-8]
        g.hit(t+.755, top, .65, .86, pick=UP)
        g.hit(t+1.506, 64 if bar in (8, 11) else 59, .62, .74)
        g.hit(t+2.255, ch[-2], .40, .69, pick=UP)
        g.hit(t+2.755, top, .95, .87)
    s.section(48, 'Home, still ringing')
    g.hit(48, shapes[0], 4.0, .83)
    g.hit(49.005, 64, 2.95, .63, pick=UP)
    return g.finish()


def _copper():
    s = Score('04-copper-crunch', 'Copper Crunch', 104,
        'A swaggering low-E riff answers clipped palm mutes with broad power '
        'chords and blues double stops, then lands on a sustained final E.',
        engine=dict(mode=2, pickupSelector=2, pickupType=.65, toneKnob=.74,
                    bodyResonance=.34, stringAge=.29, pickPosition=.15,
                    pickHardness=.72, pickNoise=.46, fingerNoise=.34,
                    releaseNoise=.31, muteDamping=.62, artifactAmount=.18,
                    sympatheticAmount=.18, strumSpreadSeconds=.002,
                    velocityAmount=.88, outputDb=3.0),
        fx=dict(ampModel=1, amp=.69, distortion=.15, compressor=.12,
                delay=.035, room=.115),
        features=['British crunch at High quality', 'Low-E power chords',
                  'Palm-muted push and release', 'Blues double-stop answer',
                  'Instrument-native double mode'])
    g = _Guitar(s)
    e5, g5, a5, b5 = [28, 35, 40], [31, 38, 43], [33, 40, 45], [35, 42, 47]
    s.section(0, 'Copper riff')
    def riff(t, variation=False, force=.0):
        g.hit(t, e5, .67, .89+force)
        g.hit(t+.755, 28, .21, .75, MUTE)
        g.hit(t+1.004, 28, .22, .86, MUTE, UP)
        g.hit(t+1.506, [35, 40], .36, .82, MUTE)
        g.hit(t+2.006, e5, .43, .89)
        g.hit(t+2.755, 28, .20, .77, MUTE, UP)
        g.hit(t+3.006, g5, .40, .91)
        g.hit(t+3.507, a5 if not variation else [38, 45, 50], .39, .94, pick=UP)
    riff(0)
    riff(4, True)
    riff(8, force=.02)
    g.hit(12, e5, .72, .94)
    g.hit(13.005, [55, 59], .42, .85)
    g.hit(13.505, [57, 61], .39, .90, pick=UP)
    g.hit(14.005, [55, 59], .57, .89)
    g.hit(14.755, [52, 55], .88, .84, pick=UP)
    s.section(16, 'The riff climbs')
    for bar, ch in enumerate([a5, a5, b5], 4):
        t = bar*4
        g.hit(t, ch, .78, .91)
        for at, vel in [(1.005, .72), (1.505, .80), (2.005, .89)]:
            g.hit(t+at, ch[0], .27, vel, MUTE, ALT)
        g.hit(t+2.758, ch, .84, .94, pick=UP)
    g.hit(28, [38, 45, 50], .78, .91)
    g.hit(29.005, a5, .74, .88)
    g.hit(30.005, g5, .73, .91)
    g.hit(31.005, [55, 59], .36, .86)
    g.hit(31.505, [54, 57], .33, .82, pick=UP)
    s.section(32, 'Final gear')
    riff(32, force=.04)
    riff(36, True, force=.03)
    riff(40, force=.05)
    g.hit(44, e5, .63, .95)
    g.hit(44.755, 28, .2, .82, MUTE)
    g.hit(45.005, g5, .38, .93)
    g.hit(45.505, a5, .4, .95, pick=UP)
    g.hit(46.005, [40, 47, 52], .88, .94)
    g.hit(47.006, [55, 59], .37, .87)
    g.hit(47.506, [52, 55], .32, .84, pick=UP)
    s.section(48, 'Copper seal')
    g.hit(48, [28, 35, 40, 47, 52], 3.6, .97)
    # Picked G/A/D changes belong to one moving hand. Leave a short lift
    # before its next chord; keep the longer written breaks elsewhere.
    hits = sorted(g.hits, key=lambda hit: hit[0])
    for i, hit in enumerate(hits[:-1]):
        beat, pitches, duration, velocity, style, pick = hit
        next_beat = hits[i + 1][0]
        if (tuple(pitches) in ((31, 38, 43), (33, 40, 45), (38, 45, 50))
                and duration < .5 and next_beat - beat <= .55):
            hits[i] = (beat, pitches, next_beat - beat - .016,
                       velocity, style, pick)
    g.hits = hits
    s = g.finish()
    # Keep the fretting finger planted during a repeated picked phrase.
    # Repeated pitched Note Ons still use the host's complete chord path;
    # every added MIDI owner receives its own balanced terminal Note Off.
    # The true string-repick keys currently add scalar strum pre-roll, so
    # they would put these rhythmic attacks behind the written groove.
    release_for, pending = {}, {}
    for index, (beat, op, args) in sorted(enumerate(s.events), key=lambda item: (
            item[1][0], 0 if item[1][1] == 'O' else 1)):
        if op == 'N':
            for pitch in args[3:]:
                assert pitch not in pending
                pending[pitch] = round(beat, 9)
        elif op == 'O':
            pitch = args[0]
            release_for[(pending.pop(pitch), pitch)] = index
    assert not pending

    def hold_finger(pitch, attacks, until):
        for beat in attacks:
            index = release_for[(round(beat, 9), pitch)]
            s.events[index] = (until, 'O', (pitch,))

    for t in (0, 4, 8, 32, 36, 40):
        hold_finger(28, [t+x for x in (0, .755, 1.004, 2.006, 2.755)], t+2.955)
        for pitch in (35, 40):
            hold_finger(pitch, [t+x for x in (0, 1.506, 2.006)], t+2.436)
    # Keep the deliberate breath after each middle-section bar; ownership
    # joins the within-bar pulses, rather than bridging those written rests.
    for t, root, upper in ((16, 33, (40, 45)), (20, 33, (40, 45)),
                            (24, 35, (42, 47))):
        hold_finger(root, [t+x for x in (0, 1.005, 1.505, 2.005, 2.758)], t+3.598)
        for pitch in upper:
            hold_finger(pitch, [t, t+2.758], t+3.598)
    s.allow_repeated_pitches = True
    # These are played hand shapes, not a succession of unrelated keyboard
    # chords. Keep each riff on its intended strings so a release tail cannot
    # send the next double stop from the open treble pair to frets 16/17.
    # Bit zero is the low E1 string; all masks are reachable in Drop E.
    strings = {
        (28, 35, 40): 0x07, (31, 38, 43): 0x07,
        (33, 40, 45): 0x07,
        (35, 42, 47): 0x0e, (38, 45, 50): 0x0e,
        (28,): 0x01, (33,): 0x01, (35,): 0x02,
        (35, 40): 0x06,
        (55, 59): 0x60, (57, 61): 0x60,
        (52, 55): 0x30, (54, 57): 0x30,
        (40, 47, 52): 0x1c,
        (28, 35, 40, 47, 52): 0x1f,
    }
    for beat, op, args in list(s.events):
        if op == 'N':
            s.control(beat, 'solo', strings[tuple(args[3:])])
    s.control(s.end, 'solo', 0)
    return s


def _ghost_pocket():
    s = Score('05-ghost-pocket', 'Ghost Pocket', 108,
        'A dry, close funk performance: syncopated E9 stabs, genuine dead-string '
        'ghosts, low-string pickups and a short horn-like chord answer.',
        engine=dict(pickupSelector=1, pickupType=.20, toneKnob=.84,
                    bodyResonance=.26, stringAge=.22, pickPosition=.13,
                    pickHardness=.65, pickNoise=.58, fingerNoise=.32,
                    releaseNoise=.36, muteDamping=.73, artifactAmount=.16,
                    sympatheticAmount=.12, strumSpreadSeconds=.006,
                    velocityAmount=.95, outputDb=2.0),
        fx=dict(ampModel=0, amp=.43, compressor=.34, room=.07),
        features=['True dead-note articulation', 'Sixteenth-note funk pocket',
                  'E9 and A13 chord color', 'Tight left-hand releases',
                  'Velocity and alternating pick direction'])
    g = _Guitar(s)
    e9, a13, b9 = [52, 56, 62, 66], [45, 55, 61, 66], [47, 51, 57, 61, 66]
    s.section(0, 'Pocket, immediately')
    def pocket(t, chord, root, fill=False, lift=.0):
        g.hit(t, chord, .225, .84+lift)
        g.hit(t+.502, [52, 56, 62], .115, .46, DEAD, UP)
        g.hit(t+.755, chord, .19, .77+lift, pick=DOWN)
        g.hit(t+1.255, root, .205, .75, MUTE, UP)
        g.hit(t+1.505, [52, 56, 62], .105, .39, DEAD)
        g.hit(t+1.755, chord[-3:], .185, .84, pick=UP)
        g.hit(t+2.256, chord, .21, .79+lift)
        g.hit(t+2.505, [52, 56, 62], .11, .48, DEAD, UP)
        g.hit(t+2.755, chord[-3:], .145, .75)
        if fill:
            g.hit(t+3.005, [59, 64], .20, .84)
            g.hit(t+3.255, [61, 66], .19, .88, pick=UP)
            g.hit(t+3.505, [59, 64], .31, .80)
        else:
            g.hit(t+3.255, root, .19, .81, MUTE, UP)
            g.hit(t+3.505, chord[-2:], .30, .85)
    for bar in range(4):
        pocket(4*bar, e9 if bar != 2 else a13, 40 if bar != 2 else 45,
               fill=bar == 3)
    s.section(16, 'Bass-side conversation')
    for bar, root in [(4, 45), (5, 40)]:
        t = bar*4
        chord = a13 if root == 45 else e9
        g.hit(t, chord, .28, .83)
        # A compact blues pickup, always back inside the groove.
        for at, pitch, vel in [(.755, root, .75), (1.005, root+3, .68),
                               (1.255, root+4, .83), (1.755, root+7, .75)]:
            g.hit(t+at, pitch, .19, vel, MUTE, ALT)
        g.hit(t+2.255, chord, .23, .89)
        g.hit(t+2.756, chord[-3:], .11, .45, DEAD, UP)
        g.hit(t+3.256, chord[-3:], .31, .82)
    pocket(24, b9, 47, lift=.02)
    pocket(28, a13, 45, fill=True)
    s.section(32, 'Pocket with a grin')
    for bar in range(8, 12):
        pocket(bar*4, e9 if bar in (8, 9, 11) else a13,
               40 if bar in (8, 9, 11) else 45, fill=bar == 11, lift=.045)
    s.section(48, 'One last nine')
    g.hit(48, e9, .24, .93)
    g.hit(48.505, e9[-3:], .115, .46, DEAD, UP)
    g.hit(48.755, [40, 52, 56, 62, 66], 2.7, .84)
    return g.finish()


def _northern():
    s = Score('06-northern-lights', 'Northern Lights', 250/3,
        'Midpoint harmonics sparkle above an E-minor open-string figure. '
        'The instrument\'s 360 ms delay answers on the eighth-note grid, '
        'with a clear melodic return and a long final chord.',
        engine=dict(pickupSelector=1, pickupType=.23, toneKnob=.79,
                    bodyResonance=.48, stringAge=.18, pickPosition=.24,
                    pickHardness=.43, pickNoise=.27, fingerNoise=.24,
                    releaseNoise=.23, artifactAmount=.10, sympatheticAmount=.36,
                    strumSpreadSeconds=.021, velocityAmount=.94, outputDb=2.5),
        fx=dict(ampModel=0, amp=.42, compressor=.19, delay=.31, room=.30),
        features=['Midpoint natural harmonics', '360 ms eighth-note delay pocket',
                  'Open-string minor arpeggios', 'Wide natural stereo',
                  'Harmonic-to-regular articulation contrast'])
    g = _Guitar(s)
    em = [40, 47, 52, 55, 59, 66]
    cmaj = [48, 52, 55, 59, 64]
    gmajor = [43, 50, 55, 59, 67]
    dsus = [50, 57, 62, 64]
    s.section(0, 'Lights on the horizon')
    g.hit(0, [40, 59], 3.45, .84, HARMONIC)
    g.hit(.755, 55, 2.50, .77, HARMONIC, UP)
    g.hit(1.505, 64, 2.30, .88, HARMONIC)
    g.hit(2.505, 59, 1.22, .78, HARMONIC, UP)
    g.hit(3.255, 55, .57, .65, HARMONIC)
    def arp(t, ch, lift=0.):
        g.hit(t, ch[0], 3.76, .74+lift)
        for at, index, dur, vel in [(.505, 1, 2.97, .55),
                                   (1.005, 2, 2.48, .65),
                                   (1.755, -1, .68, .84),
                                   (2.505, -2, .67, .66),
                                   (3.255, -1, .59, .78)]:
            g.hit(t+at, ch[index], dur, min(.98, vel+lift), pick=ALT)
    arp(4, em)
    arp(8, cmaj)
    arp(12, dsus)
    s.section(16, 'Constellations in motion')
    arp(16, gmajor, .045)
    arp(20, dsus, .03)
    arp(24, cmaj, .015)
    g.hit(28, [45, 52, 55, 59, 64], 3.72, .72)
    g.hit(29.005, 55, 1.14, .76, HARMONIC)
    g.hit(30.255, 59, 1.17, .80, HARMONIC, UP)
    g.hit(31.505, 64, .38, .78, HARMONIC)
    s.section(32, 'The horizon returns')
    arp(32, em, .055)
    arp(36, cmaj, .035)
    arp(40, dsus, .04)
    s.section(44, 'Lights held in the air')
    g.hit(44, em, 4.8, .85)
    g.hit(45.505, 64, 3.28, .70, pick=UP)
    g.hit(46.255, 59, 2.53, .73, HARMONIC)
    return g.finish()


def _midnight():
    s = Score('09-midnight-hollow', 'Midnight Hollow', 90,
        'A warm neck-pickup chord melody with a laid-back lilt: ninths, '
        'thirteenths and a borrowed minor-four color lead home to D major.',
        engine=dict(pickupSelector=0, pickupType=.59, toneKnob=.50,
                    bodyResonance=.56, stringAge=.52, pickPosition=.29,
                    pickHardness=.34, pickNoise=.25, fingerNoise=.27,
                    releaseNoise=.24, artifactAmount=.11, sympatheticAmount=.24,
                    strumSpreadSeconds=.017, velocityAmount=.95, outputDb=2.0),
        fx=dict(ampModel=0, amp=.46, compressor=.20, delay=.045, room=.18),
        features=['Warm neck-pickup voicing', 'Playable jazz chord melody',
                  'ii-V-I and borrowed minor harmony', 'Laid-back microtiming',
                  'Dynamic melodic top voice'])
    g = _Guitar(s)
    em9 = [40, 52, 55, 62, 66]          # 0x2032, with low E root
    a13 = [45, 55, 61, 66]              # x0x022
    dmaj9 = [50, 54, 61, 64]            # x5465x
    gmaj7 = [43, 54, 59, 62]            # 3x443x
    gm6 = [43, 52, 58, 62]              # 3x233x
    fsm7 = [42, 52, 57, 61]             # 2x222x
    b7flat9 = [47, 51, 57, 60, 66]      # x21212
    s.section(0, 'After-hours melody')
    # Chords and melody occupy one guitar; no parallel backing layer.
    def phrase(t, chord, melody, strong=False):
        g.hit(t, chord, 2.57, .76 if strong else .68)
        g.hit(t+.765, melody[0], .62, .83 if strong else .76, pick=UP)
        g.hit(t+1.516, melody[1], .60, .69, pick=DOWN)
        g.hit(t+2.267, melody[2], .61, .75, pick=UP)
        g.hit(t+3.015, chord[0], .67, .59, pick=DOWN)
        g.hit(t+3.517, chord[-2:], .34, .63, pick=UP)
    phrase(0, em9, [66, 67, 66], True)
    phrase(4, a13, [66, 64, 61])
    phrase(8, dmaj9, [64, 66, 69], True)
    g.hit(12, [50, 54, 61, 66], 2.69, .73)
    g.hit(12.765, 69, .62, .79, pick=UP)
    g.hit(13.516, 66, .66, .67)
    g.hit(14.517, 64, 1.20, .74, pick=UP)
    s.section(16, 'A borrowed shade')
    phrase(16, gmaj7, [62, 66, 62])
    phrase(20, gm6, [62, 64, 62])
    phrase(24, fsm7, [61, 64, 61])
    g.hit(28, b7flat9, 2.49, .75)
    g.hit(28.765, 66, .64, .79, pick=UP)
    g.hit(29.516, 63, .55, .69)
    g.hit(30.267, 60, .62, .72, pick=UP)
    g.hit(31.015, 47, .65, .65)
    g.hit(31.517, [57, 60, 63], .34, .69, pick=UP)
    s.section(32, 'The last table')
    phrase(32, em9, [66, 67, 66], True)
    phrase(36, a13, [66, 64, 61], True)
    phrase(40, dmaj9, [64, 66, 69], True)
    g.hit(44, [50, 54, 61, 66], 2.69, .78)
    g.hit(44.765, 69, .63, .81, pick=UP)
    g.hit(45.516, 66, .71, .72)
    g.hit(46.517, 64, 1.18, .67, pick=UP)
    s.section(48, 'Goodnight, D major')
    g.hit(48, [50, 54, 59, 64, 69], 4., .71)  # D6/9: x54455
    return g.finish()


def build_scores():
    return [_glasshouse(), _copper(), _ghost_pocket(), _northern(), _midnight()]
