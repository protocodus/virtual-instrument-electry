#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    using Filter = ElectryEngine::ModalReleaseFilter;

    struct Source
    {
        int phase = 0, remaining = 0, length = 0, tailLength = 0;
        float amplitude = 0, polarity = 0, load = 0, slip = 0, releasePole = 0;
        float legacyPole = 0;
        double g = 0, period = 0, rate = 0;
        bool flatRelease = true;
        bool zeroProjectionState = false;
    };

    static Source source(const ElectryEngine& engine, int stringIndex)
    {
        const auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        const auto& filter = voice.excitationModalShaper;
        return {static_cast<int>(voice.excitationPhase), voice.excitationRemaining,
                voice.excitationLength, voice.excitationTailLength,
                voice.excitationAmplitude, voice.excitationPolarity,
                voice.excitationLoadScale, voice.excitationSlipScale,
                voice.excitationReleaseCoefficient, voice.excitationModalCoefficient,
                filter.a2 / filter.a1, voice.lastCompensatedPeriod, engine.sampleRate_, filter.flatRelease,
                filter.ic1 == 0 && filter.ic2 == 0
                    && filter.contactShaper1.state == 0 && filter.contactShaper2.state == 0};
    }

    static void isolateSource(ElectryEngine& engine, int stringIndex)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        voice.vertical.loopGain = voice.horizontal.loopGain = 0;
        voice.excitationTransientAmplitude = 0;
        voice.noiseRemaining = 0;
        voice.touchDepth = 0;
        voice.releasing = false;
        voice.verticalWeight = 1;
        voice.horizontalWeight = 0;
        engine.feedbackDrive_ = engine.bridgeCouplingInjection_ = 0;
    }

    static float renderWrittenSource(ElectryEngine& engine, int stringIndex)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        engine.sympatheticBus_ = engine.sympatheticBusDelayed_ = 0;
        voice.busContribution = 0;
        ElectryEngine::RenderSums sums {};
        engine.renderVoice(voice, sums);
        return voice.vertical.line[static_cast<std::size_t>(
            (voice.vertical.writeIndex - 1) & (ElectryEngine::delayLineSize - 1))];
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
constexpr double pi = 3.1415926535897932384626433832795;
constexpr double damping = 1.7320508075688772935274463415059;
int assertions = 0, failures = 0;

void expect(bool condition, const std::string& message)
{
    ++assertions;
    if (!condition)
    {
        ++failures;
        if (failures <= 25)
            std::cerr << message << '\n';
    }
}

// Independent direct-form bilinear transform of
// H(s)=wn^2/(s^2+sqrt(3)*wn*s+wn^2). No TPT state update is repeated here.
struct BilinearReference
{
    double b0, b1, b2, a1, a2;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    explicit BilinearReference(double g)
    {
        const double denominator = 1 + damping * g + g * g;
        b0 = g * g / denominator;
        b1 = 2 * b0;
        b2 = b0;
        a1 = 2 * (g * g - 1) / denominator;
        a2 = (1 - damping * g + g * g) / denominator;
    }

    double process(double input)
    {
        const double result = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = input;
        y2 = y1; y1 = result;
        return result;
    }

    std::complex<double> transfer(double omega) const
    {
        const auto z = std::polar(1.0, -omega);
        return (b0 + b1 * z + b2 * z * z) / (1.0 + a1 * z + a2 * z * z);
    }

    double poleDecay() const
    {
        const auto root = (-a1 + std::sqrt(std::complex<double>(a1 * a1 - 4 * a2))) / 2.0;
        return -std::log(std::abs(root));
    }
};

