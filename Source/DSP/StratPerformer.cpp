#include "StratPerformer.h"

#include <cmath>

namespace electry
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr int openNote[StratEngine::stringCount] = {40, 45, 50, 55, 59, 64};
constexpr double scale = StratString::scaleMetres;

// The hand's force on the pick. Velocity reads linearly as force, so the
// string's deflection and the pickup's output follow it linearly, from the
// lightest stroke that still slips off the pick at 1/20 of full. The
// ordinary stroke, velocity 77, is measured by its pitch glide: a string
// swinging harder is stretched further and starts sharper, by what its
// steel, length and tension say (StratString::setTuning), so a recording's
// glide gives its stroke's force. A Strat recorded direct (IDMT-SMT-Guitar
// dataset 1, neck pickup, 74 of its 78 notes) starts 2.1 cents sharp over
// 15 to 60 ms against 0.4 to 0.8 s (median), and this model's ordinary
// stroke of 1.88 N starts 2.1; at least absolute error the recording glides
// 0.95 of the model's (0.65 to 1.05 over 90 % of resamples), a stroke of
// 1.83 N (1.5 to 1.9). It is harder than the 1.4 N a push from nothing
// gave: the share the pick holds at rest (below) leaves the string less to
// swing with. The same Strat's dataset 2, 118 notes of fret runs recorded
// apart, gives a stroke 0.86 times as hard, and the glide grows along the
// neck there as it does here (the G and B 1 to 2 cents low on the neck, 5
// to 9 at frets 15 to 20). The fit took the plate floating; held by the
// stabilizer (StratBridge) the same stroke glides 1.26 times as far (median
// over the 78 notes, the fundamental's peak over 15 to 60 ms against 0.4 to
// 0.8 s: 1.95 cents against 1.55), the hand being the same and the guitar
// stiffer. Full force against the ordinary is voiced until phase 2 fits the
// reference's velocity layers: 3.0 N holds the low E 4 mm aside at a fifth
// of its length.
constexpr double fullForce = 3.0;
constexpr double lightestShare = 0.05;

// The string leaves the pick in two stages: it slides across the tip's grip
// depth d at its kink velocity F / Z, taking Z d / F, and the tip then springs
// clear in a quarter of its own period, (pi/2) sqrt(m / k); StratString lets
// the force go as the spring's does. The tip is a 0.73 mm celluloid medium
// of about 6 kN/m moving 20 mg, 90 us. The grip is fitted with the stroke
// below to a Strat recorded direct (IDMT-SMT-Guitar dataset 1, neck pickup,
// 78 picked notes): over each note's first 260 ms, in third octaves 100 Hz
// to 10 kHz level-matched below 1.2 kHz, d = 0.6 mm leaves the model 2.7 dB
// off the recording (median over notes and bands), 0.45 and 0.8 mm 2.9 and
// 3.2, and the 0.084 mm a push from nothing wanted 3.9. Against that push
// and grip every string's 1 to 10 kHz comes nearer (2.2 to 4.1 dB off by
// string, mean over bands, from 3.3 to 6.2); the exception is the wound
// strings' 2.5 to 5 kHz in their first 25 ms, now up to 8 dB short where
// they stood within 5 either way. A tip's stiffness grows with the cube of
// its thickness and its mass in proportion, so a thicker pick springs clear
// sooner.
constexpr double gripDepth = 0.0006;
constexpr double mediumStiffness = 6000.0;
constexpr double mediumTipKilograms = 20e-6;
constexpr double mediumThickness = 0.73e-3;
constexpr double thinThickness = 0.46e-3;
constexpr double heavyThickness = 1.0e-3;

// Before it slips, the pick drives the string at the hand's speed through
// the grip (StratString::contactStiffness), from a share of the slip force
// it already holds with the string at rest under it: the stroke takes the
// string up slowly at first, more slowly than a wave crosses it, and only
// its last part is fast. Two Strats recorded direct show it at the neck
// pickup (IDMT-SMT-Guitar dataset 1, 78 notes, and EGFxSet's clean American
// Stratocaster, CC BY 4.0, 78 notes at frets 0 to 12): the first swing after
// the release is 0.86 and 0.90 of the largest in the 27 ms that follow, the
// push before it 0.58 and 0.44 (medians), and the settled swing's peaks
// either side of rest differ by +1.1 and -0.5 dB. A push from nothing gave
// 0.53, 0.54 and +3.5 dB: its own wave stays in every period and sets it
// lopsided. Holding half, at twice each string's release speed F / 2Z,
// gives 0.71, 0.47 and +1.2 dB, and on EGFxSet's bridge pickup 0.89, 0.46
// and +0.6 against 0.84, 0.30 and +0.6. Holding 0.4 and 0.6 gives +1.5 and
// +0.9 dB with pushes of 0.54 and 0.38; 1.5 and 2.5 times the speed give
// first swings of 0.63 and 0.75, and the attack's third octaves (above)
// 0.2 to 0.3 dB further off.
constexpr double heldShare = 0.5;
constexpr double pushRatio = 2.0;

// No two strokes are alike: the hand lands a little nearer the bridge or
// further from it, and pushes a little harder or softer. On the Strat
// recorded direct (IDMT-SMT-Guitar dataset 1, neck pickup, 78 notes), the
// pick position each note's partials give (their ratio to this model's over
// 20 to 120 ms, against the comb sin(n pi p / L) and a smooth tilt; frets 0
// to 8, where it is unambiguous) spreads 5.9 mm about each string's median
// (robust SD pooled over the strings, 1.5 to 8.9 mm by string), and each
// note's level below 1.2 kHz over 10 to 260 ms, against this model's at one
// stroke from that place, 0.86 dB about a line over the frets (0.5 to 1.4 dB
// by string): a force 10 % either way. Both include whatever else differs
// from note to note, so they bound the hand's own scatter from above; they
// are taken whole. The player picked the low E and A at medians of 91 and 93
// mm from the bridge and the others at 116 to 122.
constexpr double pickScatterMetres = 0.006;

// The picking hand crosses the strings on a slant, the forearm reaching over
// the body from its upper bout: on the same recording the pick struck the
// low E and A 25 mm nearer the bridge than the plain strings. Each string's
// place, fitted by rendering every note at picks 80 to 135 mm from the
// bridge and keeping the one whose first 12 partials over 5 to 105 ms come
// nearest the recording's: 89, 93, 112, 116, 121 and 121 mm, about their
// mean of 109 mm (StratControls::pickMetres). At one place for every
// string, 130 mm, the partials stood 6.6 dB from the recording's (mean
// absolute, all 78 notes); at these, 4.5.
constexpr double pickLine[6] = {-0.020, -0.016, 0.003, 0.007, 0.012, 0.012};
constexpr double forceScatter = 0.10;

// Nor does the hand cross the strings at an even pace. IDMT-SMT-Guitar's
// chord strums (dataset 1, 44 downstrokes on each of the Stratocaster and
// the Ibanez, every string's onset annotated to the millisecond) put each
// gap from one string to the next off its stroke's own mean pace with a log
// standard deviation of 0.32 and 0.26 (193 and 198 gaps), at a median pace
// of 19 and 23 ms. Each gap after the first stroke from rest takes the
// middle of the two around strumSeconds. Annotation error, if any, is in it.
constexpr double strumGapScatter = 0.29;

// The palm, fitted to the palm mutes of IDMT-SMT-Guitar's Stratocaster
// (dataset 2: five increasingly heavy mutes at fret 5 on every string, each
// against the same string's unmuted fret-5 note, per-partial decay). A point
// dashpot 20 mm in front of the saddle, working in parallel with the short
// stretch behind it, fits all thirty mutes within 1.8 dB per partial (15 to
// 25 mm within 0.05 dB of that): light, it damps partials as n^2; pressed
// harder than that stretch is stiff, every partial loses the same share each
// period. The lightest rest adds the same decay on every string, R = 12.7/s
// times the string's mass per metre (all six within 30%); the heaviest press
// is the same flesh on every string, 1.7 to 2.2 kg/s on the four whose
// partials last long enough to measure. Pressure blends the two
// geometrically; the player's three middle mutes sat between 0.1 and 0.9 of
// the way, and the blend at 0, 0.15, 0.45, 0.75 and 1 reproduces the thirty
// in this waveguide within 1.6 dB median per partial.
constexpr double palmMetres = 0.020;
constexpr double palmRestPerSecond = 12.7;
constexpr double palmPressedResistance = 2.0;

