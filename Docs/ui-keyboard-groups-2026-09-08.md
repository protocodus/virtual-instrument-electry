# Keyboard groups and Performance spacing — 8 September 2026

The low keyboard register now uses permanent group colors. Tinted key faces,
colored label strips and a matching legend distinguish the controls before
anything is selected:

| Group | Color | Keys |
| --- | --- | --- |
| Pick stroke | Brass | DN, UP, ALT |
| Play style | Ember | SUS, MUT, H/P, HRM, PNC, SLD, X |
| Gestures | Violet | VIB, TRM |
| String solo | Cyan | S8–S1, CLR |

Selected keys use a solid badge with dark lettering and a bright lower edge.
Pick and style selections remain independent, and multiple solo strings can
be active together. CLR shares the solo color and lights while pressed.
Unused keys remain neutral. The pitched keyboard retains its ivory/black
appearance. The legend adds no keyboard focus stops.

![Electry interface](screenshots/electry-standalone.png)

Performance is 520 pixels wide, with five evenly spaced 92-pixel control
columns and 10-pixel gaps. Palm Pressure now fits on one line at its existing
font size. The fretboard panel is 612 pixels wide and retains all 22 frets.
The legend now places each range in gray directly after its colored group
label: C0–D0 for pick stroke, D#0–A0 for play style, A#0 VIB / B0 TRM for
gestures, and C1–G1 / G#1 CLR for solo strings. The playable E2–D7 range is at
the right. This replaces the separate instruction line below the keyboard.
The editor is 1180 × 910, retaining the 24-pixel legend row, the 100-pixel
playing keyboard and the other control rows. Complete instructions remain in
the keyboard's accessibility title and help.

The optional editor snapshot export also writes `-keyboard-groups.png`, using
the public MIDI path to show independent UP/MUTE selections, two active solo
strings and held VIB/TRM keys. It restores those controls before the existing
sounding-performance capture. Local captures and native validation logs are
under `build-keyboard-groups-20260908/`.
The subsequent inline-range captures and validation logs are under
`build-keyboard-ranges-20260908/`.

The widened controls exposed a JUCE text-box initialization issue: a value
field could retain its stock font and accessibility behavior when its width
matched the constructor default. Knobs now refresh their slider text boxes
when joining the editor, so the custom editable label is installed independently
of control width. The existing accessibility checks cover this behavior.

Validation: the native arm64 processor/editor, VST3, AU and CLAP checks all
pass. VST3, AU, CLAP and Standalone build. Default 1×/2× and simultaneous
function-group captures were visually reviewed. The guitar DSP and FX bypass
processor are unchanged by this UI pass.
