// CPU and output-parity harness for the whole JUCE-free instrument: the four
// factory rigs played through ElectryPerformer (strings, pickups, Performer
// MIDI handling and the FX chain), exactly the code the Rack Extension
// compiles.
//
//   ElectryBenchmarkPerformer [options] <Docs/audio/midi> [out.f32]
//     --seconds S     render at most S seconds of each rig's demo (default 6)
//     --rate R        sample rate (default 48000)
//     --block N       frames per render call (default 64, the Rack's batch)
//     --fx-quality Q  standard (shipping default) or high
//     --rigs 0123     which rigs to render, in program order
//     --amp M         american, british or modern: override the FX rigs' amp
//                     voice (coverage for parity checks; not a factory rig)
//
// It prints one FNV-1a hash per rig over the float32 output bits and, when a
// path is given, writes every rig's interleaved stereo float32 output there
// (rig 0 first) for a byte comparison or a null test against another build.
//
// Measurement protocol (results: Docs/benchmarks/cpu-exact-trims-2026-10-25.md):
// - The regression stream is fixed: each rig's committed demo MIDI from
//   Docs/audio/midi, events delivered on their own sample, the rig's factory
//   parameters as RenderPresetDemos loads them (FX on for Drop-E Metal and
//   Blues Rock Lead), identical rate, block and length on every build. The
//   engine has no seed input; its noise sources are deterministic from reset.
// - Instruction counts come from `valgrind --tool=callgrind` on this program
//   (Ir of the whole run, setup included; setup is a small constant). They
//   are reproducible on a shared machine, where timings are not.
// - The primary workload is the Rack-like build: clang, C++17, -O3
//   -fno-builtin, ELECTRY_EMBEDDED_TARGET=1 (configure with
//   -DELECTRY_EMBEDDED_TARGET=ON). The host build is GCC -O3 Release.
// - Parity: the float32 file of a candidate must equal the baseline byte for
//   byte on every rig ("last bit"); otherwise the residual is reported per rig
//   in dB below the signal RMS and as a peak in dBFS.
//
// Rig parameters mirror Source/PluginProcessor.cpp (defaults from the
// engine's own structs plus each rig's overrides); this tool exists for
// regression comparison, so a drift from the processor changes only which
// stream is measured, never a shipped sound.
#include "DSP/ElectryPerformer.h"
#include "PresetDemoRenderer.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace
{
using electry::AmpModel;
using electry::ElectryEngine;
using electry::ElectryPerformer;
using electry::EngineParameters;
using electry::FxOversampling;
using electry::FxParameters;
using electry::PickupSelector;

constexpr const char* rigStems[] = {
    "01-factory-default", "02-drop-e-metal", "03-mute-dead-di",
    "04-blues-rock-lead"
};

float decibelsToGain(float decibels)
{
    return decibels > -100.0f ? std::pow(10.0f, decibels * 0.05f) : 0.0f;
}

struct Rig
{
    EngineParameters engine {};
    FxParameters effects {};
    int outputMode = 0;
    bool fxEnabled = false;
};

Rig rigFor(int index, FxOversampling quality)
{
    Rig rig;
    electry::applyGuitarBuild(rig.engine, electry::defaultGuitarBuild);
    rig.engine.outputGain = decibelsToGain(-6.0f);
    rig.effects.oversampling = quality;
    auto& e = rig.engine;
    auto& f = rig.effects;
    if (index == 1) // Drop-E Metal
    {
        e.toneKnob = 1.00f; e.stringAge = 0.10f; e.pickHardness = 0.85f;
        e.fingerNoise = 0.55f; e.muteDamping = 0.85f; e.velocityAmount = 0.70f;
        e.outputGain = decibelsToGain(6.0f); e.artifactAmount = 0.15f;
        f.distortion = 0.45f; f.amp = 0.95f; f.compressor = 0.60f;
        e.sympatheticAmount = 0.25f;
        rig.fxEnabled = true;
    }
    else if (index == 2) // Mute / Dead DI
    {
        e.pickPosition = 0.20f; e.pickHardness = 0.82f; e.fingerNoise = 0.55f;
        e.outputGain = decibelsToGain(6.0f); e.artifactAmount = 0.15f;
        e.sympatheticAmount = 0.00f;
    }
    else if (index == 3) // Blues Rock Lead
    {
        e.pickupSelector = PickupSelector::Neck; e.pickupType = 0.56f;
        e.toneKnob = 0.78f; e.bodyResonance = 0.62f; e.stringAge = 0.20f;
        e.pickPosition = 0.38f; e.pickHardness = 0.52f; e.fingerNoise = 0.48f;
        e.bendTimeSeconds = 0.15f; e.velocityAmount = 0.90f;
        e.outputGain = decibelsToGain(3.2f); rig.outputMode = 1;
        f.distortion = 0.14f; f.amp = 0.62f; f.compressor = 0.28f;
        f.delay = 0.18f; f.room = 0.28f; e.sympatheticAmount = 0.35f;
        rig.fxEnabled = true;
    }
    return rig;
}

bool isConditioning(const protocodus::demos::MidiEvent& event)
{
    const unsigned kind = event.bytes[0] & 0xf0u;
    if ((kind == 0x90u || kind == 0x80u) && event.size >= 3)
        return ElectryPerformer::isAttackConditioningNote(event.bytes[1]);
    return kind == 0xb0u && event.size >= 3 && event.bytes[1] == 2u;
}

void dispatch(ElectryPerformer& performer,
              const protocodus::demos::MidiEvent& event)
{
    const unsigned kind = event.bytes[0] & 0xf0u;
    const int data1 = event.bytes[1] & 0x7f;
    const int data2 = event.size >= 3 ? event.bytes[2] & 0x7f : 0;
    if (kind == 0x90u && data2 != 0)
        performer.noteOn(data1, static_cast<float>(data2) / 127.0f);
    else if (kind == 0x90u || kind == 0x80u)
        performer.noteOff(data1);
    else if (kind == 0xb0u && data1 == 64)
        performer.setSustainPedal(data2 >= 64);
    else if (kind == 0xb0u && data1 == 1)
        performer.setResonance(static_cast<float>(data2) / 127.0f);
    else if (kind == 0xb0u && data1 == 2)
        performer.setPalmMutePressure(static_cast<float>(data2) / 127.0f);
    else if (kind == 0xe0u)
    {
        const int value14 = data1 | (data2 << 7);
        const float bend = value14 < 8192
            ? static_cast<float>(value14 - 8192) / 8192.0f
            : static_cast<float>(value14 - 8192) / 8191.0f;
        performer.setPitchBend(bend);
    }
}

// The processor's same-sample handling, reduced to what the demo phrases
// use: keyswitches and CC2 condition every attack at their timestamp, then
// playable note-ons at one sample form one chord.
void dispatchGroup(ElectryPerformer& performer,
                   const std::vector<protocodus::demos::MidiEvent>& events,
                   std::size_t begin, std::size_t end)
{
    for (std::size_t i = begin; i < end; ++i)
        if (isConditioning(events[i]))
            dispatch(performer, events[i]);
    ElectryEngine::NoteOnEvent chord[128];
    std::size_t chordSize = 0;
    const auto flush = [&] {
        if (chordSize != 0)
            performer.noteOnChord(chord, chordSize);
        chordSize = 0;
    };
    for (std::size_t i = begin; i < end; ++i)
    {
        const auto& event = events[i];
        if (isConditioning(event))
            continue;
        const unsigned kind = event.bytes[0] & 0xf0u;
        if (kind == 0x90u && event.size >= 3 && event.bytes[2] != 0
            && ElectryEngine::isMidiPlayableNote(event.bytes[1]) && chordSize < 128)
        {
            chord[chordSize++] = { event.bytes[1] & 0x7f,
                                   static_cast<float>(event.bytes[2] & 0x7f) / 127.0f,
                                   ElectryEngine::legacyExpressionId };
            continue;
        }
        flush();
        dispatch(performer, event);
    }
    flush();
}
} // namespace

