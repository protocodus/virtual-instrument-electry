# Levelling off the twin planes' split (open work, 2026-11-07)

Electry 2.0.1 left one measured difference in how the Strat's partials beat.
This note records it, the candidates considered and what each would cost, so
the next change starts from the numbers.

## What the recording shows

IDMT-SMT-Guitar's Strat, neck pickup, 78 single notes. The beat rate is the
strongest line, 0.4–12 Hz, of each partial's envelope from 0.3 s after the
onset, detrended as a quadratic (`beat.py`'s protocol, as quoted beside
`inPlaneHeard` in `Source/DSP/StratEngine.cpp`). Medians:

| band | recording | 2.0.1 |
| --- | --- | --- |
| 0.5–1 kHz | 1.22 Hz, 0.85 dB | 1.07 Hz, 0.82 dB |
| 1–2 kHz | 1.61 Hz, 1.01 dB | 2.05 Hz, 1.18 dB |
| 2–4 kHz | 1.98 Hz, 0.84 dB | 3.76 Hz, 1.32 dB |
| 4–6.5 kHz | 1.76 Hz, 0.84 dB | 6.49 Hz, 1.14 dB |

On 44 recorded chords the 1.5–10 Hz waver is 1.54 and 1.46 dB at 1–2.5 and
2.5–6 kHz; 2.0.1 gives 1.93 and 1.92, 2.0.0 gave 2.02 and 2.09.

The model splits every partial by the same share of its frequency (the top
plane 0.66 mm shorter, `StratString::inPlaneShortening`), so its rate grows
as 1.3e-3 of the partial's frequency. The recording's split follows that up
to about 1 kHz (the shortening was fitted there, across frets 0–12) and then
levels off near 2 Hz.

## Candidates

The shortening stands for a support that yields toward the poles more than
along the top. A yield that levels off has to stop yielding above some
frequency; each way of doing that has a cost.

1. **A mass on the spring (the saddle).** The yield of k = T / 0.66 mm
   (103–132 kN/m for the EXL110 tensions) is a spring at low frequencies and
   gives way to the saddle's mass above sqrt(k/m) / 2π: about 0.9 kHz for a
   3.5 g bent-steel saddle, which is near where the recording levels off.
   The termination becomes a second-order allpass in the poles plane's bridge
   reflection, and the loop design must subtract its phase delay at the
   fundamental. Cost: lossless, it swings the reflection's phase by 2π over a
   band of about Z0/m (8 Hz wide on the high E, 30 Hz on the low E) around
   the resonance, so a partial that lands there moves by up to half a
   harmonic in the heard plane. Real guitars have such dead spots, but their
   depth needs the saddle's loss, which is not measured.
2. **A damper beside the spring.** Above k/r the yield turns resistive and
   the split stops growing. Levelling off at 1.2 kHz needs r near 14 kg/s,
   which takes 2Z0/r, about 2.4 % of the high E's wave, at every reflection:
   some 70 dB/s more decay above 1.2 kHz in the heard plane than the fitted
   loss allows. Rejected on that number unless the loss fit is redone with it.
3. **Rolling off the heard share above 1 kHz** (tried for 2.0.1). Fits the
   envelopes slightly closer, but takes away the top plane's 10–16 kHz, which
   carries the high E's top octave, and uncovers the poles plane's top
   octave, which decays 1.5 dB apart at 44.1 and 192 kHz. Not shipped.

Candidate 1 is the only one that keeps both the losses and the top octave;
it needs the saddle's mass and loss, from a measurement or a source, before
it can be fitted. If two supported formulations remain, the choice between
them is an A–Z listening test (`CLAUDE.md`).
