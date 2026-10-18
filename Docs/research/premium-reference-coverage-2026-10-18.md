# Premium guitar reference coverage — 18 October 2026

This audit adds verified reference acquisition and an independent pickup research
lead. It makes no new tone measurement or DSP change. The inspected shipping
source is `6c83ff453ff7ac346598e8e73367ced128a90838`. Exact assets, licences,
formats, hashes and local receipt bindings are in the
[portable inventory](premium-reference-coverage-2026-10-18.json).

The requested premium eight-string, cross-genre authenticity target remains
unproven. The [previous tone comparisons](tone-body-2026-10-18.md) establish
particular improvements and remaining differences on conventional six-string
recordings. They cannot establish a percentage of eight-string authenticity.
Two excerpts of one P3 performance do not supply independent player/guitar
holdouts. Passing engineering tests also does not substitute for recorded
performance or perceptual evidence.

## A new exact-eight research lead: PolyMap

The [primary paper](https://arxiv.org/html/2608.27522v1),
[creator's account](https://darkartguitars.com/blog/polymap) and
[official source repository](https://github.com/IIP-Group/polymap) identify a
custom eight-string with eight simultaneous Cycfi Nu sensor positions per
string. The [official archive](https://iis-people.ee.ethz.ch/~datasets/PolyMap/)
is available without an account. Its one raw recording is 97.115333 seconds,
64-channel PCM24 at 48 kHz. The complete WAV member was streamed to validate
its ZIP CRC and SHA-256; no musical signal was analyzed or converted into
product data. Neither the paper nor archive supplies the gauge, tuning,
bridge-to-sensor dimensions, pickup height or note/articulation annotations.

The audio archive expressly uses **CC BY-NC-SA 4.0**. The paper's CC BY 4.0,
plugin/RTL MIT and hardware CERN OHL-P 2.0 licences are separate grants. This
audit excludes the NC audio from product calibration, training, samples and
public listening rooms. The paper's simultaneous-position experiment is an
independent physical research lead: it avoids the different-performance
confound in separately played pickup-selector recordings. It does not close
the conventional premium-pickup or controlled eight-string capture gate.

## New permissive repeated-note and gesture originals

The [Longitudinal Guitar String Ageing dataset](https://zenodo.org/records/19823590)
assigns CC BY 4.0 in its live Zenodo API. The
[university author manuscript](https://www.pure.ed.ac.uk/ws/portalfiles/portal/656673718/A_Longitudinal_Dataset_for_Guitar_String_Ageing.pdf)
describes two six-string players: a Fender Stratocaster with Ernie Ball Extra
Slinky nickel-wound `.008–.038` strings through a Scarlett 2i2, and an Epiphone
Les Paul with Elixir Super Light nickel-plated-steel `.009–.042` strings through
a Roland UA-25EX. Exact guitar model/year and pickup selector are unknown.
Selector and volume/tone remain fixed within each performer across sessions.

Its 28 daily sessions start with new strings. The fixed 60 BPM routine includes
30 low-E open plucks plus open/12th-fret low/high-E conditions, chromatics,
bends, funk strumming, slides, legato, chords and a blues solo. That provides a new
repetition and gesture opportunity, with player, guitar and interface still
confounded. It is not an exact-eight or premium-model benchmark.

Both fresh day-one originals were selected before any candidate or musical
signal analysis in this audit. HTTP range acquisition inventories all 182 ZIP
members and verifies the selected members' sizes, decompression, CRCs and
SHA-256 values. The published whole-archive MD5 is recorded but was **not**
verified: the entire 3.7 GB archive was not downloaded.

| Original member | Verified format | Frames | SHA-256 |
| --- | --- | ---: | --- |
| `guitarist_1/day_1/02-260312_2001.wav` | Mono PCM24, 48 kHz | 33,590,483 | `d63b49a4f62c66eae1e361077aa5d5f9a9c8c994e7a0b65ea5c0762f2e2ea392` |
| `guitarist_2/day_1/12_03_26_1839 day1.wav` | Mono PCM16, 48 kHz | 33,172,368 | `5a6d213430e9cf4e91a2e4d74ffb62fbe28053b2f57a43b7ab27026a1b3542a5` |

The archive contains recording date/time/file CSVs, but no segment start/end
CSV promised by the paper. Reference-only segment/onset annotation and quality
masks must precede comparisons. No new spectral, decay or performance result
is claimed. Untouched dates test session generalization; they do not create
independent player/guitar clusters. All originals and receipts remain in the
ignored `build-premium-reference-audit/` cache, outside product assets and
listening rooms.

## Alternatives rechecked

[GOAT's paper](https://arxiv.org/html/2509.22655v1) describes six-string EADGBe
material, including 79.83 minutes of standard-tuned Strandberg audio. Dynamics
are absent from its tablature/MIDI annotations and remain only in the DI.
The [live record](https://zenodo.org/records/15690894) is restricted, reports
no public files in its API, uses CC BY-NC 4.0 and expressly limits the data to
research rather than commercial products. The
[authors' repository](https://github.com/JackJamesLoth/GOAT-Dataset) directs
users to request access. No access request was made and no audio acquired.
Its paper licence is not an audio grant.

The existing [F#-string pack](https://freesound.org/people/cabled_mess/packs/29585/)
and [Drop-E phrase](https://freesound.org/people/minus_28_and_falling/sounds/557299/)
still offer only qualified exploratory eight-string context without a login
for original downloads. Their CC0 grants do not supply missing make, gauge,
pickup or controlled-articulation metadata. This audit found no new permissive
premium exact-eight corpus that supersedes those limits.

## Independent active-sensor circuit opportunity

[Cycfi's current Nu capsule page](https://www.cycfi.com/projects/nu-capsule/)
and [v2 page](https://www.cycfi.com/projects/nu-v2-capsule/) publish a measured
response graph, actual preamp schematic and dimensions. The v2 coil is specified
at 320–350 ohms, 12 mH, 54 pF and 1,300 turns, with a 10 kohm output and a
manufacturer-reported flat 20 Hz–20 kHz response. Hardware is CC BY-SA 4.0;
software is MIT. This is a concrete independent active sensor family, not an
EMG 808 or Fishman Fluence model. Older v1 specification text uses a different
capacitance unit; a circuit experiment must inspect a consistent version's
actual schematic. No circuit was implemented here.

## Next comparison and completion evidence

Keep shipping A and candidate B on identical production events, seed, controls,
rate and block size. Use one alignment offset and one whole-crop RMS gain with
shared headroom. Preserve all old listening rooms. Do not add per-note gain,
EQ, time warping or candidate-dependent exclusions.

The newly acquired repeated notes can expose body, decay and repick variability
without pretending that the reference's pick force is known. Freeze pitch,
string, articulation, onset and usable windows from the reference alone. Track
H1/H2/H3 and higher partials around their own moving peaks; retain noise-floor
limits, fit quality and censored decay observations. Inspect attack/body/held
spectral shape separately from modal decay. For gestures, measure same-note
and same-string continuity, pitch settling, release/choke tails and transient
windows, then bends, vibrato, slides and chord/string-crossing behaviour. Dry
and identically calibrated common-reamp comparisons answer separate questions.

A credible final assessment needs untouched player/guitar clusters and phrases
covering conventional premium active/passive eight-string instruments, low and
high registers, dynamics, muting depths, stroke directions, and metal/djent,
rock and blues performance. The existing
[capture and blind-listening contracts](evaluation.md#commissioned-palmdead-capture-gate)
are not completed studies; no new response cohort was created by this audit.
A market-leading claim also requires lawful same-score competitor evaluation.

There is no validated scalar that converts the current ratio improvements into
“95% close.” A future registered statistic could report how many held-out
feature cells fall within real-repeat prediction regions, with cluster-aware
uncertainty and critical domains reported separately. A proposed 95% coverage
target would describe that protocol only; it would not establish 95% subjective
similarity. Neither a confidence level nor a convenient weighted average should
be relabelled as an authenticity percentage. The full goal remains active.
