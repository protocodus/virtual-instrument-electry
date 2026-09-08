#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    struct Contact
    {
        float impedance;
        float loadScale;
        float slipScale;
        float amplitude;
        float transient;
        float noiseCoefficient;
        int length;
        double sampleRate;
    };

    static Contact contact(const ElectryEngine& engine, int stringIndex)
    {
        const auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        return { voice.stringWaveImpedance, voice.excitationLoadScale,
                 voice.excitationSlipScale, voice.excitationAmplitude,
                 voice.excitationTransientAmplitude, voice.noiseBandCoefficient,
                 voice.excitationLength, engine.sampleRate_ };
    }

    static Contact reexcite(ElectryEngine& engine, int stringIndex,
                           float impedance, bool fingerOnly)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        voice.stringWaveImpedance = impedance;
        if (fingerOnly)
            voice.playStyle = PlayStyle::Hammer;
        engine.startExcitation(voice, voice.velocity, fingerOnly, true);
        return contact(engine, stringIndex);
    }
};
} // namespace electry

namespace
{
using electry::ElectryEngine;
using electry::EngineParameters;
using Access = electry::ElectryEngineTestAccess;

int failures = 0;
void expect(bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::unique_ptr<ElectryEngine> makeEngine(double rate, int stringIndex,
                                        float gauge, float hardness = 0.58f)
{
    auto engine = std::make_unique<ElectryEngine>();
    engine->prepare(rate, 512);
    EngineParameters parameters;
    parameters.stringGauge = gauge;
    parameters.pickHardness = hardness;
    parameters.pickNoise = 0.25f;
    parameters.fingerNoise = 0.0f;
    parameters.releaseNoise = 0.0f;
    parameters.artifactAmount = 0.0f;
    parameters.sympatheticAmount = 0.0f;
    engine->setParameters(parameters);
    engine->reset();
    engine->setSoloStringMask(static_cast<std::uint8_t>(1u << stringIndex));
    return engine;
}

double pulseArea(const Access::Contact& contact)
{
    const auto smooth = [] (double x)
    {
        x = std::clamp(x, 0.0, 1.0);
        return x * x * (3.0 - 2.0 * x);
    };
    double sum = 0.0;
    for (int sample = 0; sample < contact.length; ++sample)
    {
        const double phase = static_cast<double>(sample) / contact.length;
        sum += smooth(phase * contact.loadScale)
             * smooth((1.0 - phase) * contact.slipScale);
    }
    return sum * contact.amplitude;
}

double noiseCutoff(const Access::Contact& contact)
{
    return -std::log(contact.noiseCoefficient) * contact.sampleRate
         / (2.0 * 3.14159265358979323846);
}

void testPhysicalStringImpedanceAndWinding()
{
    constexpr std::array<int, 8> notes { 28, 35, 40, 45, 50, 55, 59, 64 };
    constexpr std::array<float, 8> diameters {
        2.0320f, 1.5240f, 1.0668f, 0.8128f,
        0.6096f, 0.4064f, 0.2794f, 0.2286f
    };
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        for (int stringIndex = 0; stringIndex < 8; ++stringIndex)
        {
            auto light = makeEngine(rate, stringIndex, 0.0f);
            auto heavy = makeEngine(rate, stringIndex, 1.0f);
            light->noteOn(notes[static_cast<std::size_t>(stringIndex)], 0.8f);
            heavy->noteOn(notes[static_cast<std::size_t>(stringIndex)], 0.8f);
            const auto a = Access::contact(*light, stringIndex);
            const auto b = Access::contact(*heavy, stringIndex);
            // Equal scale/tuning gives Z=mu*c, so diameter scales it squared.
            expect(std::abs(b.impedance / a.impedance - 121.0f / 81.0f) < 2.0e-5f,
                   "live impedance must follow gauge-squared mass at fixed pitch");
            expect(b.slipScale <= a.slipScale + 1.0e-5f,
                   "heavier strings must not slip faster at the same force");
            const double cutoffRatio = noiseCutoff(b) / noiseCutoff(a);
            if (stringIndex < 5)
            {
                const double diameter = diameters[static_cast<std::size_t>(stringIndex)];
                const double expected = (0.100 + 0.130 * diameter)
                    / (0.100 + 0.130 * diameter * 11.0 / 9.0);
                expect(std::abs(cutoffRatio - expected) < 1.0e-5,
                       "wound pick scrape must track ridge spacing across gauges");
            }
            else
            {
                expect(a.noiseCoefficient == b.noiseCoefficient,
                       "plain strings must bypass winding scrape scaling");
            }

            auto fretted = makeEngine(rate, stringIndex, 1.0f);
            fretted->noteOn(notes[static_cast<std::size_t>(stringIndex)] + 12, 0.8f);
            const auto fret = Access::contact(*fretted, stringIndex);
            expect(std::abs(fret.impedance / b.impedance - 1.0f) < 2.0e-5f,
                   "fretting an octave must not change the physical string impedance");
            expect(fret.noiseCoefficient == b.noiseCoefficient,
                   "fretting must not move a stationary pick's winding scrape band");
        }
    }
}

void testReleaseShapeForceAndFingerIsolation()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        for (float hardness : { 0.0f, 0.58f, 1.0f })
        {
            auto engine = makeEngine(rate, 2, 1.0f, hardness);
            engine->noteOn(40, 0.8f);
            const float referenceZ = Access::contact(*engine, 2).impedance;
            const auto nominal = Access::reexcite(*engine, 2, referenceZ, false);
            const auto slow = Access::reexcite(*engine, 2, 2.0f * referenceZ, false);
            const auto fast = Access::reexcite(*engine, 2, 0.5f * referenceZ, false);
            expect(slow.length >= nominal.length && fast.length <= nominal.length,
                   "release duration must follow characteristic impedance");
            expect(slow.slipScale < nominal.slipScale
                       && fast.slipScale > nominal.slipScale,
                   "impedance must alter slip duration, not merely delay the attack");
            const double loadTime = nominal.length / nominal.loadScale;
            expect(std::abs(slow.length / slow.loadScale - loadTime) < 1.5,
                   "the hand's loading duration must survive impedance scaling");
            expect(std::abs(pulseArea(slow) / pulseArea(nominal) - 1.0) < 2.0e-5,
                   "slower release must retain the original projected force area");
            expect(std::abs(pulseArea(fast) / pulseArea(nominal) - 1.0) < 2.0e-5,
                   "faster release must retain the original projected force area");

            const auto fingerA = Access::reexcite(*engine, 2, 0.01f, true);
            const auto fingerB = Access::reexcite(*engine, 2, 100.0f, true);
            expect(fingerA.length == fingerB.length
                       && fingerA.loadScale == fingerB.loadScale
                       && fingerA.slipScale == fingerB.slipScale
                       && fingerA.amplitude == fingerB.amplitude
                       && fingerA.noiseCoefficient == fingerB.noiseCoefficient,
                   "finger-only contact must bypass plectrum recoil and winding changes");
        }
    }
}

