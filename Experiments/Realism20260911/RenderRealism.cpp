// Standalone before/after probes for the September 11 fifth realism pass.
// Build against either archived or current Source with the same production
// definitions. Every score produces simultaneous unnormalised pre/post-FX taps.
// For example, from the repository root:
// clang++ -std=c++20 -O2 -DNDEBUG -DELECTRY_DECOUPLED_PICK_RELEASE=1
//   -DELECTRY_MEASURED_BODY_RESPONSE=1 -DELECTRY_ENERGY_ATTACK_PITCH=1 -ISource
//   Experiments/Realism20260908Pass4/RenderRealism.cpp Source/DSP/ElectryEngine.cpp
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
    explicit Take(std::string name, float pickHardness = 0.85f,
                  int stringMask = 1, float spread = 0.018f) : id(std::move(name))
    {
        engine.prepare(rate, block);
        engine.setVariationSeed(0);
        auto settings = guitar();
        settings.pickHardness = pickHardness;
        settings.strumSpreadSeconds = spread;
        engine.setParameters(settings);
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
        engine.setSoloStringMask(static_cast<std::uint8_t>(stringMask));
        events.push_back({position(), "solo_mask", -1, static_cast<float>(stringMask)});
        events.push_back({position(), "strum_spread_seconds", -1, spread});
        control("pick_style", ElectryEngine::firstKeyswitchNote
                             + static_cast<int>(PickStyle::Alternate));
        events.push_back({ position(), "pick_position", -1, settings.pickPosition });
        events.push_back({ position(), "pick_hardness", -1, pickHardness });
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

    void chord(std::initializer_list<int> notes, float velocity)
    {
        std::vector<ElectryEngine::NoteOnEvent> input;
        for (int note : notes)
        {
            input.push_back({ note, velocity });
            events.push_back({ position(), "note_on", note, velocity });
        }
        engine.noteOnChord(input);
    }

    void pressure(float value)
    {
        events.push_back({ position(), "palm_pressure", -1, value });
        engine.setPalmMutePressure(value);
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
    if (argc != 2) return 2;
    try
    {
        const std::filesystem::path directory(argv[1]);
        std::filesystem::create_directories(directory);
        std::ofstream manifest(directory / "manifest.json");
        manifest << std::setprecision(9)
            << "{\n  \"schema\": \"electry-realism/20260911-v1\",\n"
               "  \"sample_rate\": 44100, \"block_size\": 256, \"variation_seed\": 0,\n"
               "  \"format\": \"mono IEEE float32 WAV; no normalisation\",\n"
               "  \"pitch_convention\": \"engine sounding MIDI; host is +12\",\n"
               "  \"string\": \"solo_mask event: 1 low E, 4 E2, 128 high E, 0 automatic chord assignment\",\n"
               "  \"fx_parameters\": {\"distortion\": 0.45, \"amp\": 0.95,"
               " \"amp_model\": \"ModernHighGain\", \"compressor\": 0.60,"
               " \"delay\": 0, \"room\": 0, \"oversampling\": \"Standard\"},\n"
               "  \"performance_controls\": {\"pitch_bend\": 0, \"resonance\": 0,"
               " \"palm_pressure\": 0, \"vibrato\": 0, \"sustain\": false,"
               " \"acoustic_return_level\": 1},\n";
        parameters(manifest);
        manifest << "  \"takes\": [\n";
        bool first = true;
        auto save = [&](const Take& take) { writeTake(manifest, directory, take, first); };
        for (float hardness : {0.15f, 0.50f, 0.95f})
        {
            Take t("pick_edge_" + std::to_string(static_cast<int>(hardness * 100)), hardness);
            for (float velocity : {0.55f, 0.90f, 1.0f})
            { t.on(28, velocity); t.wait(0.36); t.off(28); t.wait(0.18); }
            t.wait(0.5); save(t);
        }
        for (int note : {28, 40, 64})
        {
            const int mask = note == 28 ? 1 : (note == 40 ? 4 : 128);
            Take t("sustained_note_" + std::to_string(note), 0.85f, mask);
            t.on(note, 0.90f); t.wait(2.0); t.off(note); t.wait(0.5); save(t);
            Take stop("short_note_stops_" + std::to_string(note), 0.85f, mask);
            for (int fret : {0, 2, 5, 7})
            { stop.on(note + fret, 0.85f); stop.wait(0.13); stop.off(note + fret); stop.wait(0.20); }
            stop.wait(0.5); save(stop);
        }
        for (int fret : {0, 5, 12})
        {
            Take t("palm_chugs_fret_" + std::to_string(fret));
            t.style(PlayStyle::PalmMute);
            t.on(28 + fret, 0.90f); t.wait(0.125);
            for (int i = 0; i < 11; ++i) { t.repick(i % 4 == 0 ? 0.95f : 0.78f); t.wait(0.125); }
            t.off(28 + fret); t.wait(0.5); save(t);
        }
        {
            Take t("palm_riff_across_frets"); t.style(PlayStyle::PalmMute);
            for (int fret : {0, 0, 3, 5, 0, 0, 7, 5, 0, 0, 3, 5, 7, 5, 3, 0})
            { t.on(28 + fret, 0.88f); t.wait(0.105); t.off(28 + fret); t.wait(0.020); }
            t.wait(0.5); save(t);
        }
        for (auto pick : {PickStyle::Down, PickStyle::Up, PickStyle::Alternate})
        {
            Take t("power_chords_pick_" + std::to_string(static_cast<int>(pick)), 0.85f, 0, 0.032f);
            t.control("pick_style", ElectryEngine::firstKeyswitchNote + static_cast<int>(pick));
            for (int root : {28, 31, 33, 30})
            {
                t.chord({root, root + 7, root + 12}, 0.86f); t.wait(0.35);
                t.off(root); t.off(root + 7); t.off(root + 12); t.wait(0.15);
            }
            t.wait(0.5); save(t);
        }
        {
            Take t("zero_spread_chord_control", 0.85f, 0, 0.0f);
            t.chord({28, 35, 40}, 0.86f); t.wait(0.8);
            t.off(28); t.off(35); t.off(40); t.wait(0.5); save(t);
        }
        {
            Take t("open_to_muted_rhythm");
            for (int i = 0; i < 4; ++i)
            {
                t.style(PlayStyle::Sustain); t.on(28, 0.90f); t.wait(0.24);
                t.style(PlayStyle::PalmMute);
                for (int j = 0; j < 3; ++j) { t.repick(0.82f); t.wait(0.12); }
                t.off(28); t.wait(0.05);
            }
            t.wait(0.5); save(t);
        }
        {
            Take t("picked_lead_with_legato", 0.75f, 128);
            t.on(69, 0.86f); t.wait(0.2);
            t.style(PlayStyle::Hammer); t.on(71, 0.7f); t.wait(0.18); t.on(69, 0.7f); t.wait(0.18);
            t.style(PlayStyle::Slide); t.on(76, 0.8f); t.wait(0.3);
            t.style(PlayStyle::Sustain); t.repick(0.85f); t.wait(0.45); t.off(76); t.wait(0.5); save(t);
        }
        manifest << "\n  ]\n}\n";
        return manifest ? 0 : 1;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
