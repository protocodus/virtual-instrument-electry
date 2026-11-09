#include "StratEngine.h"

#include "DspCompatibility.h"

#include <algorithm>
#include <cmath>

namespace electry
{
namespace
{
constexpr double pi = 3.14159265358979323846;

// Coil centres from the saddles: neck 162 mm and middle 98 mm, square to the
// strings; the bridge coil slants from 46 mm under the low E to 36 mm under
// the high E, each string's own saddle where the intonation (prepare) has
// set it back.
constexpr double neckMetres = 0.162;
constexpr double middleMetres = 0.098;
constexpr double bridgeBassMetres = 0.046;
constexpr double bridgeTrebleMetres = 0.036;

// Pole-to-string gap, Fender's vintage-style setting: 8/64 in on the bass
// side and 6/64 in on the treble side, set with the string held at the last
// fret, and taken as one gap per string between them; the vintage pole
// stagger follows the 7.25 in fingerboard radius, so it keeps the middle
// strings at about the same gap.
constexpr double gapBassMetres = 8.0 / 64.0 * 0.0254;
constexpr double gapTrebleMetres = 6.0 / 64.0 * 0.0254;

// Sensitivity against the gap: the slope of the flux law Novak et al. fitted
// to a measured Seymour Duncan SSL-5 Strat single coil ("Experimental study
// of guitar pickup nonlinearity", DAFx-18, Eq. 6: equivalent length
// 12.98 mm, radius 2.77 mm). The flux falls off with the distance from the
// string to the poles, so the coil reads the string's velocity times this
// slope at the string's own distance: closer than the gap it reads more,
// farther less, and a big swing comes out uneven, which is the single
// coil's bark. They measured it with the string resting 3 to 10 mm off the
// pole and swinging at 60 to 400 Hz.
double fluxSlope(double gap) noexcept
{
    constexpr double length = 0.01298, radius = 0.00277;
    const auto term = [](double x) {
        const double base = radius * radius + x * x;
        return (x * x + 3.0 * radius * radius) / (3.0 * base * std::cbrt(base));
    };
    return term(gap + length) - term(gap);
}

// The string's induced magnetic moment, and so the flux it carries through
// the coil as it moves, grows with the steel in its cross-section; that is
// its mass per length for these all-steel strings, counting the nickel-
// plated steel wrap of the wound strings as solid (open until phase 2 fits
// it). For the same pick force the low E then gives about 10 dB more than
// the high E, which the gap tilt above reduces by 2.5 dB and the poles'
// balance below by 6.
//
// The absolute coupling depends on the magnet and the turns, which nothing
// here measures, and only the level at the jack depends on it: 1 V per m/s
// under the low E is a level convention, not a pickup specification.
constexpr double lowEVoltsPerMetrePerSecond = 1.0;

// How well each pole reads its string, over what the gap and the steel
// give: a Strat's poles are staggered, and their pull differs from string
// to string. At one stroke on every string the rules above put the plain
// strings 6 to 10 dB under the low E; a Strat recorded direct reads them
// nearer even. Its notes' level below 1.2 kHz over their first 250 ms,
// against this model's at one force, first stood over the low E's by 0 and
// 1.6 dB on the A, 1.8 and 1.7 on the D, 5.5 and 7.2 on the G, 4.4 and 6.9
// on the B and 6.8 and 5.9 on the high E (IDMT-SMT-Guitar: dataset 1, 78
// notes at frets 0-12 on the neck pickup; dataset 2, 126 at frets 0-20,
// recorded apart, then also read through the neck). Dataset 2's open notes
// match the middle coil's comb best (partials 1 to 14, frets 0 to 7), not
// the neck's; read through it, against the engine as it now stands, the
// balance below is the two sessions' mean, each session's string medians
// within 2.2 dB of it. The '62 reference, another guitar, puts its open D,
// G and high E 1.3, 6.2 and 8.8 dB over on the first reckoning. Nothing
// here says which part of the pickup or the stroke makes the difference, so
// it stands with the poles, as one per string.
constexpr double poleBalanceDb[StratString::stringCount] = {0.0, 1.55, 2.0, 5.8, 5.15, 5.2};

double poleBalance(int stringIndex) noexcept
{
    return std::pow(10.0, poleBalanceDb[stringIndex] / 20.0);
}

double gapMetres(int stringIndex) noexcept
{
    const double across = stringIndex / static_cast<double>(StratString::stringCount - 1);
    return gapBassMetres + (gapTrebleMetres - gapBassMetres) * across;
}

double steelCoupling(int stringIndex) noexcept
{
    return lowEVoltsPerMetrePerSecond
        * StratString::construction(stringIndex).kilogramsPerMetre
        / StratString::construction(0).kilogramsPerMetre;
}

// How much of the string's swing crosses the coil. A pick moves the string
// mostly along the body, which a pole under it hardly reads, so this share
// of it moves toward the poles. It is fitted to the flux law's own mark on
// a Strat recorded direct (IDMT-SMT-Guitar dataset 1, neck pickup, at the
// 1.88 N its pitch glide gives the ordinary stroke: StratPerformer).
// The neck coil sits a quarter of the way along an open string and halfway
// along one stopped at the 12th fret, so a linear coil leaves out the open
// strings' 4th partial and the 12th-fret notes' 2nd; the recording has them
// 21 dB under the 2nd partial and 17 dB under the fundamental and the 3rd,
// on all six strings, where a linear coil leaves 42 and 30. 0.5 brings each
// within 1.1 dB on average over its six notes, in the first 40 ms and at
// 200 ms, and +0.1 dB over all four; 0.4 leaves them 1.9 dB short. It was
// 0.268 while the stroke pushed the string from nothing: with the share the
// pick now holds at rest (StratPerformer) that leaves them 5.4 dB short.
// A string swinging along the body over a round pole also gives
// second-order flux, and it rests 1 to 2 mm above the set gap at the low
// frets; the share stands for both. Which way a stroke leans is not
// measured: a downstroke (positive force) leans toward the poles here.
constexpr double acrossShare = 0.5;

// What the coil hears of the string's swing along the top, against its swing
// toward the poles, the stroke driving both as hard (StratString): the pole
// hears the string's drift sideways much less than its swing toward it (Pate
// et al., JASA 135(5), 2014, on a P-90), and the stroke drives that drift
// more. Fitted to the same Strat recorded direct (IDMT-SMT-Guitar, neck
// pickup, all 78 notes rendered at one stroke, which the beating does not
// depend on) on every partial to 6.5 kHz, read two ways. Over each note's
// first 2 s a partial's level strays from a straight decay 0.88 dB on the
// fundamentals, 0.85 on the 2nd partials and 1.39 on the 3rd to 6th, and
// 1.07, 1.43 and 1.20 dB under 1 kHz, at 1-2 and at 2-4 kHz (medians). From
// 0.15 to 1.6 s after the onset (until it falls within 8 dB of the
// recording's noise, the model read over the same span), its envelope,
// detrended as a quadratic and band-passed to 0.7-15 Hz, beats 0.85 dB at
// 0.5-1 kHz, 1.01 at 1-2, 0.84 at 2-4 and 0.84 at 4-6.5 (RMS, medians), at no
// common rate. 0.15 strays 0.77, 0.91, 1.13, 0.98, 1.29 and 1.50 dB and beats
// 0.82, 1.18, 1.32 and 1.14 dB, a mean |ln| error of 0.18 over the ten; 0.13
// and 0.17 score 0.20 and 0.19. 0.2, fitted to the fundamentals alone, scores
// 0.29: its upper partials beat 1.48 to 1.61 dB at 1.3e-3 of their frequency,
// 4 to 6 Hz high up, a shimmer the recording does not have. No one share
// follows both readings, and above 2 kHz the model still beats more than the
// recording. A share falling as one pole above 1 kHz fits closer (0.16) but
// uncovers the poles plane's top octave, which the top plane's otherwise
// masks: it then decays 1.5 dB apart at 44.1 and 192 kHz (Electry.StratEngine).
// Whether a pair starts at its sum or its difference is the stroke's tilt
// (StratPerformer).
constexpr float inPlaneHeard = 0.15f;

// The flux law is tabulated over the string's distance from the poles,
// 0 to 12.8 mm, finely enough that interpolating it errs by under 1e-4.
constexpr double fluxStepMetres = 0.00005;

// The displacement under each coil is the integral of the string's velocity
// there, forgetting its drift at 5 Hz, far below the slackest string's
// fundamental (41 Hz, a full dive): 0.7% and 7 degrees off there.
constexpr double swingLeakHz = 5.0;

// A string's loop reads its fret end between samples, and that read loses
// the top octave on every round trip: at 44.1 kHz the seventh-order Lagrange
// costs the high E 27, 94, 253 and 568 dB/s at 10, 12, 14 and 16 kHz, over
// its own 28 to 30, and less or more from note to note as the read point
// falls. At 88.2 kHz it costs under 7. On a Strat recorded direct
// (IDMT-SMT-Guitar dataset 1, neck pickup, the 39 notes on the plain
// strings) 12 to 16 kHz over 80 to 250 ms stands 68 dB under the notes'
// 150 to 1200 Hz in their first 10 ms; the strings at 44.1 kHz left it 98
// under, at 88.2 kHz 66. The circuit, fed the coils' EMF as straight lines
// between samples (StratCircuit), loses 1.5 to 3.9 dB at 10 to 16 kHz to
// those lines at 44.1 kHz and under 1 at 88.2. So under this rate the
// strings and the circuit run at twice the host's.
constexpr double oversampleBelowHz = 64000.0;

// The halfband lowpass back to the host's rate: a Kaiser-windowed sinc,
// flat within 0.003 dB to 19 kHz at a 44.1 kHz host (0.3 dB at 20 kHz) and
// 68 dB down from 25.1 kHz, where anything it lets through folds back above
// 19 kHz. It delays the jack by 15.5 of the host's samples, 0.35 ms there.
constexpr double halfbandBeta = 6.76;

double besselI0(double x) noexcept
{
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 100 && term > 1e-15 * sum; ++k)
    {
        const double h = x / (2.0 * k);
        term *= h * h;
        sum += term;
    }
    return sum;
}

} // namespace

