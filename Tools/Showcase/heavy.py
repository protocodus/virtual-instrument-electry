"""Five original guitar performances: weight, touch, melody and velocity.

The arrangements are single performances. Double mode is Electry's own output
mode; no second instrument, backing track, or post-render layer is involved.
"""
from score import Score, SUSTAIN, MUTE, HAMMER, PINCH, SLIDE, DOWN, UP, ALT


def _finish(score, beat):
    for control in ("bend", "vibrato", "palm", "tremolo", "solo"):
        score.control(beat, control, 0)
    return score


def _power(score, beat, root, duration, velocity=.90, style=SUSTAIN,
           pick=DOWN):
    # On the three lowest strings this is one straight barre in Drop E.
    score.chord(beat, [root, root + 7, root + 12], duration,
                velocity, style, pick)


def _vibrato(score, start, end, strength=.42):
    score.ramp("vibrato", start, start + .22, .08, strength, steps=8)
    score.control(end, "vibrato", 0)


def _bend(score, start, peak, release, end, amount=1):
    score.ramp("bend", start, peak, 0, amount, steps=14)
    score.ramp("bend", release, end, amount, 0, steps=12)


def drop_e_titan():
    s = Score("01-drop-e-titan", "Drop-E Titan", 112,
        "A tightly voiced Drop-E riff with a memorable displaced accent, "
        "open power-chord answers, a spacious halftime bridge and a final stomp.",
        engine=dict(mode=2, toneKnob=1.0, stringAge=.10, pickHardness=.85,
            fingerNoise=.55, muteDamping=.85, velocityAmount=.70,
            outputDb=6.0, artifactAmount=.15, sympatheticAmount=.25,
            pickupSelector=2, pickupType=.32, strumSpreadSeconds=.003),
        fx=dict(distortion=.45, amp=.95, ampModel=2, compressor=.60,
                delay=.025, room=.10),
        features=["Factory Drop-E Metal foundation", "Internal Double mode",
                  "Lowest E1 and reachable power chords", "Palm-pressure groove",
                  "Picked accents and dynamic releases", "Halftime contrast"])
    s.section(0, "Titan hook: open impact, tight reply")

    def hook(at, answer=31, turn=False):
        s.control(at, "palm", 0)
        _power(s, at, 28, .64, .98)
        s.control(at+.69, "palm", .34)
        # The .75 / 1.5 / 2.75 accent is the identifying rhythmic cell.
        for t, d, v in ((.75,.18,.81),(1,.19,.73),(1.5,.40,.91),
                        (2.25,.17,.73),(2.5,.17,.79),(2.75,.39,.92)):
            s.note(at+t,28,d,v,MUTE,ALT)
        s.control(at+3.20, "palm", 0)
        _power(s, at+3.25, answer, .64, .93)
        s.control(at+4, "palm", .32)
        for t, v in ((4,.89),(4.5,.78),(4.75,.72),(5.25,.86)):
            s.note(at+t,28,.20,v,MUTE,ALT)
        s.control(at+5.70, "palm", 0)
        _power(s, at+5.75, 33, .65, .93)
        _power(s, at+6.50, 31 if not turn else 30, .41, .90)
        if turn:
            # One octave bark answers the bass hook, followed by real air.
            s.note(at+7.10,40,.48,.91,PINCH,DOWN)
        else:
            s.note(at+7.25,28,.42,.91,MUTE,DOWN)

    hook(0)
    hook(8,30,True)
    s.section(16, "Hook develops: climbing chord answers")
    hook(16,33)
    hook(24,31,True)
    s.section(32, "Halftime: let the low strings breathe")
    for i, root in enumerate((28,36,31,30)):
        b=32+i*4
        s.control(b,"palm",0)
        _power(s,b,root,1.72,.96)
        s.control(b+1.9,"palm",.50)
        s.note(b+2.0,28,.29,.86,MUTE,DOWN)
        s.note(b+2.75,28,.21,.77,MUTE,UP)
        s.note(b+3.25,28,.29,.94,MUTE,DOWN)
    s.section(48, "The hook returns")
    hook(48,31,True)
    s.section(56, "Final climb and low-E resolve")
    s.control(56,"palm",0)
    _power(s,56,36,1.35,.91)
    _power(s,57.5,33,.67,.93)
    _power(s,58.5,31,.43,.95)
    _power(s,59.25,30,.39,.96)
    _power(s,60,28,4,.99)
    return _finish(s,64)


