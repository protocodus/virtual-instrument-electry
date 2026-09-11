# Open / muted / open benchmark — 11 September 2026

The user identified open-to-muted-to-open playing as the next audible weak
point after the fifth realism pass. This benchmark starts from `aa1f7e0` and
uses an original rolling picked-chord riff, inspired by the general southern
rock feel requested, rather than a transcription of a recorded song.

## Frozen musical score

An eight-bar phrase at 105 BPM uses D, Cadd9 and G voicings with retained common
tones. It repeatedly crosses open, palm-muted and reopened attacks while the
strings stay owned. A common D4 remains fretted through chord changes. New
pitches get a single Note On, unchanged pitches are repicked, and departing
fingers receive one Note Off. Articulation changes have no hidden reset or
note-off gap.

The same phrase is also performed with continuous Palm Pressure. A low Drop-E
companion exercises the instrument's extended range. Separate no-pick palm
landing/lift probes distinguish changes to an existing vibration from a fresh
open attack; lifting a hand must not recreate energy it has already absorbed.
Main musical examples retain the shipping sympathetic amount, while isolated
probes disable coupling to make the transition easier to measure.

Dry, British crunch and Modern taps use the identical generated DI. Event
frames, physical string routing, controls, settings and variation seed are
retained in the manifest. The baseline score and parameters are reused for the
candidate; comparison playback uses constant gains per complete recording.

## Reproduced issues

A Palm Pressure change to zero could miss the voicing cache's 0.002 refresh
quantum, leaving a slight bridge-hand loss engaged. A fresh note could also
capture a pressure below that quantum while the global applied cache still
said zero. The new endpoint test reproduces both cases at 44.1, 48, 96 and
192 kHz. Exact endpoints now invalidate stale damping without changing the
same-sample pressure-and-pick setup.

The previous style transition replaced the damping fit on a ringing string
immediately. A muted repick also forcibly cleared the residual attack-tension
coordinate at physical pick release, even though the old wave was still
travelling along the string. Its compensated pitch target could fall by
5.096 cents in the isolated matrix. The actual delay already had smoothing,
so this was a short artificial pitch bend, not a literal five-cent jump in
one output sample.

## Model changes

1. **Finite heel landing and lift.** An already-ringing string now follows a
   continuous hand position. The chosen 4 ms landing and 8 ms lift time
   constants reach 95% contact in approximately 12 and 24 ms. The existing
   positive-loss damping fit follows that position, retaining the string and
   filter memories. Both style-driven contact and continuous Palm Pressure
   use it. Fresh plucks still begin with their requested calibrated hand
   position; the separate Dead fretting-hand choke retains its established
   immediate behaviour. These time constants are design choices, not a new
   measurement fit to a real recording.
2. **Residual tension survives muted repicks.** Palm and Dead attacks no
   longer erase the old string's bounded tension coordinate. Actual hand
   damping accelerates its decay on the picked string and ringing siblings.
   This uses the additional fundamental loss rate and the longest-lived
   polarisation's existing 1.7× decay scale, as a conservative approximation
   to residual energy loss. Ordinary Sustain plucks retain their existing
   six-cent cap; fresh muted plucks still do not acquire an unsupported new
   tension seed.
3. **Exact open endpoints.** Returning Palm Pressure to zero now clears
   sub-quantum cached loss, including the fresh-note case. The existing
   same-sample pressure-plus-pick setup remains covered by regression tests.

Review also caught two required cache cases in the finite-contact prototype:
changing stroke force under held pressure without changing heel position,
and reversing Open → Palm → Open before a control tick. Both now request
the necessary fit; six added tests cover them at 44.1, 48 and 96 kHz while
checking that the original fretting owner remains held.

The finite contact changes the string's loss, not its output volume. Opening
the hand cannot bring back energy it previously absorbed: an open repick
supplies new energy, while an unpicked lift only changes the decay of what
remains. Listening copies use constant whole-file level matching, with no
individual-note normalisation.

## Real-time cost

An independent eight-string test at 96 kHz exposed a cost problem missed by
the ordinary held-note CPU guard: solving the complete hand filter on every
control tick produced a 1.62× real-time callback p95 even at four transitions
per second. The finite hand motion must therefore be separated from the
expensive coefficient solve. The final model moves the hand at the existing
control rate but fits its passive filter approximately once per millisecond,
staggered across physical strings. Reaching an exact endpoint forces a final
fit immediately; direct pitch/fret refits still run when needed. This keeps
the established passive solver and delay-phase compensation instead of
interpolating arbitrary filter coefficients.

Final engine-only timing on an Apple M1 Max at 96 kHz:

