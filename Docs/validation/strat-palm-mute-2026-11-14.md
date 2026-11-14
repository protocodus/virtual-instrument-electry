# Strat palm-mute attack, 14 November 2026

A firm palm mute on the six-string model left too much high-harmonic energy through the first 48 ms of the picked note. The revised plectrum release adds one quarter of the sounding string’s period at full wheel, scaled by the square of the wheel position. At full pressure that gives a low E about 3 ms of additional contact and the open high E about 0.76 ms. This is an empirical excitation term. We used the samples to match the sound, without an independent measurement of contact time. The existing heel-contact geometry, resistance and finger-pluck excitation are retained.

## Setup

The reference was the mapped up- and downstroke palm-mute WAVs in Clean El. Guitar. We used zone-center MIDI velocities (24, 68, 108), aligned onsets, screened for pitch and compared each model stroke to the median of the available recorded takes for the same pitch, velocity and direction. The controls, 44.1 kHz native Rack adapter, stroke sequence and compensated lookahead were identical for the baseline `d8f3d404` and this change. **Init** is the original factory neck pickup at full wheel. **Clean Electric** is the previously fitted middle pickup, neck and middle tones at 25%, pick 131 mm from the saddles, factory medium pick and 35% compression, at wheel 116/127.

Across 22 training pitches we compared pick-release duration and palm-contact width. A wider firm contact reduced upper harmonics but caused a previously measured high-E decay to become too fast. A period-dependent release improved the attack across the three pitch ranges in both test patches. The chosen quarter-period duration also improved the average of three short-window brightness and two envelope measurements on both training sets. We then froze the code and scored 14 reserved roots (250 pitch-verified recordings, 84 pitch/velocity/direction conditions). These pitches came from the same library previously used in development.

## Held-out results

The attack score gives equal weight to four mean absolute differences in dB. They measure harmonics 2–4 and 5–12 relative to the fundamental and the 2–8 kHz energy share, all at 8–48 ms, plus the RMS envelope at 45–55 ms relative to 10–30 ms. Each available pitch has equal weight within each measure. Windows that extend beyond a reference take are excluded from that measurement. Lower is closer.

| Measure | Clean Electric before | Clean Electric after | Init before | Init after |
| --- | ---: | ---: | ---: | ---: |
| Attack score | 12.48 | 10.93 | 18.42 | 14.52 |
| Harmonics 2–4 / fundamental | 5.98 | 6.62 | 6.60 | 6.92 |
| Harmonics 5–12 / fundamental | 17.79 | 14.42 | 28.02 | 22.06 |
| 2–8 kHz share | 13.86 | 10.93 | 27.91 | 17.97 |
| Early RMS change | 12.28 | 11.74 | 11.16 | 11.14 |
| Previous six-measure attack and decay score | 9.86 | 8.82 | 14.22 | 11.74 |

The low and middle registers improved most. In the upper register the attack score for Clean Electric went from 17.24 to 16.86 dB. The very first click is a tradeoff. At 0–8 ms the 2–8 kHz-share error on Clean Electric rose from 7.25 to 10.70 dB. At 24–48 ms it fell from 14.85 to 12.36 dB. On Init both windows improved, from 13.31 to 7.00 dB and from 28.34 to 19.45 dB. The 96 unmuted sustain controls at wheel zero rendered sample-identically across the two patches.