def velvet_blues():
    s = Score("03-velvet-blues", "Velvet Blues", 88,
        "A warm neck-pickup voice sings an original blues melody: unhurried "
        "whole-tone bends, small chord replies and a lyrical twelve-bar arc.",
        engine=dict(mode=1, pickupSelector=0, pickupType=.56, toneKnob=.78,
            bodyResonance=.62, stringAge=.20, pickPosition=.20,
            pickHardness=.82, fingerNoise=.48, bendTimeSeconds=.15,
            velocityAmount=.90, outputDb=3.2, sympatheticAmount=.35,
            strumSpreadSeconds=.007),
        fx=dict(distortion=.10, amp=.58, ampModel=1, compressor=.28,
                delay=.18, room=.28),
        features=["Warm neck-pickup British crunch", "Defined picked attacks",
                  "Original twelve-bar melody",
                  "Whole-tone finger bends", "Delayed fretting-hand vibrato",
                  "Same-string hammer-ons", "Velocity-sensitive chord replies"])
    s.section(0,"The voice: a bend, a breath, an answer")
    s.control(0,"solo",128)

    def hook(b, higher=False):
        shift=5 if higher else 0
        s.note(b,69+shift,.53,.79)
        s.note(b+.67,72+shift,.25,.72, pick=UP)
        s.note(b+1,74+shift,1.75,.88)
        _bend(s,b+1.18,b+1.65,b+2.20,b+2.65)
        s.note(b+2.88,72+shift,.40,.71, pick=UP)
        s.note(b+3.43,69+shift,.44,.78)

    hook(0)
    s.note(4,67,.42,.70)
    s.note(4.66,69,1.13,.85)
    _vibrato(s,5.18,5.72,.40)
    s.note(6.12,64,.46,.63)
    s.note(6.83,67,.48,.69)
    s.note(7.49,69,.42,.79)
    s.control(8,"solo",0)
    s.chord(8,[45,52,55,59,64],.68,.64,SUSTAIN,DOWN)
    s.control(8.9,"solo",128)
    s.note(9,69,.72,.77)
    s.note(9.66,72,.52,.72,HAMMER)
    s.note(10.35,71,.31,.68)
    s.note(10.85,67,.62,.71)
    s.note(11.60,69,.31,.76)
    hook(12)
    s.section(16,"Lift to the IV: a brighter answer")
    hook(16,True)
    s.note(20,77,.43,.76)
    s.note(20.65,76,.53,.74)
    s.note(21.40,74,1.19,.84)
    _vibrato(s,21.92,22.51,.39)
    s.note(22.82,72,.39,.72)
    s.note(23.40,69,.46,.77)
    s.section(24,"A little more fire, then space")
    s.control(24,"solo",0)
    s.chord(24,[45,52,57,60,64],.72,.72)
    s.control(24.9,"solo",128)
    s.note(25,67,.49,.70)
    s.note(25.42,69,.49,.70,HAMMER)
    s.note(25.84,72,.48,.74,HAMMER)
    s.note(26.50,74,1.15,.88)
    _bend(s,26.65,27.05,27.28,27.60)
    s.note(28,72,.46,.73)
    s.note(28.65,69,1.21,.82)
    _vibrato(s,29.18,29.78,.48)
    s.note(30.2,67,.35,.66)
    s.note(30.83,64,.55,.61)
    s.section(32,"Turnaround: tension and a falling reply")
    for beat,pitch,duration,velocity in (
        (32,76,.58,.87),(32.84,74,.44,.77),(33.50,73,.70,.79),
        (34.50,71,.31,.74),(35,70,.29,.68),(35.5,69,.34,.73),
        (36,74,.72,.84),(37,72,.49,.74),(37.67,69,.48,.71),
        (38.40,67,.50,.68),(39.15,64,.58,.65)):
        s.note(beat,pitch,duration,velocity)
    s.section(40,"One last singing bend, a warm final A")
    s.note(40,72,.48,.77)
    s.note(40.66,74,1.81,.92)
    _bend(s,40.85,41.30,41.95,42.40)
    s.note(42.68,72,.39,.72)
    s.note(43.30,69,2.16,.81)
    _vibrato(s,44.0,45.30,.43)
    s.control(45.62,"solo",0)
    s.chord(45.67,[45,52,57,60,64],2.33,.66,SUSTAIN,DOWN)
    return _finish(s,48)


