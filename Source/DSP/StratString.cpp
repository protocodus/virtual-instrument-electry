#include "StratString.h"

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
// trip, so a fretted note loses the same per reflection and decays faster in
// proportion to its pitch, as the reference's fretted notes do.
constexpr StratString::Construction strings[StratString::stringCount] = {
    {82.4068892, 6.82460e-3, 1.50e-4, 25.0, 11.5, true},   // E2 .046w
    {110.000000, 4.27948e-3, 7.1e-5, 24.5, 10.0, true},    // A2 .036w
    {146.832384, 2.26278e-3, 6.2e-5, 24.0, 8.5, true},     // D3 .026w
    {195.997718, 1.14327e-3, 1.128e-4, 23.4, 7.0, false},  // G3 .017
    {246.941651, 6.68602e-4, 4.15e-5, 18.5, 5.5, false},   // B3 .013
    {329.627557, 3.95554e-4, 1.38e-5, 14.2, 4.0, false},   // E4 .010
};

double clamp(double value, double low, double high) noexcept
{
    return value < low ? low : (value > high ? high : value);
}

} // namespace

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

void StratString::prepare(double rate, int stringIndex) noexcept
{
    sampleRate = rate;
    spec = &construction(stringIndex);

    // The open string's loss per round trip at its fundamental and eighth
    // harmonic. A wound string's rises steeply above them, the wraps rubbing
    // on the core, and a one-pole lowpass follows it (Valimaki et al.,
    // "Physical modeling of plucked string instruments with application to
    // real-time sound synthesis", JAES 44(5), 1996): the '62's D string at
    // fret 2 loses 38, 50 and 66 dB/s at 3.2, 3.5 and 4.0 kHz, the one-pole
    // 40, 52 and 61. Solid steel loses a constant share of each cycle, so its
    // decay rate grows in proportion to frequency, and a one-pole loses its
    // top far too fast: at 6 kHz the high E and B lose 25 to 35 dB/s in both
    // the '62 and a Strat recorded direct (IDMT-SMT-Guitar, neck pickup),
    // where the one-pole gives 58 and 70 and the straight line through the two
    // points 31 and 29. Plain strings take a pole-zero filter through the two
    // points and that line at 8 kHz.
    const double period = 1.0 / spec->openHz;
    const double d1 = -60.0 / spec->t60Fundamental;
    const double d8 = -60.0 / spec->t60EighthHarmonic;
    const auto perTrip = [&](double decibelsPerSecond) {
        return std::pow(10.0, decibelsPerSecond * period / 20.0);
    };
    const double g1 = perTrip(d1), g8 = perTrip(d8);
    const double w1 = 2.0 * pi * spec->openHz / sampleRate;
    const double w8 = 8.0 * w1;
    double gain = g1, zero = 0.0, pole = 0.0;
    if (spec->wound)
    {
        const double r = (g8 / g1) * (g8 / g1);
        if (r < 1.0)
        {
            const double beta = (std::cos(w1) - r * std::cos(w8)) / (1.0 - r);
            pole = -beta + std::sqrt(beta * beta - 1.0);
        }
        gain = g1 * std::sqrt(1.0 + 2.0 * pole * std::cos(w1) + pole * pole);
    }
    else
    {
        // |H|^2 (1 + a^2 + 2a cos w) = k^2 (1 + b^2) + 2 k^2 b cos w at the
        // three points is linear in P = 1 + a^2, Q = 2a and S = 2 k^2 b once
        // k^2 (1 + b^2) is set to 1; the roots inside the unit circle follow.
        constexpr double highHz = 8000.0;
        const double dh = d1 + (d8 - d1) * (highHz - spec->openHz) / (7.0 * spec->openHz);
        const double c[3] = {std::cos(w1), std::cos(w8), std::cos(2.0 * pi * highHz / sampleRate)};
        const double g[3] = {g1 * g1, g8 * g8, perTrip(dh) * perTrip(dh)};
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
            // x from ratio = 2x / (1 + x^2), |x| < 1.
            return ratio == 0.0 ? 0.0 : (1.0 - std::sqrt(1.0 - ratio * ratio)) / ratio;
        };
        pole = root(q / p);
        const double scale = (1.0 + pole * pole) / p;
        zero = root(s);
        gain = std::sqrt(scale / (1.0 + zero * zero));
    }
    lossGain = static_cast<float>(gain);
    lossZero = static_cast<float>(zero);
    lossPole = static_cast<float>(pole);

    // 5 Hz, an octave under the lowest note the tremolo can dive to.
    blockCoefficient = std::exp(-2.0 * pi * 5.0 / sampleRate);

    tuned = false;
    designedStiffness = -1.0;
    reset();
    setTuning(0.0, 1.0);
}

