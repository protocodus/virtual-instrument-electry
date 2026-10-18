// Production-Performer performance probes. A/B run this identical event list,
// seed, silence, controls and block segmentation against immutable snapshots.
// No reference performance is reconstructed here; these are model comparisons.
#include "DSP/ElectryPerformer.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr int block = 64;
constexpr std::uint32_t seed = 0x160c2026u;
enum class Kind { on, off, articulation, pressure };
struct Event { int frame; Kind kind; int note; float value; };
struct Score { std::vector<Event> events; double seconds = 4; };
Score score(const std::string& name, int rate)
{
    Score s;
    const auto event = [&](double t, Kind k, int n = 0, float v = 0) {
        s.events.push_back({static_cast<int>(std::lround(t * rate)), k, n, v});
    };
    const auto note = [&](double t, int n, double duration) {
        event(t, Kind::on, n, .85f); event(t + duration, Kind::off, n);
    };
    if (name == "aba-ownership")
    {
        // Old 40's release must not terminate the new 40 after 43 reused the
        // same physical string. Public MIDI has no hidden expression IDs.
        event(.2, Kind::on, 40, .85f); event(.4, Kind::on, 43, .85f);
        event(.6, Kind::on, 40, .85f); event(.65, Kind::off, 40);
        event(.7, Kind::off, 43); event(.95, Kind::off, 40);
        s.seconds = 2;
    }
    else if (name == "repeat-8hz" || name == "repeat-12hz")
    {
        const double interval = name == "repeat-8hz" ? .125 : 1. / 12;
        const std::array<double, 4> gaps {0, .002, .010, .030};
        for (int n = 0; n < 24; ++n)
            note(.2 + n * interval, 40, interval - gaps[static_cast<std::size_t>(n % 4)]);
        s.seconds = .2 + 24 * interval + .8;
    }
    else if (name == "overlapping-string")
    {
        const std::array<int, 4> notes {40, 43, 40, 47};
        for (int n = 0; n < 16; ++n) note(.2 + .125 * n, notes[n % 4], .4);
        s.seconds = 3.4;
    }
    else if (name == "alternating-string")
    {
        const std::array<int, 4> notes {40, 52, 43, 59};
        for (int n = 0; n < 20; ++n) note(.2 + .15 * n, notes[n % 4], .14);
        s.seconds = 4;
    }
    else if (name == "rapid-palm")
    {
        for (int n = 0; n < 24; ++n)
        {
            const double t = .2 + .125 * n;
            event(t - .02, Kind::articulation, 3 + static_cast<int>(n % 2 ? electry::PlayStyle::PalmMute : electry::PlayStyle::Sustain));
            event(t - .02, Kind::pressure, 0, n % 4 == 3 ? 1.f : 0.f);
            note(t, 40, .115);
        }
        s.seconds = 4;
    }
    else throw std::runtime_error("Unknown performance probe");
    std::stable_sort(s.events.begin(), s.events.end(), [](const Event& a, const Event& b) { return a.frame < b.frame; });
    return s;
}
void u16(std::ostream& out, std::uint16_t n)
{ for (int b = 0; b < 2; ++b) out.put(static_cast<char>((n >> (8 * b)) & 255)); }
void u32(std::ostream& out, std::uint32_t n)
{ for (int b = 0; b < 4; ++b) out.put(static_cast<char>((n >> (8 * b)) & 255)); }
}

