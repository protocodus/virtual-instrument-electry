#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    struct Loss
    {
        double fundamental, high, horizontal, deadSpot;
        float gain, coefficient;
        double maximumMagnitude;
    };

    static Loss at(ElectryEngine& engine, int stringIndex, float fret,
                   float frequency = 100.0f)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        engine.configureVoiceDamping(voice, PlayStyle::Sustain, frequency, fret);
        const auto magnitude = [&] (const auto& loop, double observedFrequency)
        {
            // Independently evaluate the realised one-pole response, including
            // the scalar gain. Its traversal count is the string fundamental,
            // even when measuring a higher-frequency component in that loop.
            const double a = loop.loopDampingCoefficient;
            const double omega = 6.2831853071795864769 * observedFrequency
                               / engine.sampleRate_;
            const auto z = std::polar(1.0, -omega);
            const auto& b = loop.materialLossDip;
            const double material = loop.materialLossDepth > 0.0f
                ? std::abs((b.b0 + b.b1 * z + b.b2 * z * z)
                         / (1.0 + b.a1 * z + b.a2 * z * z)) : 1.0;
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
            if (loop.lossShelfActive)
            {
                const auto& loss = loop.lossShelf;
                return material * loop.loopGain * (1.0 - a)
                    / std::abs(1.0 - a * z) * loop.lossShelfGain
                    * std::abs(1.0 - static_cast<double>(loop.lossShelfZeroCoefficient) * z)
                    * std::abs((loss.b0 + loss.b1 * z + loss.b2 * z * z)
                        / (1.0 + loss.a1 * z + loss.a2 * z * z));
            }
#endif
            return material * loop.loopGain * (1.0 - a)
                / std::sqrt(1.0 + a * a - 2.0 * a * std::cos(omega));
        };
        const auto t60 = [&] (const auto& loop, double observedFrequency)
        {
            return -3.0 / (frequency * std::log10(
                magnitude(loop, observedFrequency)));
        };
        double maximumMagnitude = 0.0;
        for (int bin = 0; bin <= 64; ++bin)
            for (const auto* loop : { &voice.vertical, &voice.horizontal })
                maximumMagnitude = std::max(maximumMagnitude,
                    magnitude(*loop, 0.5 * engine.sampleRate_ * bin / 64.0));
        return { t60(voice.vertical, frequency), t60(voice.vertical, 3600.0),
                 t60(voice.horizontal, frequency),
                 engine.deadSpotFactor(stringIndex, fret),
                 voice.vertical.loopGain,
                 voice.vertical.loopDampingCoefficient, maximumMagnitude };
    }

    static std::array<float, 3> live(const ElectryEngine& engine)
    {
        const auto& v = engine.voices_[0];
        return { v.lastDampedLiveFret, v.lastConfiguredLiveFret,
                 v.lastDampedFrequency };
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
constexpr std::array<int, 8> openNotes { 28, 35, 40, 45, 50, 55, 59, 64 };
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::unique_ptr<Engine> makeEngine(double rate, int stringIndex = 0)
{
    auto e = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.sympatheticAmount = p.artifactAmount = p.bodyResonance = 0.0f;
    p.pickNoise = p.fingerNoise = p.releaseNoise = 0.0f;
    p.stringAge = 0.25f; // stay below the pre-existing 26-second target ceiling
    p.stringGauge = 0.65f;
    p.palmMute = 0.0f;
    e->setParameters(p);
    e->prepare(rate, 512);
    e->setVariationSeed(0);
    e->setSoloStringMask(static_cast<std::uint8_t>(1u << stringIndex));
    return e;
}

void lengthAndUpperAnchor(double rate)
{
    for (int stringIndex = 0; stringIndex < 8; ++stringIndex)
    {
        auto e = makeEngine(rate, stringIndex);
        e->noteOn(openNotes[static_cast<std::size_t>(stringIndex)], 0.8f);
        const auto open = Access::at(*e, stringIndex, 0.0f);
        const auto repeatedOpen = Access::at(*e, stringIndex, 0.0f);
        expect(open.gain == repeatedOpen.gain
                   && open.coefficient == repeatedOpen.coefficient,
               "open-string solve is deterministic and has no added state");

        for (float fret : { 0.5f, 6.0f, 12.0f, 22.0f })
        {
            const auto shortened = Access::at(*e, stringIndex, fret);
            // An octave halves speaking length. Subtract the already-existing
            // local neck dead-spot contribution before checking the material law.
            const double localCorrection = open.deadSpot / shortened.deadSpot;
            const double ratio = shortened.fundamental / open.fundamental
                               * localCorrection;
            const double horizontal = shortened.horizontal / open.horizontal
                                    * localCorrection;
            const double expected = stringIndex < 5
                ? std::pow(0.5, static_cast<double>(fret) / 12.0) : 1.0;
            expect(std::abs(ratio / expected - 1.0) < 0.002,
                   "wound decay tracks length; plain-string composite loss bypasses it");
            expect(std::abs(horizontal / expected - 1.0) < 0.003,
                   "both transverse polarisations retain the same length law");
            expect(std::abs(shortened.high / open.high * localCorrection - 1.0)
                       < 0.002,
                   "fretting preserves the fixed-Hz upper decay anchor");
            expect(shortened.fundamental > shortened.high
                       && shortened.gain > 0.0f && shortened.gain < 1.0f,
                   "length-dependent loop remains passive with a longer low tail");
        }

        // Hold physical length fixed while changing the pitch coordinate. A
        // bend must not be mistaken for a shorter speaking string.
        const auto unbent = Access::at(*e, stringIndex, 6.0f, 100.0f);
        const auto bent = Access::at(*e, stringIndex, 6.0f, 112.2462f);
        expect(std::abs(bent.fundamental / unbent.fundamental - 1.0) < 0.002,
               "pitch bend does not masquerade as movement of the stopped fret");
    }
}

void oldStringFeasibility(double rate)
{
    // These are the existing open-string fundamental targets before Age,
    // Gauge and the local neck contribution. Test the realised transfer, not
    // the solver's requested gain: a gain clamp used to pass a nominal target
    // while producing only 1.16 s instead of 11.36 s on old heavy E1 at 48 kHz.
    constexpr std::array<double, 8> nominalT60 {
        20.0, 19.0, 18.0, 16.5, 15.0, 12.0, 10.0, 8.5
    };
    for (int stringIndex = 0; stringIndex < 8; ++stringIndex)
    {
        auto e = makeEngine(rate, stringIndex);
        const float frequency = static_cast<float>(440.0 * std::exp2(
            (openNotes[static_cast<std::size_t>(stringIndex)] - 69.0) / 12.0));
        electry::EngineParameters p;
        p.sympatheticAmount = p.artifactAmount = p.bodyResonance = 0.0f;
        p.pickNoise = p.fingerNoise = p.releaseNoise = p.palmMute = 0.0f;
        p.construction = p.bodyWood = p.bodySize = 0.0f;
        p.stringGauge = 1.0f;
        double previousFundamental = 0.0, previousHigh = 0.0;
        for (int step = 0; step <= 100; ++step)
        {
            p.stringAge = static_cast<float>(step) / 100.0f;
            e->setParameters(p);
            e->reset();
            e->setSoloStringMask(static_cast<std::uint8_t>(1u << stringIndex));
            e->noteOn(openNotes[static_cast<std::size_t>(stringIndex)], 0.8f);
            const auto loss = Access::at(*e, stringIndex, 0.0f, frequency);
            const double target = std::clamp(nominalT60[stringIndex]
                * (1.08 - 0.70 * p.stringAge) * 1.16 * 1.18 * 1.05 * 1.04
                * loss.deadSpot, 0.02, 26.0);
            expect(std::abs(loss.fundamental / target - 1.0) < 0.003,
                   "all ages retain the existing fundamental target at physical open pitch");
            expect(std::abs(loss.horizontal / (1.7 * target) - 1.0) < 0.005,
                   "both polarisations preserve their fundamental under infeasible tilt");
            expect(loss.maximumMagnitude <= 1.000002
                       && loss.gain > 0.0f && loss.gain <= 0.99999f
                       && loss.fundamental > 0.0 && loss.high > 0.0,
                   "old-string loss stays passive across the frequency range");
            if (step > 0)
            {
                // Adjacent Age steps change the prescribed fundamental by at
                // most 1.85%. Crossing the feasibility boundary must not add a
                // jump or reverse it. The upper target may relax continuously.
                expect(loss.fundamental <= 1.003 * previousFundamental
                           && loss.fundamental >= 0.975 * previousFundamental,
                       "fundamental remains continuous and nonincreasing through Age");
                expect(std::abs(std::log(loss.high / previousHigh)) < 0.15,
                       "upper decay has no discontinuity at the passive-solve boundary");
            }
            previousFundamental = loss.fundamental;
            previousHigh = loss.high;
        }
    }
}

std::vector<float> performance(double rate, int block)
{
    auto e = makeEngine(rate);
    std::vector<float> output;
    const auto render = [&] (double seconds)
    {
        int left = static_cast<int>(std::round(seconds * rate));
        std::array<float, 512> l {}, r {};
        while (left > 0)
        {
            const int n = std::min(block, left);
            e->process(l.data(), r.data(), n);
            output.insert(output.end(), l.begin(), l.begin() + n);
            left -= n;
        }
    };
    const auto style = [&] (electry::PlayStyle value)
    {
        e->noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(value), 1);
    };
    e->noteOn(40, 0.8f); render(0.10);
    style(electry::PlayStyle::Slide);
    e->noteOn(50, 0.8f); render(0.040);
    const auto live = Access::live(*e);
    expect(live[0] > 12.0f && live[0] < 22.0f
               && std::abs(live[0] - live[1]) < 0.02f,
           "loss follows the travelling finger before a high-fret slide lands");
    render(0.25);
    style(electry::PlayStyle::Hammer);
    e->noteOn(42, 0.7f); render(0.04);
    e->noteOn(34, 0.7f); render(0.04);
    e->noteOn(28, 0.7f); render(0.16);
    const auto opened = Access::live(*e);
    expect(std::abs(opened[0]) < 0.02f,
           "pull-off to open restores the open-string decay coordinate");
    style(electry::PlayStyle::PalmMute);
    e->noteOn(50, 0.9f); render(0.10);
    e->setPalmMutePressure(1.0f); render(0.04);
    style(electry::PlayStyle::Dead);
    e->noteOn(50, 0.7f); render(0.10);
    e->noteOff(50); render(0.12);
    expect(std::all_of(output.begin(), output.end(), [] (float sample)
    {
        return std::isfinite(sample) && std::abs(sample) < 2.0f;
    }), "wound sustain, slide, pull-off and strong hand losses remain finite and bounded");
    e->reset();
    std::array<float, 512> l {}, r {};
    e->process(l.data(), r.data(), static_cast<int>(l.size()));
    expect(std::all_of(l.begin(), l.end(), [] (float x) { return x == 0.0f; })
               && std::all_of(r.begin(), r.end(), [] (float x) { return x == 0.0f; }),
           "reset clears a high-fret damped phrase to exact silence");
    return output;
}
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        lengthAndUpperAnchor(rate);
        oldStringFeasibility(rate);
        expect(performance(rate, 1) == performance(rate, 257),
               "live length-dependent decay is sample-identical across callback sizes");
    }
    if (!failures)
        std::cout << "Realism decay tests passed at all four sample rates.\n";
    return failures ? 1 : 0;
}
