// Standalone before/after probes for the September 2026 realism pass.
// Build against either archived or current Source with the same production
// definitions. Every score produces simultaneous unnormalised pre/post-FX taps.
// For example, from the repository root:
// clang++ -std=c++20 -O2 -DNDEBUG -DELECTRY_DECOUPLED_PICK_RELEASE=1
//   -DELECTRY_MEASURED_BODY_RESPONSE=1 -ISource
//   Experiments/Realism20260908/RenderRealism.cpp Source/DSP/ElectryEngine.cpp
//   Source/DSP/ElectryFx.cpp Source/DSP/ElectryVisuals.cpp -o build/render-realism
// build/render-realism build/realism
#include "DSP/ElectryEngine.h"
#include "DSP/ElectryFx.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr int rate = 44100;
constexpr int block = 256;
constexpr double lead = 0.25;
using electry::ElectryEngine;
using electry::PickStyle;
using electry::PlayStyle;

struct Event
{
    int frame;
    std::string kind;
    int note;
    float value;
};

electry::EngineParameters guitar()
{
    electry::EngineParameters p;
    electry::applyGuitarBuild(p, electry::defaultGuitarBuild);
    p.pickupSelector = electry::PickupSelector::Bridge;
    p.outputMode = electry::OutputMode::Mono;
    p.toneKnob = 1.0f;
    p.stringAge = 0.10f;
    p.pickHardness = 0.85f;
    p.pickPosition = 0.18f;
    p.velocityAmount = 0.70f;
    p.sympatheticAmount = 0.0f;
    p.muteDamping = 0.85f;
    p.outputGain = 1.0f;
    p.fingerNoise = 0.55f;
    p.artifactAmount = 0.15f;
    return p;
}

electry::FxParameters amplifier()
{
    electry::FxParameters p;
    p.distortion = 0.45f;
    p.amp = 0.95f;
    p.ampModel = electry::AmpModel::ModernHighGain;
    p.compressor = 0.60f;
    return p;
}

void little(std::ostream& out, std::uint32_t value, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        out.put(static_cast<char>((value >> (i * 8)) & 255));
}

void wav(const std::filesystem::path& path, const std::vector<float>& samples)
{
    std::ofstream out(path, std::ios::binary);
    const auto size = static_cast<std::uint32_t>(samples.size() * sizeof(float));
    out.write("RIFF", 4); little(out, size + 36, 4); out.write("WAVEfmt ", 8);
    little(out, 16, 4); little(out, 3, 2); little(out, 1, 2);
    little(out, rate, 4); little(out, rate * 4, 4);
    little(out, 4, 2); little(out, 32, 2); out.write("data", 4);
    little(out, size, 4);
    for (const auto value : samples)
        little(out, std::bit_cast<std::uint32_t>(value), 4);
    out.close();
    if (! out)
        throw std::runtime_error("Could not write " + path.string());
}

class Take
{
public:
    explicit Take(std::string name) : id(std::move(name))
    {
        engine.prepare(rate, block);
        engine.setVariationSeed(0);
        engine.setParameters(guitar());
        engine.setResonance(0.0f);
        engine.setPalmMutePressure(0.0f);
        engine.setPitchBend(0.0f);
        engine.setVibrato(0.0f);
        engine.setSustainPedal(false);
        engine.reset();
        fx.prepare(rate);
        fx.setParameters(amplifier());
        fx.reset();
        engine.setAcousticReturnLevel(1.0f);
        // Lock all scores to physical string eight. F#1 is consequently fret 2
        // of the shipping Drop-E build, not a claim of alternate F# tuning.
        control("solo_string", ElectryEngine::firstSoloStringKeyswitchNote);
        control("pick_style", ElectryEngine::firstKeyswitchNote
                             + static_cast<int>(PickStyle::Alternate));
        wait(lead);
    }

    void control(const char* kind, int note)
    {
        events.push_back({ position(), kind, note, 1.0f });
        engine.noteOn(note, 1.0f);
    }

    void style(PlayStyle value)
    {
        control("play_style", ElectryEngine::firstPlayStyleKeyswitchNote
                              + static_cast<int>(value));
    }

    void on(int note, float velocity)
    {
        events.push_back({ position(), "note_on", note, velocity });
        const std::array<ElectryEngine::NoteOnEvent, 1> notes {{ { note, velocity } }};
        engine.noteOnChord(notes);
    }

    void off(int note)
    {
        events.push_back({ position(), "note_off", note, 0.0f });
        engine.noteOff(note);
    }

