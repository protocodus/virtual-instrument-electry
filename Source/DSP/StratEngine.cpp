#include "StratEngine.h"

#include "DspCompatibility.h"

#include <cmath>

namespace electry
{
namespace
{
constexpr double pi = 3.14159265358979323846;

// Coil centres from the saddles: neck 162 mm and middle 98 mm, square to the
// strings; the bridge coil slants from 46 mm under the low E to 36 mm under
// the high E. Saddle intonation offsets of a few millimetres are left out:
// every string is 25.5 in long here.
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
// the high E, which the gap tilt above reduces by 2.5 dB.
//
// The absolute coupling depends on the magnet and the turns, which nothing
// here measures, and only the level at the jack depends on it: 1 V per m/s
// under the low E is a level convention, not a pickup specification.
constexpr double lowEVoltsPerMetrePerSecond = 1.0;

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
// a Strat recorded direct (IDMT-SMT-Guitar dataset 1, neck pickup, the
// ordinary stroke taken as 2.5 N as StratPerformer's release fit takes it).
// The neck coil sits a quarter of the way along an open string and halfway
// along one stopped at the 12th fret, so a linear coil leaves out the open
// strings' 4th partial and the 12th-fret notes' 2nd; the recording has them
// 21 dB under the 2nd partial and 17 dB under the fundamental and the 3rd,
// on all six strings, where a linear coil leaves 42 and 30. 0.15 brings
// each within 4 dB on average over its six notes, in the first 20 ms and
// at 200 ms, and -0.3 dB over all four; 0.125 leaves them 1.7 dB short and
// 0.175 1.0 dB over. A string swinging along the body over a round pole
// also gives second-order flux, and it rests 1 to 2 mm above the set gap at
// the low frets; the share stands for both. Which way a stroke leans is not
// measured: a downstroke (positive force) leans toward the poles here.
constexpr double acrossShare = 0.15;

// The flux law is tabulated over the string's distance from the poles,
// 0 to 12.8 mm, finely enough that interpolating it errs by under 1e-4.
constexpr double fluxStepMetres = 0.00005;

// The displacement under each coil is the integral of the string's velocity
// there, forgetting its drift at 5 Hz, far below the slackest string's
// fundamental (41 Hz, a full dive): 0.7% and 7 degrees off there.
constexpr double swingLeakHz = 5.0;

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
    return steelCoupling(stringIndex) * fluxSlope(gapMetres(stringIndex))
        / fluxSlope(gapBassMetres);
}

void StratEngine::prepare(double sampleRate) noexcept
{
    for (int s = 0; s < stringCount; ++s)
    {
        StratString& string = strings[static_cast<size_t>(s)];
        string.prepare(sampleRate, s);
        string.setTaps({coilMetres(0, s), coilMetres(1, s), coilMetres(2, s)});
        steel[static_cast<size_t>(s)] = static_cast<float>(steelCoupling(s));
        gapSteps[static_cast<size_t>(s)] = static_cast<float>(gapMetres(s) / fluxStepMetres);
        // Two loops of the open string slackened to a quarter of its
        // tension, the longest the string can be.
        quietLimit[static_cast<size_t>(s)] = static_cast<int>(
            4.0 * sampleRate / StratString::construction(s).openHz);
        pending[static_cast<size_t>(s)] = Pending {};
    }
    for (int i = 0; i < fluxTableSize; ++i)
        fluxTable[static_cast<size_t>(i)] = static_cast<float>(
            fluxSlope(i * fluxStepMetres) / fluxSlope(gapBassMetres));
    swingLeak = static_cast<float>(1.0 - 2.0 * pi * swingLeakHz / sampleRate);
    swingSteps = static_cast<float>(acrossShare / (sampleRate * fluxStepMetres));
    tremolo = StratBridge {};
    bridgeMoved = false;
    circuit.prepare(sampleRate);
    reset();
}

void StratEngine::reset() noexcept
{
    for (int s = 0; s < stringCount; ++s)
    {
        strings[static_cast<size_t>(s)].reset();
        quietSamples[static_cast<size_t>(s)] = quietLimit[static_cast<size_t>(s)];
        swing[static_cast<size_t>(s)] = {};
    }
    circuit.reset();
    phase = 0;
}

void StratEngine::setControls(const StratCircuit::Controls& controls) noexcept
{
    circuit.setControls(controls);
}

void StratEngine::setFret(int stringIndex, double fret, bool glide) noexcept
{
    if (stringIndex < 0 || stringIndex >= stringCount)
        return;
    Pending& p = pending[static_cast<size_t>(stringIndex)];
    p.fret = fret;
    if (glide)
    {
        p.glide = true;
        p.changed = true;
        return;
    }
    strings[static_cast<size_t>(stringIndex)].setTuning(
        fret, tremolo.tensionRatio(stringIndex));
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
                        double releaseSeconds, double strokeMetresPerSecond) noexcept
{
    if (stringIndex < 0 || stringIndex >= stringCount)
        return;
    strings[static_cast<size_t>(stringIndex)].pluck(forceNewtons, metresFromBridge,
                                                    releaseSeconds, strokeMetresPerSecond);
    quietSamples[static_cast<size_t>(stringIndex)] = 0;
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

void StratEngine::applyPending() noexcept
{
    for (int s = 0; s < stringCount; ++s)
    {
        Pending& p = pending[static_cast<size_t>(s)];
        if (!p.changed && !bridgeMoved)
            continue;
        strings[static_cast<size_t>(s)].setTuning(p.fret, tremolo.tensionRatio(s),
                                                  p.glide);
        p.changed = false;
        p.glide = false;
    }
    bridgeMoved = false;
}

void StratEngine::process(float* jackVolts, int samples) noexcept
{
    // Below this on every tap for two loops a string is skipped until it is
    // played again: 200 dB under a pluck, far above the float subnormals
    // its decay would otherwise reach.
    constexpr float quietVelocity = 1e-10f;
    std::array<float, StratString::tapCount> taps {};
    for (int i = 0; i < samples; ++i)
    {
        if (phase == 0)
            applyPending();
        phase = (phase + 1) % controlPeriod;

        float emf[coilCount] {0.0f, 0.0f, 0.0f};
        for (int s = 0; s < stringCount; ++s)
        {
            const size_t k = static_cast<size_t>(s);
            if (quietSamples[k] >= quietLimit[k])
                continue;
            strings[k].process(taps);
            bool quiet = !strings[k].isDriven();
            for (int c = 0; c < coilCount; ++c)
            {
                const float v = taps[static_cast<size_t>(c)];
                quiet = quiet && scalarAbs(v) < quietVelocity;
                float& y = swing[k][static_cast<size_t>(c)];
                y = y * swingLeak + v;
                // A string driven onto the poles reads as touching them,
                // and one far off as at the table's end.
                float d = gapSteps[k] - swingSteps * y;
                d = d < 0.0f ? 0.0f : (d > fluxTableSize - 1.001f ? fluxTableSize - 1.001f : d);
                const int i = static_cast<int>(d);
                const float t = d - static_cast<float>(i);
                const float slope = fluxTable[static_cast<size_t>(i)]
                    + t * (fluxTable[static_cast<size_t>(i + 1)] - fluxTable[static_cast<size_t>(i)]);
                emf[c] += steel[k] * slope * v;
            }
            quietSamples[k] = quiet ? quietSamples[k] + 1 : 0;
        }
        jackVolts[i] = static_cast<float>(circuit.process(emf[0], emf[1], emf[2]));
    }
}

} // namespace electry
