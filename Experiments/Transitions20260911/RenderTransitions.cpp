// Original, continuous picked-chord benchmark. See README.md for reproduction.
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

namespace electry {
struct ElectryEngineTestAccess {
    static void verifyOwners(const ElectryEngine& engine, const std::array<int, 8>& held) {
        for (std::size_t string = 0; string < held.size(); ++string) {
            const int count = held[string] < 0 ? 0 : 1;
            if (engine.heldNoteCounts_[string] != count
                || (count != 0 && engine.heldMidiNotes_[string] != held[string]))
                throw std::runtime_error("Physical fretting ownership disagrees with the benchmark score");
        }
    }
};
}

namespace {
constexpr int rate = 44100, block = 256;
constexpr double lead = 0.25, tempo = 105.0, beat = 60.0 / tempo;
using electry::ElectryEngine;
using electry::PlayStyle;
using Shape = std::array<int, 8>;
struct Event { int frame; std::string kind; int note; float value; int string = -1; };
struct Marker { int frame; std::string label; };

electry::EngineParameters guitar(float sympathetic) {
    electry::EngineParameters p;
    electry::applyGuitarBuild(p, electry::defaultGuitarBuild);
    p.pickupSelector = electry::PickupSelector::Bridge;
    p.outputMode = electry::OutputMode::Mono;
    p.toneKnob = 0.85f; p.stringAge = 0.15f;
    p.pickHardness = 0.72f; p.pickPosition = 0.18f;
    p.velocityAmount = 0.70f; p.sympatheticAmount = sympathetic;
    p.muteDamping = 0.72f; p.outputGain = 1.0f;
    p.fingerNoise = 0.40f; p.artifactAmount = 0.15f;
    // The score supplies explicit crossing times; a second scheduler must not
    // add its own spread to already placed attacks.
    p.strumSpreadSeconds = 0.0f;
    return p;
}

electry::FxParameters amp(bool modern) {
    electry::FxParameters p;
    p.ampModel = modern ? electry::AmpModel::ModernHighGain
                        : electry::AmpModel::BritishCrunch;
    p.distortion = modern ? 0.45f : 0.0f;
    p.amp = modern ? 0.95f : 0.50f;
    p.compressor = modern ? 0.60f : 0.10f;
    return p;
}

void little(std::ostream& out, std::uint32_t value, int bytes) {
    for (int i = 0; i < bytes; ++i) out.put(static_cast<char>((value >> (i * 8)) & 255));
}
void wav(const std::filesystem::path& path, const std::vector<float>& samples) {
    std::ofstream out(path, std::ios::binary);
    const auto size = static_cast<std::uint32_t>(samples.size() * sizeof(float));
    out.write("RIFF", 4); little(out, size + 36, 4); out.write("WAVEfmt ", 8);
    little(out, 16, 4); little(out, 3, 2); little(out, 1, 2);
    little(out, rate, 4); little(out, rate * 4, 4);
    little(out, 4, 2); little(out, 32, 2); out.write("data", 4); little(out, size, 4);
    for (float sample : samples) little(out, std::bit_cast<std::uint32_t>(sample), 4);
    out.close();
    if (!out) throw std::runtime_error("Could not write " + path.string());
}

class Take {
public:
    Take(std::string name, std::string titleText, std::string descriptionText,
         float sympathetic = 0.0f)
        : id(std::move(name)), title(std::move(titleText)), description(std::move(descriptionText)),
          settings(guitar(sympathetic)) {
        held.fill(-1);
        engine.prepare(rate, block); engine.setVariationSeed(0);
        engine.setParameters(settings); engine.setResonance(0.0f);
        engine.setPalmMutePressure(0.0f); engine.setPitchBend(0.0f);
        engine.setVibrato(0.0f); engine.setSustainPedal(false); engine.reset();
        // Every FX tap receives precisely the same DI. With feedback disabled,
        // an amp's gain cannot alter the excitation being compared in another tap.
        engine.setAcousticReturnLevel(0.0f);
        crunch.prepare(rate); crunch.setParameters(amp(false)); crunch.reset();
        modern.prepare(rate); modern.setParameters(amp(true)); modern.reset();
        engine.noteOn(ElectryEngine::firstKeyswitchNote + 2, 1.0f);
        events.push_back({0, "pick_style", ElectryEngine::firstKeyswitchNote + 2, 1.0f});
        wait(lead);
    }