double StratEngine::coilMetres(int coil, int stringIndex) noexcept
{
    if (coil == 0)
        return neckMetres;
    if (coil == 1)
        return middleMetres;
    const double across = stringIndex / static_cast<double>(stringCount - 1);
    return bridgeBassMetres + (bridgeTrebleMetres - bridgeBassMetres) * across;
}

double StratEngine::coilCoupling(int stringIndex) noexcept
{
    return steelCoupling(stringIndex) * poleBalance(stringIndex)
        * fluxSlope(gapMetres(stringIndex)) / fluxSlope(gapBassMetres);
}

// The guitar is set up so its fretted notes play in tune with its open
// strings. The fretting finger sharpens every fretted note by its stretch
// (fretPressMetres, the action's stretch on top): the low E 15 to 21 cents,
// the G 12 to 17, the A 8 to 11, the B 7 to 10, the D and high E 4 to 6 over
// frets 1 to 19, with the open strings within a cent of their pitch. Setting
// the saddle back lengthens every fretted string by the same distance,
// flattening the high frets most; moving the nut toward the bridge (a
// compensated nut) shortens only the open string, which is then tuned back up
// to pitch. The two together put frets 1 to 21 in tune where the saddle
// alone, set at the 12th fret as Fender's setup does it, leaves the low frets
// as sharp as before (Varieschi and Gower, "Intonation and compensation of
// fretted string instruments", Am. J. Phys. 78(1), 2010). Both are fitted
// here to this finger's stretch, least squares over the 21 frets: a fretted
// note's pitch over the open string's is openMetres / (L 2^(-n/12) + setback)
// times the root of its tension ratio, which is linear in openMetres and
// setback once the tempered ratio is asked of it. The saddles go back 0.3 to
// 1.3 mm and the nut comes forward 1.4 to 5.4 mm, the low E's most, more than
// a shop's compensated nut because this finger presses harder than a light
// player's; every fret then plays within 0.56 cents of its pitch, and so does
// every note of a chord, as the stabilizer keeps one finger's pull off the
// other strings (StratBridge; StratEngineTests' testIntonation). The strings
// take the setup's lengths: the coils, the pick and the fingers are placed
// from each string's own saddle (StratString::fretMetres). The owner asked
// for the guitar in tune (Docs/decisions.md); the recorded Strat the stretch
// comes from was not set up this way.
std::pair<double, double> StratEngine::intonation(int stringIndex) noexcept
{
    constexpr int frets = 21;
    const double length = StratString::scaleMetres;
    double n = 0.0, sw = 0.0, sww = 0.0, sy = 0.0, swy = 0.0;
    for (int fret = 1; fret <= frets; ++fret)
    {
        StratBridge alone;
        alone.setPress(stringIndex, fretPressMetres + actionStretchMetres(fret));
        const double sharp = std::sqrt(alone.tensionRatio(stringIndex));
        // openMetres - setback * w = y puts this fret on its tempered pitch.
        const double w = std::pow(2.0, fret / 12.0) / sharp;
        const double y = length / sharp;
        n += 1.0;
        sw += w;
        sww += w * w;
        sy += y;
        swy += w * y;
    }
    const double det = n * sww - sw * sw;
    const double openMetres = (sy * sww - sw * swy) / det;
    const double setback = (sw * sy - n * swy) / det;
    return {openMetres, setback};
}