// Nor does the palm press alike from stroke to stroke. Over the same
// session's 177 muted lick notes, each against its own note picked open
// (dataset 2, middle coil), what the mute takes from the 250 Hz, 500 Hz and
// 1 kHz octaves by 120 to 200 ms spreads 8.1, 13.5 and 13.6 dB from note to
// note (robust SD), the model's at one pressure 4.1, 5.4 and 6.6 dB from the
// pick's own scatter; rendering the licks at pressures 0.5 and 0.75 moves
// those octaves 42, 78 and 85 dB per unit pressure, so the spread left over
// is 0.17, 0.16 and 0.14 of the pressure. Each stroke under Palm mute takes
// the middle of them about the Palm control. It includes whatever else
// differs between muted notes, so it bounds the palm's own scatter from
// above.
constexpr double palmScatter = 0.15;

// A finger lying on the string without pressing it, fitted to the 72 dead
// notes of the same Stratocaster (frets 3, 8 and 14 on every string; decay of
// each octave band 250 Hz to 4 kHz over the first 80 ms). A point cannot
// stop the string fast enough: at its best resistance the 2 and 4 kHz bands
// still ring 4 to 6 times too long (median of the 18 cases), an error of
// 5.9 dB in decay rate (mean of each case's RMS over the bands). Spread along
// 25 mm of string, 2 kg/s in all, it fits within 2.0 dB and the early tilt of
// the spectra within a few dB, with the same 2 kg/s on every string; 20 to
// 40 mm and 1.5 to 3 kg/s score within 0.4 dB of that. It is the palm's
// heaviest press: flesh pressed on a string.
constexpr double fingerResistance = 2.0;
constexpr double fingerWidth = 0.025;

// The finger that lets a fretted note go lets the string up off the fret
// before it leaves the string: for a moment the flesh, not the fret, ends
// it, and takes what arrives there, which no finger lying beside a fret can
// (a touch on a point the waves pass leaves at least half of them each
// time). On IDMT-SMT-Guitar's Stratocaster fret runs (dataset 2, 114
// fretted releases over the six strings) a released note's 0.1 to 1 kHz
// band falls 26 to 30 dB in its steepest 20 ms (medians by string). The
// finger beside the fret alone left it falling 16 to 19, 8.4 dB short at
// the median release, and least at the frets over the open string's nodes
// (the 7th, 12th and 19th, 9 to 14 dB). Ending the string with the
// finger's own flesh for 15 ms first, the releases fall 21 to 26 dB, 4.2 dB
// from the recording's at the median (mean 9.7 dB to 5.8); 10 to 30 ms fit
// within 0.7 dB of that and 1 to 4 kg/s within 0.4. A wider finger instead,
// 50 to 160 mm of it, fits no better than 5.7 dB and stops the treble
// strings far faster than the bass. The click at the release (the loudest
// 4 ms of 1 to 8 kHz around it, over the note before) moves from 11.7, 9.2,
// 1.2, -9.2, -0.9 and -1.6 dB to 6.4, 5.6, 0.8, 0.4, -0.1 and -0.2, the
// recording's 18.5, 9.2, 0.1, -4.3, -2.2 and -0.5.
constexpr double liftSeconds = 0.015;
// No two releases are alike: each takes its lift and its flesh at random,
// evenly in log, across the ranges that fit the recording's releases within
// 0.7 and 0.4 dB of the best fit above. How a player's releases spread is
// not measured; drawing within the fit is voiced.
constexpr double shortestLiftSeconds = 0.010, longestLiftSeconds = 0.030;
constexpr double softestLift = 1.0, firmestLift = 4.0;

// A harmonic touch has to absorb, not reflect: a rigid point at the node
// splits the string into two halves whose motion turns back into the
// fundamental when the finger lifts (20 dB of it left, measured). The finger
// rests on the node lightly and stays: on IDMT-SMT-Guitar's Stratocaster
// (dataset 2, its natural harmonics at frets 4, 5, 7, 9 and 12 on every
// string, 30 notes, read through the middle coil, whose comb its open notes
// match best), the open fundamental over the sounding partial (50 ms windows
// from the pick to 0.4 s, both floored at -55 dB) sits within a few dB of
// it at first and falls to -45 to -55 dB over 60 to 100 ms, 200 ms at fret
// 4. A dashpot of a quarter of the string's impedance held for 0.2 s follows
// that within 7.0 dB (median over notes and windows), 0.2 to 0.35 of it for
// 0.15 to 0.25 s within 7.5; one matched to the impedance and lifted after
// 40 ms, which leaves the fourth and fifth partials' harmonics a fundamental
// as loud as they are, within 12.3.
constexpr double harmonicTouchShare = 0.25;
constexpr double harmonicTouchSeconds = 0.2;
// The pinch's thumb catches the string with the matched touch, lifted after
// this, 30 to 40 dB clear of the modes without a node under it.
constexpr double touchSeconds = 0.040;

// A pinch harmonic: the thumb beside the pick catches the string as the pick
// leaves it and, as a harmonic's touch does, takes the waves passing it for
// the same time, leaving the partials with a node under it. The player moves
// the hand until the thumb lands on a node, the lowest partial's within reach
// of the pick, and the pick strikes the thumb's width off it (on the node it
// would leave out the very partial the thumb keeps). At the ordinary pick
// position the open strings' 5th partial then stands 26 to 63 dB further
// over the fundamental than a stroke leaves it; with the pick 5 mm off, 20
// to 56; the thumb lifting after 10 or 20 ms instead, 10 to 45. Voiced: no
// recording of a pinch was at hand.
constexpr double pinchReachMetres = 0.010;
constexpr double thumbOffsetMetres = 0.010;
constexpr int pinchHighestPartial = 24;

// Plucking with the fingers. IDMT-SMT-Guitar's Stratocaster plays its licks
// three ways (dataset 2), and 173 notes of 15 licks match note for note
// between the picked and the finger-plucked takes. Against the same note
// picked, read through the middle coil in octave bands over 0-20, 20-60 and
// 60-200 ms, each against its own 125-500 Hz, a plucked note stands 2 dB
// richer at 125 Hz and 3 dB poorer at 1 to 4 kHz (medians), from the first
// 20 ms on. A finger leaving the string as the pick does, at the pick's
// place, leaves those cells 2.0 dB from the recording's (mean absolute);
// leaving it 40 mm further from the bridge, and 1.5 times as slowly as the
// medium pick does at the same force, 0.8 (the neighbours tried, 20 to 60
// mm and 1.25 to 2 times, within 1.6; at the pick's place no release tried
// fits under 1.8). By ear the 1.5 times slower release sounded detuned, as a
// chorus, on the ringing arpeggios of the Alabama demo, and the finger now
// leaves the string as fast as the pick, 20 mm further from the bridge than
// it, the near edge of the places that fit (Docs/decisions.md).
constexpr double fingerOffsetMetres = 0.020;
constexpr double fingerReleaseShare = 1.0;

// A hammer-on or pull-off: the finger strikes or flicks the string at its
// fret with a share of a pick's force, and releases it slowly, being soft.
// Voiced at half the 1.4 N ordinary stroke the glide gave before the pick
// held a share at rest; the same 0.7 N is this share of the 1.88 N one.
constexpr double legatoShare = 0.375;
constexpr double fingerReleaseSeconds = 0.002;

// The fretting finger's own sounds. Between the notes of IDMT-SMT-Guitar's
// Stratocaster fret runs (dataset 2, every string from fret 0 to 20, picked
// one fret apart, 114 gaps), every gap but one holds one sharp tick before
// the next note, where the finger reaches its fret: 64, 135 and 265 ms
// before the pick (10th, 50th and 90th percentiles). Its loudest sample
// above 1 kHz stands 35, 38, 42, 48, 46 and 52 dB under the next note's
// attack from the low E up (medians), its swing below 2 kHz 42, 42, 46,
// 53, 46 and 50 dB under the note's peak.
//
// The finger takes the string down from its height over the fret
// (StratEngine::actionMetres) onto the fret; under it the string's end there
// moves with it and stops on the fret, which is the tick.
constexpr double pressSeconds = 0.035;
// Nor are two landings alike. The ticks before the recorded notes spread by
// 6.2 dB from note to note (the loudest sample above 1 kHz in each gap of
// 30 ms or more against the next note's attack, robust SD about each
// string's median, 120 gaps), and a tick's level goes as the speed it lands
// at, so each finger lands at the fitted speed times exp(0.71 z). The spread
// includes everything else that differs between the gaps, so it bounds the
// hand's from above; it is taken whole.
constexpr double landingScatter = 0.71;
// A finger gliding along a string it rests on to the next fret, with the
// flesh of a light touch; and one pressing a slide, lighter than a fretted
// note's press as a player eases off to move.
constexpr double shiftNewtons = 0.3;
constexpr double slideNewtons = 0.6;
// Each glide or slide presses as hard as the landings spread, voiced: no
// recording isolates a rub's level.
constexpr double rubScatter = landingScatter;
// The hand reaches the fret this long before the pick at the latest: the
// recordings' 10th percentile is 64 ms, but the lookahead leaves 50.
constexpr double pickGapSeconds = 0.010;