int main(int argc, char** argv)
{
    try
    {
        if (argc < 3 || argc > 6) throw std::runtime_error("Usage: renderer OUTPUT_DIR PROBE [RATE], or OUTPUT_DIR score:TSV RATE FRAMES STRING_INDEX");
        int rate = 48000;
        if (argc >= 4)
        {
            std::size_t used = 0;
            const std::string text(argv[3]);
            rate = std::stoi(text, &used);
            if (used != text.size()) throw std::runtime_error("Rate must be an integer");
        }
        if (rate < 44100 || rate > 192000) throw std::runtime_error("Unsupported rate");
        const std::filesystem::path out = argv[1];
        const std::string requested = argv[2];
        const bool external = requested.rfind("score:", 0) == 0;
        if ((external && argc != 6) || (!external && argc > 4)) throw std::runtime_error("Wrong score argument count");
        const std::string probe = external ? "external-score" : requested;
        Score s;
        int stringIndex = 0;
        if (external)
        {
            const int requestedFrames = std::stoi(argv[4]);
            stringIndex = std::stoi(argv[5]);
            if (requestedFrames < 1 || requestedFrames > rate * 60 || stringIndex < 0 || stringIndex > 7)
                throw std::runtime_error("Invalid score length/string");
            s.seconds = static_cast<double>(requestedFrames) / rate;
            std::ifstream input(requested.substr(6));
            if (!input) throw std::runtime_error("Cannot read score TSV");
            std::string line;
            while (std::getline(input, line))
            {
                if (line.empty() || line[0] == '#') continue;
                std::istringstream fields(line);
                int frame = 0, kind = 0, note = 0;
                float value = 0;
                std::string excess;
                if (!(fields >> frame >> kind >> note >> value) || fields >> excess
                    || frame < 0 || frame >= requestedFrames || kind < 0 || kind > 3
                    || !std::isfinite(value) || value < 0 || value > 1
                    || ((kind == 0 || kind == 1) && (note < 40 || note > 98))
                    || (kind == 0 && value == 0))
                    throw std::runtime_error("Invalid score TSV row");
                if (!s.events.empty() && frame < s.events.back().frame) throw std::runtime_error("Unsorted score TSV");
                s.events.push_back({frame, static_cast<Kind>(kind), note, value});
            }
            if (s.events.empty()) throw std::runtime_error("Empty score TSV");
        }
        else s = score(probe, rate);
        electry::PerformerParameters parameters;
        electry::applyGuitarBuild(parameters.engine, electry::defaultGuitarBuild);
        parameters.engine.strumSpreadSeconds = 0;
        parameters.outputMode = 0;
        parameters.fxEnabled = false;
        auto performer = std::make_unique<electry::ElectryPerformer>();
        performer->setParameters(parameters);
        performer->getEngine().setVariationSeed(seed);
        performer->getDoubleEngine().setVariationSeed(seed ^ 0x9e3779b9u);
        performer->prepare(rate, block);
        performer->selectArticulation(3 + static_cast<int>(electry::PlayStyle::Sustain));
        performer->noteOn(24 + stringIndex, 1); // Public written solo-string key.
        const int frames = static_cast<int>(std::lround(s.seconds * rate));
        std::vector<float> audio(static_cast<std::size_t>(frames) * 2);
        std::array<float, block> left {}, right {};
        std::size_t next = 0;
        const auto started = std::chrono::steady_clock::now();
        for (int at = 0; at < frames;)
        {
            while (next < s.events.size() && s.events[next].frame == at)
            {
                const auto& e = s.events[next++];
                switch (e.kind)
                {
                    case Kind::on: performer->noteOn(e.note, e.value); break;
                    case Kind::off: performer->noteOff(e.note); break;
                    case Kind::articulation: performer->selectArticulation(e.note); break;
                    case Kind::pressure: performer->setPalmMutePressure(e.value); break;
                }
            }
            const int until = next < s.events.size() ? s.events[next].frame - at : frames - at;
            const int count = std::min({block, until, frames - at});
            if (count <= 0) throw std::runtime_error("Invalid event sequence");
            performer->render(left.data(), right.data(), count);
            for (int i = 0; i < count; ++i)
            {
                if (!std::isfinite(left[i]) || !std::isfinite(right[i])) throw std::runtime_error("Nonfinite audio");
                audio[2 * (at + i)] = left[i]; audio[2 * (at + i) + 1] = right[i];
            }
            at += count;
        }
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        std::filesystem::create_directories(out);
        std::ofstream wav(out / "raw.wav", std::ios::binary);
        const auto bytes = static_cast<std::uint32_t>(audio.size() * sizeof(float));
        wav.write("RIFF", 4); u32(wav, bytes + 36); wav.write("WAVEfmt ", 8);
        u32(wav, 16); u16(wav, 3); u16(wav, 2); u32(wav, rate); u32(wav, rate * 8); u16(wav, 8); u16(wav, 32);
        wav.write("data", 4); u32(wav, bytes); wav.write(reinterpret_cast<const char*>(audio.data()), bytes);
        if (!wav) throw std::runtime_error("Could not write WAV");
        std::ofstream json(out / "render.json");
        json << std::setprecision(12) << "{\"schema\":\"electry-performance-render/v1\",\"probe\":\"" << probe
             << "\",\"rate\":" << rate << ",\"frames\":" << frames << ",\"block\":" << block
             << ",\"seed\":" << seed << ",\"physical_string_index\":" << stringIndex
             << ",\"fx_enabled\":false,\"output_mode\":0,\"default_guitar_build\":"
             << electry::defaultGuitarBuild << ",\"initial_articulation\":3,\"initial_solo_key\":" << 24 + stringIndex << ",\"events\":[";
        for (std::size_t i = 0; i < s.events.size(); ++i)
        {
            const auto& e = s.events[i];
            json << (i ? "," : "") << '[' << e.frame << ',' << static_cast<int>(e.kind) << ',' << e.note << ',' << e.value << ']';
        }
        json << "],\"event_kinds\":[\"note-on-written\",\"note-off-written\",\"articulation\",\"palm-pressure\"],\"render_seconds\":" << elapsed << "}\n";
        if (!json) throw std::runtime_error("Could not write receipt");
        std::cout << probe << ": " << s.seconds << " s in " << elapsed << " s\n";
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