// The player tunes each open string with a tuner at an ordinary stroke and
// reads it once the attack has passed. A string swinging that hard is
// stretched and reads sharp of its pitch at rest (StratString::setTuning),
// and the bridge and the neck it couples to pull its loop a little further,
// so each string sits that much flat at rest: every open string at the
// ordinary stroke reads within 0.3 cents of its pitch over 0.4 to 0.8 s
// (StratPerformerTests' testTunedAtTheOrdinaryStroke, which measured these).
double StratEngine::restPitch(int stringIndex) noexcept
{
    constexpr double flatCents[stringCount] = {1.86, 1.10, 1.14, 2.26, 1.37, 0.63};
    return std::pow(2.0, -flatCents[stringIndex < 0 ? 0 : stringIndex % stringCount] / 1200.0);
}

void StratEngine::prepare(double sampleRate) noexcept
{
    oversampling = sampleRate < oversampleBelowHz ? 2 : 1;
    // The strings, the coils over them and the circuit run at this rate.
    stringRate = sampleRate * oversampling;
    for (int s = 0; s < stringCount; ++s)
    {
        StratString& string = strings[static_cast<size_t>(s)];
        const auto [openMetres, setback] = intonation(s);
        string.setIntonation(openMetres, setback);
        string.setRestPitch(restPitch(s));
        string.prepare(stringRate, s, controlPeriod * oversampling);
        string.setTaps({coilMetres(0, s), coilMetres(1, s), coilMetres(2, s)});
        steel[static_cast<size_t>(s)] = static_cast<float>(steelCoupling(s) * poleBalance(s));
        gapSteps[static_cast<size_t>(s)] = static_cast<float>(gapMetres(s) / fluxStepMetres);
        // Two loops of the open string slackened to a quarter of its
        // tension, the longest the string can be.
        quietLimit[static_cast<size_t>(s)] = static_cast<int>(
            4.0 * stringRate / StratString::construction(s).openHz);
        pending[static_cast<size_t>(s)] = Pending {};
    }
    for (int i = 0; i < fluxTableSize; ++i)
        fluxTable[static_cast<size_t>(i)] = static_cast<float>(
            fluxSlope(i * fluxStepMetres) / fluxSlope(gapBassMetres));
    swingLeak = static_cast<float>(1.0 - 2.0 * pi * swingLeakHz / stringRate);
    swingSteps = static_cast<float>(acrossShare / (stringRate * fluxStepMetres));
    tremolo = StratBridge {};
    bridgeMoved = false;
    circuit.prepare(stringRate);
    neck.prepare(stringRate);
    {
        const double centre = 2.0 * halfbandPairs - 1.0;
        double sum = 0.0;
        for (int j = 0; j < halfbandPairs; ++j)
        {
            const double n = 2.0 * j + 1.0;
            const double r = n / centre;
            const double tap = ((j & 1) != 0 ? -1.0 : 1.0) / (pi * n)
                * besselI0(halfbandBeta * std::sqrt(1.0 - r * r)) / besselI0(halfbandBeta);
            halfband[static_cast<size_t>(j)] = static_cast<float>(tap);
            sum += tap;
        }
        // The pairs carry the other half of the gain at DC.
        for (float& tap : halfband)
            tap = static_cast<float>(tap * 0.25 / sum);
    }
    reset();
}