void impulseAreaTransferTailAndReset()
{
    for (const double rate : {16000., 44100., 48000., 88200., 96000., 192000., 384000.})
        for (const double cutoff : {28., 40., 330., 900.})
        {
            Access::Filter filter;
            const float legacyPole = std::exp(-2 * static_cast<float>(pi)
                                              * static_cast<float>(cutoff / rate));
            filter.configureFromLegacyPole(legacyPole);
            BilinearReference reference(damping * (1 - legacyPole) / (4 * legacyPole));
            const int length = static_cast<int>(std::ceil(32 / reference.poleDecay()));
            std::vector<float> impulse(static_cast<std::size_t>(length));
            double area = 0, moment = 0, energy = 0, finalQuarterEnergy = 0, referenceError = 0;
            bool finite = true;
            for (int sample = 0; sample < length; ++sample)
            {
                const float input = sample == 0 ? 1.f : 0.f;
                const float actual = filter.process(input);
                impulse[static_cast<std::size_t>(sample)] = actual;
                finite = finite && std::isfinite(actual);
                referenceError = std::max(referenceError,
                    std::abs(static_cast<double>(actual) - reference.process(input)));
                area += actual;
                moment += sample * static_cast<double>(actual);
                energy += static_cast<double>(actual) * actual;
                if (sample >= 3 * length / 4)
                    finalQuarterEnergy += static_cast<double>(actual) * actual;
            }
            const std::string label = " (rate=" + std::to_string(rate)
                                    + ", cutoff=" + std::to_string(cutoff) + ")";
            expect(finite, "The modal source filter produced non-finite state/output" + label);
            expect(std::abs(area - 1) < 1e-6,
                   "The modal filter changed the released pulse's DC area" + label);
            const double legacyDelay = 2.0 * legacyPole / (1 - legacyPole);
            expect(std::abs(moment / area - legacyDelay) < 1e-6 * legacyDelay,
                   "The actual modal impulse changed the established mean release latency" + label);
            expect(referenceError < 1e-6,
                   "The actual impulse differs from the independent Bessel reference" + label);
            expect(energy > 0 && finalQuarterEnergy < energy * 1e-12,
                   "The modal source retains an unbounded or unexpectedly long tail" + label);
            for (const double frequency : {0., .25 * cutoff, .5 * cutoff, cutoff,
                                          2 * cutoff, 4 * cutoff, std::min(16000., .4 * rate)})
            {
                const double omega = 2 * pi * frequency / rate;
                std::complex<double> measured {};
                for (int sample = 0; sample < length; ++sample)
                    measured += static_cast<double>(impulse[static_cast<std::size_t>(sample)])
                              * std::polar(1.0, -omega * sample);
                expect(std::abs(measured - reference.transfer(omega)) < 1e-6,
                       "The rendered modal impulse has the wrong complex frequency response" + label);
                expect(std::abs(measured) <= 1 + 1e-6,
                       "The source low-pass gained above its unit-DC reference" + label);
            }
            filter.reset();
            bool silent = true;
            for (int sample = 0; sample < 32; ++sample)
                silent = silent && filter.process(0) == 0;
            expect(silent, "Reset retained a previous modal excitation" + label);
        }
}

float smooth(float value)
{
    value = std::clamp(value, 0.f, 1.f);
    return value * value * (3 - 2 * value);
}

void actualReleaseAndTailUseTheProjection()
{
    for (const int rate : {48000, 384000})
        for (const auto stringNote : {std::array<int, 2>{0, 28}, {2, 47}, {5, 55}, {7, 79}})
            for (const auto style : {electry::PlayStyle::Sustain, electry::PlayStyle::PalmMute,
                                     electry::PlayStyle::Hammer, electry::PlayStyle::Harmonics,
                                     electry::PlayStyle::Pinch, electry::PlayStyle::Slide,
                                     electry::PlayStyle::Dead})
                for (const bool upstroke : {false, true})
                {
                    auto engine = std::make_unique<Engine>();
                    electry::EngineParameters parameters;
                    parameters.pickNoise = parameters.fingerNoise = parameters.releaseNoise = 0;
                    parameters.artifactAmount = parameters.sympatheticAmount = parameters.bodyResonance = 0;
                    engine->setParameters(parameters);
                    engine->setVariationSeed(0);
                    engine->prepare(rate, 64);
                    engine->setSoloStringMask(static_cast<std::uint8_t>(1u << stringNote[0]));
                    engine->noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(style), 1);
                    if (upstroke)
                        engine->noteOn(Engine::firstKeyswitchNote + static_cast<int>(electry::PickStyle::Up), 1);
                    engine->noteOn(stringNote[1], .85f);
                    std::array<float, 1> left {}, right {};
                    int waited = 0;
                    while (Access::source(*engine, stringNote[0]).phase != 2 && waited < rate / 5)
                    {
                        engine->process(left.data(), right.data(), 1);
                        ++waited;
                    }
                    const auto initial = Access::source(*engine, stringNote[0]);
                    const std::string label = " (rate=" + std::to_string(rate)
                        + ", note=" + std::to_string(stringNote[1])
                        + ", style=" + std::to_string(static_cast<int>(style))
                        + ", up=" + std::to_string(upstroke) + ")";
                    expect(initial.phase == 2 && initial.remaining > 0,
                           "The public picked note did not reach physical Release" + label);
                    expect(initial.zeroProjectionState,
                           "A fresh public contact retained a previous projection" + label);
                    if (initial.phase != 2 || initial.remaining <= 0)
                        continue;
                    Access::isolateSource(*engine, stringNote[0]);
                    BilinearReference reference(initial.g);
                    float releaseState = 0, legacyState1 = 0, legacyState2 = 0;
                    const bool ordinaryProjection = style != electry::PlayStyle::Hammer
                        && style != electry::PlayStyle::Pinch && style != electry::PlayStyle::Dead;
                    expect(initial.flatRelease == ordinaryProjection,
                           "A separately calibrated contact used the ordinary source projection" + label);
                    if (ordinaryProjection)
                    {
                        const int expectedTail = std::clamp(static_cast<int>(8 / std::min(
                            reference.poleDecay(), -std::log(static_cast<double>(initial.releasePole)))),
                            16, static_cast<int>(.075 * initial.rate));
                        expect(std::abs(initial.tailLength - expectedTail) <= 1,
                               "The source Tail did not follow its actual pole decay and existing cap" + label);
                    }
                    int releaseSamples = 0, tailSamples = 0;
                    bool finite = true, legacyIdentical = true;
                    double maxReleaseError = 0, maxTailError = 0;
                    for (int sample = 0; sample <= initial.remaining + initial.tailLength; ++sample)
                    {
                        const auto before = Access::source(*engine, stringNote[0]);
                        if (before.phase != 2 && before.phase != 3)
                            break;
                        float input = 0;
                        if (before.phase == 2)
                        {
                            const float progress = 1 - static_cast<float>(before.remaining)
                                                      / static_cast<float>(before.length);
                            const float window = smooth(progress * before.load)
                                               * smooth((1 - progress) * before.slip);
                            input = before.amplitude * (window * before.polarity);
                            ++releaseSamples;
                        }
                        else
                            ++tailSamples;
                        float projected = 0;
                        if (ordinaryProjection)
                            projected = static_cast<float>(reference.process(input));
                        else
                        {
                            legacyState1 += (1 - before.legacyPole) * (input - legacyState1);
                            legacyState2 += (1 - before.legacyPole) * (legacyState1 - legacyState2);
                            projected = legacyState2;
                        }
                        releaseState += (1 - before.releasePole) * (projected - releaseState);
                        const float actual = Access::renderWrittenSource(*engine, stringNote[0]);
                        legacyIdentical = legacyIdentical && (ordinaryProjection || actual == releaseState);
                        finite = finite && std::isfinite(actual);
                        const double error = std::abs(static_cast<double>(actual) - releaseState);
                        if (before.phase == 2)
                            maxReleaseError = std::max(maxReleaseError, error);
                        else
                            maxTailError = std::max(maxTailError, error);
                    }
                    const double tolerance = 2e-6 * std::max(1e-5f, initial.amplitude);
                    expect(finite && maxReleaseError < tolerance,
                           "Actual released string input bypassed or misconfigured the standard projection" + label);
                    expect(maxTailError < tolerance,
                           "Actual source Tail differs from the same continuous filter state" + label);
                    expect(legacyIdentical,
                           "A finger/thumb/dead-hand source differs bitwise from its established two poles" + label);
                    expect(releaseSamples == initial.remaining && tailSamples == initial.tailLength
                               && Access::source(*engine, stringNote[0]).phase == 0,
                           "Changing modal shape changed the existing Release/Tail clock" + label);
                    expect(Access::renderWrittenSource(*engine, stringNote[0]) == 0,
                           "The completed source emitted an extra excitation frame" + label);
                    engine->reset();
                    expect(Access::source(*engine, stringNote[0]).zeroProjectionState,
                           "Engine reset left the new source filter ringing" + label);
                }
}