// A slide's travel time. On IDMT-SMT-Guitar's Stratocaster (dataset 2, 19
// picked slides up 2 to 7 frets: its slide take, up the G from the 5th fret
// by 2 to 7, and the slides of its licks; pYIN, 1.5 ms hops) the pitch
// crosses from 10 to 90 % of the way in 61, 96, 118, 152, 160 and 196 ms
// over 2 to 7 frets on the G, 49 to 80 ms over 2 in the licks and 26 to 49
// over 1; the line through them (Theil-Sen), over the 0.61 of the travel
// this smooth step takes from 10 to 90 %, is 8 ms and 43 ms a fret.
// Rendered up the G from the 5th fret and read the same way, the slides take
// 55, 81, 107, 139, 164 and 195 ms over 2 to 7 frets and 28 over 1; the 20
// and 30 ms voiced before took 47 to 143, 1.3 to 1.8 times too fast.
constexpr double slideBaseSeconds = 0.008;
constexpr double slideSecondsPerFret = 0.043;
// How soon a stroke must follow the last one to go the other way, as a
// strumming hand keeps moving. Voiced.
constexpr double alternateWithinSeconds = 0.35;

// Fretting-hand vibrato: a bend rocking up from the note and back, up to a
// quarter tone above it at full channel pressure. On IDMT-SMT-Guitar's Stratocaster
// (dataset 2, the 18 vibrato notes of 0.8 s or more at their own rate over
// 3.5 Hz, from its licks and its vibrato takes on every string; pYIN at 1
// cent, 2.7 ms hops) the hand rocks at 5.5 Hz (median), its swing up from
// the note and back down even (each rise 0.50 of its cycle), 26 cents deep
// (median), its troughs on the note; it starts from the note, its first peak
// 36 ms later than half a cycle after the pick (median of 16 notes over
// 3.5 Hz, timed from each note's own attack, the first 1 ms at 12 dB under
// its loudest; the annotated onsets run 32 ms early at the median, and the
// 68 ms read from them before was late by that); and no two cycles
// are alike: each lasts 9.5 % longer or shorter than the last (cycle to
// cycle, drift removed) and swings 42 % deeper or shallower (median of each
// note's spread). Those spreads include the tracker's own, so they bound
// the hand's from above; they are taken whole.
constexpr double vibratoHz = 5.5;
constexpr double vibratoSemitones = 0.5;
constexpr double vibratoDelaySeconds = 0.036;
constexpr double vibratoPeriodScatter = 0.095;
constexpr double vibratoDepthScatter = 0.40; // log SD of a 42 % spread

// Bend: the finger pushes the string a whole tone up (Half bend: a
// semitone; the licks below bend 0.78 to 1.2 semitones five times and 1.5 to
// 2.0 six, one 2.9), bendDelaySeconds after the pick, along a minimum-jerk path (reach) over
// bendRiseSeconds. On IDMT-SMT-Guitar's Stratocaster (dataset 2, the 12
// bends of its licks 3 to 6 that rise over half a semitone, every take; pYIN
// at 0.01 semitone, 1.5 ms hops, semitones over the struck note, from each
// note's own attack: the first 1 ms at 12 dB under its loudest within 150 ms
// of the annotation, which runs up to 50 ms early) the pitch is 10 % of the
// way up 138 ms after the pick (median; 28 to 93 ms for quick bends, 184 to
// 232 for notes held first), 10 to 90 % in 89 ms (48 to 120, one 287), and
// half way at 0.48 of that (median), as even as minimum jerk's 0.5. Minimum
// jerk crosses 10 to 90 % in 0.506 of its length and 10 % at 0.247 of it:
// 0.175 s, starting 95 ms after the pick. The isolated bends of its bending
// take, 0.3 to 0.6 s, are slower than any played in a lick and are left out.
// Slide in: the finger starts three frets below and glides at the measured
// slide's pace (slideSecondsPerFret) from the pick.
constexpr int bendFrets = 2;
constexpr int slideInFrets = 3;
constexpr double bendDelaySeconds = 0.095;
constexpr double bendRiseSeconds = 0.175;
// The Vibrato key rocks a fretted note as deep as the recorded hand's median,
// unless channel pressure asks for more. Read alike on the 16 vibrato notes
// over 3.5 Hz above and on 15 notes rendered with the key, A3 to C6 (pYIN at
// 0.01 semitone, 2.7 ms hops, smoothed over 13.6 ms; median of each note's
// peaks less its troughs of 6 cents' prominence), the recording swings 30
// cents and 0.26 semitones swung 23, so 0.34.
constexpr double latchedVibratoSemitones = 0.34;

// MPE's default member-channel bend range; a finger bends a string up only,
// and four semitones is as far as a string goes under a finger.
constexpr double memberBendSemitones = 48.0;
constexpr double maximumFingerBend = 4.0;

// The string's wave impedance at standard tension, kg/s.
double impedance(int s) noexcept
{
    const auto& spec = StratString::construction(s);
    return spec.kilogramsPerMetre * 2.0 * scale * spec.openHz;
}

// Where each finger lands behind its fret (fingerMetres): a third of the way
// to the next fret at the median, as players are taught to press close
// behind it, the share scattered by a factor of exp(0.25 z) and kept within
// 0.15 to 0.6, short of the next fret's buzz. Voiced at the owner's request
// (Docs/decisions.md, 2026-11-07). It moves where the flesh touches the
// string and where it rubs, not the pitch: a player in tune presses each
// note down onto its fret alike (StratEngine::fretPressMetres).
constexpr double placeShare = 1.0 / 3.0;
constexpr double placeScatter = 0.25;
constexpr double nearestPlace = 0.15, farthestPlace = 0.6;

// A fingertip presses just behind its fret, `place` of the way to the next
// one toward the nut; an open string is stopped by the hand near fret 3.
// Metres from string s's saddle, as the intonation has set it.
double fingerMetres(const StratEngine& guitar, int s, double fret, double place = placeShare) noexcept
{
    const double f = fret < 1.0 ? 3.0 : std::floor(fret + 0.5);
    const double at = guitar.fretMetres(s, f);
    return at + (guitar.fretMetres(s, f - 1.0) - at) * place;
}

double clamp(double value, double low, double high) noexcept
{
    return value < low ? low : (value > high ? high : value);
}

// A uniform draw in (0, 1) from xorshift32 `state`.
double uniformDraw(std::uint32_t& state) noexcept
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<double>(state) + 0.5) / 4294967296.0;
}

// A standard normal draw from xorshift32 `state`, within 2.5 deviations.
double normalDraw(std::uint32_t& state) noexcept
{
    const double a = uniformDraw(state), b = uniformDraw(state);
    return clamp(std::sqrt(-2.0 * std::log(a)) * std::cos(2.0 * pi * b), -2.5, 2.5);
}

// From `low` to `high` evenly in log, at u in [0, 1].
double logBetween(double low, double high, double u) noexcept
{
    return low * std::pow(high / low, u);
}

// Minimum jerk: where a hand moving from 0 to 1 over u in [0, 1] is, and its
// speed over the mean.
double reach(double u) noexcept { return u * u * u * (10.0 - 15.0 * u + 6.0 * u * u); }
double reachSpeed(double u) noexcept { return 30.0 * u * u * (1.0 - u) * (1.0 - u); }

// Where the thumb lands for a pinch on `length` of string picked `pick` from
// the bridge.
double pinchNode(double length, double pick) noexcept
{
    double nearest = 0.5 * length;
    for (int k = 2; k <= pinchHighestPartial; ++k)
        for (int j = 1; j < k; ++j)
        {
            const double x = j * length / k;
            if (std::abs(x - pick) <= pinchReachMetres)
                return x;
            if (std::abs(x - pick) < std::abs(nearest - pick))
                nearest = x;
        }
    return nearest;
}

} // namespace

StratPerformer::StratPerformer() : guitar(std::make_unique<StratEngine>()) {}

void StratPerformer::prepare(double rate)
{
    sampleRate = rate;
    guitar->prepare(rate);
    reset();
    setControls(current);
}