void StratEngine::reset() noexcept
{
    for (int s = 0; s < stringCount; ++s)
    {
        strings[static_cast<size_t>(s)].reset();
        quietSamples[static_cast<size_t>(s)] = quietLimit[static_cast<size_t>(s)];
        swing[static_cast<size_t>(s)] = {};
        swingTop[static_cast<size_t>(s)] = {};
        // reset clears motion, not the held fret and bend. Keep the pickup
        // geometry consistent with the bridge's retained tension.
        updateBendGeometry(s);
    }
    neck.reset();
    circuit.reset();
    jackHistory.fill(0.0f);
    jackWrite = 0;
    phase = 0;
}

void StratEngine::setControls(const StratCircuit::Controls& controls) noexcept
{
    circuit.setControls(controls);
}

// Fretting stretches the string: the finger pushes it past the fret's crown
// toward the board, and its tension rises by its axial stiffness times the
// stretch (StratBridge). One stretch for every string and fret fits the
// intonation of both IDMT-SMT-Guitar Strat sessions (dataset 1, frets 1-12;
// dataset 2's fret runs, frets 1-20; each fretted note's late pitch against
// its own open string's, 192 notes): 30.5 um over frets 1-12 takes the median
// error from 6.4 to 2.8 cents, and the strings' own stiffness over tension
// gives the pattern: the low E 13 cents sharp, the G 11, the A 8, the B 7,
// the D and high E 4, against medians of 16, 10, 6, 3, 6 and 1. The sessions
// alone fit 21 and 32 um. It is the finger about 0.8 mm past the crown 10 mm
// behind the fret. The fit took the plate floating, giving back some of each
// stretch; held by the stabilizer, the same finger sharpens its string 5 %
// (the high E) to 22 % (the low E) more, which the intonation takes up.
//
// On top of it the string comes down from its straight line onto the crown
// (actionStretchMetres): 3.6 um at the 1st fret, 8.5 at the 12th, 16.4 at
// the 20th under Fender's vintage action. Its mean over frets 1-12, 5.2 um,
// comes off the finger's share, so the fit above holds there. Above the
// 12th fret dataset 2's fret runs read 4.8 cents sharper than the finger's
// share alone gave (median against each string's open note, frets 13-20).
const double StratEngine::fretPressMetres = 25.3e-6;