void StratString::reset() noexcept
{
    right.fill(0.0f);
    left.fill(0.0f);
    lossInput = lossState = 0.0f;
    allpassState.fill(0.0f);
    force = forceStart = forceEnd = 0.0;
    releaseLength = releasePosition = 0;
    stickRemaining = 0;
    forceHeld = blockedForce = 0.0;
    injected.fill(0.0f);
    stretch = stretchStep = 0.0;
    stretchCountdown = 0;
    historySum = historySquares = 0.0;
    historyLength = recountCountdown = 0;
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
        const double base = sampleRate / f0 - lossPhaseDelay(w1, zero, pole)
            - dispersionSections * allpassPhaseDelay(w1, a);
        double sum = 0.0;
        for (int n = 2; n <= upper; ++n)
        {
            const double fn = n * ideal * std::sqrt(1.0 + stiffnessNow * n * n);
            if (fn > 0.45 * sampleRate)
                break;
            const double wn = 2.0 * pi * fn / sampleRate;
            const double loop = base + lossPhaseDelay(wn, zero, pole)
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

    // A string swinging with energy E is stretched along its speaking length
    // L, on average by E / T of slope squared, so its tension rises by
    // EA E / (2 L T) and its pitch by EA E / (4 L T^2). EA is the steel's
    // that bends it: B = pi^3 E d^4 / (64 T L^2) gives EA = 2 L sqrt(E T B
    // / pi) at the open string's standard tension, 36 kN for the low E's core
    // and 10 kN for the high E. The pitch rises; the loop shortens.
    {
        const double openTension = spec->kilogramsPerMetre * std::pow(2.0 * scaleMetres * spec->openHz, 2.0);
        const double stretchStiffness
            = 2.0 * scaleMetres * std::sqrt(steelModulus * openTension * spec->stiffness / pi);
        const double tensionNow = spec->kilogramsPerMetre * waveSpeed * waveSpeed;
        const double speaking = scaleMetres * std::pow(2.0, -fret / 12.0);
        stretchSamplesPerJoule = stretchStiffness / (4.0 * speaking * tensionNow * tensionNow)
            * sampleRate / fundamental;
    }

    // B grows as the speaking length squared shrinks and falls with tension.
    const double stiffnessNow = spec->stiffness * std::pow(2.0, fret / 6.0) / tensionRatio;
    if (designedStiffness < 0.0
        || std::abs(stiffnessNow / designedStiffness - 1.0) > 0.02)
        designDispersion(stiffnessNow, fundamental);

    const double w0 = 2.0 * pi * fundamental / sampleRate;
    const double filterDelay = lossPhaseDelay(w0, lossZero, lossPole)
        + dispersionSections * allpassPhaseDelay(w0, allpassCoefficient);
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
    rampRemaining = controlPeriod;
    loopDelayStep = (loopDelayTarget - loopDelay) / controlPeriod;
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
        // On its sample point the dashpot gets the resistance that loses as
        // much each period at the second partial as R would at its true
        // place. A palm 20 mm from the saddle is two samples along a high E at
        // 48 kHz, and rounding alone moves it 2 mm and changes what a light
        // palm damps by a fifth. Near the saddle the dashpot and the short
        // stretch behind it (a spring, Z cot kd) load the string in parallel,
        // and a wave keeps |(Z - ZL) / (Z + ZL)|^2 of its energy. A dashpot
        // lighter than that stretch is stiff loses as R (kd)^2, so the
        // correction holds at every partial; a heavier one depends far less on
        // where it is and is left as it is. Beyond a quarter of the string the
        // stretch is no spring, and a sample is a small share of the distance.
        double r = d.resistance / z;
        const double kd = 2.0 * pi * 2.0 * fundamental / sampleRate;
        const double spring = 1.0 / std::tan(kd * at), rounded = 1.0 / std::tan(kd * d.position);
        if (r > 0.0 && kd * std::max(at, static_cast<double>(d.position)) < 0.5 * pi
            && r * r < 1.0 + spring * spring)
        {
            // loss r'^2 + (2 loss - 4) r' + loss (1 + rounded^2) = 0, its
            // lighter root; past the most a dashpot there can absorb, that.
            const double loss = 4.0 * r / ((1.0 + r) * (1.0 + r) + spring * spring);
            const double b = 4.0 - 2.0 * loss;
            const double disc = b * b - 4.0 * loss * loss * (1.0 + rounded * rounded);
            r = disc <= 0.0 ? std::sqrt(1.0 + rounded * rounded)
                            : (b - std::sqrt(disc)) / (2.0 * loss);
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
        return static_cast<double>(left[static_cast<size_t>((write + leftOffset - age) & railMask)]);
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
    for (int s = fromPosition + 1; s <= toPosition; ++s)
        left[static_cast<size_t>((write + leftOffset + s) & railMask)] = 0.0f;
}

void StratString::clearRight(int fromPosition, int toPosition) noexcept
{
    for (int s = fromPosition + 1; s <= toPosition; ++s)
        right[static_cast<size_t>((write - s) & railMask)] = 0.0f;
}

void StratString::pluck(double forceNewtons, double metresFromBridge,
                        double releaseSeconds, double strokeMetresPerSecond) noexcept
{
    // A release or a contact still running completes first.
    force = forceEnd;
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
        contact = 0.0;
        stickSpeed = forceNewtons > 0.0 ? strokeMetresPerSecond : -strokeMetresPerSecond;
        stickLimit = forceNewtons;
        stickBase = force;
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

void StratString::startRelease(double from, double to, double seconds) noexcept
{
    forceStart = from;
    forceEnd = to;
    const double samples = seconds * sampleRate;
    releaseLength = samples > 1.0 ? static_cast<int>(std::ceil(samples)) : 1;
    releaseRate = samples > 1.0 ? 1.0 / samples : 1.0;
    releasePosition = 0;
}

float StratString::readRight(const Tap& tap) const noexcept
{
    const int index = write - tap.base;
    return tap.weight[0] * right[static_cast<size_t>(index & railMask)]
         + tap.weight[1] * right[static_cast<size_t>((index - 1) & railMask)]
         + tap.weight[2] * right[static_cast<size_t>((index - 2) & railMask)]
         + tap.weight[3] * right[static_cast<size_t>((index - 3) & railMask)];
}

float StratString::readRight(const LoopTap& tap) const noexcept
{
    const int index = write - tap.base;
    float sum = 0.0f;
    for (int k = 0; k < 8; ++k)
        sum += tap.weight[k] * right[static_cast<size_t>((index - k) & railMask)];
    return sum;
}

float StratString::readLeft(const Tap& tap) const noexcept
{
    const int index = write + leftOffset + tap.base;
    return tap.weight[0] * left[static_cast<size_t>(index & railMask)]
         + tap.weight[1] * left[static_cast<size_t>((index + 1) & railMask)]
         + tap.weight[2] * left[static_cast<size_t>((index + 2) & railMask)]
         + tap.weight[3] * left[static_cast<size_t>((index + 3) & railMask)];
}

void StratString::process(std::array<float, tapCount>& tapVelocity) noexcept
{
    // The window of arrivals at the bridge moves on a sample: one a sample
    // old is final, as nothing behind the bridge is written.
    {
        const float in = left[static_cast<size_t>((write + leftOffset - 1) & railMask)];
        const float out
            = left[static_cast<size_t>((write + leftOffset - 1 - historyLength) & railMask)];
        historySum += static_cast<double>(in) - out;
        historySquares += static_cast<double>(in) * in - static_cast<double>(out) * out;
    }

    // Bridge: a rigid saddle reflects the arriving wave inverted.
    const float arriving = left[static_cast<size_t>((write + leftOffset) & railMask)];
    right[static_cast<size_t>(write & railMask)] = -arriving;

    // Fret end.
    if (rampRemaining > 0)
    {
        loopDelay += loopDelayStep;
        if (--rampRemaining == 0)
            loopDelay = loopDelayTarget;
    }
    int skipped = -1;
    if (leftLength < leftTarget)
        skipped = leftLength++;
    else if (leftLength > leftTarget)
        --leftLength;
    if (stretchCountdown-- == 0)
        measureStretch();
    stretch += stretchStep;
    const double readAt = loopDelay - stretch - leftLength;
    if (readAt != loopTapPosition)
    {
        loopTapPosition = readAt;
        loopTap = loopTapAt(readAt);
    }
    const float atFret = readRight(loopTap);
    lossState = lossGain * (atFret + lossZero * lossInput) - lossPole * lossState;
    lossInput = atFret;
    float y = lossState;
    const float a = allpassCoefficient;
    for (float& state : allpassState)
    {
        const float w = y - a * state;
        y = a * w + state;
        state = w;
    }
    left[static_cast<size_t>((write + leftOffset + leftLength) & railMask)] = -y;
    if (skipped >= 0)
        left[static_cast<size_t>((write + leftOffset + skipped) & railMask)] = -y;

    if (stickRemaining > 0)
    {
        // The pick pushes its point of the string at the stroke's speed less
        // what the arriving waves already give it. The pick's read spans its
        // own last three injections; take them out.
        const double arriving = readRight(pick) + readLeft(pick)
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
        ++releasePosition;
        const double u = clamp(releasePosition * releaseRate, 0.0, 1.0);
        force = forceStart + (forceEnd - forceStart) * (0.5 - 0.5 * std::cos(pi * u));
    }
    // One-pole DC blocker on the force.
    blockedForce = force - forceHeld + blockCoefficient * blockedForce;
    forceHeld = force;
    if (std::abs(blockedForce) < 1e-9)
        blockedForce = 0.0;
    const float v = static_cast<float>(blockedForce / (2.0 * characteristicImpedance));
    injected = {v, injected[0], injected[1]};
    if (v != 0.0f)
    {
        const int r = write - pick.base;
        const int l = write + leftOffset + pick.base;
        for (int j = 0; j < 4; ++j)
        {
            right[static_cast<size_t>((r - j) & railMask)] += pick.weight[j] * v;
            left[static_cast<size_t>((l + j) & railMask)] += pick.weight[j] * v;
        }
    }

    for (const Damper& d : dampers)
    {
        if (d.absorption == 0.0f)
            continue;
        for (int k = d.position; k < d.position + d.count; ++k)
        {
            float& r = right[static_cast<size_t>((write - k) & railMask)];
            float& l = left[static_cast<size_t>((write + leftOffset + k) & railMask)];
            const float v = d.absorption * (r + l);
            r -= v;
            l -= v;
        }
    }

    for (int k = 0; k < tapCount; ++k)
        tapVelocity[static_cast<size_t>(k)] = readRight(taps[static_cast<size_t>(k)])
            + readLeft(taps[static_cast<size_t>(k)]);

    write = (write + 1) & railMask;
}

} // namespace electry