    void repick(float velocity)
    {
        events.push_back({ position(), "repick", ElectryEngine::firstRepickNote,
                           velocity });
        engine.noteOn(ElectryEngine::firstRepickNote, velocity);
    }

    int position() const { return static_cast<int>(dry.size()); }

    void wait(double seconds) { until(position() + static_cast<int>(std::lround(seconds * rate))); }

    void until(int end)
    {
        std::array<float, block> left {}, right {};
        while (position() < end)
        {
            const int count = std::min({ block, end - position(),
                std::max(1, engine.getAcousticReturnDelaySamples()) });
            engine.process(left.data(), right.data(), count);
            append(left, right, count, dry);
            fx.process(left.data(), right.data(), count);
            append(left, right, count, wet);
            engine.pushAcousticReturn(left.data(), right.data(), count);
        }
    }

    std::string id;
    std::vector<Event> events;
    std::vector<float> dry, wet;

private:
    static void append(const std::array<float, block>& left,
                       const std::array<float, block>& right, int count,
                       std::vector<float>& destination)
    {
        for (int frame = 0; frame < count; ++frame)
        {
            const auto l = left[static_cast<std::size_t>(frame)];
            const auto r = right[static_cast<std::size_t>(frame)];
            if (! std::isfinite(l) || ! std::isfinite(r) || l != r)
                throw std::runtime_error("Non-finite or non-mono render");
            destination.push_back(l);
        }
    }

    ElectryEngine engine;
    electry::ElectryFx fx;
};

void writeTake(std::ostream& manifest, const std::filesystem::path& directory,
               const Take& take, bool& first)
{
    wav(directory / (take.id + "-dry.wav"), take.dry);
    wav(directory / (take.id + "-modern.wav"), take.wet);
    if (! first) manifest << ",\n";
    first = false;
    manifest << "    {\"id\": \"" << take.id << "\", \"frames\": "
             << take.dry.size() << ", \"dry\": \"" << take.id
             << "-dry.wav\", \"modern\": \"" << take.id
             << "-modern.wav\", \"events\": [";
    for (std::size_t i = 0; i < take.events.size(); ++i)
    {
        const auto& e = take.events[i];
        if (i != 0) manifest << ',';
        manifest << "\n      {\"frame\": " << e.frame << ", \"kind\": \""
                 << e.kind << "\", \"engine_note\": " << e.note
                 << ", \"value\": " << e.value << '}';
    }
    manifest << "\n    ]}";
    std::cout << take.id << ": " << take.dry.size() << " frames\n";
}

