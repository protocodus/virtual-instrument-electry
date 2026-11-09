# Strat pickup height follows the fret (2.4.3)

## What changed

The Strat's pole-to-string gap is Fender's vintage setting, 8/64 in on the
bass side and 6/64 in on the treble side, set with the string held at the last
fret. Until 2.4.2 every string read at that gap whatever its fret. Below the
last fret the string runs higher: it lies straight from the saddle to the crown
of the fret that stops it, or to the nut, at the action the model already uses
for fretting stretch (`StratEngine::actionMetres`: 0.5 mm over the 1st fret,
5/64 in over the 17th, so the saddle stands 2.96 mm and the nut 0.35 mm over
the crowns' plane). `StratEngine::updateCoilGeometry` now gives each coil its
own gap at the string's fret, beside the bend geometry it already kept there.

Over the low E (the treble strings within 0.4 dB of it):

| | neck coil | middle coil | bridge coil |
| --- | --- | --- | --- |
| open: height over the set gap | +1.84 mm | +1.11 mm | +0.52 mm |
| open: flux-law sensitivity | −5.1 dB | −3.2 dB | −1.6 dB |
| 12th fret | −2.9 dB | −1.8 dB | −0.9 dB |
| mean over frets 0–12 | −4.0 dB | −2.5 dB | −1.2 dB |
| 21st fret | 0 | 0 | 0 |

## Measurement

Both Strat sessions of IDMT-SMT-Guitar (fetched for measurement, never
committed), each note against the model's render of the same string and fret
at the ordinary stroke (velocity 77, 1.88 N), through `RenderStrat --tab` at
48 kHz. The level is each note's energy below 1.2 kHz (8th-order Butterworth)
over 10 to 260 ms after an onset at a tenth of its peak, the protocol of the
pick-scatter fit in `StratPerformer.cpp`. Per string, the recording-minus-model
difference is taken about its own mean, and one line is fitted through all
strings' differences against fret; the interval is a 2000-resample bootstrap.

| Session | Notes | 2.4.2 rise left over | 2.4.3 rise left over |
| --- | --- | --- | --- |
| Dataset 1, neck pickup, frets 0–12 | 78 | +2.63 dB over 12 frets (90 %: +1.75 to +3.46) | +0.57 (−0.32 to +1.41) |
| Dataset 2 fret runs, read through the middle coil, frets 0–20 | 126 | +4.29 dB over 20 frets (+3.44 to +5.06) | +1.57 (+0.76 to +2.32) |
| Dataset 2, read through the neck coil | 126 | +6.14 (+4.92 to +7.26) | +1.63 (+0.51 to +2.66) |

The scatter about the lines is unchanged (1.01 dB on dataset 1). Dataset 2's
pickup setting is not annotated; its open notes match the middle coil's comb
best (`StratEngine.cpp`, pole balance), and both readings are shown. Its
remaining +1.6 dB is not explained here.

### A second Strat through each pickup

EGFxSet's clean set (an American Stratocaster recorded direct, CC BY 4.0;
fetched, never committed) plays every string at frets 0–22 through each of
the five switch positions, each position a separate take. Frets 0–21 of the
neck, middle and bridge positions, 132 notes each, rendered through the same
positions and measured as above:

| Rise left over frets 0–21 | 2.4.2 | 2.4.3 |
| --- | --- | --- |
| neck | +0.85 (−0.31 to +1.99) | −4.08 (−5.13 to −3.01) |
| middle | −0.29 (−1.50 to +0.82) | −3.26 (−4.42 to −2.20) |
| bridge | −2.36 (−3.57 to −1.24) | −3.52 (−4.69 to −2.42) |
| neck against bridge | +3.21 (+2.49 to +3.96) | −0.55 (−1.26 to +0.19) |
| middle against bridge | +2.07 (+1.35 to +2.77) | +0.27 (−0.47 to +0.99) |
| neck against middle | +1.14 (+0.23 to +2.17) | −0.82 (−1.71 to +0.19) |

