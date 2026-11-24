# What differs between a Strat and a Telecaster, and what does not, 24 November 2026

What 2.11.0's two guitars are built from, which of their differences a
measurement carries, and which stay open. The values and their sources sit
beside the code (`Source/DSP/StratCircuit.h`, `StratEngine.h`,
`StratEngine.cpp`); this note has the reasoning that spans them.

Sources: M. Zollner, *Physics of the Electric Guitar* (GITEC,
<https://gitec-forum-eng.de/the-book/>), by section and figure below;
H. Fleischer, *Schwingungsuntersuchungen an elektrischen Gitarren*, Beiträge
zur Vibro- und Psychoakustik 2/01; Fender's listings of the Pure Vintage '57
Stratocaster and '52 Telecaster sets; Terry Downs, *The Telecaster Body*
(drawing rev. D), and Ed Hawley's full-size Telecaster neck drawing.

## The two guitars side by side

| | Strat | Telecaster | Evidence |
| --- | --- | --- | --- |
| Coils | three alike: 5.95 kΩ, 2.47 H, 100 pF | neck 7.2 kΩ, 2.4 H, 150 pF; bridge 6.6 kΩ, 3.8 H, 120 pF | Fender's listings; the Tele neck's capacitance measured (Zollner 5.15), the others reported or estimated |
| Loss across each coil | 1.0 MΩ (the rods) | neck 320 kΩ (rods and cover), bridge 340 kΩ (rods and base plate) | measured (Zollner 5.15, 5.10, 5.1) |
| Between strings and coil | nothing | the neck's cover, a first-order lag at 13 kHz | measured (Zollner Fig. 5.10.15) |
| Sensitivity re a Strat coil | 0 dB | neck −1.8 dB, bridge +2.2 dB | neck measured (Zollner 5.15), bridge estimated |
| Coil places from the saddles | neck 162 mm, middle 98 mm, bridge 46 to 36 mm (10°) | neck 160 mm, bridge 53.5 to 39.3 mm (15°) | drawings and routing templates |
| Switch and tone | 5-way, two Tones of 0.1 µF, none on the bridge coil | 3-way, one master Tone of 0.05 µF | the guitars' wiring; the Tele's capacitor reported |
| Bridge | tremolo, stabilized, with its arm | fixed, no arm | since 2.7.0 |
| Strings, neck, pole balance, decay | the recorded Strat's | the Strat's | open, see below |

At the jack with every knob at 10, a 300 pF cable and a 1 MΩ amplifier
(level at 200 Hz, the resonance's height over it, and where the response
falls 3 dB under it):

| Pickup | 2.10.0 | 2.11.0 |
| --- | --- | --- |
| Strat bridge | −0.5 dB, +2.7 dB at 4.3 kHz, 7.1 kHz | −0.3 dB, +5.7 dB at 4.8 kHz, 7.6 kHz |
| Strat neck | −0.7 dB, +0.7 dB at 3.3 kHz, 6.4 kHz | −0.5 dB, +2.4 dB at 4.2 kHz, 7.1 kHz |
| Tele bridge | −0.7 dB, +0.1 dB at 1.6 kHz, 4.5 kHz | +1.6 dB, +0.5 dB at 2.4 kHz, 4.9 kHz |
| Tele neck | −0.8 dB, +0.8 dB at 3.4 kHz, 6.6 kHz | −2.5 dB, +1.3 dB at 3.6 kHz, 6.2 kHz |

## The coils' losses

A coil's own loss decides how far its resonance stands out once the guitar's
pots and the cable load it. Zollner's data sheets (5.15) give an equivalent
circuit and three impedance peaks for each pickup he measured: unloaded, and
with 450 and 750 pF across it.

- **Strat.** His Strat-72 (5.88 kΩ, 2.19 H, 133 pF) has 1.0 MΩ across its
  inductance. With it the sheet's impedance peaks (57.4, 51.9 and 49.6 dB re
  1 kΩ) and its loaded peaks into 111 kΩ (3.8 and 4.8 dB) come out within
  0.1 dB. The 220 kΩ of 2.0.0 to 2.10.0 came with citations for the effect
  and no measured coil that gives the value; it left the Strat's bare coil
  ringing less than a covered Telecaster neck coil, the reverse of what the
  bench shows.