// Fender's vintage setup leaves the strings 0.5 mm over the first fret and
// 5/64 in over the 17th; between them, and on to the nut and the saddle,
// the height grows along the string.
double StratEngine::actionMetres(double fret) noexcept
{
    constexpr double atFirst = 0.0005, atSeventeenth = 5.0 / 64.0 * 0.0254;
    const auto fromNut = [](double f) {
        return StratString::scaleMetres * (1.0 - std::pow(2.0, -f / 12.0));
    };
    return atFirst + (atSeventeenth - atFirst) * (fromNut(fret) - fromNut(1.0))
        / (fromNut(17.0) - fromNut(1.0));
}

double StratEngine::actionStretchMetres(double fret) noexcept
{
    if (fret <= 0.0)
        return 0.0;
    const double length = StratString::scaleMetres;
    const double nut = actionMetres(0.0), saddle = actionMetres(1e9);
    const double toFret = length * (1.0 - std::pow(2.0, -fret / 12.0));
    return std::hypot(toFret, nut) + std::hypot(length - toFret, saddle)
        - std::hypot(length, saddle - nut);
}

void StratEngine::setFret(int stringIndex, double fret, bool glide) noexcept
{
    if (stringIndex < 0 || stringIndex >= stringCount)
        return;
    Pending& p = pending[static_cast<size_t>(stringIndex)];
    strings[static_cast<size_t>(stringIndex)].setFingerEnd(0.0);
    p.fret = fret;
    const double press = fret > 0.0 ? fretPressMetres + actionStretchMetres(fret) : 0.0;
    if (press != tremolo.pressMetres(stringIndex))
    {
        tremolo.setPress(stringIndex, press);
        bridgeMoved = true;
    }
    if (glide)
    {
        p.glide = true;
        p.changed = true;
        return;
    }
    strings[static_cast<size_t>(stringIndex)].setTuning(
        fret, tremolo.tensionRatio(stringIndex));
    updateBendGeometry(stringIndex);
}