def iron_cathedral():
    s = Score("07-iron-cathedral", "Iron Cathedral", 108,
        "A broad modern-rock anthem built from an ascending high-string "
        "answer and a weighty E-minor chord progression, with a low halftime "
        "breakdown before its final open-string lift.",
        engine=dict(mode=2, pickupSelector=2, pickupType=.48, toneKnob=.87,
            stringAge=.14, pickPosition=.20, pickHardness=.78,
            bodyResonance=.44, muteDamping=.80, velocityAmount=.80,
            outputDb=5.2, artifactAmount=.16, sympatheticAmount=.24,
            strumSpreadSeconds=.005),
        fx=dict(distortion=.35,amp=.91,ampModel=2,compressor=.49,
                delay=.09,room=.19),
        features=["Wide internal Double mode", "Modern high-gain anthem",
                  "Reachable six-string chord voicings", "High-string hook",
                  "Drop-E halftime breakdown", "Open chord sustain"])
    s.section(0,"Open the cathedral: the anthem hook")
    # Common, reachable six-string voicings, including the ringing open tops.
    shapes=([40,47,52,55,59,64], [48,52,55,62,67],
            [43,50,55,59,67], [50,57,62,66])
    answer=([64,67],[64,67],[62,67],[62,66])

    def anthem(at, repeat=False):
        for bar,(shape,upper) in enumerate(zip(shapes,answer)):
            b=at+bar*4
            s.control(b,"palm",0)
            s.chord(b,shape,1.43,.92 if bar==0 else .84,SUSTAIN,DOWN)
            s.chord(b+1.65,upper,.55,.76,SUSTAIN,UP)
            s.chord(b+2.50,shape,.61,.88,SUSTAIN,DOWN)
            if repeat and bar==3:
                s.note(b+3.25,66,.22,.79)
                s.note(b+3.58,67,.29,.86)
            else:
                s.chord(b+3.34,upper,.45,.78,SUSTAIN,UP)

    anthem(0)
    s.section(16,"The anthem answers itself")
    anthem(16,True)
    s.section(32,"Stone floor: halftime low-string breakdown")
    for bar,root in enumerate((28,36,31,35)):
        b=32+bar*4
        s.control(b,"palm",0)
        _power(s,b,root,1.52,.95)
        s.control(b+1.75,"palm",.40)
        s.note(b+2,28,.36,.85,MUTE,DOWN)
        s.note(b+2.75,28,.21,.75,MUTE,UP)
        s.note(b+3.25,28,.32,.91,MUTE,DOWN)
    s.section(48,"The roof opens: final lift")
    for bar,shape in enumerate((shapes[1],shapes[2],shapes[3])):
        b=48+bar*4
        s.control(b,"palm",0)
        s.chord(b,shape,1.72,.88+bar*.03,SUSTAIN,DOWN)
        s.chord(b+2,shape,.70,.79,SUSTAIN,UP)
        s.chord(b+3,shape,.69,.92,SUSTAIN,DOWN)
    s.section(60,"E minor: let it ring")
    s.chord(60,shapes[0],4,.96,SUSTAIN,DOWN)
    return _finish(s,64)


def gravity_lead():
    s=Score("08-gravity-lead","Gravity Lead",84,
        "An original soaring E-minor lead with a clear melodic hook, fluid "
        "same-string legato, a short high-energy flourish and patient bends "
        "that bloom into late vibrato.",
        engine=dict(mode=1,pickupSelector=1,pickupType=.48,toneKnob=.82,
            bodyResonance=.46,stringAge=.14,pickPosition=.27,
            pickHardness=.61,velocityAmount=.88,bendTimeSeconds=.18,
            outputDb=3.8,sympatheticAmount=.26,artifactAmount=.16),
        fx=dict(distortion=.28,amp=.82,ampModel=2,compressor=.38,
                delay=.24,room=.30),
        features=["Singing Modern lead in Stereo", "Original melodic refrain",
                  "Whole-step bends with late vibrato", "Held-source legato",
                  "Brief virtuoso flourish", "Wide dynamic phrasing"])
    s.section(0,"Gravity's theme")
    s.control(0,"solo",128)

    def melody(b, climax=False):
        s.note(b,71,.58,.81)
        s.note(b+.50,74,.52,.74,HAMMER)
        s.note(b+1,76,1.56,.87)
        _vibrato(s,b+1.80,b+2.47,.43)
        s.note(b+2.80,74,.38,.72)
        s.note(b+3.40,71,.42,.79)
        s.note(b+4,69,.51,.74)
        s.note(b+4.75,71,1.87,.87)
        _bend(s,b+5.05,b+5.56,b+6.03,b+6.52)
        s.note(b+6.86,69,.36,.71)
        s.note(b+7.40,67,.44,.77)

    melody(0)
    s.section(8,"Answer and lift")
    s.note(8,71,.68,.81)
    s.note(8.60,74,.47,.73,HAMMER)
    s.note(9.25,76,.48,.82)
    s.note(10,79,1.57,.92)
    _vibrato(s,10.72,11.49,.47)
    s.note(11.72,76,.23,.75)
    s.note(12.15,74,.46,.73)
    s.note(12.85,71,.74,.76)
    s.note(13.50,74,.49,.73,HAMMER)
    s.note(14.3,76,1.53,.85)
    _vibrato(s,15.0,15.74,.40)
    s.section(16,"A lower, more intimate reply")
    for b,p,d,v in ((16,67,.77,.66),(17.15,69,.52,.69),
                    (18,71,1.60,.78),(20,69,.53,.69),
                    (20.8,67,.43,.64),(21.5,66,.62,.69),
                    (22.5,67,1.20,.75)):
        s.note(b,p,d,v)
    _vibrato(s,18.75,19.49,.33)
    _vibrato(s,23.05,23.61,.35)
    s.section(24,"The flourish: one breath of speed")
    # A short picked/legato figure, always <= four frets between connected
    # notes on the explicitly chosen high E string. Overlaps are intentional
    # and only between different pitches, preserving note ownership.
    run=(71,74,76,74,71,74,76,79,76,74,76,79)
    for i,p in enumerate(run):
        t=24+i*.25
        style=SUSTAIN if i%3==0 else HAMMER
        s.note(t,p,.31,.76+(i%3)*.035,style,ALT)
    s.note(27.25,81,.46,.88)
    s.note(28,79,2.61,.94)
    _bend(s,28.27,28.89,29.85,30.49)
    _vibrato(s,29.20,29.80,.37)
    s.note(30.82,76,.41,.76)
    s.note(31.43,74,.44,.77)
    s.section(32,"The theme returns, fully voiced")
    melody(32)
    s.section(40,"Final ascent and a held E")
    s.note(40,71,.60,.80)
    s.note(40.52,74,.55,.74,HAMMER)
    s.note(41.20,76,.49,.84)
    s.note(42,79,1.17,.91)
    _bend(s,42.15,42.47,42.75,43.09,.5)
    s.note(43.44,74,.36,.76)
    s.note(44,76,4,.87)
    _vibrato(s,45.18,47.78,.48)
    return _finish(s,48)