void StratPerformer::reset() noexcept
{
    guitar->reset();
    guitar->setArm(0.0);
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        guitar->setBend(s, 1.0);
        guitar->setFret(s, 0.0);
        guitar->setDamper(s, 1, 0.0, 0.0);
        strings[static_cast<size_t>(s)] = StringState {};
    }
    channelBend.fill(0.0);
    wheel = palmOffset = 0.0;
    vibratoDepth = vibratoPhase = 0.0;
    vibratoCycleHz = vibratoHz;
    vibratoCycleDepth = 1.0;
    vibratoHold = 0;
    wobble = wobbleSeed;
    pedal = false;
    damping = false;
    fingers = false;
    hand = 2.0;
    nextStrokeUp = false;
    tilt = tiltSeed;
    scatter = scatterSeed;
    pace = paceSeed;
    press = pressSeed;
    fretHand = fretHandSeed;
    strokePick = 0.0;
    steadyStroke = true;
    lastStroke = -1000000;
    pendingCount = eventCount = 0;
    delayedHead = delayedCount = 0;
    now = 0;
    applyPalm();
}

void StratPerformer::setControls(const StratControls& controls) noexcept
{
    if (!controls.lookahead)
        while (delayedCount > 0)
        {
            // What was held back plays now, in order.
            const Midi m = delayed[static_cast<size_t>(delayedHead)];
            delayedHead = (delayedHead + 1) % maxMidi;
            --delayedCount;
            dispatch(m);
        }
    latency = controls.lookahead ? static_cast<int>(std::lround(lookaheadSeconds * sampleRate)) : 0;
    current = controls;
    current.pickMetres = clamp(controls.pickMetres, 0.025, 0.250);
    current.pick = clamp(controls.pick, 0.0, 1.0);
    current.palmMute = clamp(controls.palmMute, 0.0, 1.0);
    current.strumSeconds = clamp(controls.strumSeconds, 0.0, 0.040);
    current.outputDecibels = clamp(controls.outputDecibels, -24.0, 12.0);
    current.hum = clamp(controls.hum, 0.0, 1.0);
    current.mainsHz = controls.mainsHz < 55.0 ? 50.0 : 60.0;
    guitar->setControls(current.circuit);
    guitar->setHum(current.hum, current.mainsHz);
    outputGain = std::pow(10.0, current.outputDecibels / 20.0);
    applyPalm();
}

int StratPerformer::stringNote(int s) const noexcept
{
    return s >= 0 && s < StratEngine::stringCount ? strings[static_cast<size_t>(s)].note : -1;
}

int StratPerformer::stringFret(int s) const noexcept
{
    return s >= 0 && s < StratEngine::stringCount
        ? static_cast<int>(std::floor(strings[static_cast<size_t>(s)].fret + 0.5))
        : 0;
}

double StratPerformer::velocityForce(int velocity) const noexcept
{
    const double v = clamp(velocity / 127.0, 0.0, 1.0);
    return fullForce * (lightestShare + (1.0 - lightestShare) * v);
}

double StratPerformer::releaseSeconds(int s, double force) const noexcept
{
    if (fingers)
    {
        const double tip = 0.5 * pi * std::sqrt(mediumTipKilograms / mediumStiffness);
        return fingerReleaseShare * (impedance(s) * gripDepth / force + tip);
    }
    const double thickness = thinThickness + (heavyThickness - thinThickness) * current.pick;
    const double ratio = thickness / mediumThickness;
    const double stiffness = mediumStiffness * ratio * ratio * ratio;
    const double tip = 0.5 * pi * std::sqrt(mediumTipKilograms * ratio / stiffness);
    return impedance(s) * gripDepth / force + tip;
}

// The mod wheel lowers the palm onto the strings: its resistance grows as the
// wheel's square, to Palm mute's at the top, so the lower half of the wheel
// is the palm's lightest touches and the upper half rests it to the Palm
// control's press. The square is a playing law, not a measurement.
void StratPerformer::applyPalm() noexcept
{
    const double share = latched == Articulation::palmMute ? 1.0 : wheel * wheel;
    const double pressure = clamp(current.palmMute + palmOffset, 0.0, 1.0);
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        const double rest = palmRestPerSecond * StratString::construction(s).kilogramsPerMetre;
        const double resistance
            = rest * std::pow(palmPressedResistance / rest, pressure);
        guitar->setDamper(s, 0, palmMetres, share * resistance);
    }
}

bool StratPerformer::queue(const Midi& m) noexcept
{
    if (latency == 0)
        return false;
    if (delayedCount == maxMidi)
    {
        // Full: the oldest plays now rather than be lost.
        const Midi oldest = delayed[static_cast<size_t>(delayedHead)];
        delayedHead = (delayedHead + 1) % maxMidi;
        --delayedCount;
        dispatch(oldest);
    }
    delayed[static_cast<size_t>((delayedHead + delayedCount) % maxMidi)] = m;
    ++delayedCount;
    return true;
}

void StratPerformer::dispatch(const Midi& m) noexcept
{
    switch (m.kind)
    {
    case MidiKind::noteOn: playNoteOn(m.channel, m.note, m.velocity, m.string); break;
    case MidiKind::noteOff: playNoteOff(m.channel, m.note); break;
    case MidiKind::bend: playPitchBend(m.channel, m.value); break;
    case MidiKind::wheel: wheel = m.value; applyPalm(); break;
    case MidiKind::pressure: vibratoDepth = m.value; break;
    case MidiKind::pedal: playSustain(m.value > 0.5); break;
    }
}

void StratPerformer::noteOn(int channel, int note, int velocity, int stringIndex) noexcept
{
    if (velocity <= 0)
    {
        noteOff(channel, note);
        return;
    }
    Midi m;
    m.time = now + latency;
    m.kind = MidiKind::noteOn;
    m.channel = channel & 15;
    m.note = note;
    m.velocity = velocity;
    m.string = stringIndex;
    if (!queue(m))
    {
        playNoteOn(channel, note, velocity, stringIndex);
        return;
    }
    if (note >= lowestNote)
    {
        // The hand reads ahead: it goes to the notes as soon as it can.
        Event e;
        e.time = now;
        e.kind = EventKind::prepare;
        e.due = m.time;
        schedule(e);
    }
}

void StratPerformer::playNoteOn(int channel, int note, int velocity, int stringIndex) noexcept
{
    if (note >= firstArticulationKey && note < firstArticulationKey + articulationCount)
    {
        latched = static_cast<Articulation>(note - firstArticulationKey);
        applyPalm();
        return;
    }
    if (note >= firstStrokeKey && note < firstStrokeKey + strokeCount)
    {
        strokes = static_cast<Stroke>(note - firstStrokeKey);
        fingers = false;
        return;
    }
    if (note == fingerKey)
    {
        fingers = true;
        return;
    }
    if (note >= firstGestureKey && note < firstGestureKey + gestureCount)
    {
        latched = static_cast<Articulation>(articulationCount + note - firstGestureKey);
        applyPalm();
        return;
    }
    if (note == dampKey)
    {
        dampStrings(true);
        return;
    }
    const int top = latched == Articulation::harmonic ? highestNote + 10 : highestNote;
    if (note < lowestNote || note > top || pendingCount >= maxPending)
        return;
    pending[static_cast<size_t>(pendingCount++)] = {note, channel & 15, velocity, stringIndex};
}

void StratPerformer::noteOff(int channel, int note) noexcept
{
    Midi m;
    m.time = now + latency;
    m.kind = MidiKind::noteOff;
    m.channel = channel & 15;
    m.note = note;
    if (!queue(m))
        playNoteOff(channel, note);
}

void StratPerformer::playNoteOff(int channel, int note) noexcept
{
    if (note == dampKey)
    {
        dampStrings(false);
        return;
    }
    channel &= 15;
    for (int i = 0; i < pendingCount; ++i)
        if (pending[static_cast<size_t>(i)].note == note
            && pending[static_cast<size_t>(i)].channel == channel)
        {
            pending[static_cast<size_t>(i)] = pending[static_cast<size_t>(--pendingCount)];
            return;
        }
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        StringState& st = strings[static_cast<size_t>(s)];
        if (st.note != note || st.channel != channel || !st.keyDown)
            continue;
        st.keyDown = false;
        bool struck = true;
        for (int e = 0; e < eventCount; ++e)
            struck = struck
                && !(events[static_cast<size_t>(e)].kind == EventKind::strike
                     && events[static_cast<size_t>(e)].string == s);
        if (pedal)
            st.sustained = true;
        else if (struck)
            release(s);
        return;
    }
}