void StratEngine::rub(int stringIndex, double metresFromBridge, double speed,
                      double newtons) noexcept
{
    if (stringIndex < 0 || stringIndex >= stringCount)
        return;
    StratString& string = strings[static_cast<size_t>(stringIndex)];
    if (speed == 0.0 && !string.isRubbing())
        return;
    string.rub(metresFromBridge, speed, newtons);
    quietSamples[static_cast<size_t>(stringIndex)] = 0;
}

void StratEngine::press(int stringIndex, double fret, double depthMetres, double seconds,
                        double landingScale) noexcept
{
    if (stringIndex < 0 || stringIndex >= stringCount)
        return;
    setFret(stringIndex, fret);
    strings[static_cast<size_t>(stringIndex)].press(depthMetres, seconds, landingScale);
    quietSamples[static_cast<size_t>(stringIndex)] = 0;
}

void StratEngine::liftFromFret(int stringIndex, double kilogramsPerSecond) noexcept
{
    if (stringIndex >= 0 && stringIndex < stringCount)
        strings[static_cast<size_t>(stringIndex)].setFingerEnd(kilogramsPerSecond);
}

void StratEngine::setArm(double position) noexcept
{
    tremolo.setArm(position);
    bridgeMoved = true;
}

void StratEngine::setBend(int stringIndex, double pitchRatio) noexcept
{
    tremolo.setBend(stringIndex, pitchRatio);
    bridgeMoved = true;
    if (stringIndex >= 0 && stringIndex < stringCount)
        updateBendGeometry(stringIndex);
}

void StratEngine::updateBendGeometry(int stringIndex) noexcept
{
    // A finger bend pulls the string sideways across the fretboard and over
    // the cylindrical pole pieces: a whole-tone bend (pitchRatio ~ 1.122)
    // displaces the string laterally by ~1-1.5 mm over the coils.
    const double r = tremolo.bendRatio(stringIndex);
    const double lateralMetres = 0.0022 * std::sqrt(std::min(r * r - 1.0, 1.0));
    const double speaking = fretMetres(stringIndex, pending[static_cast<size_t>(stringIndex)].fret);
    for (int c = 0; c < coilCount; ++c)
    {
        const double share = speaking > 0.0 ? std::min(coilMetres(c, stringIndex) / speaking, 1.0) : 0.0;
        bendSteps[static_cast<size_t>(stringIndex)][static_cast<size_t>(c)]
            = static_cast<float>(lateralMetres * share / fluxStepMetres);
    }
}

void StratEngine::setDamper(int stringIndex, int damper, double metresFromBridge,
                            double kilogramsPerSecond, double widthMetres) noexcept
{
    if (stringIndex < 0 || stringIndex >= stringCount)
        return;
    strings[static_cast<size_t>(stringIndex)].setDamper(damper, metresFromBridge,
                                                        kilogramsPerSecond, widthMetres);
}