- **Tele neck.** His Tele-52 neck (7.63 kΩ, about 2.45 H) resonates unloaded
  at 8.3 kHz, which is 150 pF, and its three impedance peaks (49.1, 45.9 and
  44.5 dB) leave 320 kΩ in parallel within 0.2 dB. Another Fender neck
  pickup read by laser with its cover on and off (Fig. 5.10.15) puts the
  cover's own share at 370 kΩ, which beside rods that lose as a Strat's is
  270 kΩ: the two readings agree on a third of a megohm.
- **Tele bridge.** No sheet. Its rods are taken to lose as a Strat's, and
  the steel base plate under the coil takes about 1 dB more off the loaded
  peak (5.1, Fig. 5.1.4): 340 kΩ together.

The Strat's other parts were fitted to recorded Strats with the 220 kΩ in
place (`Docs/decisions.md`, 2026-11-05 and 2026-11-07): decay rates, which
the circuit does not touch, and partial levels up to 6.5 kHz, which it does.
The new loss moves the neck coil by under 1 dB below 3 kHz and every
position by 2 to 3 dB from 4 to 7 kHz, so there the Strat now stands over
the state those fits were made in. The recordings do not state their cable
or input, which move that band by as much, so they cannot say which loss is
right; the bench measurement can. The fits have not been run again.

## The Telecaster's neck cover

The cover is 0.5 mm of nickel silver, thin against its skin depth (2.7 mm at
10 kHz). It does two things. As a shorted turn around the coil it adds the
loss above, which the coil's impedance shows. And the eddy currents the
strings' field drives in it oppose that field on its way to the coil, which
the impedance does not show: a first-order lag.

Zollner's laser reading of one pickup with its cover on and off, into
110 kΩ and 700 pF, has the cover taking 0.5 dB at 2 kHz, 1.4 at 3, 1.7 from
3.5 to 4.5 and 1.2 to 1.6 from 5.5 to 9.5 kHz. The added loss alone gives
most of that up to the resonance and almost none above 6 kHz. With the loss,
a lag at 13 kHz follows the reading within 0.4 dB at each of 18 points from
1.25 to 9.5 kHz, 0.15 dB rms (`testTeleNeckCoverAgainstMeasurement`). The
two curves were read off the figure by tracing its pixels.

## Sensitivities

Zollner marks each pickup's low-frequency sensitivity on its sheet against a
Stratocaster pickup's. The Tele-52 neck stands 1.8 dB under the Strat-72,
the same pair the losses come from. He withholds the absolute values of his
table of 40 pickups (5.4.5), but it ranks the Tele-52 bridge over every
Fender Strat coil in it and the Tele-52 neck under them all.

The bridge coil has no such mark, so its +2.2 dB is an estimate: its turns
over the Strat coil's by the two sets' listed inductances, with the 6 % the
base plate adds taken off (√(3.58 H / 2.47 H), 1.6 dB), and the plate's own
0.6 dB (5.1, Fig. 5.1.4). Fig. 5.10.13's transfer coefficient for a Tele
neck coil is no basis for it: that figure's string distance is not stated.

## Where the coils sit

The neck coil's rout is centred 0.803 in behind the end of the neck pocket
(Downs), and a 25.5 in neck's heel ends 18.385 in from the nut (Hawley):
6.31 in, 160 mm from the saddles, 2 mm nearer them than the Strat's. The
bridge coil's centre is 0.214 in further from the saddles than the Strat's
(the two routing templates), and it slants 15 degrees where the Strat's
slants 10 (Seymour Duncan's figures; Downs's rout reads 14.1): 14.1 mm
across the 53 mm between the E strings, so the low E is read 53.5 mm from
its saddle and the high E 39.3 mm. 2.7.0 had it at the Strat's slant.

## Body shape and materials

The owner asked for these to be accounted for. What the evidence says is
that at the pickups they are nearly nothing, and that the little they may
do is not measured for a Telecaster.