void StratPerformer::release(int s) noexcept
{
    StringState& st = strings[static_cast<size_t>(s)];
    // The fretting finger lets go of the fret but stays on the string.
    const double rest = st.fret, place = st.place;
    guitar->setDamper(s, 1, fingerMetres(*guitar, s, rest, place), fingerResistance, fingerWidth);
    if (st.fret > 0.0)
    {
        const double lift = steadyStroke
            ? liftSeconds
            : logBetween(shortestLiftSeconds, longestLiftSeconds, uniformDraw(fretHand));
        const double flesh = steadyStroke
            ? fingerResistance
            : logBetween(softestLift, firmestLift, uniformDraw(fretHand));
        guitar->liftFromFret(s, flesh);
        Event leave;
        leave.time = now + static_cast<std::int64_t>(lift * sampleRate);
        leave.kind = EventKind::leaveString;
        leave.string = s;
        schedule(leave);
    }
    guitar->setBend(s, 1.0);
    const std::uint64_t order = st.order;
    const int prepared = st.prepared;
    const std::int64_t preparedDue = st.preparedDue;
    st = StringState {};
    st.order = order;
    st.released = true;
    st.restFret = rest;
    st.place = place;
    st.prepared = prepared;
    st.preparedDue = preparedDue;
    // The hand that lets a note go lies across the free strings with it
    // until it next picks. On IDMT-SMT-Guitar's Stratocaster fret runs
    // (dataset 2: frets 0 to 20 on every string, 126 releases with the other
    // five strings free), the other strings' own lines, 291 that stood 10 dB
    // over the noise before a release, fall a median 17 dB at it, into the
    // noise (32 stay 6 dB over it), and the open strings a note drives in
    // tune (unisons, octaves, fifths) are left 56 to 69 dB under its
    // strongest line (medians by string). Free, they were left here 29 to 41
    // dB under it, still ringing; under the hand, 58 dB and more, below the
    // recording's noise 77 to 85 dB down. The flesh is the finger's, where
    // the finger lies. The strings ring free from the next stroke, as the
    // recording's own lines do in each note.
    for (int t = 0; t < StratEngine::stringCount; ++t)
    {
        StringState& free = strings[static_cast<size_t>(t)];
        if (free.note >= 0 || free.released || free.handResting)
            continue;
        guitar->setDamper(t, 1, fingerMetres(*guitar, t, rest, place), fingerResistance, fingerWidth);
        free.handResting = true;
    }
}

void StratPerformer::liftRestingHand() noexcept
{
    for (int t = 0; t < StratEngine::stringCount; ++t)
    {
        StringState& st = strings[static_cast<size_t>(t)];
        if (!st.handResting)
            continue;
        guitar->setDamper(t, 1, 0.0, 0.0);
        st.handResting = false;
    }
}

// The fretting hand laid flat across the strings where it is: the flesh of a
// released note's finger (release, above) on every string, which lets go of
// its fret. It stays while the key is held, so strokes under it are muted.
void StratPerformer::dampStrings(bool down) noexcept
{
    damping = down;
    if (!down)
    {
        liftRestingHand();
        return;
    }
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        StringState& st = strings[static_cast<size_t>(s)];
        guitar->setDamper(s, 1, fingerMetres(*guitar, s, hand), fingerResistance, fingerWidth);
        if (st.fret > 0.0)
            guitar->setFret(s, 0.0);
        guitar->setBend(s, 1.0);
        const std::uint64_t order = st.order;
        st = StringState {};
        st.order = order;
        st.handResting = true;
    }
}

// A pick leaves the string at a tilt that changes from stroke to stroke, so
// the string's swing along the top sets off with its swing toward the poles
// or against it, and its partials' beats start at their sum or at their
// difference. On the Strat recorded direct (IDMT-SMT-Guitar, neck pickup, 78
// notes, one stroke each), the two decaying components fitted to each of the
// first six partials' envelopes over 2 s (295 fits where a pair beats one
// decay by 0.3 dB), taken back to the onset, start at their sum (cos of
// their phase over 0.5) in 30 % and at their difference (under -0.5) in
// 38 %, every string both ways. The same fit puts 73 % and 12 % on renders
// whose strokes all set the planes off together and 9 % and 72 % on renders
// whose strokes all set them apart. With every stroke alike every note beat
// alike: the 450 to 1400 Hz partials of the plain strings dipped together a
// second in and then held, where the recorded ones decline steadily. Half
// the 78 notes each way brings their early balance at 1.3 to 3.2 kHz to 2.8
// to 4.0 dB from the recording's, from 4.1 to 5.9.
bool StratPerformer::nextTiltOpposed() noexcept
{
    // xorshift32; its top bit is even enough from stroke to stroke, and from
    // this seed the first stroke after a reset sets the planes off together.
    tilt ^= tilt << 13;
    tilt ^= tilt >> 17;
    tilt ^= tilt << 5;
    return (tilt >> 31) != 0;
}

// A standard normal draw for the hand's scatter, within 2.5 deviations; the
// first stroke after a reset lands exactly where the controls put the hand, so
// a single note measures the controls.
double StratPerformer::nextScatter() noexcept
{
    return steadyStroke ? 0.0 : normalDraw(scatter);
}

double StratPerformer::strokeForce(int velocity) noexcept
{
    return velocityForce(velocity) * std::exp(forceScatter * nextScatter());
}

void StratPerformer::pitchBend(int channel, double position) noexcept
{
    Midi m;
    m.time = now + latency;
    m.kind = MidiKind::bend;
    m.channel = channel & 15;
    m.value = position;
    if (!queue(m))
        playPitchBend(channel, position);
}

void StratPerformer::playPitchBend(int channel, double position) noexcept
{
    channel &= 15;
    position = clamp(position, -1.0, 1.0);
    if (channel == 0)
        guitar->setArm(position);
    else
        channelBend[static_cast<size_t>(channel)]
            = clamp(position * memberBendSemitones, 0.0, maximumFingerBend);
}

void StratPerformer::modWheel(double position) noexcept
{
    Midi m;
    m.time = now + latency;
    m.kind = MidiKind::wheel;
    m.value = clamp(position, 0.0, 1.0);
    if (!queue(m))
        dispatch(m);
}

void StratPerformer::channelPressure(double position) noexcept
{
    Midi m;
    m.time = now + latency;
    m.kind = MidiKind::pressure;
    m.value = clamp(position, 0.0, 1.0) * vibratoSemitones;
    if (!queue(m))
        dispatch(m);
}

void StratPerformer::sustain(bool down) noexcept
{
    Midi m;
    m.time = now + latency;
    m.kind = MidiKind::pedal;
    m.value = down ? 1.0 : 0.0;
    if (!queue(m))
        playSustain(down);
}

void StratPerformer::playSustain(bool down) noexcept
{
    pedal = down;
    if (down)
        return;
    for (int s = 0; s < StratEngine::stringCount; ++s)
        if (strings[static_cast<size_t>(s)].sustained)
            release(s);
}

void StratPerformer::allNotesOff() noexcept
{
    pendingCount = 0;
    eventCount = 0;
    delayedHead = delayedCount = 0;
    pedal = false;
    damping = false;
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        const StringState& st = strings[static_cast<size_t>(s)];
        if (st.note >= 0)
            release(s);
        else if (!st.fretted)
            guitar->setFret(s, 0.0); // a lift under way, its end dropped above
    }
}

void StratPerformer::schedule(const Event& e) noexcept
{
    if (eventCount >= maxEvents)
        return;
    int i = eventCount++;
    while (i > 0 && events[static_cast<size_t>(i - 1)].time > e.time)
    {
        events[static_cast<size_t>(i)] = events[static_cast<size_t>(i - 1)];
        --i;
    }
    events[static_cast<size_t>(i)] = e;
}

bool StratPerformer::playOnSoundingString(const Pending& p) noexcept
{
    // The line continues on the string it is ringing on.
    int s = -1;
    std::uint64_t latest = 0;
    for (int k = 0; k < StratEngine::stringCount; ++k)
    {
        const StringState& st = strings[static_cast<size_t>(k)];
        if (st.order > latest && guitar->isSounding(k))
        {
            latest = st.order;
            s = k;
        }
    }
    if (s < 0)
        return false;
    const int fret = p.note - openNote[s];
    if (fret < 0 || fret > frets)
        return false;

    StringState& st = strings[static_cast<size_t>(s)];
    const double from = st.fret;
    guitar->setDamper(s, 1, 0.0, 0.0);
    st.bendSemitones = 0.0; // the finger lets the bend down as it moves on
    if (latched == Articulation::legato)
    {
        guitar->setFret(s, fret);
        if (fret > 0)
            startVibrato();
        // A hammer-on strikes at the new fret, a pull-off flicks at the old;
        // the finger then on the string lands where it lands.
        const double place = nextPlace();
        const double at = fret > from ? fingerMetres(*guitar, s, fret, place)
                                      : fingerMetres(*guitar, s, from, st.place);
        const double force = legatoShare * velocityForce(p.velocity);
        guitar->pluck(s, nextStrokeUp ? -force : force, at, fingerReleaseSeconds);
        st.place = place;
        st.slide = false;
    }
    else
    {
        st.slide = true;
        st.rubNewtons = slideNewtons * handScale(rubScatter);
        st.slideFrom = from;
        st.slideTo = fret;
        st.slideStart = now;
        st.slideLength = static_cast<std::int64_t>(
            (slideBaseSeconds + slideSecondsPerFret * std::abs(fret - from)) * sampleRate);
    }
    st.note = p.note;
    st.channel = p.channel;
    st.keyDown = true;
    st.sustained = false;
    st.fret = latched == Articulation::legato ? fret : from;
    st.fretted = true;
    st.order = ++orderCounter;
    if (fret > 0)
        hand = fret;
    return true;
}