void StratEngine::pluck(int stringIndex, double forceNewtons, double metresFromBridge,
                        double releaseSeconds, double strokeMetresPerSecond,
                        bool opposedPlanes, double heldShare) noexcept
{
    if (stringIndex < 0 || stringIndex >= stringCount)
        return;
    StratString& string = strings[static_cast<size_t>(stringIndex)];
    string.pluck(forceNewtons, metresFromBridge, releaseSeconds, strokeMetresPerSecond,
                 opposedPlanes, heldShare);
    // The rails leave out the deflected rest the pluck starts from, so the
    // coils, which read the string's distance from their poles, take it
    // here: the string under them already stands that far aside.
    const double held = strokeMetresPerSecond > 0.0 ? heldShare * forceNewtons : forceNewtons;
    const double topSign = opposedPlanes ? -1.0 : 1.0;
    for (int c = 0; c < coilCount; ++c)
    {
        const double rest = string.restingDeflection(held, coilMetres(c, stringIndex)) * stringRate;
        swing[static_cast<size_t>(stringIndex)][static_cast<size_t>(c)] += static_cast<float>(rest);
        swingTop[static_cast<size_t>(stringIndex)][static_cast<size_t>(c)]
            += static_cast<float>(topSign * rest);
    }
    quietSamples[static_cast<size_t>(stringIndex)] = 0;
}

double StratEngine::fretMetres(int stringIndex, double fret) const noexcept
{
    return strings[static_cast<size_t>(stringIndex < 0 ? 0 : stringIndex % stringCount)]
        .fretMetres(fret);
}

double StratEngine::fundamentalHz(int stringIndex) const noexcept
{
    return strings[static_cast<size_t>(stringIndex < 0 ? 0 : stringIndex % stringCount)]
        .fundamentalHz();
}

bool StratEngine::isSounding(int stringIndex) const noexcept
{
    return stringIndex >= 0 && stringIndex < stringCount
        && quietSamples[static_cast<size_t>(stringIndex)]
            < quietLimit[static_cast<size_t>(stringIndex)];
}

bool StratEngine::isDriven(int stringIndex) const noexcept
{
    return stringIndex >= 0 && stringIndex < stringCount
        && strings[static_cast<size_t>(stringIndex)].isDriven();
}

bool StratEngine::planesOpposed(int stringIndex) const noexcept
{
    return stringIndex >= 0 && stringIndex < stringCount
        && strings[static_cast<size_t>(stringIndex)].planesOpposed();
}

void StratEngine::applyPending() noexcept
{
    for (int s = 0; s < stringCount; ++s)
    {
        Pending& p = pending[static_cast<size_t>(s)];
        if (!p.changed && !bridgeMoved)
            continue;
        strings[static_cast<size_t>(s)].setTuning(p.fret, tremolo.tensionRatio(s),
                                                  p.glide);
        updateBendGeometry(s);
        p.changed = false;
        p.glide = false;
    }
    bridgeMoved = false;
}