    int position() const { return static_cast<int>(dry.size()); }
    int at(double beats) const { return static_cast<int>(std::lround((lead + beats * beat) * rate)); }
    void time(double seconds) { until(static_cast<int>(std::lround(seconds * rate))); }
    void wait(double seconds) { until(position() + static_cast<int>(std::lround(seconds * rate))); }
    void mark(const std::string& label) { markers.push_back({position(), label}); }
    void style(PlayStyle style) {
        const int key = ElectryEngine::firstPlayStyleKeyswitchNote + static_cast<int>(style);
        engine.noteOn(key, 1.0f); events.push_back({position(), "play_style", key, 1.0f});
    }
    void pick(electry::PickStyle stroke) {
        const int key = ElectryEngine::firstKeyswitchNote + static_cast<int>(stroke);
        engine.noteOn(key, 1.0f); events.push_back({position(), "pick_style", key, 1.0f});
    }
    void pressure(float value) {
        engine.setPalmMutePressure(value); events.push_back({position(), "palm_pressure", -1, value});
    }
    void on(int string, int note, float velocity) {
        if (held[static_cast<std::size_t>(string)] >= 0) off(string);
        engine.setSoloStringMask(static_cast<std::uint8_t>(1 << string));
        const std::array<ElectryEngine::NoteOnEvent, 1> notes {{{note, velocity}}};
        engine.noteOnChord(notes);
        engine.setSoloStringMask(0);
        held[static_cast<std::size_t>(string)] = note;
        events.push_back({position(), "note_on", note, velocity, string});
        electry::ElectryEngineTestAccess::verifyOwners(engine, held);
    }
    void off(int string) {
        auto& note = held[static_cast<std::size_t>(string)];
        if (note < 0) return;
        engine.noteOff(note); events.push_back({position(), "note_off", note, 0.0f, string}); note = -1;
        electry::ElectryEngineTestAccess::verifyOwners(engine, held);
    }
    void repick(int string, float velocity) {
        if (held[static_cast<std::size_t>(string)] < 0)
            throw std::runtime_error("The score repicks an unheld string");
        const int key = ElectryEngine::firstRepickNote + string;
        engine.noteOn(key, velocity); events.push_back({position(), "repick", key, velocity, string});
        electry::ElectryEngineTestAccess::verifyOwners(engine, held);
    }
    void finish(double tail = 0.7) {
        mark("Fretting hand releases");
        for (int string = 0; string < 8; ++string) off(string);
        wait(tail);
    }

    void shape(const Shape& next, float velocity) {
        const int start = position();
        pick(electry::PickStyle::Down);
        // A chord change removes only changed fretting fingers. Shared notes
        // retain their owner and waveguide state, including each D4 common tone.
        for (int string = 0; string < 8; ++string)
            if (held[static_cast<std::size_t>(string)] != next[static_cast<std::size_t>(string)])
                off(string);
        int crossed = 0;
        for (int string = 0; string < 8; ++string) {
            const int note = next[static_cast<std::size_t>(string)];
            if (note < 0) continue;
            until(start + static_cast<int>(std::lround(0.012 * crossed++ * rate)));
            if (held[static_cast<std::size_t>(string)] == note) repick(string, velocity);
            else on(string, note, velocity);
        }
        pick(electry::PickStyle::Alternate);
    }

    void until(int end) {
        if (end < position()) throw std::runtime_error("Non-monotone score time");
        std::array<float, block> left{}, right{}, copyLeft{}, copyRight{};
        while (position() < end) {
            const int count = std::min(block, end - position());
            engine.process(left.data(), right.data(), count); append(left, right, count, dry);
            copyLeft = left; copyRight = right;
            crunch.process(copyLeft.data(), copyRight.data(), count);
            append(copyLeft, copyRight, count, crunchy);
            modern.process(left.data(), right.data(), count); append(left, right, count, heavy);
        }
    }

