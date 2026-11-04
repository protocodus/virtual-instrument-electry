#include "StratString.h"

#include <cmath>

namespace electry
{
namespace
{
constexpr double pi = 3.14159265358979323846;

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
// on their cores: the open low E is the reference's measured 1.5e-4 (a
// 0.0185 in steel core), and A and D take the same core-to-gauge ratio, an
// estimate until phase 2 fits them.
//
// Decay: the reference's open E2, G3 and E4 (three velocity layers each,
// medians); A, D and B are interpolated between them until phase 2 fits every
// string and fret. The loop filter holds the open string's loss per round
// trip, so a fretted note loses the same per reflection and decays faster in
// proportion to its pitch, as the reference's fretted notes do.
constexpr StratString::Construction strings[StratString::stringCount] = {
    {82.4068892, 6.82460e-3, 1.50e-4, 25.0, 11.5},  // E2 .046w
    {110.000000, 4.27948e-3, 5.04e-5, 24.5, 10.0},  // A2 .036w
    {146.832384, 2.26278e-3, 1.45e-5, 24.0, 8.5},   // D3 .026w
    {195.997718, 1.14327e-3, 1.128e-4, 23.4, 7.0},  // G3 .017
    {246.941651, 6.68602e-4, 4.15e-5, 18.5, 5.5},   // B3 .013
    {329.627557, 3.95554e-4, 1.38e-5, 14.2, 4.0},   // E4 .010
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

double StratString::lossPhaseDelay(double omega, double pole) noexcept
{
    // g (1 + a) / (1 + a z^-1)
    const double s = std::sin(omega), c = std::cos(omega);
    return -std::atan2(pole * s, 1.0 + pole * c) / omega;
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

void StratString::prepare(double rate, int stringIndex) noexcept
{
    sampleRate = rate;
    spec = &construction(stringIndex);

    // The open string's loss per round trip at its fundamental and eighth
    // harmonic, met by a one-pole lowpass (Valimaki et al., "Physical
    // modeling of plucked string instruments with application to real-time
    // sound synthesis", JAES 44(5), 1996).
    const double period = 1.0 / spec->openHz;
    const double g1 = std::pow(10.0, -3.0 * period / spec->t60Fundamental);
    const double g8 = std::pow(10.0, -3.0 * period / spec->t60EighthHarmonic);
    const double w1 = 2.0 * pi * spec->openHz / sampleRate;
    const double w8 = 8.0 * w1;
    const double r = (g8 / g1) * (g8 / g1);
    double pole = 0.0;
    if (r < 1.0)
    {
        const double beta = (std::cos(w1) - r * std::cos(w8)) / (1.0 - r);
        pole = -beta + std::sqrt(beta * beta - 1.0);
    }
    lossPole = static_cast<float>(pole);
    lossGain = static_cast<float>(
        g1 * std::sqrt(1.0 + 2.0 * pole * std::cos(w1) + pole * pole) / (1.0 + pole));

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
    lossState = 0.0f;
    allpassState.fill(0.0f);
    force = forceStart = forceEnd = 0.0;
    releaseLength = releasePosition = 0;
    forceHeld = blockedForce = 0.0;
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
    const double pole = lossPole;
    const auto error = [&](double a) {
        const double base = sampleRate / f0 - lossPhaseDelay(w1, pole)
            - dispersionSections * allpassPhaseDelay(w1, a);
        double sum = 0.0;
        for (int n = 2; n <= upper; ++n)
        {
            const double fn = n * ideal * std::sqrt(1.0 + stiffnessNow * n * n);
            if (fn > 0.45 * sampleRate)
                break;
            const double wn = 2.0 * pi * fn / sampleRate;
            const double loop = base + lossPhaseDelay(wn, pole)
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

    // B grows as the speaking length squared shrinks and falls with tension.
    const double stiffnessNow = spec->stiffness * std::pow(2.0, fret / 6.0) / tensionRatio;
    if (designedStiffness < 0.0
        || std::abs(stiffnessNow / designedStiffness - 1.0) > 0.02)
        designDispersion(stiffnessNow, fundamental);

    const double w0 = 2.0 * pi * fundamental / sampleRate;
    const double filterDelay = lossPhaseDelay(w0, lossPole)
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
                clearRight(static_cast<int>(oldRead) + 3,
                           static_cast<int>(newRead) + 3);
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
}

void StratString::setDamper(int damper, double metresFromBridge,
                            double kilogramsPerSecond) noexcept
{
    if (damper < 0 || damper >= damperCount)
        return;
    dampers[static_cast<size_t>(damper)].metres = metresFromBridge;
    dampers[static_cast<size_t>(damper)].resistance = kilogramsPerSecond;
    placeDampers();
}

void StratString::placeDampers() noexcept
{
    for (Damper& d : dampers)
    {
        d.position = static_cast<int>(std::lround(
            clamp(positionSamples(d.metres), 1.0, lastUsablePosition())));
        // A dashpot of resistance R on a string of impedance Z passes
        // 2Z / (2Z + R) of the local velocity and absorbs the rest.
        d.absorption = d.resistance > 0.0
            ? static_cast<float>(d.resistance / (2.0 * characteristicImpedance + d.resistance))
            : 0.0f;
    }
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
                        double releaseSeconds) noexcept
{
    // The release is the force falling from F to 0 at the pick; relative to
    // the deflected rest it is a step of -F, which a point on a string meets
    // with impedance 2Z, sending -F / 2Z both ways. A release still running
    // completes first.
    force = forceEnd;
    forceStart = force;
    forceEnd = force - forceNewtons;
    releaseLength = static_cast<int>(releaseSeconds * sampleRate) + 1;
    releasePosition = 0;
    pickMetres = metresFromBridge;
    pick = tapAt(clamp(positionSamples(pickMetres), 1.0, lastUsablePosition()));
}

float StratString::readRight(const Tap& tap) const noexcept
{
    const int index = write - tap.base;
    return tap.weight[0] * right[static_cast<size_t>(index & railMask)]
         + tap.weight[1] * right[static_cast<size_t>((index - 1) & railMask)]
         + tap.weight[2] * right[static_cast<size_t>((index - 2) & railMask)]
         + tap.weight[3] * right[static_cast<size_t>((index - 3) & railMask)];
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
    const float atFret = readRight(tapAt(loopDelay - leftLength));
    lossState = lossGain * (1.0f + lossPole) * atFret - lossPole * lossState;
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

    if (releasePosition < releaseLength)
    {
        ++releasePosition;
        const double u = static_cast<double>(releasePosition) / releaseLength;
        force = forceStart + (forceEnd - forceStart) * (0.5 - 0.5 * std::cos(pi * u));
    }
    // One-pole DC blocker on the force.
    blockedForce = force - forceHeld + blockCoefficient * blockedForce;
    forceHeld = force;
    if (blockedForce != 0.0)
    {
        if (std::abs(blockedForce) < 1e-9)
            blockedForce = 0.0;
        const float v = static_cast<float>(blockedForce / (2.0 * characteristicImpedance));
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
        float& r = right[static_cast<size_t>((write - d.position) & railMask)];
        float& l = left[static_cast<size_t>((write + leftOffset + d.position) & railMask)];
        const float v = d.absorption * (r + l);
        r -= v;
        l -= v;
    }

    for (int k = 0; k < tapCount; ++k)
        tapVelocity[static_cast<size_t>(k)] = readRight(taps[static_cast<size_t>(k)])
            + readLeft(taps[static_cast<size_t>(k)]);

    write = (write + 1) & railMask;
}

} // namespace electry
