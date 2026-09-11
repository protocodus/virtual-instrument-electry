#include "DSP/ElectryEngine.h"
#include "DSP/ElectryFx.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    // Ablate one mechanism without changing the score, noise stream or any
    // other contact state. These seams exist only in this test executable.
    static void removePickRidges(ElectryEngine& engine)
    {
        for (auto& voice : engine.voices_)
            voice.pickRidgeCyclesPerSample = 0.0f;
    }

    static void removeReleaseCooling(ElectryEngine& engine)
    {
        for (auto& voice : engine.voices_)
            voice.releaseSpectralTarget = 0.0f;
    }

    struct OperatorResponse
    {
        float maximumGain { 0.0f };
        double maximumComplexError { 0.0 };
    };

    static OperatorResponse measureReleaseOperator(double rate)
    {
        auto engine = std::make_unique<ElectryEngine>();
        engine->prepare(rate, 257);
        auto& loop = engine->voices_[0].vertical;
        OperatorResponse result;

        for (float delay : { 18.25f, 75.5f, 1200.125f })
        {
            const float availableSpan = std::max(0.0f, std::min(
                delay - 4.0f,
                static_cast<float>(ElectryEngine::delayLineSize - 8) - delay));
            const float width = std::min(
                std::round(0.000125f * static_cast<float>(engine->sampleRate_)),
                std::floor(availableSpan));

            for (int bin = 0; bin <= 2048; ++bin)
            {
                const double omega = 3.14159265358979323846 * bin / 2048;
                std::complex<double> response, centre;
                for (int part = 0; part < 2; ++part)
                {
                    for (int cell = 0; cell < ElectryEngine::delayLineSize; ++cell)
                    {
                        loop.line[static_cast<std::size_t>(cell)] =
                            static_cast<float>(part == 0
                                ? std::cos(omega * cell) : std::sin(omega * cell));
                    }
                    loop.writeIndex = 4096;
                    const float sample = 0.5f * loop.readFractional(delay)
                        + 0.25f * (loop.readFractional(delay - width)
                                   + loop.readFractional(delay + width));
                    if (part == 0)
                    {
                        response.real(sample);
                        centre.real(loop.readFractional(delay));
                    }
                    else
                    {
                        response.imag(sample);
                        centre.imag(loop.readFractional(delay));
                    }
                }

                // Integer tap spacing gives this real, nonnegative multiplier
                // on the actual cubic read, not only on an ideal delay line.
                const auto expected = centre * (0.5 + 0.5 * std::cos(omega * width));
                result.maximumComplexError = std::max(
                    result.maximumComplexError, std::abs(response - expected));
                result.maximumGain = std::max(
                    result.maximumGain, static_cast<float>(std::abs(response)));
            }
        }
        return result;
    }

    static bool contactStateIsClear(const ElectryEngine& engine)
    {
        for (const auto& voice : engine.voices_)
        {
            if (voice.pickRidgeCyclesPerSample != 0.0f
                || voice.releaseSpectralDepth != 0.0f
                || voice.releaseSpectralTarget != 0.0f)
                return false;
        }
        return true;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Style = electry::PlayStyle;
using TestAccess = electry::ElectryEngineTestAccess;
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

struct Render
{
    std::vector<float> dry, modern;
};

Render render(int rate, int note, int stringIndex, float pickNoise, Style style,
              bool removePickRidges, bool removeReleaseCooling,
              int block = 257, bool repeats = false, bool noteOff = true,
              float fingerNoise = 0.0f)
{
    auto engine = std::make_unique<Engine>();
    electry::EngineParameters parameters;
    electry::applyGuitarBuild(parameters, electry::defaultGuitarBuild);
    parameters.outputGain = 1.0f;
    parameters.pickHardness = 0.85f;
    parameters.pickNoise = pickNoise;
    parameters.fingerNoise = fingerNoise;
    parameters.releaseNoise = parameters.artifactAmount = 0.0f;
    parameters.sympatheticAmount = parameters.bodyResonance = 0.0f;
    parameters.stringAge = 0.1f;
    parameters.velocityAmount = 0.7f;
    engine->prepare(rate, block);
    engine->setParameters(parameters);
    engine->setVariationSeed(0);
    engine->reset();
    engine->setSoloStringMask(1 << stringIndex);
    engine->noteOn(Engine::firstKeyswitchNote
                  + static_cast<int>(electry::PickStyle::Alternate), 1.0f);
    engine->noteOn(Engine::firstPlayStyleKeyswitchNote
                  + static_cast<int>(style), 1.0f);

    electry::ElectryFx effects;
    electry::FxParameters settings;
    settings.amp = 0.95f;
    settings.distortion = 0.45f;
    settings.compressor = 0.6f;
    settings.ampModel = electry::AmpModel::ModernHighGain;
    effects.prepare(rate);
    effects.setParameters(settings);
    effects.reset();

    Render result;
    result.dry.resize(static_cast<std::size_t>(rate / 2));
    result.modern.resize(result.dry.size());
    std::array<float, 511> left {}, right {};
    const int length = static_cast<int>(result.dry.size());
    int position = 0, nextEvent = 0;
    while (position < length)
    {
        if (position == nextEvent)
        {
            if (position == 0 || (repeats && position < rate * 3 / 10))
            {
                engine->noteOn(note, 0.9f);
                if (removePickRidges)
                    TestAccess::removePickRidges(*engine);
                nextEvent = position + (repeats ? rate / 10 : rate * 8 / 100);
            }
            else if (noteOff)
            {
                engine->allNotesOff();
                if (removeReleaseCooling)
                    TestAccess::removeReleaseCooling(*engine);
                nextEvent = length;
            }
            else
            {
                nextEvent = length;
            }
        }
        const int count = std::min({ block, length - position, nextEvent - position });
        engine->process(left.data(), right.data(), count);
        std::copy_n(left.data(), count, result.dry.data() + position);
        effects.process(left.data(), right.data(), count);
        std::copy_n(left.data(), count, result.modern.data() + position);
        position += count;
    }

    for (const auto* tap : { &result.dry, &result.modern })
    {
        expect(std::all_of(tap->begin(), tap->end(), [] (float sample)
        {
            return std::isfinite(sample) && std::abs(sample) < 1.0f;
        }), "ordinary contact remains finite with dry and Modern headroom");
    }
    engine->reset();
    expect(TestAccess::contactStateIsClear(*engine), "reset clears contact state");
    return result;
}

double rms(const std::vector<float>& signal, int rate, double start, double end)
{
    double sum = 0.0;
    int count = 0;
    for (int i = static_cast<int>(rate * start); i < static_cast<int>(rate * end); ++i)
    {
        sum += static_cast<double>(signal[static_cast<std::size_t>(i)])
             * signal[static_cast<std::size_t>(i)];
        ++count;
    }
    return std::sqrt(sum / count);
}

double differenceRms(const std::vector<float>& signal, const std::vector<float>& reference,
                     int rate, double start, double end)
{
    std::vector<float> difference(signal.size());
    for (std::size_t i = 0; i < signal.size(); ++i)
        difference[i] = signal[i] - reference[i];
    return rms(difference, rate, start, end);
}

double relativeDb(double numerator, double denominator)
{
    return 20.0 * std::log10(std::max(numerator, 1.0e-30)
                           / std::max(denominator, 1.0e-30));
}

void saveRaw(const std::filesystem::path& path, const Render& result)
{
    for (const auto* suffix : { "dry", "amp" })
    {
        const auto& signal = std::string(suffix) == "dry" ? result.dry : result.modern;
        std::ofstream output(path.string() + "-" + suffix + ".f32", std::ios::binary);
        output.write(reinterpret_cast<const char*>(signal.data()),
                     static_cast<std::streamsize>(signal.size() * sizeof(float)));
        expect(static_cast<bool>(output), "write optional raw contact render");
    }
}

void checkContacts(int rate, int note, int stringIndex,
                   const std::filesystem::path& outputDirectory)
{
    const std::string fixture = std::to_string(rate) + "-" + std::to_string(note);
    const auto release = render(rate, note, stringIndex, 0.0f, Style::Sustain, false, false);
    const auto releaseControl = render(rate, note, stringIndex, 0.0f, Style::Sustain,
                                       false, true);
    expect(std::equal(release.dry.begin(), release.dry.begin() + rate * 8 / 100,
                      releaseControl.dry.begin()),
           "held note before Note Off remains unchanged: " + fixture);
    const double releaseDifference = relativeDb(
        differenceRms(release.dry, releaseControl.dry, rate, 0.09, 0.18),
        rms(releaseControl.dry, rate, 0.09, 0.18));
    std::cout << "RELEASE " << fixture << " difference " << releaseDifference
              << " dB; level " << relativeDb(rms(release.dry, rate, 0.09, 0.18),
                                              rms(releaseControl.dry, rate, 0.09, 0.18))
              << " dB\n";
    expect(releaseDifference > -55.0, "release has a measurable waveform change: " + fixture);

    const auto pick = render(rate, note, stringIndex, 0.5f, Style::Sustain,
                             false, true, 257, true);
    const auto pickControl = render(rate, note, stringIndex, 0.5f, Style::Sustain,
                                    true, true, 257, true);
    const double pickDifference = relativeDb(
        differenceRms(pick.dry, pickControl.dry, rate, 0.0, 0.03),
        rms(pickControl.dry, rate, 0.0, 0.03));
    const double pickLevelChange = relativeDb(rms(pick.dry, rate, 0.0, 0.03),
                                             rms(pickControl.dry, rate, 0.0, 0.03));
    std::cout << "PICK " << fixture << " difference " << pickDifference
              << " dB; level " << pickLevelChange << " dB\n";
    if (stringIndex == 7)
    {
        expect(pick.dry == pickControl.dry, "plain-string pick remains identical: " + fixture);
    }
    else
    {
        expect(pickDifference > -45.0, "wound pick has a measurable onset change: " + fixture);
        expect(std::abs(pickLevelChange) < 0.25, "pick texture preserves onset level: " + fixture);
    }

    const auto smallBlocks = render(rate, note, stringIndex, 0.5f, Style::Sustain,
                                    false, true, 17, true);
    expect(smallBlocks.dry == pick.dry && smallBlocks.modern == pick.modern,
           "contact rendering is callback-identical dry and through Modern: " + fixture);
    for (Style style : { Style::PalmMute, Style::Dead, Style::Hammer })
    {
        const auto excluded = render(rate, note, stringIndex, 0.5f, style, false, false);
        const auto control = render(rate, note, stringIndex, 0.5f, style, true, true);
        expect(excluded.dry == control.dry, "excluded contact paths remain identical: " + fixture);
    }

    const auto fingerOnly = render(rate, note, stringIndex, 0.0f, Style::Sustain,
                                   false, true, 257, false, true, 0.4f);
    const auto fingerControl = render(rate, note, stringIndex, 0.0f, Style::Sustain,
                                      true, true, 257, false, true, 0.4f);
    expect(fingerOnly.dry == fingerControl.dry,
           "zero Pick Noise does not recolor a finger landing: " + fixture);
    const auto tinyPick = render(rate, note, stringIndex, 1.0e-6f, Style::Sustain,
                                 false, true, 257, false, true, 0.4f);
    const auto tinyControl = render(rate, note, stringIndex, 1.0e-6f, Style::Sustain,
                                    true, true, 257, false, true, 0.4f);
    expect(relativeDb(differenceRms(tinyPick.dry, tinyControl.dry, rate, 0.0, 0.03),
                      rms(tinyControl.dry, rate, 0.0, 0.03)) < -70.0,
           "ridge texture tends continuously to zero beside a finger landing: " + fixture);
    const auto zeroNoise = render(rate, note, stringIndex, 0.0f, Style::Sustain, false, true);
    const auto zeroControl = render(rate, note, stringIndex, 0.0f, Style::Sustain, true, true);
    expect(zeroNoise.dry == zeroControl.dry, "zero Pick Noise bypasses ridge texture: " + fixture);

    if (! outputDirectory.empty())
    {
        saveRaw(outputDirectory / (fixture + "-release-candidate"), release);
        saveRaw(outputDirectory / (fixture + "-release-baseline"), releaseControl);
        saveRaw(outputDirectory / (fixture + "-pick-candidate"), pick);
        saveRaw(outputDirectory / (fixture + "-pick-baseline"), pickControl);
    }
}
}

int main(int argc, char** argv)
{
    const std::filesystem::path outputDirectory = argc > 1 ? argv[1] : "";
    if (! outputDirectory.empty())
        std::filesystem::create_directories(outputDirectory);

    for (int rate : { 44100, 48000, 96000 })
    {
        const auto response = TestAccess::measureReleaseOperator(rate);
        expect(response.maximumGain <= 1.000002f,
               "cubic release operator is contractive across Nyquist at " + std::to_string(rate));
        expect(response.maximumComplexError <= 2.0e-6,
               "cubic release operator adds no relative phase at " + std::to_string(rate));
        for (const auto [note, stringIndex] :
             { std::pair { 28, 0 }, std::pair { 40, 2 }, std::pair { 64, 7 } })
            checkContacts(rate, note, stringIndex, outputDirectory);
    }
    std::cout << "Failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