    std::string id, title, description;
    electry::EngineParameters settings;
    std::vector<Event> events;
    std::vector<Marker> markers;
    std::vector<float> dry, crunchy, heavy;
private:
    static void append(const std::array<float, block>& left, const std::array<float, block>& right,
                       int count, std::vector<float>& samples) {
        for (int i = 0; i < count; ++i) {
            const float l = left[static_cast<std::size_t>(i)], r = right[static_cast<std::size_t>(i)];
            if (!std::isfinite(l) || !std::isfinite(r) || l != r)
                throw std::runtime_error("Non-finite or non-mono render");
            samples.push_back(l);
        }
    }
    ElectryEngine engine;
    electry::ElectryFx crunch, modern;
    Shape held;
};

void parameters(std::ostream& out, const electry::EngineParameters& p) {
    out << "{\"pickup_selector\":\"Bridge\",\"output_mode\":\"Mono\",\n";
#define PARAM(field) out << "\"" #field "\":" << p.field << ",\n"
    PARAM(bodyWood); PARAM(bodySize); PARAM(bodyShape); PARAM(construction);
    PARAM(scaleLength); PARAM(pickupType); PARAM(toneKnob); PARAM(bodyResonance);
    PARAM(stringGauge); PARAM(stringAge); PARAM(pickPosition); PARAM(pickHardness);
    PARAM(pickNoise); PARAM(fingerNoise); PARAM(releaseNoise); PARAM(muteDamping);
    PARAM(bendTimeSeconds); PARAM(velocityAmount); PARAM(outputGain);
    PARAM(artifactAmount); PARAM(sympatheticAmount); PARAM(palmMute);
    PARAM(strumSpreadSeconds); PARAM(tremoloRateHz); PARAM(resonanceDepth);
#undef PARAM
    out << "\"vibratoDepth\":" << p.vibratoDepth << '}';
}

void save(std::ostream& out, const std::filesystem::path& directory, const Take& take, bool& first) {
    wav(directory / (take.id + "-dry.wav"), take.dry);
    wav(directory / (take.id + "-crunch.wav"), take.crunchy);
    wav(directory / (take.id + "-modern.wav"), take.heavy);
    if (!first) out << ",\n"; first = false;
    out << "{\"id\":\"" << take.id << "\",\"title\":\"" << take.title
        << "\",\"description\":\"" << take.description << "\",\"frames\":" << take.dry.size()
        << ",\"dry\":\"" << take.id << "-dry.wav\",\"crunch\":\"" << take.id
        << "-crunch.wav\",\"modern\":\"" << take.id << "-modern.wav\",\"engine_parameters\":";
    parameters(out, take.settings);
    out << ",\"markers\":[";
    for (std::size_t i = 0; i < take.markers.size(); ++i) {
        if (i) out << ',';
        out << "{\"frame\":" << take.markers[i].frame << ",\"label\":\"" << take.markers[i].label << "\"}";
    }
    out << "],\"events\":[";
    for (std::size_t i = 0; i < take.events.size(); ++i) {
        if (i) out << ',';
        const auto& e = take.events[i];
        out << "\n{\"frame\":" << e.frame << ",\"kind\":\"" << e.kind
            << "\",\"engine_note\":" << e.note << ",\"value\":" << e.value
            << ",\"string_index\":" << e.string << '}';
    }
    out << "]}";
    std::cout << take.id << ": " << take.dry.size() << " frames\n";
}

void rollingRiff(Take& t, bool low, bool pressureDriven) {
    const Shape d {{-1,-1,-1,-1,50,57,62,66}};
    const Shape c {{-1,-1,-1,48,52,55,62,67}};
    const Shape g {{-1,-1,43,47,50,55,62,67}};
    const std::array<Shape, 8> chords {{d,c,g,g,d,c,g,d}};
    const std::array<std::string, 8> labels {"D", "Cadd9", "G", "G", "D", "Cadd9", "G", "D"};
    const std::array<int, 8> roots {28,28,31,33,28,31,38,28};
    // This is an original rolling rhythm, not the referenced song's melody,
    // transcription, recording or signature guitar figure.
    for (int bar = 0; bar < 8; ++bar) {
        const double firstBeat = 4.0 * bar;
        t.until(t.at(firstBeat));
        t.style(PlayStyle::Sustain); t.pressure(0.0f);
        const int root = roots[static_cast<std::size_t>(bar)];
        Shape shape = low ? Shape{{root,root+7,root+12,-1,-1,-1,-1,-1}}
                          : chords[static_cast<std::size_t>(bar)];
        t.mark("Bar " + std::to_string(bar + 1) + " - "
               + (low ? "low power chord" : labels[static_cast<std::size_t>(bar)]) + " - open");
        t.shape(shape, bar % 2 == 0 ? 0.84f : 0.79f);
        int bass = 0;
        while (shape[static_cast<std::size_t>(bass)] < 0) ++bass;
        const std::array<int, 7> strings = low ? std::array<int,7>{2,0,1,0,2,0,1}
                                              : std::array<int,7>{6,5,7,bass,6,5,7};
        const std::array<float, 7> force {.70f,.78f,.66f,.86f,.73f,.80f,.71f};
        for (int eighth = 1; eighth < 8; ++eighth) {
            const double subdivision = eighth * 0.5;
            // Change hand position a little before the next attack, while
            // every fretting owner and every ongoing voice is still present.
            if (eighth == 2 || eighth == 4 || eighth == 6 || eighth == 7) {
                t.until(t.at(firstBeat + subdivision - 0.15));
                const bool muted = eighth == 2 || eighth == 6;
                t.mark(muted ? "Palm down" : "Palm lifts");
                if (pressureDriven) t.pressure(muted ? 0.68f : 0.0f);
                else t.style(muted ? PlayStyle::PalmMute : PlayStyle::Sustain);
            }
            t.until(t.at(firstBeat + subdivision));
            t.repick(strings[static_cast<std::size_t>(eighth - 1)], force[static_cast<std::size_t>(eighth - 1)]);
        }
    }
    t.until(t.at(32.0)); t.finish(0.9);
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: render-transitions OUTPUT_DIRECTORY\n"; return 2; }
    try {
        const std::filesystem::path directory(argv[1]); std::filesystem::create_directories(directory);
        std::ofstream manifest(directory / "manifest.json");
        manifest << std::setprecision(9)
            << "{\"schema\":\"electry-transitions/20260911-v1\",\"sample_rate\":44100,"
               "\"block_size\":256,\"variation_seed\":0,\"tempo_bpm\":105,\"beats_per_bar\":4,"
               "\"format\":\"mono IEEE float32 WAV; no normalisation\","
               "\"pitch_convention\":\"engine sounding MIDI; host playable and repick notes are +12\","
               "\"physical_strings\":[28,35,40,45,50,55,59,64],"
               "\"note_on_routing\":\"Temporary solo mask 1<<string_index, noteOnChord with one event, restore mask 0\","
               "\"control_policy\":\"Play-style keyswitches latch the next contact; CC2 pressure is continuous\","
               "\"performance_controls\":{\"pitch_bend\":0,\"resonance\":0,\"palm_pressure\":0,"
               "\"vibrato\":0,\"sustain\":false,\"acoustic_return_level\":0},"
               "\"fx_parameters\":{\"crunch\":{\"distortion\":0,\"amp\":0.5,\"amp_model\":\"BritishCrunch\","
               "\"compressor\":0.1,\"delay\":0,\"room\":0,\"oversampling\":\"Standard\"},"
               "\"modern\":{\"distortion\":0.45,\"amp\":0.95,\"amp_model\":\"ModernHighGain\","
               "\"compressor\":0.6,\"delay\":0,\"room\":0,\"oversampling\":\"Standard\"}},\"takes\":[\n";
        bool first = true;
        auto write = [&](const Take& t) { save(manifest, directory, t, first); };
        {
            Take t("rolling_chords_styles", "Rolling open chords - style switching",
                   "Original 8-bar D / Cadd9 / G riff. Held arpeggio notes cross repeated Open / Palm / Open contacts; D4 stays owned across all chord changes.", .20f);
            rollingRiff(t, false, false); write(t);
        }
        {
            Take t("rolling_chords_pressure", "Rolling open chords - continuous palm pressure",
                   "The same original score, with Sustain latched throughout and CC2 pressure moving the palm independently of the pick.", .20f);
            rollingRiff(t, false, true); write(t);
        }
        {
            Take t("drop_e_styles", "Drop-E companion - held power chords",
                   "Low E / G / A / D power chords use the same rolling rhythm and continuous style changes, with no silent articulation-boundary gaps.", .20f);
            rollingRiff(t, true, false); write(t);
        }
        for (int string : {0,2}) {
            const int note = string == 0 ? 28 : 40;
            for (bool control : {false,true}) {
                Take t("held_pressure_" + std::to_string(note) + (control ? "_control" : ""),
                       std::string(string == 0 ? "E1" : "E2") + (control ? " - untouched decay control" : " - palm touches and lifts, no new pick"),
                       "One initial pick only. Pressure contacts at 0.85, 1.65 and 2.5 s; lifts at 1.03, 1.85 and 2.75 s. The untouched control uses zero pressure at all corresponding events.");
                t.on(string, note, .86f);
                t.time(.85); t.mark(control ? "Zero-pressure control" : "Light palm contact"); t.pressure(control ? 0.0f : .45f);
                t.time(1.03); t.mark("Palm lifts - no pick"); t.pressure(0);
                t.time(1.65); t.mark(control ? "Zero-pressure control" : "Firm palm contact"); t.pressure(control ? 0.0f : .80f);
                t.time(1.85); t.mark("Palm lifts - no pick"); t.pressure(0);
                t.time(2.50); t.mark(control ? "Zero-pressure control" : "Light palm contact"); t.pressure(control ? 0.0f : .35f);
                t.time(2.75); t.mark("Palm lifts - no pick"); t.pressure(0);
                t.time(3.30); t.finish(.70); write(t);
            }
        }
        {
            Take t("pressure_lift_then_repick", "Lift the palm, then pick again",
                   "Muted pressure damps a held E1. Each lift has 250 ms without a new pick before the open restrike, separating irreversible energy loss from restored attack brightness.");
            t.on(0,28,.86f);
            t.time(.75); t.mark("Palm down"); t.pressure(.8f);
            t.time(.85); t.repick(0,.86f);
            t.time(1.10); t.mark("Palm lifts - no pick"); t.pressure(0);
            t.time(1.35); t.mark("Open repick"); t.repick(0,.86f);
            t.time(1.65); t.mark("Palm down"); t.pressure(.8f);
            t.time(1.80); t.repick(0,.86f);
            t.time(2.05); t.mark("Palm lifts - no pick"); t.pressure(0);
            t.time(2.30); t.mark("Open repick"); t.repick(0,.86f);
            t.time(2.85); t.finish(.70); write(t);
        }
        {
            Take t("held_style_alternation", "Held E1 - repeated muted / open repicks",
                   "One fretting note remains held for all 13 attacks. Style changes occur 100 ms before each pick, exposing latch/contact timing and residual ringing.");
            t.on(0,28,.86f);
            for (int i = 0; i < 12; ++i) {
                t.time(.55 + .30 * i); const bool muted = i % 2 == 0;
                t.mark(muted ? "Palm style selected" : "Open style selected");
                t.style(muted ? PlayStyle::PalmMute : PlayStyle::Sustain);
                t.wait(.10); t.mark(muted ? "Muted repick" : "Open repick"); t.repick(0,.86f);
            }
            t.wait(.50); t.finish(.70); write(t);
        }
        manifest << "\n]}\n"; manifest.close();
        if (!manifest) throw std::runtime_error("Could not write manifest");
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
    return 0;
}
