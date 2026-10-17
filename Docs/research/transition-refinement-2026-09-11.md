# Transition refinement — second pass, 11 September 2026

The user preferred the previous open/muted/open pass but still found it
imperfect. This follow-up investigates two remaining continuity faults. It
uses that previous candidate as its baseline; the earlier listening page
and audio are preserved. It is a small refinement, not evidence of a perfect
or market-leading guitar model.

## The two changes

**Damp the strings nobody is playing.** The sympathetic renderer applied a
coefficient derived for one audio sample once per complete string round
trip. The extra hand loss therefore acted far too slowly. A rendered,
undriven-string test measured an added hand-loss T60 of 32–559 seconds where
the model intended about 120 ms. These numbers describe the *additional
hand loss*, not the duration of audible ringing: the ordinary string loss
was still present in parallel.

Each idle string now receives `exp(-3 ln(10) × handLossRate × period/Fs)`
once per round trip. `period` includes the existing filter-phase correction
and current bend. The positive hand-loss rate moves with the established
4 ms landing / 8 ms lift time constants; the scalar adds no phase or energy.
Continuous Palm Pressure retains its existing independent loss fit, avoiding
a second application of pressure damping.

The existing analytic test had raised the old coefficient to the period
despite the runtime applying it only once. Its oracle now reads the actual
runtime gain, and a separate regression measures rendered decay after all
excitation and feedback are removed. The measured added Palm loss now gives
119.74–120.23 ms across representative strings and four sample rates. A
separate 72-case bend matrix has median error 0.100%, maximum 1.373%.
All 108 pressure-only waveform fingerprints match the baseline exactly.

**Keep the heel relaxed through the reopened pick.** Every repick reset the
hand envelope and its peak, briefly interpreting the old palm dip as fully
engaged even while the heel was lifting. In the isolated probe its depth
increased by 19–35% during the first millisecond after an open repick.

An already-ringing Sustain repick now retains those follower coordinates
while style contact remains or pressure is falling. Fresh notes, stationary
Palm, steady-pressure repicks and other articulations retain their existing
reset behaviour. This adds no attack gain, noise or excitation. The 24 new
regression cases fail on the previous version and pass on the candidate.
The fix removes the artificial reset; it does not force the entire hand
filter to decrease monotonically, since changing pick force still matters.

## Matched musical result

The original nine phrases and their 27 baseline WAVs match the previous
candidate byte for byte. Two additional 7.95-second low-string rhythms keep
one E1 held through repeated open/muted/open picks, with gaps between them.
One uses the default 0.20 sympathetic amount; its control disables coupling.
All eleven score manifests match between versions, with balanced fretting
owners and no hidden resets.

The combined dry rolling style riff changes by +0.023 dB RMS and the Drop-E
riff by +0.020 dB. The pressure-driven riff and all unpicked controls remain
sample-identical in every tap. These are subtle changes in the complete
performance. The level differences alone are not a measure of realism, and
the listening pairs use constant whole-file RMS matching.

The comparison validates 33 pairs, 66 raw float WAVs and 66 PCM listening
files. All are finite; the largest playback peak is below 0.354. The largest
matched-pair RMS error after PCM quantisation is below 0.000018 dB. Local
links and embedded JavaScript syntax pass validation. Browser interaction
was not tested; automated local-file browsing was policy-blocked in the
preceding pass. No new recorded-performer calibration is claimed here.

## Tests and real-time cost

- All 23 core CTest checks pass on the combined final source.
- All four native processor/VST3/CLAP/AU checks pass.
- The focused combined transition suite passes ASan and UBSan.
- The independent 72-case bend/decay matrix also passes ASan and UBSan.
- Existing reference fixtures and tolerances are unchanged.
- Standalone, VST3, AU and CLAP Release products were rebuilt for macOS arm64.

The new idle-string path was timed with one held A2 and up to seven idle
strings at 96 kHz, 256 frames, in three alternating paired rounds. Style
changes at four/sixteen per second use 0.0667/0.0763 seconds of CPU per second
of audio, approximately 1–2% above the previous engine. Pressure-only and
settled paths remain within measurement noise. This is engine-only local
timing, excluding FX and the host; no Windows/universal result is claimed.

The listening page is
`build-transitions-20260911-pass2/comparison/listening.html`. Before is the
previous transition pass; After contains both corrections. Start with the
original rolling style riff, then the low-string rhythm with gaps.

Reproduction, source hashes, isolated results and final validation receipts
are under `Experiments/Transitions20260911Pass2/`. The final local logs are
`core-tests.log`, `native-tests.log` and `sanitize.log` under
`build-transitions-20260911-pass2/`.
