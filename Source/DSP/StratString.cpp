#include "StratString.h"

#include "StratBridge.h"

#include <algorithm>
#include <cmath>

namespace electry
{
namespace
{
constexpr double pi = 3.14159265358979323846;
// Young's modulus of the strings' steel.
constexpr double steelModulus = 207e9;

// Fender's standard 10-46 set (.010 .013 .017 .026w .036w .046w, nickel-
// plated steel, the gauges of D'Addario EXL110). Masses are D'Addario's
// published unit weights (PL010 .00002215, PL013 .00003744, PL017 .00006402,
// NW026 .00012671, NW036 .00023964, NW046 .00038216 lb/in), which give the
// published EXL110 tensions of 17.5, 19.5, 18.4, 16.6, 15.4 and 16.2 lb at
// 25.5 in, and a density of 7807 kg/m^3 for the plain strings.
//
// Stiffness: the plain strings follow B = pi^3 E d^4 / (64 T L^2) with
// E = 207 GPa; the '62 reference measures 1.2e-4 on its open G and 1.6e-5 on
// its open high E against these 1.13e-4 and 1.38e-5. The wound strings bend
// on their cores, measured on the '62's soft and medium layers: the open low
// E 1.5e-4 (a 0.0185 in steel core), the A 7.1e-5 from its D-flat 3 at fret
// 4 (1.12e-4 and 1.14e-4) and the D 6.2e-5 from its E3 at fret 2 (7.7e-5 and
// 8.0e-5); no other string can play those notes with that stiffness. A
// Strat recorded direct (IDMT-SMT-Guitar) gives 9.4e-5 and 5.9e-5 from all
// thirteen frets of its own set.
//
// Decay: the reference's open E2, G3 and E4 (three velocity layers each,
// medians); A, D and B are interpolated between them until phase 2 fits every
// string and fret. The loop filter holds the open string's loss per round
// trip at those two partials and at one high frequency (StratString::
// designLoss), and a fretted note keeps the share of it that goes per round
// trip (woundPerSecondShare).
//
// Wraps: a wound string loses its partials above 1 kHz far faster than its
// core alone would, and faster the higher they are. Read on the open strings
// of three recorded guitars (IDMT-SMT-Guitar's Strat recorded direct, the
// '62 reference and a clean electric library, each partial from 30 ms until
// it nears its noise floor), the low E loses 31, 17 and 30 dB/s at 1 kHz,
// 152, 35 and 78 at 2 kHz and 282, 64 and 85 at 2.8 kHz; the A 18-26, 58-65
// and 95-117; the D 24, 34-91 and 67-74. The plain strings stay at 10 to 20
// up to 2.8 kHz. Our wound strings lost 11 to 24 there. A one-pole lowpass
// on top of the pole-zero, unity at DC, brings each wound string to the
// three guitars' median at 2.8 kHz (wrapLoss is what it adds there, after
// the 14, 17 and 20 dB/s of the pole-zero); below it the lowpass grows as f
// squared, which lands at 18-29, 19-33 and 13-22 dB/s at 1-1.4 kHz, and it
// adds 4, 10 and 9 dB/s at the eighth harmonic, between the '62's and the
// IDMT Strat's. Its share per second is the string's. (A tracker that kept
// only the partials it could follow for 1.5 s had read the wound strings'
// top at 10 to 40 dB/s: the fast ones were gone before it began.)
constexpr StratString::Construction strings[StratString::stringCount] = {
    {82.4068892, 6.82460e-3, 1.50e-4, 25.0, 11.5, 4000.0, 4.0, 71.0, true, 0.35e-3},  // E2 .046w
    {110.000000, 4.27948e-3, 7.1e-5, 24.5, 10.0, 4000.0, 3.0, 88.0, true, 0.23e-3},   // A2 .036w
    {146.832384, 2.26278e-3, 6.2e-5, 24.0, 8.5, 4000.0, 2.14, 50.0, true, 0.15e-3},   // D3 .026w
    {195.997718, 1.14327e-3, 1.128e-4, 23.4, 7.0, 8000.0, 1.07, 0.0, false, 0.0},     // G3 .017
    {246.941651, 6.68602e-4, 4.15e-5, 18.5, 5.5, 8000.0, 1.82, 0.0, false, 0.0},      // B3 .013
    {329.627557, 3.95554e-4, 1.38e-5, 14.2, 4.0, 8000.0, 2.22, 0.0, false, 0.0},      // E4 .010
};
// Wrap wire: a round steel core wound with one round wrap, the gauge being
// the core and two wraps, solved for the published unit weight above: the
// low E's 0.0185 in core (the stiffness above) leaves a 0.35 mm wrap, the A
// 0.23 mm and the D 0.15 mm. A finger sliding at 0.5 m/s rides over them at
// 1.4, 2.2 and 3.3 kHz.

// A finger rubbing along a string (StratString::rub): the share of its
// pressing force that rises and falls over each wrap, reached at speeds well
// over ridgeSpeed, and that of the skin's roughness, low-passed at 2 kHz;
// fitted in StratPerformer.
constexpr double ridgeDepth = 0.04;
constexpr double ridgeSpeed = 0.1;
constexpr float roughness = 0.02f;
constexpr double roughHz = 2000.0;
// A pressing finger meets its fret at this share of the speed a rigid
// quarter-cosine stroke would: fitted in StratPerformer so the ticks sit at
// the recordings' level.
constexpr double landingShare = 0.55;

double clamp(double value, double low, double high) noexcept
{
    return value < low ? low : (value > high ? high : value);
}

} // namespace

// The pickup hears each partial as a pair beating a fraction of a hertz
// apart, as Pate, Le Carrou and Fabre found on a solid body ("Predicting the
// decay time of solid body electric guitar tones", JASA 135(5), 2014): the
// string's two planes of swing reflect at slightly different frequencies, the
// pickup hears mostly the one toward its poles, and an ordinary stroke drives
// both. On a Strat recorded direct (IDMT-SMT-Guitar, neck pickup, all 78
// notes, every string at frets 0 to 12), two decaying components fitted to
// each of the first six partials' envelopes over 2 s (310 fits where a pair
// beats one decay by 0.3 dB) sit 1e-3 to 1.7e-3 of their frequency apart,
// the split rising with the fret as a shortfall of fixed length does: 0.59,
// 0.62, 0.63, 0.62 and 0.53 mm at frets 0-2, 3-5, 6-8, 9-11 and 12. On
// the first two partials, where the model's own pickup comb and flux law add
// no ripple to blur the fit, 78 fits give 0.66 mm (0.43 to 0.97). The '62's
// long notes put the same partials' pairs 9e-4 of their frequency apart.
// The heard plane is the longer one, the saddle and the fret giving more
// toward the poles than along the top (Smith, "Physical Audio Signal
// Processing", on yielding terminations), and the '62's notes slide sharp
// as the other takes over. Their losses are left alike: on the first two
// partials the fitted pairs' weaker partner decays at 0.6 of the stronger's
// rate on the direct Strat (median) and 1.2 on the '62, which no one ratio
// follows.
const double StratString::inPlaneShortening = 0.00066;

// Not all of a string's loss goes at its ends. A round trip loses the
// terminations' share whatever its length, but the wire's own friction and
// the air's act for as long as the trip takes, which a fret or a bend
// shortens. The Strat recorded direct (IDMT-SMT-Guitar, neck pickup) shows
// both: across frets 0 to 12, each partial's decay grows with the fretted
// pitch r = 2^(fret/12) as d = D (a + (1 - a) r), with D free for each
// string and third-octave band (93 bands, 1529 partials 250 Hz to 9 kHz,
// least absolute log error). The wound strings put a = 0.64 per second
// (0.46 to 0.72, 90 % over bands resampled), their wraps rubbing on the
// core along the whole length; the plain strings 0.36 (0.30 to 0.46). Every
// octave from 250 Hz to 11 kHz alone gives 0.35 to 0.62. The open strings
// keep their table; a fretted note loses that share of each trip in
// proportion to the trip's time.
const double StratString::woundPerSecondShare = 0.64;
const double StratString::plainPerSecondShare = 0.36;

// A fretted string loses more than that law gives, from the first fret on:
// the finger behind the fret takes what crosses it, which the nut's slot,
// with only the free length to the tuner beyond it, gives back. Read on the
// same recording against this model (band decays over 0.1 to 0.8 s of each
// note, 250 Hz to 4 kHz in octaves, the model rendered note for note with
// the same pickup), the open strings decay with the recording's (median
// residual -0.9 to +1.9 dB/s by band) and every fretted group too slowly
// (frets 1-3, 4-6, 7-9, 10-12: -0.7 to -4.1 dB/s); taken against each
// string's own open note, the recording's fretted notes lose 1.11, 1.17,
// 1.17 and 1.30 times what the model's do over 500 Hz to 4 kHz (medians over
// strings and octaves by fret group, string by string 0.85 to 1.7). The
// Strat read through its middle coil (dataset2, the fret runs) agrees in
// direction on four of six strings. Each fretted trip's loss takes the
// median factor; the open strings keep theirs.
const double StratString::fretFingerLoss = 1.17;

// The neck is not rigid under the fret. Fleischer and Zwicker measured a
// Stratocaster's ("Dead Spots", Beitraege zur Vibro- und Psychoakustik 1/96,
// Fig. 44): driven at the fret, square to the fretboard, it yields far more
// than the bridge, and around 450 Hz in a chain of two peaks that leaves the
// B string high up and the high E near fret 5 dead (their Fig. 22: the high
// E at fret 6 and the B at 10, 466 and 440 Hz). The Strat recorded direct
// (IDMT-SMT-Guitar, neck pickup) loses its partials at 400 to 500 Hz faster
// than this string's loss gives, on every string (210 partials of its 78
// notes, the first three of each, decay of the stronger of two fitted
// components): one resonance of the neck at every fret, with a smooth
// correction to each string's loss beside it, fits 423 Hz, Q 9.6 and 5.8e-3
// s/kg at its peak. Fitted without a fifth of the notes, it halves the error
// left on the others' partials at 380 to 520 Hz (mean 4.0 to 2.0 dB/s too
// slow, the correction alone against both; median error 4.0 to 3.6 dB/s).
// The '62 reference agrees where it can be read: the 466 Hz partial of its
// B-flat 3 decays at 10 to 13 dB/s, the 394 Hz of its G 3 at 5.5 to 6.6.
// Resampled by note the frequency holds (406 to 453 Hz) but Q and the peak
// trade against each other (Q 10 to 40, 5.4e-3 to 3.4e-2 s/kg); this is the
// broadest of them. Which frets are deadest differs from guitar to guitar
// and no fit of a shape along the neck predicted the notes left out, so the
// resonance is the same under every fret. Only the swing toward the poles,
// the direction the neck was measured in, sees it.
//
// The strings share it (StratNeck): each pushes on the neck at its own fret,
// or at the nut if open, and what one moves it by reaches the others at
// theirs. On the same recording an open string that neither hand touches
// rings along with the played note: over 0.3 to 1.8 s of the 78 notes, 79
// of the open strings' partials that the note's own and the mains' do not
// cover stand within 62 dB of the note's fundamental (the treble-side
// strings of a fretted note almost never do, a fretting finger lying across
// them). With the resonance above under all six strings and nothing else
// changed, the same partials of the model's free strings come out a median
// 11 dB under the recording's (quartiles 18 and 3 dB under), and the notes'
// own balance moves by no more than 0.3 dB early and 0.6 dB late, and their
// partials' decay by 0.9 dB/s, on any string.
//
// The shortfall is low down. Of the 347 lines of the open strings' first
// four partials that stand 10 dB over the noise around them, those at 60 to
// 130, 130 to 200 and 200 to 280 Hz came out 22, 28 and 12 dB under the
// recording's (band medians), those at 280 to 600 Hz within 3 dB. There,
// under the resonance above, the neck sways in a second, broad mode, where
// Fleischer and Zwicker's dead spots at 123 to 278 Hz lie (their Fig. 22).
// Read on the notes' first six partials (401 decays, against this model with
// a rigid neck), the one resonance leaves those under 160 Hz decaying
// 2.8 dB/s too slowly on average; a second, fitted free beside it, lands at
// 98 to 144 Hz, Q 1.3 to 5.7 and 3.0e-3 to 8.2e-3 s/kg (resampled by note,
// 10 to 90 %). With the resonance above held, the decays fit within 1 % as
// well anywhere from 110 Hz at Q 1.5 to 140 Hz at Q 4, so the sympathetic
// lines, which that fit never saw, choose in that valley: at 130 Hz, Q 1.5
// and 3.68e-3 s/kg the three bands stand +3.6, -13.1 and -1.3 dB from the
// recording's (all 347 lines: median -7.7 to -1.1 dB), and those at 460 to
// 600 Hz rise from +1.5 to +7.0 dB. Fitted without a fifth of the notes, the
// two modes take the error left on the others' partials under 160 Hz from
// 2.8 to 2.0 dB/s (median; on every partial, 3.06 to 2.93).
const std::array<StratString::NeckMode, StratNeck::modeCount> StratString::neckModes {{
    {423.0, 9.6, 5.8e-3},
    {130.0, 1.5, 3.68e-3},
}};

const StratString::Construction& StratString::construction(int stringIndex) noexcept
{
    return strings[stringIndex < 0 ? 0
                                   : (stringIndex >= stringCount ? stringCount - 1
                                                                 : stringIndex)];
}

double StratString::allpassPhaseDelay(double omega, double a) noexcept
{
    // (a + z^-1) / (1 + a z^-1)
    const double s = std::sin(omega), c = std::cos(omega);
    const double phase = -std::atan2(s, a + c) + std::atan2(a * s, 1.0 + a * c);
    return -phase / omega;
}

double StratString::lossPhaseDelay(double omega, double zero, double pole) noexcept
{
    const double s = std::sin(omega), c = std::cos(omega);
    return (std::atan2(zero * s, 1.0 + zero * c) - std::atan2(pole * s, 1.0 + pole * c)) / omega;
}

std::complex<double> StratString::wrapResponse(double omega) const noexcept
{
    const double p = wrapPole;
    return (1.0 - p) / (1.0 - p * std::polar(1.0, -omega));
}

StratString::Tap StratString::tapAt(double position) noexcept
{
    // Third-order Lagrange over the four samples at base..base+3, with the
    // read point inside the middle interval.
    Tap tap;
    tap.base = static_cast<int>(std::floor(position)) - 1;
    const double t = position - tap.base;
    tap.weight[0] = static_cast<float>(-(t - 1.0) * (t - 2.0) * (t - 3.0) / 6.0);
    tap.weight[1] = static_cast<float>(t * (t - 2.0) * (t - 3.0) / 2.0);
    tap.weight[2] = static_cast<float>(-t * (t - 1.0) * (t - 3.0) / 2.0);
    tap.weight[3] = static_cast<float>(t * (t - 1.0) * (t - 2.0) / 6.0);
    return tap;
}

StratString::LoopTap StratString::loopTapAt(double position) noexcept
{
    // Seventh-order Lagrange over base..base+7, the read point inside the
    // middle interval: weight k is the product of (t - m) / (k - m), m != k,
    // the products before and after k built up from either end. Written out
    // in float: the read point moves every sample while the string's stretch
    // settles, and the weights are kept in float anyway. Positions are
    // positive, so truncation is the floor.
    LoopTap tap;
    tap.base = static_cast<int>(position) - 3;
    const float t = static_cast<float>(position - tap.base);
    const float b1 = t, b2 = b1 * (t - 1.0f), b3 = b2 * (t - 2.0f), b4 = b3 * (t - 3.0f),
                b5 = b4 * (t - 4.0f), b6 = b5 * (t - 5.0f), b7 = b6 * (t - 6.0f);
    const float a6 = t - 7.0f, a5 = a6 * (t - 6.0f), a4 = a5 * (t - 5.0f), a3 = a4 * (t - 4.0f),
                a2 = a3 * (t - 3.0f), a1 = a2 * (t - 2.0f), a0 = a1 * (t - 1.0f);
    tap.weight[0] = a0 * (-1.0f / 5040.0f);
    tap.weight[1] = b1 * a1 * (1.0f / 720.0f);
    tap.weight[2] = b2 * a2 * (-1.0f / 240.0f);
    tap.weight[3] = b3 * a3 * (1.0f / 144.0f);
    tap.weight[4] = b4 * a4 * (-1.0f / 144.0f);
    tap.weight[5] = b5 * a5 * (1.0f / 240.0f);
    tap.weight[6] = b6 * a6 * (-1.0f / 720.0f);
    tap.weight[7] = b7 * (1.0f / 5040.0f);
    return tap;
}

void StratString::prepare(double rate, int stringIndex, int updateSamples) noexcept
{
    sampleRate = rate;
    rampSamples = updateSamples;
    spec = &construction(stringIndex);
    {
        const double h = StratBridge::saddleHeightMetres();
        plateHold = StratBridge::springStiffness();
        for (int j = 0; j < stringCount; ++j)
            if (j != stringIndex)
                plateHold += h * h * StratBridge::axialStiffness(j);
    }

    designedLossShare = -1.0;

    ownNeck.prepare(sampleRate);

    // 5 Hz, an octave under the lowest note the tremolo can dive to.
    blockCoefficient = std::exp(-2.0 * pi * 5.0 / sampleRate);
    roughPole = static_cast<float>(1.0 - std::exp(-2.0 * pi * roughHz / sampleRate));

    tuned = false;
    designedStiffness = -1.0;
    reset();
    setTuning(0.0, 1.0);
}

void StratString::reset() noexcept
{
    for (Plane* plane : {&poles, &top})
    {
        plane->right.fill(0.0f);
        plane->left.fill(0.0f);
        plane->lossInput = plane->lossState = 0.0f;
        plane->wrapState = 0.0f;
        plane->allpassState.fill(0.0f);
    }
    ownNeck.reset();
    skipped = -1;
    arrivingAtNeck = 0.0f;
    force = forceStart = forceEnd = 0.0;
    releaseLength = releasePosition = 0;
    stickRemaining = 0;
    forceHeld = blockedForce = 0.0;
    injected.fill(0.0f);
    stretch = stretchStep = 0.0;
    stretchCountdown = 0;
    historySum = historySquares = 0.0;
    historyLength = recountCountdown = 0;
    lifted = false;
    fingerActive = false;
    rubSpeed = rubTarget = rubStep = rubNewtons = 0.0f;
    rubRemaining = 0;
    rubPhase = 0.0;
    rubRough = 0.0f;
    pressPosition = pressLength = 0;
}

void StratString::setFingerEnd(double kilogramsPerSecond) noexcept
{
    const double z = characteristicImpedance;
    lifted = kilogramsPerSecond > 0.0;
    fingerEnd = lifted ? static_cast<float>((kilogramsPerSecond - z) / (kilogramsPerSecond + z)) : 0.0f;
}

void StratString::designLoss(double share) noexcept
{
    designedLossShare = share;
    // The open string's loss per round trip at its fundamental, its eighth
    // harmonic and one high frequency, through a pole-zero filter. A one-pole
    // lowpass through the first two (Valimaki et al., "Physical modeling of
    // plucked string instruments with application to real-time sound
    // synthesis", JAES 44(5), 1996) and a straight line in frequency both
    // keep steepening above them; a real string's top flattens. Read over a
    // note's first 1.8 s on the Strat recorded direct (IDMT-SMT-Guitar, neck
    // pickup, frets 0 to 12, each partial taken back to the open string by
    // its per-second share), the plain strings lose 56, 33 and 27 dB/s at
    // 8 kHz on the G, B and high E (27 to 74 partials each), where the line
    // lost 43, 40 and 42; the '62 reference's open high E 20 to 24. The
    // wound strings' pole-zero is held near flat above the eighth harmonic,
    // at 15, 20 and 28 dB/s at 4 kHz, and their wraps' lowpass gives their
    // top (wrapLoss).
    const double period = 1.0 / spec->openHz;
    {
        // |1 - p e^-jw|^2 / (1 - p)^2 = 1 + 4 p sin^2(w/2) / (1 - p)^2 is the
        // lowpass's loss at w; set to the wraps' share of a trip at wrapHz,
        // it is a quadratic in p with one root inside the circle.
        const double trip = spec->wrapLoss * share * period;
        double p = 0.0;
        if (trip > 0.0)
        {
            const double s = std::sin(pi * std::min(wrapHz, 0.45 * sampleRate) / sampleRate);
            const double k = (std::pow(10.0, trip / 10.0) - 1.0) / (4.0 * s * s);
            p = ((2.0 * k + 1.0) - std::sqrt(4.0 * k + 1.0)) / (2.0 * k);
        }
        wrapPole = static_cast<float>(p);
    }
    const double d1 = -60.0 / spec->t60Fundamental;
    const double d8 = -60.0 / spec->t60EighthHarmonic;
    const double dh = -60.0 / spec->t60High;
    const auto perTrip = [&](double decibelsPerSecond) {
        return std::pow(10.0, decibelsPerSecond * share * period / 20.0);
    };
    const double w1 = 2.0 * pi * spec->openHz / sampleRate;
    // |H|^2 (1 + a^2 + 2a cos w) = k^2 (1 + b^2) + 2 k^2 b cos w at the
    // three points is linear in P = 1 + a^2, Q = 2a and S = 2 k^2 b once
    // k^2 (1 + b^2) is set to 1; the roots inside the unit circle follow.
    const double w[3] = {w1, 8.0 * w1, 2.0 * pi * std::min(spec->highHz, 0.45 * sampleRate) / sampleRate};
    const double d[3] = {d1, d8, dh};
    double c[3], g[3];
    for (int i = 0; i < 3; ++i)
    {
        c[i] = std::cos(w[i]);
        g[i] = perTrip(d[i]) * perTrip(d[i]);
    }
    double m[3][4];
    for (int i = 0; i < 3; ++i)
    {
        m[i][0] = g[i];
        m[i][1] = g[i] * c[i];
        m[i][2] = -c[i];
        m[i][3] = 1.0;
    }
    for (int col = 0; col < 3; ++col)
    {
        int best = col;
        for (int row = col + 1; row < 3; ++row)
            if (std::abs(m[row][col]) > std::abs(m[best][col]))
                best = row;
        for (int k = 0; k < 4; ++k)
        {
            const double t = m[col][k];
            m[col][k] = m[best][k];
            m[best][k] = t;
        }
        for (int row = 0; row < 3; ++row)
            if (row != col)
            {
                const double f = m[row][col] / m[col][col];
                for (int k = col; k < 4; ++k)
                    m[row][k] -= f * m[col][k];
            }
    }
    const double p = m[0][3] / m[0][0], q = m[1][3] / m[1][1], s = m[2][3] / m[2][2];
    const auto root = [](double ratio) {
        // x from ratio = 2x / (1 + x^2), |x| < 1. Every string reaches at
        // most 0.9994 over shares 0.2 to 4 (the low E at 192 kHz); a tuning
        // past that is held just inside the circle.
        ratio = clamp(ratio, -0.9999, 0.9999);
        return ratio == 0.0 ? 0.0 : (1.0 - std::sqrt(1.0 - ratio * ratio)) / ratio;
    };
    const double pole = root(q / p);
    const double scale = (1.0 + pole * pole) / p;
    const double zero = root(s);
    const double gain = std::sqrt(scale / (1.0 + zero * zero));
    lossGain = static_cast<float>(gain);
    lossZero = static_cast<float>(zero);
    lossPole = static_cast<float>(pole);
}

void StratString::designDispersion(double stiffnessNow, double f0) noexcept
{
    // Fit identical first-order allpass sections to the stiff string's
    // partials, f_n = n f sqrt(1 + B n^2) normalised to the tuned fundamental,
    // over the first fifteen partials under 4 kHz, where dispersion is heard.
    // The fundamental is tuned exactly by the delay; the coefficient
    // minimises the partials' squared pitch error relative to it.
    designedStiffness = stiffnessNow;
    int upper = static_cast<int>(4000.0 / f0);
    upper = upper > 15 ? 15 : upper;
    const double ideal = f0 / std::sqrt(1.0 + stiffnessNow);
    const double w1 = 2.0 * pi * f0 / sampleRate;
    const double zero = lossZero, pole = lossPole;
    const auto error = [&](double a) {
        const double base = sampleRate / f0 - lossPhaseDelay(w1, zero, pole) + std::arg(wrapResponse(w1)) / w1
            - dispersionSections * allpassPhaseDelay(w1, a);
        double sum = 0.0;
        for (int n = 2; n <= upper; ++n)
        {
            const double fn = n * ideal * std::sqrt(1.0 + stiffnessNow * n * n);
            if (fn > 0.45 * sampleRate)
                break;
            const double wn = 2.0 * pi * fn / sampleRate;
            const double loop = base + lossPhaseDelay(wn, zero, pole) - std::arg(wrapResponse(wn)) / wn
                + dispersionSections * allpassPhaseDelay(wn, a);
            // A loop delay off by d samples puts partial n off by d/loop of
            // its frequency.
            const double relative = loop / (n * sampleRate / fn) - 1.0;
            sum += relative * relative;
        }
        return sum;
    };
    if (upper < 2)
    {
        allpassCoefficient = 0.0f;
        return;
    }
    double low = -0.95, high = 0.0;
    constexpr double golden = 0.6180339887498949;
    double x1 = high - golden * (high - low), x2 = low + golden * (high - low);
    double e1 = error(x1), e2 = error(x2);
    for (int i = 0; i < 40; ++i)
    {
        if (e1 < e2)
        {
            high = x2; x2 = x1; e2 = e1;
            x1 = high - golden * (high - low); e1 = error(x1);
        }
        else
        {
            low = x1; x1 = x2; e1 = e2;
            x2 = low + golden * (high - low); e2 = error(x2);
        }
    }
    allpassCoefficient = static_cast<float>(0.5 * (low + high));
}

double StratString::positionSamples(double metres) const noexcept
{
    return metres * samplesPerMetre;
}

void StratString::setTuning(double newFret, double tensionRatio, bool glide) noexcept
{
    const bool fretMoved = newFret != fret;
    fret = newFret;
    tension = tensionRatio;
    const double openNow = spec->openHz * std::sqrt(tensionRatio);
    fundamental = openNow * std::pow(2.0, fret / 12.0);
    const double waveSpeed = 2.0 * scaleMetres * openNow;
    samplesPerMetre = sampleRate / waveSpeed;
    characteristicImpedance = spec->kilogramsPerMetre * waveSpeed;
    topShortfall = 2.0 * inPlaneShortening * samplesPerMetre;

    // A string swinging with energy E is stretched along its speaking length
    // L, on average by E / T of slope squared, so its tension rises by
    // EA E / (2 L T) and its pitch by EA E / (4 L T^2). EA is the steel's
    // that bends it: B = pi^3 E d^4 / (64 T L^2) gives EA = 2 L sqrt(E T B
    // / pi) at the open string's standard tension, 36 kN for the low E's core
    // and 10 kN for the high E. The pitch rises; the loop shortens.
    //
    // The saddle rides the floating plate, which tilts forward under the
    // extra pull and gives some of the stretch back, as under a bend or a
    // press (StratBridge): with k = EA / L, the springs and the other strings
    // holding the plate with K, the tension rises by k / (1 + h^2 k / K)
    // per metre of stretch, 0.79 of k on the open low E, 0.83 on the G and
    // 0.94 on the high E. Friction at the saddle and the nut keeps the
    // stretch on the speaking length, as the time the glide takes is short.
    {
        const double openTension = spec->kilogramsPerMetre * std::pow(2.0 * scaleMetres * spec->openHz, 2.0);
        const double stretchStiffness
            = 2.0 * scaleMetres * std::sqrt(steelModulus * openTension * spec->stiffness / pi);
        const double tensionNow = spec->kilogramsPerMetre * waveSpeed * waveSpeed;
        const double speaking = scaleMetres * std::pow(2.0, -fret / 12.0);
        const double h = StratBridge::saddleHeightMetres();
        const double own = stretchStiffness / speaking;
        const double held = own / (1.0 + h * h * own / plateHold);
        stretchSamplesPerJoule = held / (4.0 * tensionNow * tensionNow) * sampleRate / fundamental;
    }

    // A round trip of a shorter or tighter string takes less time, so it
    // loses less of the share that goes per second (StratString.cpp).
    const double perSecond = spec->wound ? woundPerSecondShare : plainPerSecondShare;
    const double share = (perSecond * spec->openHz / fundamental + 1.0 - perSecond)
        * (fret > 0.0 ? fretFingerLoss : 1.0);
    if (designedLossShare < 0.0 || std::abs(share - designedLossShare) > 1e-3)
        designLoss(share);

    // B grows as the speaking length squared shrinks and falls with tension.
    const double stiffnessNow = spec->stiffness * std::pow(2.0, fret / 6.0) / tensionRatio;
    if (designedStiffness < 0.0
        || std::abs(stiffnessNow / designedStiffness - 1.0) > 0.02)
        designDispersion(stiffnessNow, fundamental);

    const double w0 = 2.0 * pi * fundamental / sampleRate;
    neckDelay = neckPhaseDelay(w0);
    const double filterDelay = lossPhaseDelay(w0, lossZero, lossPole) - std::arg(wrapResponse(w0)) / w0
        + dispersionSections * allpassPhaseDelay(w0, allpassCoefficient) + neckDelay;
    double total = sampleRate / fundamental - filterDelay;
    total = clamp(total, 8.0, 2.0 * (railSize - 2 * leftOffset - 8));
    const double half = 0.5 * total;
    const int halfWhole = static_cast<int>(half);

    if (!tuned || (fretMoved && !glide))
    {
        // Stop the string at the new fret now. Waves beyond a shorter
        // string are trapped behind the finger; a longer string has nothing
        // on its new length yet, and the right-going wave beyond the old fret
        // was already reflected there.
        if (tuned)
        {
            if (halfWhole < leftLength)
                clearLeft(halfWhole, leftLength + 3);
            else
                clearLeft(leftLength, halfWhole + 3);
            const double oldRead = loopDelay - leftLength;
            const double newRead = total - halfWhole;
            if (newRead > oldRead)
                clearRight(static_cast<int>(oldRead) + 5,
                           static_cast<int>(newRead) + 5);
        }
        leftLength = leftTarget = halfWhole;
        loopDelay = loopDelayTarget = total;
        loopDelayStep = 0.0;
        rampRemaining = 0;
        tuned = true;
        setTaps(tapMetres);
        placeDampers();
        return;
    }

    // A bend keeps the left rail and lets the right rail's read absorb the
    // change, until the split drifts from the string's middle; a glide
    // follows the middle. The left rail then moves one sample at a time.
    const double split = total - leftLength;
    if (glide || std::abs(split - half) > 0.1 * half + 2.0)
        leftTarget = halfWhole;
    loopDelayTarget = total;
    rampRemaining = rampSamples;
    loopDelayStep = (loopDelayTarget - loopDelay) / rampSamples;
    setTaps(tapMetres);
    placeDampers();
}

double StratString::lastUsablePosition() const noexcept
{
    // Both rails hold the string up to here; Lagrange reads need three more.
    const int target = leftTarget < leftLength ? leftTarget : leftLength;
    const double read = loopDelayTarget - target;
    return (read < target ? read : target) - 3.0;
}

void StratString::setTaps(const std::array<double, tapCount>& metresFromBridge) noexcept
{
    tapMetres = metresFromBridge;
    for (int k = 0; k < tapCount; ++k)
        taps[static_cast<size_t>(k)] = tapAt(
            clamp(positionSamples(tapMetres[static_cast<size_t>(k)]), 1.0,
                  lastUsablePosition()));
    pick = tapAt(clamp(positionSamples(pickMetres), 1.0, lastUsablePosition()));
    fingerTap = tapAt(clamp(positionSamples(fingerMetres), 1.0, lastUsablePosition()));
    // An injection m samples old sits m samples on along both rails, where the
    // pick's read weighs it by the weights' correlation at lag m.
    for (int m = 1; m <= 3; ++m)
    {
        double sum = 0.0;
        for (int j = m; j < 4; ++j)
            sum += pick.weight[j] * pick.weight[j - m];
        pickOverlap[static_cast<size_t>(m - 1)] = sum;
    }
}

void StratString::setDamper(int damper, double metresFromBridge, double kilogramsPerSecond,
                            double widthMetres) noexcept
{
    if (damper < 0 || damper >= damperCount)
        return;
    dampers[static_cast<size_t>(damper)].metres = metresFromBridge;
    dampers[static_cast<size_t>(damper)].resistance = kilogramsPerSecond;
    dampers[static_cast<size_t>(damper)].width = widthMetres;
    placeDampers();
}

void StratString::placeDampers() noexcept
{
    const double z = characteristicImpedance;
    for (Damper& d : dampers)
    {
        const double at = clamp(positionSamples(d.metres), 1.0, lastUsablePosition());
        const int last = static_cast<int>(lastUsablePosition());
        d.count = std::clamp(static_cast<int>(std::lround(positionSamples(d.width))), 1, last);
        if (d.count > 1)
        {
            // A contact: its resistance shared among the points it covers.
            d.position = std::clamp(static_cast<int>(std::lround(at - 0.5 * (d.count - 1))), 1,
                                    last - d.count + 1);
            const double r = d.resistance / (z * d.count);
            d.absorption = static_cast<float>(r / (2.0 + r));
            continue;
        }
        d.position = static_cast<int>(std::lround(at));
        d.next = 0.0f;
        // On its sample point the dashpot gets the resistance that loses as
        // much each period at the second partial as R would at its true
        // place. A palm 20 mm from the saddle is two samples along a high E at
        // 48 kHz, and rounding alone moves it 2 mm and changes what a light
        // palm damps by a fifth. Near the saddle the dashpot and the short
        // stretch behind it (a spring, Z cot kd) load the string in parallel,
        // and a wave keeps |(Z - ZL) / (Z + ZL)|^2 of its energy. A dashpot
        // lighter than that stretch is stiff loses as R (kd)^2, so the
        // correction holds at every partial; a heavier one depends far less on
        // where it is and is left as it is, on its sample point: shared between
        // two it pulls less the nearer it sits to halfway, and the full palm
        // on the high E at fret 5 took 345 to 539 dB/s off its fundamental at
        // 44.1 to 192 kHz, where on the point it takes 447 to 486 (the
        // recorded heaviest mutes 405 and 490). Beyond a quarter of the
        // string the stretch is no spring, and a sample is a small share of
        // the distance.
        double r = d.resistance / z;
        const double kd = 2.0 * pi * 2.0 * fundamental / sampleRate;
        const double spring = 1.0 / std::tan(kd * at), rounded = 1.0 / std::tan(kd * d.position);
        const bool nearSaddle = kd * std::max(at, static_cast<double>(d.position)) < 0.5 * pi;
        if (r > 0.0 && nearSaddle && r * r < 1.0 + spring * spring)
        {
            // loss r'^2 + (2 loss - 4) r' + loss (1 + rounded^2) = 0, its
            // lighter root; past the most a dashpot there can absorb, that.
            const double loss = 4.0 * r / ((1.0 + r) * (1.0 + r) + spring * spring);
            const double b = 4.0 - 2.0 * loss;
            const double disc = b * b - 4.0 * loss * loss * (1.0 + rounded * rounded);
            r = disc <= 0.0 ? std::sqrt(1.0 + rounded * rounded)
                            : (b - std::sqrt(disc)) / (2.0 * loss);
        }
        else if (!nearSaddle)
        {
            // Further along it sits between two samples. A harmonic touch
            // has to: rounded to a sample, the finger misses the node by up to
            // half of one, its two sides turn back into the fundamental when
            // it lifts, and how much (5 dB between neighbours, high E fret 9
            // at 48 kHz) follows that rounding.
            d.position = static_cast<int>(at);
            d.next = static_cast<float>(at - d.position);
        }
        // A dashpot of resistance R on a string of impedance Z passes
        // 2Z / (2Z + R) of the local velocity and absorbs the rest.
        d.absorption = r > 0.0 ? static_cast<float>(r / (2.0 + r)) : 0.0f;
    }
}

void StratString::measureStretch() noexcept
{
    stretchCountdown = controlPeriod - 1;
    // Each sample of each rail holds Z v^2 / rate of the string's energy,
    // and one period of what arrived at the bridge is all of it; the
    // arrivals stay behind the bridge on the left rail, untouched since.
    // (Summing the two rails instead misses what is inside the fret end's
    // filters, dips once a period, and the loop, shortened in step, pumps the
    // string louder.) The rails also hold the deflection the pick's held
    // force sets up, which the force's DC blocker lets go over some 0.1 s:
    // standing still, it arrives as a constant, the history's mean, and is no
    // part of the swing. Counted, it would start a hard pick sharper by half
    // again and more.
    const int period = std::min(static_cast<int>(std::lround(sampleRate / fundamental - stretch)),
                                railSize - leftLength - 2 * leftOffset);
    const auto arrival = [this](int age) {
        return static_cast<double>(
            poles.left[static_cast<size_t>((write + leftOffset - age) & railMask)]);
    };
    if (recountCountdown-- == 0)
    {
        // Now and then from scratch, so the sums' rounding cannot build up.
        recountCountdown = 63;
        historySum = historySquares = 0.0;
        historyLength = 0;
    }
    for (; historyLength < period; ++historyLength)
    {
        const double v = arrival(historyLength + 1);
        historySum += v;
        historySquares += v * v;
    }
    for (; historyLength > period; --historyLength)
    {
        const double v = arrival(historyLength);
        historySum -= v;
        historySquares -= v * v;
    }
    const double energy = characteristicImpedance / sampleRate
        * std::max(0.0, historySquares - historySum * historySum / period);
    // A semitone at most, far past any pick: the loop must stay inside its rails.
    const double target = std::min(stretchSamplesPerJoule * energy, 0.06 * sampleRate / fundamental);
    stretchStep = (target - stretch) / controlPeriod;
}

void StratString::clearLeft(int fromPosition, int toPosition) noexcept
{
    for (Plane* plane : {&poles, &top})
        for (int s = fromPosition + 1; s <= toPosition; ++s)
            plane->left[static_cast<size_t>((write + leftOffset + s) & railMask)] = 0.0f;
}

void StratString::clearRight(int fromPosition, int toPosition) noexcept
{
    for (Plane* plane : {&poles, &top})
        for (int s = fromPosition + 1; s <= toPosition; ++s)
            plane->right[static_cast<size_t>((write - s) & railMask)] = 0.0f;
}

void StratString::pluck(double forceNewtons, double metresFromBridge,
                        double releaseSeconds, double strokeMetresPerSecond,
                        bool opposedPlanes, double heldShare) noexcept
{
    // A release or a contact still running completes first.
    force = forceEnd;
    topDrive = opposedPlanes ? -1.0f : 1.0f;
    // The stretch is measured on a grid counted from the latest pluck, so a
    // string played again after silence sounds as a new one does.
    stretchCountdown = recountCountdown = 0;
    stickRemaining = 0;
    pickMetres = metresFromBridge;
    pick = tapAt(clamp(positionSamples(pickMetres), 1.0, lastUsablePosition()));
    // An injection m samples old sits m samples on along both rails, where the
    // pick's read weighs it by the weights' correlation at lag m.
    for (int m = 1; m <= 3; ++m)
    {
        double sum = 0.0;
        for (int j = m; j < 4; ++j)
            sum += pick.weight[j] * pick.weight[j - m];
        pickOverlap[static_cast<size_t>(m - 1)] = sum;
    }
    if (strokeMetresPerSecond > 0.0 && forceNewtons != 0.0)
    {
        // At most 30 ms, should the string run from the pick.
        stickRemaining = static_cast<int>(0.030 * sampleRate) + 1;
        // The held share is a deflected rest, kept as the stroke without a
        // push keeps its whole force: nothing is sent for it now, and the
        // release lets it go with the rest.
        contact = heldShare * forceNewtons;
        stickSpeed = forceNewtons > 0.0 ? strokeMetresPerSecond : -strokeMetresPerSecond;
        stickLimit = forceNewtons;
        stickBase = force - contact;
        stickRelease = releaseSeconds;
        forceStart = forceEnd = force;
        releasePosition = releaseLength = 0;
        return;
    }
    // Held at the force from the start: relative to that deflected rest the
    // release is a step of -F, which a point on a string meets with
    // impedance 2Z, sending -F / 2Z both ways.
    startRelease(force, force - forceNewtons, releaseSeconds);
}

double StratString::restingDeflection(double forceNewtons, double atMetres) const noexcept
{
    // A taut string under a point force stands as two straight lines from
    // its ends, y(x) = F x (L - p) / (T L) on the bridge side of the force
    // at p and F p (L - x) / (T L) beyond it.
    const double speaking = scaleMetres * std::pow(2.0, -fret / 12.0);
    const double near = std::min(pickMetres, atMetres);
    const double far = std::min(std::max(pickMetres, atMetres), speaking);
    const double tensionNow = characteristicImpedance / samplesPerMetre * sampleRate;
    return forceNewtons * near * (speaking - far) / (tensionNow * speaking);
}

void StratString::startRelease(double from, double to, double seconds) noexcept
{
    forceStart = from;
    forceEnd = to;
    const double samples = seconds * sampleRate;
    releaseLength = samples > 1.0 ? static_cast<int>(std::ceil(samples)) : 1;
    releaseRate = samples > 1.0 ? 1.0 / samples : 1.0;
    releasePosition = 0;
}

void StratString::rub(double metresFromBridge, double speed, double newtons) noexcept
{
    if (speed == 0.0 && !isRubbing())
        return;
    if (!fingerActive || rubSpeed == 0.0f)
    {
        fingerMetres = metresFromBridge;
        fingerTap = tapAt(clamp(positionSamples(fingerMetres), 1.0, lastUsablePosition()));
    }
    else if (std::abs(metresFromBridge - fingerMetres) * samplesPerMetre >= 0.5)
    {
        fingerMetres = metresFromBridge;
        fingerTap = tapAt(clamp(positionSamples(fingerMetres), 1.0, lastUsablePosition()));
    }
    rubTarget = static_cast<float>(std::abs(speed));
    rubNewtons = static_cast<float>(newtons);
    rubRemaining = rampSamples;
    rubStep = (rubTarget - rubSpeed) / static_cast<float>(rampSamples);
    fingerActive = true;
}

void StratString::press(double depthMetres, double seconds) noexcept
{
    pressDepth = depthMetres;
    pressPosition = 0;
    pressLength = std::max(1, static_cast<int>(std::ceil(seconds * sampleRate)));
}

float StratString::fingerForce() noexcept
{
    float f = 0.0f;
    if (rubRemaining > 0)
    {
        rubSpeed += rubStep;
        if (--rubRemaining == 0)
            rubSpeed = rubTarget;
    }
    if (rubSpeed > 0.0f)
        f += rubbing();
    else
        rubSpeed = rubTarget = 0.0f;
    fingerActive = rubSpeed > 0.0f;
    return f;
}

float StratString::rubbing() noexcept
{
    // The flesh rides over each wrap in turn: the force toward the poles
    // dips into the groove and rises on the next wrap, a ramp per wrap that
    // ends at each crest (band-limited, so its top does not fold back), and
    // grows with speed as the flesh has less time to settle into the
    // grooves. Every string, plain or wound, adds the skin's own roughness.
    const float speed = rubSpeed;
    float f = 0.0f;
    if (spec->wrapMetres > 0.0)
    {
        const double increment = speed / (spec->wrapMetres * sampleRate);
        rubPhase += increment;
        if (rubPhase >= 1.0)
            rubPhase -= std::floor(rubPhase);
        double saw = 2.0 * rubPhase - 1.0;
        // polyBLEP at the crest.
        const double dt = increment < 0.5 ? increment : 0.5;
        if (dt > 0.0)
        {
            if (rubPhase < dt)
            {
                const double t = rubPhase / dt;
                saw -= t + t - t * t - 1.0;
            }
            else if (rubPhase > 1.0 - dt)
            {
                const double t = (rubPhase - 1.0) / dt;
                saw -= t * t + t + t + 1.0;
            }
        }
        f += static_cast<float>(ridgeDepth * speed / (speed + ridgeSpeed) * saw);
    }
    rubNoise ^= rubNoise << 13;
    rubNoise ^= rubNoise >> 17;
    rubNoise ^= rubNoise << 5;
    const float white = static_cast<float>(static_cast<std::int32_t>(rubNoise)) * (1.0f / 2147483648.0f);
    rubRough += roughPole * (white - rubRough);
    f += roughness * speed / (speed + ridgeSpeed) * rubRough;
    return rubNewtons * f;
}

float StratString::readRight(const Plane& plane, const Tap& tap) const noexcept
{
    const int index = write - tap.base;
    return tap.weight[0] * plane.right[static_cast<size_t>(index & railMask)]
         + tap.weight[1] * plane.right[static_cast<size_t>((index - 1) & railMask)]
         + tap.weight[2] * plane.right[static_cast<size_t>((index - 2) & railMask)]
         + tap.weight[3] * plane.right[static_cast<size_t>((index - 3) & railMask)];
}

float StratString::readRight(const Plane& plane, const LoopTap& tap) const noexcept
{
    const int index = write - tap.base;
    float sum = 0.0f;
    for (int k = 0; k < 8; ++k)
        sum += tap.weight[k] * plane.right[static_cast<size_t>((index - k) & railMask)];
    return sum;
}

float StratString::readLeft(const Plane& plane, const Tap& tap) const noexcept
{
    const int index = write + leftOffset + tap.base;
    return tap.weight[0] * plane.left[static_cast<size_t>(index & railMask)]
         + tap.weight[1] * plane.left[static_cast<size_t>((index + 1) & railMask)]
         + tap.weight[2] * plane.left[static_cast<size_t>((index + 2) & railMask)]
         + tap.weight[3] * plane.left[static_cast<size_t>((index + 3) & railMask)];
}

double StratString::neckPhaseDelay(double omega) const noexcept
{
    // The fret's reflection (1 - ZY) / (1 + ZY), inverted, with Y each mode's
    // analog admittance at the frequency its bilinear transform maps omega to.
    double zyRe = 0.0, zyIm = 0.0;
    for (const NeckMode& mode : neckModes)
    {
        const double w = 2.0 * pi * mode.hz;
        const double c = w / std::tan(w / (2.0 * sampleRate));
        const double bandwidth = w / mode.q;
        const double at = c * std::tan(0.5 * omega);
        // Y = G b j at / (w^2 - at^2 + j b at)
        const double re = w * w - at * at, im = bandwidth * at;
        const double scale = characteristicImpedance * mode.conductance * bandwidth * at / (re * re + im * im);
        zyRe += scale * im;
        zyIm += scale * re;
    }
    const double phase = std::atan2(-zyIm, 1.0 - zyRe) - std::atan2(zyIm, 1.0 + zyRe);
    return -phase / omega;
}

float StratString::arriveAtFret(Plane& plane, double readAt) noexcept
{
    if (readAt != plane.loopTapPosition)
    {
        plane.loopTapPosition = readAt;
        plane.loopTap = loopTapAt(readAt);
    }
    const float atFret = readRight(plane, plane.loopTap);
    plane.lossState = lossGain * (atFret + lossZero * plane.lossInput) - lossPole * plane.lossState;
    plane.lossInput = atFret;
    float y = plane.lossState;
    if (wrapPole != 0.0f)
    {
        plane.wrapState = (1.0f - wrapPole) * y + wrapPole * plane.wrapState;
        y = plane.wrapState;
    }
    const float a = allpassCoefficient;
    for (float& state : plane.allpassState)
    {
        const float w = y - a * state;
        y = a * w + state;
        state = w;
    }
    return y;
}

void StratString::leaveFret(Plane& plane, float reflected) noexcept
{
    plane.left[static_cast<size_t>((write + leftOffset + leftLength) & railMask)] = reflected;
    if (skipped >= 0)
        plane.left[static_cast<size_t>((write + leftOffset + skipped) & railMask)] = reflected;
}

void StratString::inject(Plane& plane, const Tap& at, float v) noexcept
{
    const int r = write - at.base;
    const int l = write + leftOffset + at.base;
    for (int j = 0; j < 4; ++j)
    {
        plane.right[static_cast<size_t>((r - j) & railMask)] += at.weight[j] * v;
        plane.left[static_cast<size_t>((l + j) & railMask)] += at.weight[j] * v;
    }
}

void StratString::damp(Plane& plane) noexcept
{
    for (const Damper& d : dampers)
    {
        if (d.absorption == 0.0f)
            continue;
        if (d.next > 0.0f)
        {
            // What the dashpot reads and what it takes, shared alike, so a
            // mode with its node at the point loses nothing.
            float& r0 = plane.right[static_cast<size_t>((write - d.position) & railMask)];
            float& l0 = plane.left[static_cast<size_t>((write + leftOffset + d.position) & railMask)];
            float& r1 = plane.right[static_cast<size_t>((write - d.position - 1) & railMask)];
            float& l1
                = plane.left[static_cast<size_t>((write + leftOffset + d.position + 1) & railMask)];
            const float near = 1.0f - d.next;
            const float v = d.absorption * (near * (r0 + l0) + d.next * (r1 + l1));
            r0 -= near * v;
            l0 -= near * v;
            r1 -= d.next * v;
            l1 -= d.next * v;
            continue;
        }
        for (int k = d.position; k < d.position + d.count; ++k)
        {
            float& r = plane.right[static_cast<size_t>((write - k) & railMask)];
            float& l = plane.left[static_cast<size_t>((write + leftOffset + k) & railMask)];
            const float v = d.absorption * (r + l);
            r -= v;
            l -= v;
        }
    }
}

void StratNeck::prepare(double sampleRate) noexcept
{
    numeratorSum = 0.0f;
    for (size_t k = 0; k < modes.size(); ++k)
    {
        const StratString::NeckMode& mode = StratString::neckModes[k];
        const double w = 2.0 * pi * mode.hz;
        const double c = w / std::tan(w / (2.0 * sampleRate));
        const double bandwidth = w / mode.q;
        const double a0 = c * c + bandwidth * c + w * w;
        modes[k].numerator = static_cast<float>(mode.conductance * bandwidth * c / a0);
        modes[k].a1 = static_cast<float>((2.0 * w * w - 2.0 * c * c) / a0);
        modes[k].a2 = static_cast<float>((c * c - bandwidth * c + w * w) / a0);
        numeratorSum += modes[k].numerator;
    }
    reset();
}

void StratNeck::reset() noexcept
{
    for (Mode& mode : modes)
        mode.velocity1 = mode.velocity2 = 0.0f;
    force1 = force2 = velocity1 = 0.0f;
}

float StratNeck::move(float push, float load) noexcept
{
    // The strings push on it with push - load v while it moves at v, the sum
    // of its modes'; each mode's direct term makes v answer the push it is
    // part of, so v is solved for here.
    float rest = 0.0f;
    for (const Mode& mode : modes)
        rest -= mode.numerator * force2 + mode.a1 * mode.velocity1 + mode.a2 * mode.velocity2;
    const float v = (numeratorSum * push + rest) / (1.0f + numeratorSum * load);
    const float force = push - load * v;
    for (Mode& mode : modes)
    {
        const float vk = mode.numerator * (force - force2) - mode.a1 * mode.velocity1 - mode.a2 * mode.velocity2;
        mode.velocity2 = mode.velocity1;
        mode.velocity1 = vk;
    }
    force2 = force1;
    force1 = force;
    velocity1 = v;
    return v;
}

void StratString::process(std::array<float, tapCount>& towardPoles,
                          std::array<float, tapCount>& alongTop) noexcept
{
    const float z = static_cast<float>(characteristicImpedance);
    const float y = arrive();
    leave(onFret() ? ownNeck.move(2.0f * z * y, z) : 0.0f, towardPoles, alongTop);
}

float StratString::arrive() noexcept
{
    // The window of arrivals at the bridge moves on a sample: one a sample
    // old is final, as nothing behind the bridge is written.
    {
        const float in = poles.left[static_cast<size_t>((write + leftOffset - 1) & railMask)];
        const float out
            = poles.left[static_cast<size_t>((write + leftOffset - 1 - historyLength) & railMask)];
        historySum += static_cast<double>(in) - out;
        historySquares += static_cast<double>(in) * in - static_cast<double>(out) * out;
    }

    // Bridge: a rigid saddle reflects the arriving wave inverted.
    for (Plane* plane : {&poles, &top})
        plane->right[static_cast<size_t>(write & railMask)]
            = -plane->left[static_cast<size_t>((write + leftOffset) & railMask)];

    // Fret end.
    if (rampRemaining > 0)
    {
        loopDelay += loopDelayStep;
        if (--rampRemaining == 0)
            loopDelay = loopDelayTarget;
    }
    skipped = -1;
    if (leftLength < leftTarget)
        skipped = leftLength++;
    else if (leftLength > leftTarget)
        --leftLength;
    if (stretchCountdown-- == 0)
        measureStretch();
    stretch += stretchStep;
    // Both planes share the string's tension, and so its stretch.
    const double readAt = loopDelay - stretch - leftLength;
    // The fret does not yield along the top; a finger's end damps both
    // planes alike.
    const float topArriving = arriveAtFret(top, readAt - topShortfall + neckDelay);
    leaveFret(top, onFret() ? -topArriving : -fingerEnd * topArriving);
    arrivingAtNeck = arriveAtFret(poles, readAt);
    return arrivingAtNeck;
}

void StratString::leave(float neckVelocity, std::array<float, tapCount>& towardPoles,
                        std::array<float, tapCount>& alongTop) noexcept
{
    float fretMotion = 0.0f;
    if (pressPosition < pressLength)
    {
        // The string's end at the fret goes down with the finger and stops
        // on the fret at landingShare of a quarter-cosine's full speed, the
        // fingertip's flesh having taken the rest: a blend of a stroke that
        // lands at full speed, sin(pi u / 2), and one that lands at rest,
        // sin(pi u), each covering the height h.
        ++pressPosition;
        const double u = static_cast<double>(pressPosition) / pressLength;
        fretMotion = static_cast<float>(
            pressDepth * 0.5 * pi * sampleRate / pressLength
            * (landingShare * std::sin(0.5 * pi * u) + (1.0 - landingShare) * std::sin(pi * u)));
    }
    leaveFret(poles, onFret() ? neckVelocity + fretMotion - arrivingAtNeck
                              : -fingerEnd * arrivingAtNeck);

    if (stickRemaining > 0)
    {
        // The pick pushes its point of the string at the stroke's speed less
        // what the arriving waves already give it. The pick's read spans its
        // own last three injections; take them out.
        const double arriving = readRight(poles, pick) + readLeft(poles, pick)
            - 2.0 * (pickOverlap[0] * injected[0] + pickOverlap[1] * injected[1]
                     + pickOverlap[2] * injected[2]);
        // The hand reaches the string through a spring, its grip and the
        // pick's bend: f' = k (v - arriving - f / 2Z), stepped implicitly.
        // The string takes f less what the DC blocker below holds back, so
        // the spring is told how far the string really moved.
        const double g = contactStiffness / sampleRate;
        const double twoZ = 2.0 * characteristicImpedance;
        const double heldBack = forceHeld - blockCoefficient * blockedForce - stickBase;
        contact = (contact + g * (stickSpeed - arriving + heldBack / twoZ)) / (1.0 + g / twoZ);
        if (contact * stickLimit < 0.0)
            contact = 0.0; // the string is moving ahead of the pick
        force = stickBase + contact;
        if (std::abs(contact) >= std::abs(stickLimit) || --stickRemaining == 0)
        {
            stickRemaining = 0;
            force = stickBase + (std::abs(contact) >= std::abs(stickLimit) ? stickLimit : contact);
            startRelease(force, stickBase, stickRelease);
        }
    }
    else if (releasePosition < releaseLength)
    {
        // The bent tip lets go as a spring does from rest: its force falls
        // as the cosine of a quarter period and reaches nothing at full
        // speed, so the string leaves it at a corner, and that corner is
        // the attack's top octave. A Strat recorded direct (IDMT-SMT-Guitar
        // dataset 1, neck pickup, 78 notes, the first 50 ms, rendered at
        // 88.2 kHz) holds 2 to 5 kHz within 2.0 dB of this law and of a
        // raised cosine over the whole release alike, each with its own
        // fitted time (StratPerformer); the raised cosine, smooth at both
        // ends, then leaves 10 to 16 kHz 7 to 16 dB under the recording,
        // this one 1 to 6. Over 10 to 260 ms the recording's B and high E
        // hold 16 kHz 63 to 65 dB under their strongest band, this law 64
        // to 65, the raised cosine 79; the '62 reference and a clean
        // electric library hold 71 to 79.
        ++releasePosition;
        const double u = clamp(releasePosition * releaseRate, 0.0, 1.0);
        force = forceStart + (forceEnd - forceStart) * (1.0 - std::cos(0.5 * pi * u));
    }
    // One-pole DC blocker on the force.
    blockedForce = force - forceHeld + blockCoefficient * blockedForce;
    forceHeld = force;
    if (std::abs(blockedForce) < 1e-9)
        blockedForce = 0.0;
    const float v = static_cast<float>(blockedForce / (2.0 * characteristicImpedance));
    injected = {v, injected[0], injected[1]};
    // The stroke drives both planes as hard, the top in its own sense;
    // StratEngine weighs what the pickup hears of each.
    if (v != 0.0f)
    {
        inject(poles, pick, v);
        inject(top, pick, topDrive * v);
    }
    if (fingerActive)
    {
        // The finger presses toward the poles; what it stirs along the top
        // is left out.
        const float u = static_cast<float>(fingerForce() / (2.0 * characteristicImpedance));
        if (u != 0.0f)
            inject(poles, fingerTap, u);
    }

    damp(poles);
    damp(top);

    for (int k = 0; k < tapCount; ++k)
    {
        const Tap& tap = taps[static_cast<size_t>(k)];
        towardPoles[static_cast<size_t>(k)] = readRight(poles, tap) + readLeft(poles, tap);
        alongTop[static_cast<size_t>(k)] = readRight(top, tap) + readLeft(top, tap);
    }

    write = (write + 1) & railMask;
}

} // namespace electry