bool StratPerformer::playHarmonic(const Pending& p, double force) noexcept
{
    // A natural harmonic, the lowest partial that gives the note, else the
    // octave harmonic over a fretted note, touched at half its length.
    // The lowest partial of string s that gives the note, 0 none.
    const auto partialOn = [&](int s) {
        for (int k = 2; k <= 6; ++k)
            if (!strings[static_cast<size_t>(s)].keyDown
                && std::abs(openNote[s] + 12.0 * std::log2(k) - p.note) < 0.3)
                return k;
        return 0;
    };
    int string = -1, partial = 0;
    double fret = 0.0;
    // A string asked for, as a tab names it, plays it if it can.
    if (p.string >= 0 && p.string < StratEngine::stringCount && partialOn(p.string) > 0)
    {
        string = p.string;
        partial = partialOn(string);
    }
    for (int k = 2; k <= 6 && string < 0; ++k)
        for (int s = 0; s < StratEngine::stringCount && string < 0; ++s)
            if (partialOn(s) == k)
            {
                string = s;
                partial = k;
            }
    if (string < 0)
        for (int s = StratEngine::stringCount - 1; s >= 0 && string < 0; --s)
        {
            const int f = p.note - 12 - openNote[s];
            if (f >= 1 && f <= frets && !strings[static_cast<size_t>(s)].keyDown)
            {
                string = s;
                partial = 2;
                fret = f;
            }
        }
    if (string < 0)
        return false;

    Event e;
    e.time = now;
    e.kind = EventKind::strike;
    e.string = string;
    e.fret = fret;
    e.force = force;
    // The node nearest the nut, where a player touches.
    e.touchMetres = guitar->fretMetres(string, fret) * (partial - 1) / partial;
    e.touchResistance = harmonicTouchShare * impedance(string);
    e.pickOffset = strokePick;
    schedule(e);
    Event lift;
    lift.time = now + static_cast<std::int64_t>(harmonicTouchSeconds * sampleRate);
    lift.kind = EventKind::liftTouch;
    lift.string = string;
    schedule(lift);

    StringState& st = strings[static_cast<size_t>(string)];
    st = StringState {};
    st.note = p.note;
    st.channel = p.channel;
    st.keyDown = true;
    st.fret = fret;
    st.fretted = fret > 0.0;
    st.order = ++orderCounter;
    return true;
}

void StratPerformer::assign(const Pending* notes, int count, const std::array<bool, 6>& held,
                            std::array<int, 6>& stringFor) const noexcept
{
    // Every way of putting the notes on distinct strings, lowest note first,
    // scored as a hand would find it: frets inside a four-fret span around
    // the hand, open strings in the low positions, no held string taken, low
    // notes on low strings.
    std::array<int, 6> trial {};
    // At most one note per string.
    const int placed = count < StratEngine::stringCount ? count : StratEngine::stringCount;
    double best = 1e300;
    bool used[StratEngine::stringCount] {};
    const auto noteCost = [&](int s, int fret, const Pending& p) {
        const int note = p.note;
        double c = held[static_cast<size_t>(s)] ? 20.0 : 0.0;
        // A string asked for, if it can play the note, outweighs every
        // preference of the hand's.
        if (p.string >= 0 && p.string < StratEngine::stringCount && p.string != s
            && note >= openNote[p.string] && note - openNote[p.string] <= frets)
            c += 1000.0;
        // Where the hand has already gone for this note.
        if (strings[static_cast<size_t>(s)].prepared == note)
            c -= 10.0;
        if (fret == 0)
            return c + (hand <= 5.0 ? 0.0 : 3.0);
        const double low = hand - 1.0, high = hand + 3.0;
        c += (fret < low ? low - fret : 0.0) + (fret > high ? fret - high : 0.0);
        return c + 0.05 * fret;
    };
    const auto search = [&](const auto& self, int i, double cost) -> void {
        if (cost >= best)
            return;
        // The constant bound is what lets GCC see trial is never overrun
        // through the inlined recursion (-Warray-bounds).
        if (i >= placed || i >= StratEngine::stringCount)
        {
            int lowest = frets + 1, highest = 0;
            double extra = 0.0;
            for (int a = 0; a < placed; ++a)
            {
                if (trial[static_cast<size_t>(a)] < 0)
                    continue;
                const int f = notes[a].note - openNote[trial[static_cast<size_t>(a)]];
                if (f > 0)
                {
                    lowest = f < lowest ? f : lowest;
                    highest = f > highest ? f : highest;
                }
                for (int b = a + 1; b < placed; ++b)
                    if (trial[static_cast<size_t>(b)] >= 0
                        && trial[static_cast<size_t>(b)] < trial[static_cast<size_t>(a)])
                        extra += 2.0;
            }
            if (highest - lowest > 4)
                extra += 3.0 * (highest - lowest - 4);
            if (cost + extra < best)
            {
                best = cost + extra;
                stringFor = trial;
            }
            return;
        }
        // Left out, if no string can take it.
        trial[static_cast<size_t>(i)] = -1;
        self(self, i + 1, cost + 100.0);
        for (int s = 0; s < StratEngine::stringCount; ++s)
        {
            const int fret = notes[i].note - openNote[s];
            if (used[s] || fret < 0 || fret > frets)
                continue;
            used[s] = true;
            trial[static_cast<size_t>(i)] = s;
            self(self, i + 1, cost + noteCost(s, fret, notes[i]));
            used[s] = false;
        }
    };
    stringFor.fill(-1);
    search(search, 0, 0.0);
}

void StratPerformer::startStroke() noexcept
{
    // Sort what arrived together, lowest first.
    for (int i = 1; i < pendingCount; ++i)
        for (int j = i; j > 0 && pending[static_cast<size_t>(j - 1)].note
                                     > pending[static_cast<size_t>(j)].note;
             --j)
        {
            const Pending t = pending[static_cast<size_t>(j)];
            pending[static_cast<size_t>(j)] = pending[static_cast<size_t>(j - 1)];
            pending[static_cast<size_t>(j - 1)] = t;
        }

    const bool alternate = now - lastStroke
        < static_cast<std::int64_t>(alternateWithinSeconds * sampleRate);
    steadyStroke = lastStroke < 0;
    strokePick = pickScatterMetres * nextScatter();
    const bool palmDown = latched == Articulation::palmMute || wheel > 0.0;
    palmOffset = palmDown && !steadyStroke ? palmScatter * normalDraw(press) : 0.0;
    if (palmDown)
        applyPalm();
    const bool strokeUp = strokes == Stroke::up
        || (strokes == Stroke::alternate && alternate && nextStrokeUp);
    nextStrokeUp = !strokeUp;
    lastStroke = now;
    const double sign = strokeUp ? -1.0 : 1.0;

    // A bend or a slide in is struck below the note, where the hand starts;
    // it goes up to the note after the pick.
    const int below = latched == Articulation::bend ? bendFrets
        : latched == Articulation::halfBend            ? 1
        : latched == Articulation::slideIn             ? slideInFrets
                                                       : 0;
    Pending notes[6];
    int wanted[6] {};
    int count = 0;
    for (int i = 0; i < pendingCount; ++i)
    {
        const Pending& p = pending[static_cast<size_t>(i)];
        if ((latched == Articulation::legato || latched == Articulation::slide)
            && playOnSoundingString(p))
            continue;
        if (latched == Articulation::harmonic && playHarmonic(p, sign * strokeForce(p.velocity)))
            continue;
        if (count < 6 && p.note <= highestNote)
        {
            wanted[count] = p.note;
            notes[count] = p;
            if (p.note - below >= lowestNote)
                notes[count].note -= below;
            ++count;
        }
    }
    pendingCount = 0;
    if (count == 0)
        return;

    std::array<int, 6> stringFor {};
    std::array<bool, 6> held {};
    for (int s = 0; s < StratEngine::stringCount; ++s)
        held[static_cast<size_t>(s)] = strings[static_cast<size_t>(s)].keyDown;
    assign(notes, count, held, stringFor);
    int first = StratEngine::stringCount, last = -1;
    for (int i = 0; i < count; ++i)
        if (stringFor[static_cast<size_t>(i)] >= 0)
        {
            first = stringFor[static_cast<size_t>(i)] < first ? stringFor[static_cast<size_t>(i)] : first;
            last = stringFor[static_cast<size_t>(i)] > last ? stringFor[static_cast<size_t>(i)] : last;
        }

    std::array<double, 6> crossing {};
    for (int k = 1; k < StratEngine::stringCount; ++k)
        crossing[static_cast<size_t>(k)] = crossing[static_cast<size_t>(k - 1)]
            + current.strumSeconds
                * std::exp(strumGapScatter * (steadyStroke ? 0.0 : normalDraw(pace)));

    double lowestFret = frets + 1.0;
    for (int i = 0; i < count; ++i)
    {
        const int s = stringFor[static_cast<size_t>(i)];
        if (s < 0)
            continue;
        // From a fret, never the open string: a bend needs a finger on the
        // string and a slide one to glide. So a note the shift put on an
        // open string comes up from its first fret instead.
        const int target = wanted[i] - openNote[s];
        const int rise = std::min(wanted[i] - notes[i].note, std::max(target - 1, 0));
        const int fret = target - rise;
        if (fret > frets)
            continue;
        const bool dead = latched == Articulation::deadNote;
        StringState& st = strings[static_cast<size_t>(s)];
        // A finger the hand sent ahead is on its fret already.
        const bool pressed = st.prepared == notes[i].note && st.fretted && st.fret == fret;
        const double place = pressed ? st.place : nextPlace();
        // The pick crosses every string between, muted ones included.
        const int crossed = strokeUp ? last - s : s - first;
        Event e;
        e.time = now + static_cast<std::int64_t>(crossing[static_cast<size_t>(crossed)] * sampleRate);
        e.kind = EventKind::strike;
        e.string = s;
        e.fret = dead ? 0.0 : fret;
        e.force = sign * strokeForce(notes[i].velocity);
        e.touchMetres = dead ? fingerMetres(*guitar, s, fret, place) : 0.0;
        e.touchResistance = dead ? fingerResistance : 0.0;
        e.touchWidth = dead ? fingerWidth : 0.0;
        e.pinch = latched == Articulation::pinch;
        e.pickOffset = strokePick;
        e.slideFrets = latched == Articulation::slideIn ? rise : 0.0;
        e.bendSemitones
            = latched == Articulation::bend || latched == Articulation::halfBend ? rise : 0.0;
        schedule(e);

        st = StringState {};
        st.place = place;
        st.note = wanted[i];
        st.channel = notes[i].channel;
        st.keyDown = true;
        st.fret = e.fret;
        st.fretted = !dead && fret > 0;
        st.order = ++orderCounter;
        if (fret > 0 && fret < lowestFret)
            lowestFret = fret;
    }
    // The hand settles where the shape's lowest fretted note sits.
    if (lowestFret <= frets && (lowestFret < hand - 1.0 || lowestFret > hand + 3.0))
        hand = lowestFret;
    unprepare();
}

