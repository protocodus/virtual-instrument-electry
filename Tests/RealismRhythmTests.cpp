#include "DSP/ElectryEngine.h"
#include "DSP/ElectryFx.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// The same render fixture also measures the pre-change source snapshot.
#ifndef ELECTRY_RHYTHM_CANDIDATE
#define ELECTRY_RHYTHM_CANDIDATE 1
#endif

namespace electry
{
struct ElectryEngineTestAccess
{
    struct Voice
    {
        int midiNote;
        float handBandCentre;
        float travelSpeed;
    };

    static std::vector<Voice> voices(const ElectryEngine& engine)
    {
        std::vector<Voice> result;
        for (const auto& voice : engine.voices_)
        {
            if (! voice.active)
                continue;
            result.push_back({
                voice.midiNote,
                voice.vertical.handLossShape.dipOmega
                    * static_cast<float>(engine.sampleRate_) / 6.28318530718f,
#if ELECTRY_RHYTHM_CANDIDATE
                voice.strokeTravelSpeedScale
#else
                1.0f
#endif
            });
        }
        return result;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;

int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (! condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::unique_ptr<Engine> makeEngine(double sampleRate, float spread,
                                 float pressure = 0.0f)
{
    auto engine = std::make_unique<Engine>();
    electry::EngineParameters parameters;
    parameters.strumSpreadSeconds = spread;
    parameters.palmMute = pressure;
    parameters.bodyResonance = 0.55f;
    parameters.artifactAmount = 0.3f;
    parameters.sympatheticAmount = 0.15f;
    parameters.outputGain = 0.5f;
    engine->setParameters(parameters);
    engine->prepare(sampleRate, 512);
    return engine;
}

void renderBlock(Engine& engine, std::vector<float>& output, int start,
                 int samples, int blockSize)
{
    std::array<float, 512> right {};
    for (int position = start; position < start + samples; position += blockSize)
    {
        const int count = std::min(blockSize, start + samples - position);
        engine.process(output.data() + position, right.data(), count);
    }
}

bool isFiniteAndBounded(const std::vector<float>& output)
{
    return std::all_of(output.begin(), output.end(), [] (float sample)
    {
        return std::isfinite(sample) && std::abs(sample) < 2.0f;
    });
}

// 0: open palm, 1: fret-five palm, 2/3: down/up strums, 4: fret-twelve
// pressure, 5: zero-spread chord, 6: accented palm-muted fret changes.
std::vector<float> renderScenario(double sampleRate, int scenario,
                                  int blockSize, bool modernFx)
{
    const auto time = [sampleRate] (double seconds)
    {
        return static_cast<int>(seconds * sampleRate);
    };
    auto engine = makeEngine(sampleRate,
        scenario == 2 || scenario == 3 ? 0.018f : 0.0f,
        scenario == 4 ? 0.48f : 0.0f);
    std::vector<float> output(static_cast<std::size_t>(time(2.4)), 0.0f);
    const auto render = [&] (int start, int count)
    {
        renderBlock(*engine, output, start, count, blockSize);
    };

    if (scenario == 0 || scenario == 1 || scenario == 4)
    {
        engine->setSoloStringMask(1);
        engine->noteOn(Engine::firstPlayStyleKeyswitchNote + 1, 1.0f);
        const int note = scenario == 0 ? 28 : scenario == 1 ? 33 : 40;
        engine->noteOn(note, 0.87f);
        render(0, time(1.5));
        engine->noteOff(note);
        render(time(1.5), static_cast<int>(output.size()) - time(1.5));
    }
    else if (scenario == 2 || scenario == 3 || scenario == 5)
    {
        if (scenario == 3)
            engine->noteOn(Engine::firstKeyswitchNote + 1, 1.0f);
        const std::array<Engine::NoteOnEvent, 4> notes {{
            { 28, 0.85f }, { 35, 0.85f }, { 40, 0.85f }, { 47, 0.85f }
        }};
        for (int stroke = 0; stroke < 4; ++stroke)
        {
            const int begin = time(0.55 * stroke);
            const int end = time(0.55 * (stroke + 1));
            engine->noteOnChord(notes);
            render(begin, time(0.4));
            for (const auto& note : notes)
                engine->noteOff(note.midiNote);
            render(begin + time(0.4), end - begin - time(0.4));
        }
        render(time(2.2), static_cast<int>(output.size()) - time(2.2));
    }
    else
    {
        engine->setSoloStringMask(1);
        engine->noteOn(Engine::firstPlayStyleKeyswitchNote + 1, 1.0f);
        constexpr std::array<int, 4> frets { 0, 3, 5, 7 };
        for (int stroke = 0; stroke < 12; ++stroke)
        {
            const int begin = time(0.16 * stroke);
            const int end = time(0.16 * (stroke + 1));
            const int note = 28 + frets[static_cast<std::size_t>(stroke % 4)];
            engine->noteOn(note, stroke % 4 == 0 ? 0.95f : 0.72f);
            render(begin, time(0.105));
            engine->noteOff(note);
            render(begin + time(0.105), end - begin - time(0.105));
        }
        render(time(1.92), static_cast<int>(output.size()) - time(1.92));
    }
    expect(isFiniteAndBounded(output), "finite bounded dry rhythm");

    if (modernFx)
    {
        auto effects = std::make_unique<electry::ElectryFx>();
        electry::FxParameters parameters;
        parameters.amp = 0.66f;
        parameters.distortion = 0.15f;
        parameters.ampModel = electry::AmpModel::ModernHighGain;
        effects->prepare(sampleRate);
        effects->setParameters(parameters);
        effects->reset();
        auto right = output;
        for (std::size_t position = 0; position < output.size(); position += 256)
        {
            const int count = static_cast<int>(
                std::min(std::size_t(256), output.size() - position));
            effects->process(output.data() + position, right.data() + position,
                             count);
        }
    }
    expect(isFiniteAndBounded(output), "finite bounded processed rhythm");
    return output;
}

void writeAudio(const std::filesystem::path& path,
                const std::vector<float>& output)
{
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(output.data()),
               static_cast<std::streamsize>(output.size() * sizeof(float)));
    expect(file.good(), "audio fixture saved");
}

#if ELECTRY_RHYTHM_CANDIDATE
void testContactGeometry(double sampleRate)
{
    for (const int fret : { 0, 5, 12, 19 })
    {
        auto engine = makeEngine(sampleRate, 0.0f);
        engine->setSoloStringMask(1);
        engine->noteOn(Engine::firstPlayStyleKeyswitchNote + 1, 1.0f);
        engine->noteOn(28 + fret, 0.85f);
        const auto voice = Access::voices(*engine).front();
        const float expected = 5.0f * 41.203445f
            * std::exp2(static_cast<float>(fret) / 12.0f);
        expect(std::abs(voice.handBandCentre - expected) < 0.05f,
               "palm preserves calibrated upper-mode curvature");
    }

    auto engine = makeEngine(sampleRate, 0.018f);
    const std::array<Engine::NoteOnEvent, 4> notes {{
        { 28, 0.85f }, { 35, 0.85f }, { 40, 0.85f }, { 47, 0.85f }
    }};
    engine->noteOnChord(notes);
    const auto voices = Access::voices(*engine);
    expect(voices.front().travelSpeed == 1.0f, "first contact unchanged");
    expect(voices.back().travelSpeed > 1.10f,
           "faster later crossings affect pick response");

    // A future stroke cannot replace a still-travelling contact's wrist speed.
    const auto pending = voices.back();
    engine->noteOn(60, 0.6f);
    bool pendingRetained = false;
    for (const auto& voice : Access::voices(*engine))
    {
        if (voice.midiNote != pending.midiNote)
            continue;
        pendingRetained = true;
        expect(voice.travelSpeed == pending.travelSpeed,
               "scheduled contact retains its wrist speed");
    }
    expect(pendingRetained, "unrelated note preserves the pending chord member");

    auto scalar = makeEngine(sampleRate, 0.018f);
    scalar->setSoloStringMask(3);
    scalar->noteOn(35, 0.85f);
    expect(Access::voices(*scalar).front().travelSpeed == 1.0f,
           "provisional scalar leading contact starts at reference speed");
    scalar->noteOn(28, 0.85f);
    for (const auto& voice : Access::voices(*scalar))
        if (voice.midiNote == 35)
            expect(voice.travelSpeed > 1.0f,
                   "scalar reanchor updates the pending later contact speed");
}

void testSpreadContinuity(double sampleRate)
{
    const std::array<Engine::NoteOnEvent, 4> notes {{
        { 28, 0.85f }, { 35, 0.85f }, { 40, 0.85f }, { 47, 0.85f }
    }};
    auto simultaneous = makeEngine(sampleRate, 0.0f);
    auto tinySpread = makeEngine(sampleRate, 1.0e-12f);
    simultaneous->noteOnChord(notes);
    tinySpread->noteOnChord(notes);
    const auto sampleCount = static_cast<std::size_t>(0.25 * sampleRate);
    std::vector<float> reference(sampleCount), candidate(sampleCount);
    renderBlock(*simultaneous, reference, 0, static_cast<int>(sampleCount), 256);
    renderBlock(*tinySpread, candidate, 0, static_cast<int>(sampleCount), 37);
    expect(reference == candidate,
           "sub-sample positive Spread retains the simultaneous chord waveform");

    float previousSpeed = 1.0f;
    float resolvedSpeed = 1.0f;
    for (const float spread : { 0.0f, 1.0e-6f, 0.0001f, 0.0005f, 0.001f,
                               0.002f, 0.003f, 0.005f, 0.018f, 0.032f })
    {
        auto engine = makeEngine(sampleRate, spread);
        engine->noteOnChord(notes);
        const float speed = Access::voices(*engine).back().travelSpeed;
        expect(speed >= previousSpeed,
               "short-Spread attack expression grows monotonically");
        if (spread == 0.003f)
            resolvedSpeed = speed;
        if (spread >= 0.003f)
            expect(speed == resolvedSpeed,
                   "resolved strums retain their complete wrist-speed response");
        previousSpeed = speed;
    }
    expect(resolvedSpeed > 1.10f, "continuity blend retains audible strum response");
}
#endif
}

int main(int argc, char** argv)
{
    const bool exportAudio = argc > 1;
    const std::filesystem::path outputDirectory = exportAudio ? argv[1] : ".";
    if (exportAudio)
        std::filesystem::create_directories(outputDirectory);

    for (const double sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        for (int scenario = 0; scenario < 7; ++scenario)
        {
            const auto largeBlocks = renderScenario(sampleRate, scenario, 256, false);
            const auto smallBlocks = renderScenario(sampleRate, scenario, 37, false);
            expect(largeBlocks == smallBlocks,
                   "block partition exact for scenario " + std::to_string(scenario));
            const auto modern = renderScenario(sampleRate, scenario, 256, true);
            if (exportAudio)
            {
                const auto stem = std::to_string(static_cast<int>(sampleRate))
                                + "-" + std::to_string(scenario);
                writeAudio(outputDirectory / (stem + "-dry.f32"), largeBlocks);
                writeAudio(outputDirectory / (stem + "-modern.f32"), modern);
            }
        }
#if ELECTRY_RHYTHM_CANDIDATE
        testContactGeometry(sampleRate);
        testSpreadContinuity(sampleRate);
#endif
    }
    std::cout << "Rhythm tests " << (failures ? "FAILED" : "PASSED")
              << "; failures=" << failures << '\n';
    return failures ? 1 : 0;
}
