# The Telecaster against a T-style guitar recorded direct, 23 November 2026

What 2.7.0's Telecaster is built from, and what the one T-style DI recording
available can and cannot say about it. 2.11.0 gave the Telecaster its own
losses, neck cover, sensitivities and coil places
([what differs and what does not](../research/strat-and-tele-differences-2026-11-24.md));
the comparison below was made before that and has not been run again.

## What the Tele is

- **Coils (spec):** Fender's Pure Vintage '52 Telecaster set, product listing
  0992119000: neck 7.2 kOhm and 2.4 H, bridge 6.6 kOhm and 3.8 H.
- **Winding capacitance (estimate):** 100 pF neck, 120 pF bridge, inside
  Lemme's 80 to 200 pF for pickups in general.
- **Eddy loss (as the Strat's):** the Alnico rods' 220 kOhm on both coils.
  The neck cover's and the steel plate's own loss is left out (README, Known
  gaps).
- **Wiring:** modern 3-way, bridge / both in parallel / neck, one 250 kOhm
  audio Volume and one 250 kOhm audio Tone with 0.05 uF (estimate; reported
  for Fender's vintage-style Teles) on node X in every position.
- **Places:** the bridge coil slants as the Strat's, 5.4 mm further from the
  saddles (reported, from the two guitars' routing templates); the neck coil
  at the Strat's neck coil's place (estimate).
- **Bridge:** fixed. The arm does nothing; the wheel bends the last fretted
  string up. The strings, the neck, the gaps and the poles' balance are the
  Strat's.

With every knob at 10 the jack falls 3 dB under its 500 Hz level at
4.5 kHz on the bridge coil and 6.6 kHz on the neck (StratCircuitTests).

## The reference

Guitar-TECHS (Pedroza et al., ICASSP 2025; CC BY 4.0;
<https://zenodo.org/records/14963133>), player 3: a Sire T7, a T-style guitar
with Sire's own Alnico 5 pickups, not Fender's, recorded DI in the middle
position (both coils), 48 kHz, 24-bit: twelve pieces, 7.5 minutes, 974 notes
with per-string MIDI from a Fishman Triple Play. Fetched for measurement,
never committed. There are no isolated single notes, and the player's
stroke, pick position, knob settings and the DI split's load are not stated.

Each piece's MIDI was played through `ElectryRenderStrat --tele --tab
--pickup 2` at 48 kHz, onsets aligned by cross-correlating the onset
envelopes (the MIDI leads the audio by 20 to 40 ms).

## What it says

Long-term spectrum, third octaves, each normalised to its 200 Hz to 1 kHz
mean; model less recording, dB:

| Band (Hz) | 100 | 200 | 400 | 800 | 1270 | 1600 | 2540 | 4030 | 6400 | 10160 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Tele, pick at 109 mm (default) | −10.3 | −4.5 | 0.4 | 3.8 | 6.0 | 10.8 | 4.1 | 3.8 | 4.6 | 0.3 |
| Tele, fingers at 160 mm | −7.8 | −2.4 | 0.5 | 1.4 | 4.6 | 8.3 | 0.3 | 0.7 | 3.9 | −0.1 |
| Strat, neck + middle, pick at 109 mm | −9.8 | −4.4 | 0.6 | 2.9 | 5.5 | 4.4 | −1.7 | 1.8 | 1.8 | 0.0 |

Over the first 230 ms of the 836 notes that ring 250 ms alone, the default
stroke's partials stand 8 to 15 dB further over their fundamental than the
recording's from 500 Hz to 8 kHz. Played with the fingers 160 mm from the
saddles they stand 0 to 10 dB over (median 4.5 dB from 2 to 8 kHz).

Most of the difference follows the stroke: moving the plucking hand moves
these figures by more than the guitar's electronics do, and the recording
does not say how it was played. Its knob settings and the load of its DI
split would move 2 to 8 kHz by several dB as well. So this recording checks
the Tele's broad shape (with a soft stroke, within 1 to 5 dB from 200 Hz to
8 kHz except around 1.6 kHz) and fits none of its values. The 1.6 kHz excess
and the missing lows below 200 Hz on every stroke tried are open.

What would fit the Tele: single notes of a Telecaster recorded direct on
each pickup, at stated velocities and knob settings, as Clean El Guitar and
IDMT-SMT-Guitar gave the Strat.