void parameters(std::ostream& out)
{
    const auto p = guitar();
    out << "  \"engine_parameters\": {\"pickup_selector\": \"bridge\","
           " \"output_mode\": \"mono\",\n";
#define PARAM(field) out << "    \"" #field "\": " << p.field << ",\n"
    PARAM(bodyWood); PARAM(bodySize); PARAM(bodyShape); PARAM(construction);
    PARAM(scaleLength); PARAM(pickupType); PARAM(toneKnob); PARAM(bodyResonance);
    PARAM(stringGauge); PARAM(stringAge); PARAM(pickPosition); PARAM(pickHardness);
    PARAM(pickNoise); PARAM(fingerNoise); PARAM(releaseNoise); PARAM(muteDamping);
    PARAM(bendTimeSeconds); PARAM(velocityAmount); PARAM(outputGain);
    PARAM(artifactAmount); PARAM(sympatheticAmount); PARAM(palmMute);
    PARAM(strumSpreadSeconds); PARAM(tremoloRateHz); PARAM(resonanceDepth);
#undef PARAM
    out << "    \"vibratoDepth\": " << p.vibratoDepth << "},\n";
}
} // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: RenderRealism output-directory\n";
        return 2;
    }
    try
    {
        const std::filesystem::path directory(argv[1]);
        std::filesystem::create_directories(directory);
        std::ofstream manifest(directory / "manifest.json");
        manifest << std::setprecision(9)
                 << "{\n  \"schema\": \"electry-realism/20260908-v1\",\n"
                    "  \"sample_rate\": 44100, \"block_size\": 256,"
                    " \"variation_seed\": 0,\n"
                    "  \"format\": \"mono IEEE float32 WAV; no normalisation\",\n"
                    "  \"pitch_convention\": \"engine sounding MIDI; host is +12\",\n"
                    "  \"string\": \"Drop-E physical string 8; F#1 is fret 2\",\n"
                    "  \"fx_parameters\": {\"distortion\": 0.45, \"amp\": 0.95,"
                    " \"amp_model\": \"ModernHighGain\", \"compressor\": 0.60,"
                    " \"delay\": 0, \"room\": 0, \"oversampling\": \"Standard\"},\n"
                    "  \"performance_controls\": {\"pitch_bend\": 0,"
                    " \"resonance\": 0, \"palm_pressure\": 0, \"vibrato\": 0,"
                    " \"sustain\": false, \"acoustic_return_level\": 1},\n"
                    "  \"compile_flags\": {\"measured_body\": "
                 << ELECTRY_MEASURED_BODY_RESPONSE << ", \"decoupled_pick\": "
                 << ELECTRY_DECOUPLED_PICK_RELEASE << "},\n";
        parameters(manifest);
        manifest << "  \"takes\": [\n";
        bool first = true;
        for (const int note : { 28, 30 })
        {
            const auto name = note == 28 ? "e1" : "fs1";
            for (const int hz : { 8, 12, 16 })
            {
                Take t(std::string(name) + "-palm-repick-" + std::to_string(hz) + "hz");
                t.style(PlayStyle::PalmMute);
                const int start = t.position();
                t.on(note, 0.95f);
                // Keep the fretting hand down: these are physical string
                // repicks, not note-offs masquerading as pick strokes.
                for (int hit = 1; hit < 2 * hz; ++hit)
                {
                    t.until(start + static_cast<int>(std::lround(
                        static_cast<double>(hit) * rate / hz)));
                    t.repick(hit % 2 == 0 ? 0.95f : 0.84f);
                }
                t.until(start + 2 * rate);
                t.off(note);
                t.wait(0.75);
                writeTake(manifest, directory, t, first);
            }
            for (const bool hard : { false, true })
            {
                Take t(std::string(name) + "-sustain-" + (hard ? "hard" : "soft"));
                t.style(PlayStyle::Sustain);
                t.on(note, hard ? 0.98f : 0.40f);
                t.wait(2.0);
                t.off(note);
                t.wait(0.75);
                writeTake(manifest, directory, t, first);
            }
            for (const bool latePalm : { false, true })
            {
                Take t(std::string(name) + (latePalm ? "-late-palm-release" : "-short-release"));
                t.style(latePalm ? PlayStyle::PalmMute : PlayStyle::Sustain);
                t.on(note, 0.95f);
                t.wait(latePalm ? 2.0 : 0.12);
                t.off(note);
                t.wait(0.75);
                writeTake(manifest, directory, t, first);
            }
        }
        for (const int note : { 30, 36, 42 })
        {
            Take t("reference-midi" + std::to_string(note) + "-held-1500ms");
            t.style(PlayStyle::Sustain);
            t.on(note, 0.95f);
            t.wait(1.5);
            t.off(note);
            t.wait(0.75);
            writeTake(manifest, directory, t, first);
        }
        for (const int interval : { 2, 7, 12 })
        {
            for (const bool rapid : { false, true })
            {
                Take t("e1-legato-plus" + std::to_string(interval)
                       + (rapid ? "-20ms" : "-300ms"));
                t.style(PlayStyle::Sustain);
                t.on(28, 0.90f);
                t.wait(rapid ? 0.02 : 0.30);
                t.style(PlayStyle::Hammer);
                t.on(28 + interval, 0.78f);
                t.off(28);
                t.wait(rapid ? 0.02 : 0.30);
                t.on(28, 0.72f);
                t.off(28 + interval);
                t.wait(0.50);
                t.off(28);
                t.wait(0.75);
                writeTake(manifest, directory, t, first);
            }
        }
        {
            Take t("e1-fs1-g1-legato");
            t.style(PlayStyle::Sustain);
            t.on(28, 0.90f);
            t.wait(0.30);
            t.style(PlayStyle::Hammer);
            t.on(30, 0.78f);
            t.off(28);
            t.wait(0.30);
            t.on(31, 0.82f);
            t.off(30);
            t.wait(0.30);
            t.on(30, 0.72f);
            t.off(31);
            t.wait(0.30);
            t.on(28, 0.75f);
            t.off(30);
            t.wait(0.50);
            t.off(28);
            t.wait(0.75);
            writeTake(manifest, directory, t, first);
        }
        manifest << "\n  ]\n}\n";
        manifest.close();
        if (! manifest) throw std::runtime_error("Could not write manifest");
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