// A finger the hand sent ahead to a note that went to another string lets
// that string go as a played note's finger does.
void StratPerformer::unprepare() noexcept
{
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        StringState& st = strings[static_cast<size_t>(s)];
        if (st.prepared < 0 || st.note >= 0 || st.preparedDue > now)
            continue;
        st.prepared = -1;
        if (st.shifting)
            guitar->rub(s, st.shiftTo, 0.0, 0.0);
        st.shifting = false;
        if (st.fretted)
            release(s);
    }
}

// The fretting hand reads the notes due at `due` and goes to them: on each
// string a note will take, a finger glides along it from where it rests,
// if it rests there, and presses the string to its fret, as early as the
// lookahead lets it and after the key holding the string lets go, and a
// little before the pick (the timings above).
void StratPerformer::prepareNotes(std::int64_t due) noexcept
{
    // The articulation and the damping hand as they will stand then.
    Articulation style = latched;
    bool damped = damping;
    Pending notes[6];
    int count = 0;
    std::array<bool, 6> held {};
    std::array<std::int64_t, 6> freeAt {};
    for (int s = 0; s < StratEngine::stringCount; ++s)
        held[static_cast<size_t>(s)] = strings[static_cast<size_t>(s)].keyDown;
    for (int i = 0; i < delayedCount; ++i)
    {
        Midi& m = delayed[static_cast<size_t>((delayedHead + i) % maxMidi)];
        if (m.time > due)
            break;
        if (m.kind == MidiKind::noteOn)
        {
            if (m.note >= firstArticulationKey && m.note < firstArticulationKey + articulationCount)
                style = static_cast<Articulation>(m.note - firstArticulationKey);
            else if (m.note == dampKey)
                damped = true;
            else if (m.time == due && !m.prepared && m.note >= lowestNote && count < 6)
            {
                m.prepared = true;
                notes[count++] = {m.note, m.channel, m.velocity, m.string};
            }
        }
        else if (m.kind == MidiKind::noteOff)
        {
            if (m.note == dampKey)
                damped = false;
            for (int s = 0; s < StratEngine::stringCount; ++s)
            {
                const StringState& st = strings[static_cast<size_t>(s)];
                if (st.keyDown && st.note == m.note && st.channel == m.channel && !pedal)
                {
                    held[static_cast<size_t>(s)] = false;
                    freeAt[static_cast<size_t>(s)] = m.time;
                }
            }
        }
    }
    const bool fretting = style == Articulation::sustain || style == Articulation::palmMute
        || style == Articulation::pinch;
    if (count == 0 || !fretting || damped)
        return;

    std::array<int, 6> stringFor {};
    assign(notes, count, held, stringFor);
    const auto press = static_cast<std::int64_t>(pressSeconds * sampleRate);
    const std::int64_t latest = due - static_cast<std::int64_t>(pickGapSeconds * sampleRate);
    for (int i = 0; i < count; ++i)
    {
        const int s = stringFor[static_cast<size_t>(i)];
        if (s < 0)
            continue;
        StringState& st = strings[static_cast<size_t>(s)];
        const int fret = notes[i].note - openNote[s];
        if (fret <= 0 || held[static_cast<size_t>(s)] || st.prepared >= 0)
            continue;
        const std::int64_t start = now > freeAt[static_cast<size_t>(s)] ? now : freeAt[static_cast<size_t>(s)];
        if (latest - start < press)
            continue;
        st.prepared = notes[i].note;
        st.preparedDue = due;
        const double place = nextPlace();
        std::int64_t contact = start + press;
        // A finger already on the string, or about to be, at another fret.
        const bool resting = st.released || st.keyDown;
        const double from = st.keyDown ? st.fret : st.restFret;
        if (resting && from > 0.0 && from != fret)
        {
            const double frets = std::abs(fret - from);
            const auto glide = static_cast<std::int64_t>(
                (slideBaseSeconds + slideSecondsPerFret * frets) * sampleRate);
            const std::int64_t room = latest - start - press;
            const std::int64_t length = glide < room ? glide : room;
            Event e;
            e.time = start;
            e.kind = EventKind::shift;
            e.string = s;
            e.from = fingerMetres(*guitar, s, from, st.place) - 0.5 * fingerWidth;
            e.to = fingerMetres(*guitar, s, fret, place) - 0.5 * fingerWidth;
            e.length = length;
            schedule(e);
            contact = start + length + press;
        }
        Event p;
        p.time = contact - press;
        p.kind = EventKind::press;
        p.string = s;
        p.fret = fret;
        p.place = place;
        schedule(p);
    }
}