Read one coil at a time, this player's notes rise less up the neck than the
model's on every pickup, by about 3.5 dB over 21 frets with the height and
2.4 dB at the bridge coil even without it, where the height matters least:
something common to all three pickups, the hand or the strings, that the IDMT
sessions do not show. Read as one coil against another, which leaves out
whatever all three share, 2.4.2 misses all three differences and 2.4.3 meets
them within their intervals. The height is a difference between the coils
before it is anything else, so this is the reading that tests it.

## Refits that keep the earlier fits

- **Flux-law share (`acrossShare`, 0.5 to 0.58).** A string resting higher
  bends the flux law less, so the same swing marks the sound less. The share
  was fitted with every string at the set gap to the recording's marks: the
  open strings' 4th partial under the 2nd and the 12th-fret notes' 2nd under
  the fundamental and 3rd, at the start and at 200 ms. Read by one partial
  picker (40 ms Hann windows at 0 and 180 ms, peaks within 4 % of each
  partial) on model and recording alike:

  | model minus recording | open, 0 ms | 12th, 0 ms | open, 180 ms | 12th, 180 ms | mean |
  | --- | --- | --- | --- | --- | --- |
  | 2.4.2 (share 0.5) | −0.7 | +1.4 | +1.7 | +4.8 | +1.82 |
  | height alone (0.5) | −2.7 | +0.5 | +0.2 | +3.8 | +0.47 |
  | 2.4.3 (0.58) | −1.1 | +1.8 | +1.5 | +5.0 | +1.80 |

  This picker does not reproduce the original fit's reading (it puts 2.4.2's
  marks 1.8 dB over the recording on average, where the original protocol
  read +0.1), so 2.4.3 holds 2.4.2's marks rather than refitting to the
  recording through a different measure: each reading within 0.4 dB of
  2.4.2's, the mean within 0.05.
- **Pole balance.** Over the sessions the balance was fitted on (dataset 1 on
  the neck coil, frets 0–12; dataset 2 on the middle coil, frets 0–20), the
  height reads the treble strings up to 0.25 dB further under the low E. The
  balance rises by that much (0, +0.06, +0.12, +0.17, +0.21, +0.25 dB, rounded
  to 0.05). The string means against the low E then sit within 0.07 dB of
  2.4.2's.

The level convention, 1 V per m/s under the low E at the set gap, is unchanged,
so the jack is quieter wherever the string runs high: −3.3 dB whole-file RMS on
a neck-pickup line played low and high, −2.2 dB on the neck-blues demo phrase,
−0.65 dB on the bridge-twang phrase. The Output control trims it.

## Guards

- Attack: over each dataset-1 note's first 50 and 260 ms, third octaves 100 Hz
  to 10 kHz level-matched below 1.2 kHz, the median absolute difference from
  the recording moves from 2.51 to 2.45 dB and from 3.76 to 3.67 dB.
- `Electry.StratEngine` adds `testStringHeightFollowsFret`: a light pluck on
  the low E, open and at the 12th fret, read by the neck and the bridge
  coils; their fundamental's ratio less the mode shape's moves +1.40 dB
  between the frets, against +1.42 dB the restated flux law gives at the
  string's heights (2.4.2: 0).
- The `unit` tier (48 tests) and the `audio` tier pass; `ElectryDSPCxx17`
  builds without warnings.

## Listening

Before/after pairs (A 2.4.2, B 2.4.3, whole-file RMS matched by trimming B up
by the figures above) were rendered for review and not committed: a line on
the A and D strings played at frets 0–5 and again at 12–17, and the same E
major chord open and at the 12th fret, on the neck pickup; the neck-blues and
bridge-twang demo phrases. They are review evidence for a measured change,
not a listening test.

## Limits

EGFxSet's takes differ from position to position, so its coil differences
hold only over many notes; the absolute balance between the coils (how much
louder its neck pickup is at the same fret) depends on how that guitar's
pickups were set, which is not documented, and is not compared here. The fret is the string's target fret during a glide, as for the
bend geometry. Between the nut and the 1st fret the end height steps from
the nut's 0.35 mm to the crown's 0, 0.09 mm over the neck coil.
