# What Electry's automated tests do and do not establish

The release-hardening suites include deterministic hostile float/MIDI/state
sequences and changing block sizes. AddressSanitizer, UndefinedBehaviorSanitizer
and float-to-integer overflow checks can instrument the engine, effects and
processor tests; CI also runs a bounded, coverage-guided libFuzzer DSP session
and preserves its corpus and any crash reproducers. These checks detect tested
memory, arithmetic and lifecycle faults; they do not establish that arbitrary
future input or every host is crash-free.

Ordinary tests deliberately do not assert callback deadlines on unknown
hardware. An opt-in native-processor benchmark now times `processBlock()` after
warmup at 48, 96 and 384 kHz with 64- and 512-frame callbacks. It covers the
default Bridge/Mono path, Both/Stereo, Double, eight-string Palm, Modern with
all five FX amounts at maximum, and continuous American/British amp switching.
Every row uses the same repeatedly struck eight-open-string chord and reports
nearest-rank p50/p95/p99/max callback times plus deadline misses. Preparation,
host parameter writes and output validation are outside the timed interval.
Palm uses Both/Stereo; Modern/all-max and amp switching use Bridge/Mono, with
the latter holding Amp at 100% and changing American/British before every block.
Only silence, non-finite output and a deliberately loose one-second-per-block
runaway guard fail the test; realtime results remain machine- and load-specific.

One Release run on an Apple M1 Max under macOS 26.5.1 on 2026-08-29 recorded
zero misses in all 30 steady rows. The tightest steady p99 was Double at 384
kHz/64 frames: 0.0760 ms of its 0.1667 ms deadline; the largest individual
steady callback was a Palm outlier at 0.1541 ms. The deliberately hostile
per-block American/British switch missed 147/375 and 174/256 callbacks at 48
kHz, 598/750 and 256/256 at 96 kHz, and 2,987/3,000 and 375/375 at 384 kHz for
64 and 512 frames respectively. That row identifies an automation boundary,
not the cost of a selected, settled amplifier.

A 2026-09-14 native Release diagnostic after the hardening pass kept every
scenario's p99 below its callback budget, but still recorded 86 individual
deadline misses across 30,072 callbacks, concentrated at 384 kHz/64 frames.
This runs on an ordinary test-process thread: it does not certify scheduling
in a DAW. Separate alternating FX-only measurements at 48 kHz showed a
22–24% reduction for room-only processing across 64/128/512-frame blocks;
the complete Modern amp chain showed no reliable speedup. Master FX OFF
already skips that chain, so those percentages are not whole-instrument gains.