def voltage_tremolo():
    s=Score("10-voltage-tremolo","Voltage Tremolo",180,
        "A driving E-minor tremolo melody at twelve real pick strokes per "
        "second, shaped into a high-register refrain, a low muted contrast "
        "and an open final resolution.",
        engine=dict(mode=2,pickupSelector=2,pickupType=.41,toneKnob=.90,
            stringAge=.12,pickPosition=.20,pickHardness=.78,
            muteDamping=.87,velocityAmount=.80,outputDb=5.0,
            artifactAmount=.16,sympatheticAmount=.19,
            tremoloRateHz=12.0,strumSpreadSeconds=.003),
        fx=dict(distortion=.34,amp=.91,ampModel=2,compressor=.48,
                delay=.08,room=.18),
        features=["Automatic physical tremolo at 12 Hz",
                  "180 BPM sixteenth-note wrist", "Internal Double mode",
                  "High melody and low muted contrast",
                  "Real held-string repicking", "Tremolo-to-sustain finale"])

    def trem_phrase(at,notes,velocity=.80,low=False):
        s.control(at,"palm",.23 if low else 0)
        s.control(at,"solo",1 if low else 128)
        s.control(at+.02,"tremolo",velocity)
        elapsed=0
        for pitch,length in notes:
            s.note(at+elapsed,pitch,length-.045,
                   velocity + (.025 if elapsed%4==0 else -.025),SUSTAIN,ALT)
            elapsed+=length
        s.control(at+elapsed-.025,"tremolo",0)
        return elapsed

    # One sixteen-beat, singable motif; durations expose the hand rhythm.
    motif=((76,3),(74,1),(71,2),(74,2),(79,3),(76,1),(74,2),(71,2))
    s.section(0,"Voltage hook: a melody in sixteenth-note picks")
    trem_phrase(0,motif,.82)
    s.section(16,"The hook answers higher")
    trem_phrase(16,((76,3),(74,1),(71,2),(74,2),(79,2),(81,2),(79,2),(76,2)),.84)
    s.section(32,"Low-string counterweight")
    trem_phrase(32,((28,4),(31,2),(33,2),(28,4),(35,2),(33,2)),.83,True)
    trem_phrase(48,((28,4),(31,2),(33,2),(36,2),(35,2),(31,2),(30,2)),.85,True)
    s.section(64,"The melody returns above the weight")
    trem_phrase(64,motif,.86)
    s.section(80,"Final rise, release the wrist, let E ring")
    trem_phrase(80,((76,2),(79,2),(81,2),(79,2)),.87)
    s.control(88,"solo",0)
    s.control(88,"palm",0)
    _power(s,88,36,1.65,.91)
    _power(s,90,35,1.66,.94)
    s.chord(92,[40,47,52,55,59,64],4,.98,SUSTAIN,DOWN)
    return _finish(s,96)


def build_scores():
    return [drop_e_titan(), velvet_blues(), iron_cathedral(),
            gravity_lead(), voltage_tremolo()]
