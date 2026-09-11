# Common-rhythm realism: palm and strum proposals

## 1. Speaking-length-aware palm contact

Existing bridge-hand loss targeted the same absolute decay time at every fret. Intrinsic wound-string loss already followed speaking length, but the hand did not. A heel fixed near the bridge intersects more of the fundamental's motion as the fretting finger shortens the string, while the mode's effective vibrating mass falls.

The candidate preserves the calibrated open-string palm exactly. For fretted notes its fundamental hand-loss rate scales with `sin²(pi*x/L)/L`, normalized at the open string; the existing upper-band hand rate scales with `1/L`. Contact geometry uses a voiced heel centre at 6.5% of open length (about 45 mm on the baritone). This is a conservative model choice, not an estimate extracted from a recording. The existing passive loss-shape fit and pitch compensation remain intact. No extra output EQ or gain is added.

An initially tested frequency translation of the existing loss dip was rejected: the fitted dip is not a literal localized hand transfer function, and its feasibility solver relaxed some upper harmonics. That change is absent from the final patch.

At 48 kHz, fret 5 palm harmonics 1/2/3 decay between30–130 ms and600–800 ms changes from -3.0/-11.7/-22.9 dB to -4.7/-18.4/-27.9 dB. The accented fret 0/3/5/7 riff changes by -12.4 dB null residual, with whole-riff RMS 0.9 dB lower. This gives tighter fretted chugs while open chugs remain sample-identical.

## 2. Strum motion controls the attack

The engine already solved a slightly accelerating wrist trajectory for note timing. It still excited each crossed string with the same temporal pick response. The candidate uses the ratio of the actual solved travel intervals to scale contact time, release time, and winding-ridge scrape rate. The leading contact has scale 1; later contacts inherit the wrist speed latched with their scheduled stroke. The same pulse-area normalization preserves modal displacement, so MIDI velocity remains the force axis. No new random draw is added.

Zero-spread chords and isolated leading notes remain exact. Reanchored scalar chords update the pending speed before contact; future chords cannot replace speed reserved by an older pending contact. At 48 kHz, dry down/up strum null residuals are -3.4/-10.5 dB with total level changes of +0.07/-0.09 dB. Modern paths differ -1.1/-8.5 dB with -0.06/-0.10 dB total level changes.

## Evidence and limits

The modal/contact geometry is inferred from string eigenfunctions and passive local contact mechanics, consistent with [Schäfer, Frenstátský and Rabenstein 2016](https://dafx.de/paper-archive/2016/dafxpapers/23-DAFx-16_paper_24-PN.pdf) and [Smith's pluck model](https://www.dsprelated.com/freebooks/pasp/Pluck_Modeling.html). The [Biral et al. 2014 pressure-profile study](https://www.icmc14-smc14.net/images/proceedings/PS4-B10-TowardsaDynamicModel.pdf) motivates physical bridge-hand pressure rather than a simple output gate. These sources do not determine the voiced heel location or imply perceptual superiority.

`rhythm-tests` renders seven everyday scenarios at 44.1/48/96 kHz, dry and Modern, and checks exact 37-versus-256-sample block partition invariance. Every result is finite and bounded. `comparison-metrics.json` and `partial-metrics.json` hold actual waveform measurements. A/B float WAVs are in `baseline-audio` and `candidate-audio`. They are for listening review; objective differences do not establish preference or top-of-market quality.

The source implementation is in Source/DSP/ElectryEngine.cpp/.h. `Tests/RealismRhythmTests.cpp` is a proposed standalone test; add a CMake target linked to ElectryDSP. Audio export is opt-in through a positional output directory, so routine tests create no artifacts. For a baseline build only, define ELECTRY_RHYTHM_CANDIDATE=0 to disable candidate-specific structural assertions while keeping waveform stability/block invariance.

Recompute the frozen metrics after exporting baseline/candidate fixtures:

```sh
python3 Experiments/Realism20260911/Rhythm/analyze.py \
  --audio-dir build-realism-20260911/palm-src
```

The analyzer requires NumPy. Fixture exports use raw little-endian float32 at the rate encoded in their filename.
