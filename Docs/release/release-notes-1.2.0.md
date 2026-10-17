# Electry 1.2.0 release notes

Development of 1.2.0, newest first. The README's version history summarises
it.

- **2026-10-16 — String voicing:** ordinary notes prefer unused strings,
  then the lowest playable fret; chord batches minimize occupied strings,
  then total fret height. Repeated-note ownership, MPE bindings, string locks
  and explicit Hammer/Slide continuation retain their priorities. Release
  tails, sustained notes, pending contacts and silent held keys reserve their
  strings; exhausted allocation also takes the lowest fret before stealing
  history.

- **2026-10-16 — Realism corrections:** picked attacks and
  finger stops follow the moving palm; open-string repicks retain sympathetic
  motion. Idle bass strings use the played string's material-loss curvature
  and intrinsic sustain targets, with consistent neck-pickup sensitivity.
  Unequal humbucker coils observe both wave directions at a common time.
  Infeasible old-string loss fits preserve low-note sustain instead of
  clipping the loop gain. Matched isolated and combined listening renders
  are produced by `Tools/BuildRealismComparisons.py`. The user selected the
  combined B candidate for `main`; [the decision](../decisions.md) records
  that choice without implying named-instrument calibration.

- **2026-09-14 — Release hardening:** malformed host MIDI, parameter values,
  state chunks and output buffers are handled safely; duplicate note releases
  preserve MPE ownership, and controller resets respect event order. Extreme
  audio input is bounded before recursive effects, settled FX controls avoid
  redundant CPU work, and independent demo renders can use multiple cores.
- **2026-09-11 — [Interface finish](../research/ui-polish-2026-09-11.md):**
  quieter graphite surfaces, refined metal knobs, brighter values, aligned
  header controls and a power icon for FX, with more space between FX controls.
- **2026-09-08 — [Keyboard groups and Performance spacing](../research/ui-keyboard-groups-2026-09-08.md):**
  distinct pick, style, gesture and string-solo key colors with a matching legend,
  inline gray key ranges, plus a wider Performance panel that keeps Palm
  Pressure on one line.
- **2026-09-08 — [FX bypass and control spacing](../research/fx-toggle-2026-09-08.md):**
  a global FX ON/OFF button, dry by default, with retained settings, smooth
  switching and more space around the FX header controls and amp selector.
- **2026-09-08 — [Black-metal interface](../research/ui-metal-2026-09-08.md):**
  satin graphite, machined controls, ember accents and a wider layout, with
  clearer labels, drawn icons, ebony fretboard and matching keyboard focus.
- **2026-09-08 — [Audible pick attack and ringing improvements](../research/realism-pass4-2026-09-08.md):**
  stronger hard-pick edges on wound strings, faster bass-partial cooling and
  natural harmonics driven by a physical pluck, with 53 matched dry/metal
  audition phrases and focused waveform tests.
- **2026-09-08 — [Five more realism improvements](../research/realism-pass3-2026-09-08.md):**
  controlled bass-partial loss, accurate slide arrival, release-timed pinch
  contact, finite finger/thumb pads and harmonic Pick Position, with 47
  matched dry/metal audition phrases and reference-decay measurements.
- **2026-09-08 — [Five further realism improvements](../research/realism-pass2-2026-09-08.md):**
  speaking-length decay on wound strings, physical-distance slides, coherent
  winding texture, retained fretting-finger noise ownership and smooth
  articulation makeup, with 39 matched dry/metal audition phrases.
- **2026-09-08 — [Five contact realism improvements](../research/realism-2026-09-08.md):**
  string-dependent pick recoil, winding-aware scrape, vibration-aware release
  noise, distinct finger/hand stops and fret-local hammer-ons, with real
  eight-string reference checks and matched dry/metal auditions.
- **2026-09-08 — [Faster effects and oversampling choice](../research/development-checkpoints.md#2026-09-08-faster-effects-and-oversampling-choice):**
  prepared pedal and phase-inverter math, with Standard/High FX oversampling.
  New rigs use Standard; older saved states retain High.
- **2026-09-06 — [Automated distribution builds](https://github.com/protocodus/virtual-instrument-electry/commit/b35242589674680bf4db5c7f0b7264bcb0fe7fb4):**
  macOS, Windows and Linux packages with CLAP support, plus screenshot and audio
  refreshes on every merge to `main`.
- **2026-09-05 — [String precision and retained hand state](../research/development-checkpoints.md#2026-09-05-string-precision-and-retained-hand-state):**
  more precise fractional string delays, preserved saddle resonance on repicks,
  and retained palm damping through legato and slides.