void StratPerformer::runEvent(const Event& e) noexcept
{
    const int s = e.string;
    StringState& st = strings[static_cast<size_t>(s)];
    if (e.kind == EventKind::leaveString)
    {
        // Unless the hand has fretted or played it since.
        if (st.note < 0 && !st.fretted)
            guitar->setFret(s, 0.0);
        return;
    }
    if (e.kind == EventKind::liftTouch)
    {
        if (!st.handResting)
            guitar->setDamper(s, 1, 0.0, 0.0);
        return;
    }
    if (e.kind == EventKind::prepare)
    {
        prepareNotes(e.due);
        return;
    }
    if (e.kind == EventKind::shift || e.kind == EventKind::press)
    {
        // Unless the note went elsewhere, or a key took the string first.
        if (st.prepared < 0 || st.keyDown)
            return;
        if (e.kind == EventKind::shift)
        {
            st.shifting = true;
            st.rubNewtons = shiftNewtons * handScale(rubScatter);
            st.shiftFrom = e.from;
            st.shiftTo = e.to;
            st.shiftStart = now;
            st.shiftLength = e.length;
        }
        else
        {
            if (st.shifting)
                guitar->rub(s, st.shiftTo, 0.0, 0.0);
            st.shifting = false;
            // The fingertip leaves the speaking length for the fret.
            guitar->setDamper(s, 1, 0.0, 0.0);
            guitar->press(s, e.fret, StratEngine::actionMetres(e.fret), pressSeconds,
                          handScale(landingScatter));
            st.fret = e.fret;
            st.place = e.place;
            st.fretted = true;
            st.released = false;
        }
        return;
    }
    double pickAt = clamp(current.pickMetres + pickLine[s] + e.pickOffset, 0.025, 0.250);
    if (damping)
    {
        // A stroke under the damping hand, which stays.
        guitar->setDamper(s, 1, fingerMetres(*guitar, s, e.fret), fingerResistance, fingerWidth);
        guitar->setFret(s, 0.0);
        st.fretted = false;
        st.handResting = true;
    }
    else
    {
        liftRestingHand();
        guitar->setFret(s, e.fret);
        if (e.fret > 0.0)
            startVibrato();
        if (e.pinch)
        {
            const double node = pinchNode(guitar->fretMetres(s, e.fret), pickAt);
            pickAt = node - thumbOffsetMetres;
            guitar->setDamper(s, 1, node, impedance(s));
            Event lift;
            lift.time = now + static_cast<std::int64_t>(touchSeconds * sampleRate);
            lift.kind = EventKind::liftTouch;
            lift.string = s;
            schedule(lift);
        }
        else
            guitar->setDamper(s, 1, e.touchMetres, e.touchResistance, e.touchWidth);
    }
    guitar->setBend(s, st.bend);
    const double force = std::abs(e.force);
    if (fingers)
        pickAt = clamp(pickAt + fingerOffsetMetres, 0.025, 0.250);
    guitar->pluck(s, e.force, pickAt, releaseSeconds(s, force),
                  pushRatio * force / (2.0 * impedance(s)), nextTiltOpposed(), heldShare);
    if (e.slideFrets > 0.0 && !damping)
    {
        st.slide = true;
        st.rubNewtons = slideNewtons * handScale(rubScatter);
        st.slideFrom = e.fret;
        st.slideTo = e.fret + e.slideFrets;
        st.slideStart = now;
        st.slideLength = static_cast<std::int64_t>(
            (slideBaseSeconds + slideSecondsPerFret * e.slideFrets) * sampleRate);
    }
    if (e.bendSemitones > 0.0 && !damping)
    {
        st.bendSemitones = e.bendSemitones;
        st.bendStart = now + static_cast<std::int64_t>(bendDelaySeconds * sampleRate);
        st.bendLength = static_cast<std::int64_t>(bendRiseSeconds * sampleRate);
    }
    if (!st.keyDown && !st.sustained && st.note >= 0)
        release(s);
}

double StratPerformer::nextPlace() noexcept
{
    return steadyStroke ? placeShare
                        : clamp(placeShare * std::exp(placeScatter * normalDraw(fretHand)),
                                nearestPlace, farthestPlace);
}

double StratPerformer::handScale(double logSpread) noexcept
{
    return steadyStroke ? 1.0 : std::exp(logSpread * normalDraw(fretHand));
}

// A newly fretted note: the hand rocks from it afresh, after the pick.
void StratPerformer::startVibrato() noexcept
{
    vibratoPhase = 0.0;
    vibratoHold = static_cast<std::int64_t>(vibratoDelaySeconds * sampleRate);
    vibratoCycleHz = vibratoHz * std::exp(-vibratoPeriodScatter * normalDraw(wobble));
    vibratoCycleDepth = std::exp(vibratoDepthScatter * normalDraw(wobble));
}

void StratPerformer::controlUpdate() noexcept
{
    // Each cycle starts on the note, so a new cycle's rate and depth join
    // the last without a step.
    if (vibratoHold > 0)
        vibratoHold -= StratEngine::controlPeriod;
    else
    {
        vibratoPhase += 2.0 * pi * vibratoCycleHz * StratEngine::controlPeriod / sampleRate;
        if (vibratoPhase > 2.0 * pi)
        {
            vibratoPhase -= 2.0 * pi;
            vibratoCycleHz = vibratoHz * std::exp(-vibratoPeriodScatter * normalDraw(wobble));
            vibratoCycleDepth = std::exp(vibratoDepthScatter * normalDraw(wobble));
        }
    }
    const double rock = vibratoCycleDepth * (0.5 - 0.5 * std::cos(vibratoPhase));
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        StringState& st = strings[static_cast<size_t>(s)];
        if (st.slide)
        {
            // The finger glides; the frets under it stop the string, each in
            // turn as the fingertip, its place behind the one it stops
            // (fingerMetres), crosses the one before. So a slide steps
            // through every fret between, as a fretted glissando does.
            const double place = st.place;
            const auto stopped = [place](double fret) { return std::ceil(fret - place); };
            const double before = stopped(st.fret);
            const double u = st.slideLength > 0
                ? static_cast<double>(now - st.slideStart) / static_cast<double>(st.slideLength)
                : 1.0;
            if (u >= 1.0)
            {
                st.fret = st.slideTo;
                st.slide = false;
            }
            else
                st.fret = st.slideFrom + (st.slideTo - st.slideFrom) * u * u * (3.0 - 2.0 * u);
            if (stopped(st.fret) != before)
                guitar->setFret(s, stopped(st.fret));
            // The pressing finger rubs along the string as it goes.
            const double travel
                = std::abs(guitar->fretMetres(s, st.slideTo) - guitar->fretMetres(s, st.slideFrom));
            const double speed = st.slide && st.slideLength > 0
                ? travel * sampleRate / st.slideLength * 6.0 * u * (1.0 - u)
                : 0.0;
            guitar->rub(s, guitar->fretMetres(s, st.fret), speed, st.rubNewtons);
        }
        if (st.shifting)
        {
            const double u = st.shiftLength > 0
                ? static_cast<double>(now - st.shiftStart) / static_cast<double>(st.shiftLength)
                : 1.0;
            if (u >= 1.0)
            {
                guitar->rub(s, st.shiftTo, 0.0, 0.0);
                st.shifting = false;
            }
            else
            {
                const double travel = st.shiftTo - st.shiftFrom;
                guitar->rub(s, st.shiftFrom + travel * reach(u),
                            std::abs(travel) * sampleRate / st.shiftLength * reachSpeed(u),
                            st.rubNewtons);
            }
        }
        if (st.note < 0 || !st.fretted)
            continue;
        double pushed = 0.0;
        if (st.bendSemitones > 0.0)
            pushed = st.bendSemitones
                * (st.bendLength > 0
                       ? reach(clamp(static_cast<double>(now - st.bendStart)
                                         / static_cast<double>(st.bendLength), 0.0, 1.0))
                       : 1.0);
        const double depth = latched == Articulation::vibrato && vibratoDepth < latchedVibratoSemitones
            ? latchedVibratoSemitones
            : vibratoDepth;
        const double semitones = (st.channel != 0 ? channelBend[static_cast<size_t>(st.channel)] : 0.0)
            + pushed + depth * rock;
        const double ratio = std::pow(2.0, semitones / 12.0);
        if (ratio != st.bend)
        {
            guitar->setBend(s, ratio);
            st.bend = ratio;
        }
    }
}

void StratPerformer::render(float* out, int samples) noexcept
{
    if (pendingCount > 0)
        startStroke();
    int done = 0;
    while (done < samples)
    {
        if (now % StratEngine::controlPeriod == 0)
            controlUpdate();
        if (delayedCount > 0 && delayed[static_cast<size_t>(delayedHead)].time <= now)
        {
            while (delayedCount > 0 && delayed[static_cast<size_t>(delayedHead)].time <= now)
            {
                const Midi m = delayed[static_cast<size_t>(delayedHead)];
                delayedHead = (delayedHead + 1) % maxMidi;
                --delayedCount;
                dispatch(m);
            }
            if (pendingCount > 0)
                startStroke();
        }
        while (eventCount > 0 && events[0].time <= now)
        {
            const Event e = events[0];
            for (int i = 1; i < eventCount; ++i)
                events[static_cast<size_t>(i - 1)] = events[static_cast<size_t>(i)];
            --eventCount;
            runEvent(e);
        }
        std::int64_t chunk = samples - done;
        const std::int64_t toControl = StratEngine::controlPeriod - now % StratEngine::controlPeriod;
        chunk = chunk < toControl ? chunk : toControl;
        if (eventCount > 0 && events[0].time - now < chunk)
            chunk = events[0].time - now;
        if (delayedCount > 0 && delayed[static_cast<size_t>(delayedHead)].time - now < chunk)
            chunk = delayed[static_cast<size_t>(delayedHead)].time - now;
        guitar->process(out + done, static_cast<int>(chunk));
        for (int i = 0; i < chunk; ++i)
            out[done + i] = static_cast<float>(out[done + i] * outputGain);
        done += static_cast<int>(chunk);
        now += chunk;
    }
}

} // namespace electry