int main(int argc, char** argv)
{
    double seconds = 6.0;
    double rate = 48000.0;
    int block = 64;
    FxOversampling quality = FxOversampling::Standard;
    std::string rigs = "0123";
    int ampOverride = -1;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        const auto next = [&]() -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "%s needs a value\n", arg.c_str()); std::exit(2); }
            return argv[++i];
        };
        if (arg == "--seconds") seconds = std::atof(next().c_str());
        else if (arg == "--rate") rate = std::atof(next().c_str());
        else if (arg == "--block") block = std::atoi(next().c_str());
        else if (arg == "--fx-quality")
            quality = next() == "high" ? FxOversampling::High : FxOversampling::Standard;
        else if (arg == "--rigs") rigs = next();
        else if (arg == "--amp")
        {
            const std::string model = next();
            ampOverride = model == "american" ? 0 : model == "british" ? 1 : 2;
        }
        else positional.push_back(arg);
    }
    if (positional.empty() || block < 1 || !(rate > 0.0))
    {
        std::fprintf(stderr, "usage: %s [options] <midi-dir> [out.f32]\n", argv[0]);
        return 2;
    }
    std::FILE* out = positional.size() > 1 ? std::fopen(positional[1].c_str(), "wb") : nullptr;
    if (positional.size() > 1 && out == nullptr)
    {
        std::fprintf(stderr, "cannot write %s\n", positional[1].c_str());
        return 1;
    }

    for (const char rigChar : rigs)
    {
        const int rigIndex = rigChar - '0';
        if (rigIndex < 0 || rigIndex > 3)
            continue;
        const auto events = protocodus::demos::ReadMidi(
            positional[0] + "/" + rigStems[rigIndex] + ".mid");
        Rig rig = rigFor(rigIndex, quality);
        if (ampOverride >= 0)
            rig.effects.ampModel = static_cast<AmpModel>(ampOverride);

        auto performer = std::make_unique<ElectryPerformer>();
        performer->prepare(rate, block);
        performer->setEngineParameters(rig.engine, rig.outputMode);
        performer->setEffectParameters(rig.effects, rig.fxEnabled);
        performer->reset();

        const double end = std::min(seconds, events.back().seconds
                                                 + protocodus::demos::kTailSeconds);
        const auto frames = static_cast<std::size_t>(std::llround(end * rate));
        std::vector<float> left(frames), right(frames), interleaved(2 * frames);
        const auto due = [&](std::size_t i) {
            return static_cast<std::size_t>(std::llround(events[i].seconds * rate));
        };
        std::size_t next = 0, done = 0;
        while (done < frames)
        {
            const std::size_t blockEnd = std::min(frames, done + static_cast<std::size_t>(block));
            while (done < blockEnd)
            {
                if (next < events.size() && due(next) <= done)
                {
                    std::size_t groupEnd = next;
                    while (groupEnd < events.size() && due(groupEnd) == due(next))
                        ++groupEnd;
                    dispatchGroup(*performer, events, next, groupEnd);
                    next = groupEnd;
                }
                const std::size_t until = next < events.size()
                    ? std::min(blockEnd, std::max(done + 1, due(next))) : blockEnd;
                performer->render(&left[done], &right[done], static_cast<int>(until - done));
                done = until;
            }
        }

        std::uint64_t hash = 1469598103934665603ull;
        double sumSquares = 0.0;
        for (std::size_t i = 0; i < frames; ++i)
        {
            interleaved[2 * i] = left[i];
            interleaved[2 * i + 1] = right[i];
        }
        for (const float sample : interleaved)
        {
            std::uint32_t bits;
            std::memcpy(&bits, &sample, sizeof bits);
            for (int b = 0; b < 4; ++b)
                hash = (hash ^ ((bits >> (8 * b)) & 0xffu)) * 1099511628211ull;
            sumSquares += static_cast<double>(sample) * sample;
        }
        const double rms = std::sqrt(sumSquares / static_cast<double>(interleaved.size()));
        std::printf("%-20s %8zu frames  rms %7.2f dBFS  fnv %016llx\n",
                    rigStems[rigIndex], frames,
                    20.0 * std::log10(std::max(rms, 1.0e-12)),
                    static_cast<unsigned long long>(hash));
        if (out != nullptr)
            std::fwrite(interleaved.data(), sizeof(float), interleaved.size(), out);
    }
    if (out != nullptr)
        std::fclose(out);
    return 0;
}