void prebendAndFrettedContactTransportTheActualFilter()
{
    const auto contact = [] (int rate, int note, float bend)
    {
        auto engine = std::make_unique<Engine>();
        electry::EngineParameters parameters;
        parameters.pickNoise = parameters.fingerNoise = parameters.releaseNoise = 0;
        parameters.artifactAmount = parameters.sympatheticAmount = parameters.bodyResonance = 0;
        engine->setParameters(parameters);
        engine->setVariationSeed(0);
        engine->prepare(rate, 64);
        engine->setSoloStringMask(1u << 2);
        engine->setExpressionPitchBend(7, bend);
        engine->snapExpressionPitchBendToTarget(7);
        engine->noteOn(note, .85f, 7);
        std::array<float, 1> left {}, right {};
        for (int sample = 0; sample < rate / 5; ++sample)
        {
            const auto state = Access::source(*engine, 2);
            if (state.phase == 1)
                return state;
            engine->process(left.data(), right.data(), 1);
        }
        return Access::Source {};
    };
    for (const int rate : {44100, 48000, 96000, 384000})
        for (const int bend : {-2, 2, 7})
        {
            const auto bent = contact(rate, 47, static_cast<float>(bend));
            const auto fretted = contact(rate, 47 + bend, 0);
            const std::string label = " (rate=" + std::to_string(rate)
                                    + ", bend=" + std::to_string(bend) + ")";
            expect(bent.phase == 1 && fretted.phase == 1
                       && std::abs(bent.period / fretted.period - 1) < 2e-5,
                   "The bend/fret contact fixture did not have the same physical period" + label);
            const double tolerance = 4 * std::numeric_limits<float>::epsilon()
                / std::max(1e-8, 1 - static_cast<double>(bent.legacyPole));
            expect(bent.g > 0 && fretted.g > 0
                       && std::abs(bent.g / fretted.g - 1) < tolerance,
                   "The actual new filter followed written pitch instead of the sounding contact" + label);
            const double recoveredPole = damping / (damping + 4 * bent.g);
            expect(std::abs(recoveredPole - bent.legacyPole) < 1e-12,
                   "The actual projection did not preserve the transported legacy latency" + label);
        }
}
}

int main()
{
    impulseAreaTransferTailAndReset();
    actualReleaseAndTailUseTheProjection();
    prebendAndFrettedContactTransportTheActualFilter();
    std::cout << assertions << " assertions, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