Current automated tests establish: finite, bounded, non-silent output for
all twenty-one pick-stroke/play-style combinations at 44.1-384 kHz; the 2x/1x
internal-rate policy, exact
host-to-physical clock timing, and filtered-decimation pitch stability;
exact-silence idle output;
sample-identical renders for identical MIDI (including across engine reuse,
which caught a real aperture-state leak during development); fundamental
accuracy within 8 cents across E1..D6 at three rates; stable allpass bounds
and under-20% low/high dispersion-deficit fit error on the heavy short-scale
Drop-E case; exact played/idle-open stiffness-fit parity, all eight idle
allpass histories populated by the rendered loop, live-bend `1/T` scaling,
gauge rebuilds and phase-continuous idle wheel travel; positive bounded modal
conductance and exact structural-loss
bypass at 0%; independent keyswitch banks, LATCH/HOLD ownership and lifecycle,
same-sample controller/keyswitch conditioning, keyswitch silence, dead-zone
and range gating; the alternate sequence surviving style
changes and skipping hammered notes; measurably distinct attack spectra and
levels for down, up, hammered, muted and harmonic playing, an audibly
composed upstroke palm mute, a stroke-independent harmonic octave, and a
bit-identical hammered note under either latched stroke and across the full
Pick Position/Hardness/Noise endpoints, with no Strum Spread delay or
plectrum-contact phase; a node touch that
removes the odd partials by more than 20 dB while leaving the even ones, a
touch law whose ideal closed-form magnitude never exceeds one at any depth, an
exactly absent touch on every other articulation, a finger that lifts, and a
harmonic whose octave partial decays within 2 dB of the same partial of the
ordinarily picked note - the direct evidence that the loop is no longer
retuned; a pinch harmonic whose energy-weighted mean partial index sits well
above an ordinary pick stroke's, whose strongest partial is at least the sixth
near the bridge and a low even partial with the hand over the neck, which gains
more than 6 dB on the fundamental, and which renders as neither a pick stroke
nor a natural harmonic; palm-mute decay
contraction; the same ±2 semitone wheel ratio on played voices and a ringing
coupled string, with travel following Bend Time and settling exactly on the
target; hammer-on
same-string continuation, pitch settling, and click-free transition;
maximum-velocity attack tuning within 8 cents on every open string, less than
6 cents of cross-string spread, and the same rails on low-string Mute and Dead;
bridge-brighter-than-neck centroid ordering; tone-control
high-band reduction; independently audible internal wood, size, shape,
construction, scale and gauge endpoints plus the exposed body-level, position,
hardness and age endpoints; monotonic
multi-dimensional velocity response plus an exactly flat 0% setting;
deterministic, monotonic, exactly silent-at-zero Artifacts behavior and a
bounded maximum-artifact eight-string strum; bridge-coupled sympathetic ring
of an open string that the played note does not itself produce, exact bypass
and never-configured coupled loops at 0%, a live full-to-zero bypass that
clears the inactive delay line and every loop-filter memory, coupling
determinism, a coupled
string handed back to the player when it is picked, and bounded, ring-out-to-
exact-silence behaviour at maximum coupling across three host rates; an idle
open string whose neck/bridge tap, aperture and coil-pair geometry exactly
matches the same played open string, whose spatial delays follow live bend,
whose single-coil endpoint removes both delayed coil reads, and whose culled
pickup histories stay exactly clear, with its damping and allpass memories
choked by the same factor as its retained line when the player takes ownership;
a decay envelope that agrees across 44.1, 48, 88.2, 96, 96.001 and 192 kHz to
within 1 dB through 1.5 s and 2 dB through 3 s; a polarisation seam whose
coefficient follows only sounding pitch and remains continuous across 44.1,
48, 88.2, 96, 96.001, 192 and 384 kHz with neither clamp engaged; a 22nd-fret
high E that still sustains on both sides of that host-rate seam; a coupled ring
whose
kilohertz band sits 60 dB or more under its low band, and coupled loops whose
realised round-trip decay - read back from the gain and coefficient they
actually run - holds its fundamental target at String Age 1.0 and at half and
full bridge-hand pressure, to the same value on 44.1, 48 and 96 kHz hosts;
monotonic palm-mute decay contraction, an exact no-op at zero pressure, an
in-tune heavily muted string, the solved loop coefficient actually moving, and
CC 2 pressure including hostile input; live-damping phase continuity on both
loop polarisations at 44.1, 48, 96, 192 and 384 kHz for shared Palm/Open
contacts, first-sample full CC2 jumps on E1 and B2, every adjacent step of a
full up/down CC2 sweep at those five rates, and simultaneous CC2 plus the
minimum-time wheel glide at 44.1 kHz;
strum travel offsets in physical string order, an undelayed leading string, a
sample-identical single note at zero or
nonzero spread, reversed upstroke travel, exact render-block partitioning,
later-timestamp stroke separation, a lower stacked chord peak and cancellation
of an un-crossed string without a phantom attack;
the CC1 resonance lifting the sympathetic ring with Resonance Depth scaling
its reach and a bit-exact bypass at a lowered wheel; the closed
engine-amplifier loop self-sustaining after note release at full wheel and
distortion while the same loop decays with the wheel down or the amplifier
dry, all bounded; an exact stereo plug-in render across 64, 256 and 1024-sample
host blocks with non-aligned CC1, Note On and Note Off events, plus an acoustic
impulse returning at the fixed sample-rate-derived delay on 44.1, 48, 96 and
384 kHz hosts; a held A4 loop exceeding the idle high E by at least 6.02 dB
with the voiced quarter return; pluck position following the fretted sounding
length by 2^(fret/12), while its fixed-metre centre and contact width remain
invariant through fretting and loop-filter phase and follow inverse wave speed
under a pre-bend; both modal-release poles retaining their harmonic-number
cutoff under that same pre-bend rather than following written MIDI pitch;
fretboard geometry, meter ballistics, standing-wave shape, colour knee and a
lossless packed audio-to-editor round trip; per-string display readout naming
the right string, fret, note and articulation; selector-driven pickup culling,
click-free restoration of a culled pickup, Mono channel linking and a
click-free stereo-field opening; exact digital silence from an untouched
engine, a subnormal-free ring-out that reaches exact zero, and a clean wake
from the frozen state; all six Guitar Build anchors audibly distinct and in
tune; plectrum contact noise in the pre-attack
window; release noise that appears only after note-off; a Dead note that lands
  within 0.47 dB of Sustain, retains the calibrated dark periodic E1 body and