std::vector<float> renderRepeatedLowString(double rate, int block)
{
    auto engine = makeEngine(rate, 0, 1.0f);
    const int step = static_cast<int>(rate / 12.0);
    std::vector<float> left(static_cast<std::size_t>(step * 12));
    std::vector<float> right(left.size());
    for (int hit = 0; hit < 12; ++hit)
    {
        engine->noteOn(ElectryEngine::firstKeyswitchNote + hit % 2, 1.0f);
        engine->noteOn(28, hit % 4 == 0 ? 1.0f : 0.75f);
        for (int offset = 0; offset < step; offset += block)
        {
            const int count = std::min(block, step - offset);
            const auto index = static_cast<std::size_t>(hit * step + offset);
            engine->process(left.data() + index, right.data() + index, count);
        }
    }
    return left;
}

void testAudioRateAndBlockBehaviour()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        const auto a = renderRepeatedLowString(rate, 17);
        const auto b = renderRepeatedLowString(rate, 512);
        expect(a == b, "rapid eight-string picking must remain sample-identical across blocks");
        float peak = 0.0f;
        for (float sample : a)
        {
            expect(std::isfinite(sample), "rapid low-string picking produced nonfinite audio");
            peak = std::max(peak, std::abs(sample));
        }
        expect(peak > 0.001f && peak < 2.0f,
               "rapid low-string picking must produce bounded audible output");
    }
}
} // namespace

int main()
{
    testPhysicalStringImpedanceAndWinding();
    testReleaseShapeForceAndFingerIsolation();
    testAudioRateAndBlockBehaviour();
    if (failures == 0)
        std::cout << "Excitation realism tests passed: impedance, winding, force area, finger isolation, and rapid render invariants.\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