void StratEngine::stepStrings(float (&emf)[coilCount]) noexcept
{
    // Below this on every tap for two loops a string is skipped until it is
    // played again or the neck moves it: 200 dB under a pluck, far above the
    // float subnormals its decay would otherwise reach.
    constexpr float quietVelocity = 1e-10f;
    // While the neck moves every string runs, as each hears it at its fret.
    const bool neckMoving = scalarAbs(neck.velocity()) >= quietVelocity;
    std::array<bool, stringCount> running {};
    float push = 0.0f, load = 0.0f;
    bool any = false;
    for (int s = 0; s < stringCount; ++s)
    {
        const size_t k = static_cast<size_t>(s);
        running[k] = neckMoving || quietSamples[k] < quietLimit[k];
        if (!running[k])
            continue;
        any = true;
        const float z = static_cast<float>(strings[k].impedance());
        const float arriving = strings[k].arrive();
        if (strings[k].onFret())
        {
            push += 2.0f * z * arriving;
            load += z;
        }
    }
    if (!any)
    {
        // Every string is at rest and the neck has fallen under the line;
        // let it rest too rather than ring down into subnormals.
        neck.reset();
        return;
    }
    const float neckVelocity = neck.move(push, load);

    std::array<float, StratString::tapCount> taps {};
    std::array<float, StratString::tapCount> alongTop {};
    for (int s = 0; s < stringCount; ++s)
    {
        const size_t k = static_cast<size_t>(s);
        if (!running[k])
            continue;
        strings[k].leave(neckVelocity, taps, alongTop);
        bool quiet = true;
        for (int c = 0; c < coilCount; ++c)
        {
            const float v = taps[static_cast<size_t>(c)];
            const float u = alongTop[static_cast<size_t>(c)];
            quiet = quiet && scalarAbs(v) < quietVelocity && scalarAbs(u) < quietVelocity;
            float& y = swing[k][static_cast<size_t>(c)];
            float& x = swingTop[k][static_cast<size_t>(c)];
            y = y * swingLeak + v;
            x = x * swingLeak + u;
            // One scalar flux potential along the fixed, measured sensitivity
            // axis: d = gap - acrossShare (y + inPlaneHeard x). Above the
            // existing displacement observer's leak corner, its derivative
            // follows v + inPlaneHeard u, the velocity used below; the leak
            // retains the low-frequency stabilization, not exact DC flux.
            // The pickup belongs to the guitar, not the latest
            // stroke: subtracting +/-y from x according to that stroke's tilt
            // changed its field instantly even on a zero-force repick.
            //
            // The bend offset remains the existing quasistatic gap estimate.
            // Dynamic transverse curvature would require a measured 2-D flux
            // surface and its full gradient. The former orbit-only correction
            // was neither; it changed distance without the corresponding EMF.
            constexpr float invPoleSteps = 1.0f / 80.0f;
            const float dy = swingSteps * y;
            const float dx = swingSteps * x;
            const float bend = bendSteps[k][static_cast<size_t>(c)];
            // A string driven onto the poles reads as touching them,
            // and one far off as at the table's end.
            float d = gapSteps[k] - dy - inPlaneHeard * dx
                + 0.5f * bend * bend * invPoleSteps;
            d = d < 0.0f ? 0.0f : (d > fluxTableSize - 1.001f ? fluxTableSize - 1.001f : d);
            const int i = static_cast<int>(d);
            const float t = d - static_cast<float>(i);
            const float slope = fluxTable[static_cast<size_t>(i)]
                + t * (fluxTable[static_cast<size_t>(i + 1)] - fluxTable[static_cast<size_t>(i)]);
            emf[c] += steel[k] * slope * (v + inPlaneHeard * u);
        }
        // Asked last: a sounding string is rarely quiet at every coil, and
        // isDriven reads several of the string's fields.
        quiet = quiet && !strings[k].isDriven();
        const bool wasSounding = quietSamples[k] < quietLimit[k];
        quietSamples[k] = quiet ? std::min(quietSamples[k] + 1, quietLimit[k]) : 0;
        if (wasSounding && quietSamples[k] == quietLimit[k])
        {
            // No physical motion remains. Do not freeze the leaky displacement
            // observer's tiny integration residual while skipping a string:
            // otherwise the next stroke starts at a different pickup gap.
            swing[k] = {};
            swingTop[k] = {};
        }
    }
}

void StratEngine::process(float* jackVolts, int samples) noexcept
{
    constexpr int mask = halfbandHistory - 1;
    constexpr int centre = 2 * halfbandPairs - 1;
    for (int i = 0; i < samples; ++i)
    {
        if (phase == 0)
            applyPending();
        phase = (phase + 1) % controlPeriod;

        if (oversampling == 1)
        {
            float emf[coilCount] {0.0f, 0.0f, 0.0f};
            stepStrings(emf);
            jackVolts[i] = static_cast<float>(circuit.process(emf[0], emf[1], emf[2]));
            continue;
        }
        for (int k = 0; k < 2; ++k)
        {
            float emf[coilCount] {0.0f, 0.0f, 0.0f};
            stepStrings(emf);
            jackWrite = (jackWrite + 1) & mask;
            jackHistory[static_cast<size_t>(jackWrite)]
                = static_cast<float>(circuit.process(emf[0], emf[1], emf[2]));
        }
        const int middle = jackWrite - centre;
        float y = 0.5f * jackHistory[static_cast<size_t>(middle & mask)];
        for (int j = 0; j < halfbandPairs; ++j)
            y += halfband[static_cast<size_t>(j)]
                * (jackHistory[static_cast<size_t>((middle + 2 * j + 1) & mask)]
                   + jackHistory[static_cast<size_t>((middle - 2 * j - 1) & mask)]);
        jackVolts[i] = y;
    }
}

} // namespace electry