decays through its own loop rather than being gated; eight-string
polyphony with open-position chord mapping, repick reuse, and stealing; a
slide whose pitch travels through the intermediate semitones rather than
jumping, whose travel time scales with the interval, whose chained Slide and
Hammer retargets preserve the programmed and effective loop pitch, direction
and remaining distance, whose stiffness follows the live fractional fret
monotonically in both directions and lands on the independently settled
endpoint, whose in-flight dispersion refits cannot reverse the physical pitch
trajectory, whose friction band follows the speed of the hand,
which is far louder on a wound string than on a plain one and exactly absent at
a silent Finger Noise control, which injects no second modal attack and cannot
cancel or re-anchor a pending picked strum; a fretting hand that keeps a lead
phrase in one position instead of falling back to open strings, leaves the
open-position shapes untouched, retains a planted Palm hand through hammers,
pull-offs and legato slides without changing sibling damping, and relaxes to the
nut when the phrase ends;
legacy pitch-wheel travel plus fixed-state MPE member/master pitch composition,
same-pitch channel ownership in both directions across a zone-layout change,
an exact RPN semitone-plus-cent member range and a separately live master
interval, bounded-overlap rejection, idle-wheel
initialization, member-frozen/master-live release tails including a delayed
repick, new-finger thaw exactly at delayed contact, null RPN selection on CC
121 and re-prepare, and ownership clearing at Panic, prepare, All Sound Off and
All Notes Off;
sustain-pedal hold; the balanced A#0 fretting-vibrato
gesture with five-rate coverage for pre-held-silence/open-string onset,
immediate final-key release, sustain-tail exclusion, held-sibling continuity,
off-grid refret, stopped-open-stopped legato, overlapping delayed-note ownership
and finger continuity through live held-note repicks; B0 tremolo
picking with exact free-running cadence, shared chord direction, Strum
deferral, legato retargeting of in-flight contacts, physical-hold filtering and
balanced lifecycle; Hammer-latched E6..B6 and B0 contacts remaining real picks
with Alternate advance, Pick Noise, shared stroke state and Strum travel while
the Hammer latch and live fretting finger remain unchanged;
channel pressure and polyphonic aftertouch remaining pitch-neutral;
hostile parameter and performance
input safety; low Distortion and Modern Amp drive retaining the same physical
input, voice and cabinet response as full drive instead of leaking dry DI
around an enabled module; and a portable CPU ceiling with the eight-active and
one-active/seven-idle settled and wheel-glide render ratios printed on every
run. The active worst case uses Stereo, maximum Body Resonance, and maximum
Artifacts mode. Mono is checked sample-for-sample dual mono; Stereo tests pin
physical low/high string orientation, coherent fold-down, bounded side level,
energy balance, determinism, and opposite string endpoints. The plug-in suite
additionally pins the 30-parameter layout, including Tremolo Rate's, Amp
Voice's, FX Oversampling's and FX enabled's appended indices, Guitar Build's named anchors,
formatted values, current-state round trips, bus layout, sample-accurate
note starts, MIDI controller behavior (bit-identical channel-agnostic pitch
before zone activation; same-sample MPE setup; selective member and additive
master bends using declared ranges, measured from shared search intervals that
reject negligible captures; an exact semitone-plus-cent member range; a
member-frozen, master-live CC 64 tail; retained layout and range but nulled RPN
selection through CC 121 and re-prepare; unassigned channel pressure and
polyphonic aftertouch; cached pre-note bends; release-tail isolation; bounded
ownership overflow; sustain,
all-sound-off, all-notes-off, Panic and prepare lifecycle clearing), UI
keyswitch triggering of both banks, panic, output-gain and APVTS
output-mode effects, three visible non-overlapping mode buttons, three visible
non-overlapping Amp Voice buttons, Modern default and missing-field migration,
two visible Standard/High oversampling buttons and legacy High migration,
Space activation for every keyboard-focusable PANIC and exclusive-choice
button with disabled controls rejecting it, plus wrapping arrow-key selection
across every exclusive-choice strip,
all 25 knob accessibility handlers exposing their canonical full parameter
names while retaining the compact visible captions,
deterministic
distinct Double channels, an unchanged primary lane and clean Double re-entry,
the sympathetic, Mute Pressure (parameter and CC 2) and Strum Spread controls
reaching the rendered audio, offscreen editor rendering including the live
fretboard's bounds, and prepare/release cycles at three rates.
Black-box artifact tests additionally load the exact built VST3 bundle, and on
macOS the exact AU binary, rather than linking the processor class or searching
installed plug-in directories. They pin bundle identity, zero-input stereo
instrument buses, MIDI rendering, the 28 named Electry controls, all four
factory rigs and serialized-state restoration. The ten raw evaluation WAVs are
also read back from disk and must retain a silent lead-in, the intended E1/E2
pitch class, distinct articulations and monotonically shorter Palm tails from
light through hard; mutation fixtures prove that silence, duplicate audio,
wrong pitch and reversed mute depth are rejected.
The engine suite separately pins bounded/repeatable second-player pick timing,
one offset per chord, strum composition and pre-contact cancellation.

