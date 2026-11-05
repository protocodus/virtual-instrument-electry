#include "StratEngine.h"

#include "DspCompatibility.h"

#include <cmath>

namespace electry
{
namespace
{

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
// to a measured Seymour Duncan SSL-5 Strat single coil ("Measurements and
// modeling of the nonlinear behavior of a guitar pickup at low frequencies",
// DAFx-18, Eq. 6: equivalent length 12.98 mm, radius 2.77 mm).
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
    const double across = stringIndex / static_cast<double>(stringCount - 1);
    const double gap = gapBassMetres + (gapTrebleMetres - gapBassMetres) * across;
    return lowEVoltsPerMetrePerSecond
        * StratString::construction(stringIndex).kilogramsPerMetre
        / StratString::construction(0).kilogramsPerMetre
        * fluxSlope(gap) / fluxSlope(gapBassMetres);
}

void StratEngine::prepare(double sampleRate) noexcept
{
    for (int s = 0; s < stringCount; ++s)
    {
        StratString& string = strings[static_cast<size_t>(s)];
        string.prepare(sampleRate, s);
        string.setTaps({coilMetres(0, s), coilMetres(1, s), coilMetres(2, s)});
        coupling[static_cast<size_t>(s)] = static_cast<float>(coilCoupling(s));
        // Two loops of the open string slackened to a quarter of its
        // tension, the longest the string can be.
        quietLimit[static_cast<size_t>(s)] = static_cast<int>(
            4.0 * sampleRate / StratString::construction(s).openHz);
        pending[static_cast<size_t>(s)] = Pending {};
    }
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
                            double kilogramsPerSecond) noexcept
{
    if (stringIndex < 0 || stringIndex >= stringCount)
        return;
    strings[static_cast<size_t>(stringIndex)].setDamper(damper, metresFromBridge,
                                                        kilogramsPerSecond);
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
                emf[c] += coupling[k] * v;
            }
            quietSamples[k] = quiet ? quietSamples[k] + 1 : 0;
        }
        jackVolts[i] = static_cast<float>(circuit.process(emf[0], emf[1], emf[2]));
    }
}

} // namespace electry