| Scenario | CPU / audio duration, 256 frames | Callback p95, 64 frames |
| --- | ---: | ---: |
| Eight held strings | 0.106 | 0.109 |
| Four style changes per second | 0.146 | 0.285 |
| Four pressure changes per second | 0.132 | 0.241 |
| Sixteen style changes per second | 0.228 | 0.312 |
| Sixteen pressure changes per second | 0.200 | 0.288 |

Each callback figure is relative to its own deadline; 1.0 would consume the
entire available time. Seven alternating paired rounds used 256-frame
buffers, and three used 64-frame buffers. The worst individual observed
callback was 0.783× its deadline at 256 frames and 0.661× at 64 frames.
Ordinary four-per-second style and pressure changes cost approximately
23% and 19% more than the baseline engine, respectively, but the prototype's
deadline overruns are gone in this local test. These are engine-only thread
CPU timings; FX, host overhead and other machines are outside this result.
The complete method, individual runs and source hashes are retained in
`Experiments/Transitions20260911/Performance/`.

## Result and validation

The source, score and reproducible isolated probes are in
`Experiments/Transitions20260911/`. No new recording is incorporated into the
instrument or claimed as a matched performer reference in this pass. The
existing reference ranges and tolerances remain unchanged.

In the isolated E1 held-string lift probe at 48 kHz, the largest adjacent
sample step in the measured transition window falls from 0.00161323 to
0.000447109 (−11.1 dB). This closely retains the full-rate prototype's
0.000434401 result while making the hand solve affordable. It measures a
specific unwanted transition; it is not a perceptual realism score.

The isolated tension experiment reproduces all 48 repick cases and verifies
that ordinary settled open and Palm recordings remain byte-identical to
their baseline. The integrated regression additionally exercises the
dedicated held-string repick commands, for 96 boundary cases, plus shared
hand damping on three sibling-string cases. The experiment replay and its
source hashes are in `Experiments/Transitions20260911/Tension/`.

Repeated unpicked pressure contacts leave a silent engine exactly silent.
The maximum circulating-delay energy relative to an otherwise identical
never-muted string is 1.00033, 1.00018 and 1.00069 at 44.1, 48 and 96 kHz,
respectively (less than +0.003 dB). This bounds a measured energy proxy; it
does not establish a formal passivity proof for the complete time-varying
instrument. See `Experiments/Transitions20260911/Hand/` for the raw probes.

The final matched comparison validates **nine phrases / 27 paired taps**, all
54 raw float WAVs and all 54 PCM listening files. Every ownership trace
balances, and both untouched E1/E2 controls remain sample-identical across
dry, crunch and Modern. The three main dry riff RMS changes are −0.178,
+0.045 and −0.135 dB. Peak amplitude is below 0.354 throughout. Constant
whole-file listening gain matches each pair within 0.00003 dB after PCM
quantisation. These descriptors verify the comparison; preference still
requires listening.

All **23 core CTest checks**, **four native processor/VST3/CLAP/AU checks**,
and the focused transition suite under **ASan/UBSan** pass on the final
optimized source. Standalone, VST3, AU and CLAP Release products were rebuilt
for macOS arm64 under `build-ui-20260908/native/Electry_artefacts/Release/`.
No universal or Windows build is claimed. The listening-page links, audio
formats and embedded JavaScript syntax were checked. Automated browser
preview was blocked by browser security policy, so no visual/browser
interaction review is claimed.

Audition: `build-transitions-20260911/comparison/listening.html`.
Start with the rolling chord style riff in British crunch, then compare the
pressure version in dry mode. The no-pick controls intentionally remain
quiet once the hand has absorbed the string's energy.

Receipts and logs:

- `Experiments/Transitions20260911/source-provenance.json` — tested code,
  renderers, raw audio, native products and final validation log hashes.
- `Experiments/Transitions20260911/validation-results.json` — score,
  ownership, file and signal checks.
- `build-transitions-20260911/core-tests-final.log`
- `build-transitions-20260911/native-tests-final.log`
- `build-transitions-20260911/sanitize-transitions-final.log`
- `build-transitions-20260911/comparison/comparison.json`

To repeat the focused sanitizer pass, from the repository root:

```sh
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer -DELECTRY_DECOUPLED_PICK_RELEASE=1 \
  -DELECTRY_MEASURED_BODY_RESPONSE=1 -DELECTRY_ENERGY_ATTACK_PITCH=1 \
  -ISource Experiments/Transitions20260911/SanitizeTransitions.cpp \
  Source/DSP/ElectryEngine.cpp Source/DSP/ElectryFx.cpp \
  Source/DSP/ElectryVisuals.cpp \
  -o build-transitions-20260911/sanitize-transitions
build-transitions-20260911/sanitize-transitions
```