The amplifier chain has its own suite: the shipping rapid-Mute body's current
30-80 ms medians are 7.5623% upper-band share and 0.897633 harmonicity, above
the 6% body floor while remaining nonperiodic; halfband unity DC gain, the -6 dB
halfband symmetry point, a dense passband sweep below 0.01 dB deviation and a
dense stopband sweep above 70 dB rejection; the diode
node's independent DC points, symmetry and capacitor memory; the measured
12AX7 model's cathode/plate KCL residuals, rail-to-cutoff recovery, plate-load
convergence, quiescent normalisation and asymmetry; a monotonic 16,385-point
runtime circuit table whose maximum checked error is 0.00000072; uTracer
TubeLib 6L6GC/EL34 plate- and screen-current anchors at idle and through the
knee; TubeLib power-grid-diode anchors at +0.5/+1/+2/+5 V, monotonic lookup and
stopper/grid-return KCL; coupled two-capacitor LTP idle, sub-`1e-10 A` musical
residuals, independent common mode, overload conduction and source-ordered
100/22 nF recovery; individual-American and common-British screen-resistor KCL,
screen voltages falling under drive, ideal push-pull load-line symmetry, unity
local gain,
rising current demand, common-bias sensitivity and distinct half-drive
curvature; three-dimensional off-grid lookup errors below 0.0003 for output and
0.0004 for supply demand, with exact zero demand at interpolated idle; a bit-exact dry bypass with every control at zero
and an audible effect from each amount control on its own at 100%; exact
American and British third-order tone-stack coefficients and 1 kHz insertion
loss; the six-point response fingerprint and pairwise spectral separation of
all three amp/speaker paths; model-ordered nonlinear compression and a
same-performance Drop-E level bound; 48 alias probes covering the pedal, every
amplifier, stacked gain and a quiet input at both 44.1 and 48 kHz and on both
sides of the 96/192/384 kHz oversampling transitions in High mode, each below
-70 dB; Standard has separate lower-rate quality and switching coverage;
direct A/B circuit-state checks that
plate-plus-screen demand rises under a loud hold and recovers during silence,
plus the established full-path supply droop/recovery probe; an output transformer whose distortion falls about
46 dB per decade of frequency and 41 dB for 24 dB of level, measured at the
stage rather than through the cabinet; each Modern cabinet voicing feature
relative to 1 kHz; loudness bounds across every model's whole amp travel and every
combination of the gain and compressor controls on a rendered Drop-E rhythm
figure, dry and palm muted; 48/384 kHz response agreement for every Amp Voice;
the exact nonlinear frame-rate map, invariant 40 Hz coupled-phase-inverter
response at 384 and 705.6 kHz and the same invariant through public 48/88.2 kHz
processing;
stable and mid-switch block-size invariance, bounded switching steps, exact
one-hot settling and inactive-state clearing; the lead delay's first repeat at 360 ms with a
clean gap before it; a decaying, decorrelated room tail; bounded sample steps
across whole-block and individual-module engagement, resting-state resets for
each independently bypassed circuit, and a return to bit-exact bypass;
render determinism; finiteness, output-clamp headroom and
the expected group delay at eight host rates from 22.05 to 384 kHz; and
recovery from NaN, infinite, and out-of-range input as well as null and
zero-length calls, including a single non-finite sample in the middle of an
otherwise clean block. The cabinet's low-frequency probe sits below the modelled
box corner, which is deliberately low enough that a Drop-E eighth string's
fundamental reaches the cabinet rather than being cut before it. A further test renders a short take through the demo
renderer, so the technique takes' toolchain is covered too.

Twenty-three rendered examples of the whole path are produced locally from
this same JUCE-free code by `Tools/RenderDemos.cpp`, so what the document
describes can be listened to rather than only read. They are demonstrations, not evidence: an audible
example is not a measurement, and none of the claims above rest on them.