- **The body's wood and shape.** Zollner (7.9) measures and concludes that
  within solid-body guitars "the wood the body is made of plays a highly
  subordinate role for the electric sound": the body is "a practically
  immobile base for the bridge", and its geometry and material shape the
  sound radiated into the air, not what the pickup reads. Fleischer measures
  the same thing from the other side: on six solid bodies the bridge moves
  far less than the neck. A Strat's contours and a Telecaster's slab,
  alder or ash, are therefore not modelled, and no constant here stands in
  for them.
- **The neck.** It is what does move. Up to about 1 kHz the neck's
  resonances set which partials die early; above it, the string and its
  saddles do (Zollner 7.7.4.4). Both guitars have a 25.5 in bolt-on maple
  neck. Fleischer's two Stratocasters of the same type have their neck
  modes 5 to 10 % apart (58 and 64 Hz, 190 to 200, 235 to 258, 432 to
  465 Hz), and he measured no Telecaster. The Telecaster therefore keeps the
  two modes fitted to the recorded Strat (`StratString.cpp`): a Telecaster's
  own would lie within the spread between two Strats, and nothing says
  where.
- **The bridge.** Fleischer's Strat with a tremolo has a bridge conductance
  that rises with frequency to 700 Hz, which he puts down to the vibrato
  mechanism, where his Strat with a fixed bridge has almost none. That
  would let a Telecaster's strings ring longer. Zollner's one Telecaster
  beside one Strat (7.10.2.2, Fig. 7.115) decays slightly faster, on
  different strings, and he warns against reading a rule into it. The two
  point opposite ways and neither gives a number for a Telecaster, so its
  strings decay as the recorded Strat's. Statically the two already agree:
  the Strat's tremolo is stabilized (`StratBridge.h`), so a bend retunes
  only its own string on both.
- **The bridge plate.** The Telecaster's bridge coil hangs in a steel bridge
  plate, a second conductor around it beyond the base plate under it. It
  must add loss; no source here measures how much. It is left out, so the
  bridge coil may ring a little more than a real one.
- **Poles.** The '52 set's poles are flush under a 7.25 in fretboard radius
  where a '57 Strat's are staggered (Zollner 5.4.6). The model's pole
  balance was fitted to the recorded Strat, whose stagger is not known, so
  moving it for the Telecaster would need a number nothing gives.

## What would close the open points

Single notes of a Telecaster recorded direct on each pickup, at stated
velocities and knob settings, as IDMT-SMT-Guitar gave the Strat: they would
fit its strings' decay, its neck's modes and its pole balance, and check the
bridge coil's estimated sensitivity and capacitance. A bench sweep of a
Telecaster bridge pickup in and out of its bridge plate would give the
plate's loss.

## Checks

`Electry.StratCircuit` holds the three losses to the sheets' peaks and the
base plate's 1 dB; `Electry.StratEngine` holds the cover's lag to the laser
reading and to its analog response at 88.2, 96 and 192 kHz, each coil's
sensitivity through the whole engine, and the coils' places to the drawings.
A phrase rendered through 2.10.0 and 2.11.0 at 48 kHz (two strummed chords, a
lick on the plain strings, a riff on the wound ones) moves by:

| Position | Level | 2 to 4 kHz | 4 to 7 kHz | 7 to 12 kHz |
| --- | --- | --- | --- | --- |
| Tele bridge | +2.4 dB | +3.0 | +3.6 | +2.6 |
| Tele both | +0.5 dB | +1.3 | +2.1 | +0.4 |
| Tele neck | −1.6 dB | −1.5 | −1.9 | −3.6 |
| Strat bridge | +0.3 dB | +0.9 | +2.9 | +1.3 |
| Strat bridge and middle | +0.2 dB | +0.7 | +2.7 | +3.1 |
| Strat neck | +0.2 dB | +0.9 | +2.2 | +1.3 |

Rendering it takes the same time in both (0.73 to 0.85 s for 10.6 s of
audio on the development machine, four runs each, alternating).
