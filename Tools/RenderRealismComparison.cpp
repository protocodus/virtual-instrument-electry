// Listening protocol: build this identical driver against the unchanged
// shipping commit and each isolated candidate. Events use the host-neutral
// ElectryPerformer, WRITTEN MIDI, 48 kHz / 64 frames and seed 0x160c2026.
// The driver writes unnormalised float WAV and event/timing metadata. Packaging
// matches whole-file RMS with one constant gain per take, checks peaks and
// keeps the candidate mapping in a separate key.md. No audio approximations,
// private-state injection, post-EQ, time alignment or per-window gain changes.
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
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr int rate = 48000, block = 64;
constexpr std::uint32_t seed = 0x160c2026u;
using Engine = electry::ElectryEngine;
enum class Kind { on, off, style, palm, bend, mask, stop };
struct Event { int frame; Kind kind; int note; float value; };
struct Score
{
    electry::PerformerParameters parameters;
    std::vector<Event> events;
    double seconds = 10.0;
    void event(double time, Kind kind, int note = 0, float value = 0)
    { events.push_back({static_cast<int>(std::lround(time * rate)), kind, note, value}); }
    void note(double time, int written, float velocity, double length)
    { event(time, Kind::on, written, velocity); event(time + length, Kind::off, written); }
    void style(double time, electry::PlayStyle s)
    { event(time, Kind::style, static_cast<int>(s)); }
};

Score makeScore(const std::string& probe, bool amp)
{
    Score s;
    auto& p = s.parameters.engine;
    electry::applyGuitarBuild(p, electry::defaultGuitarBuild);
    p.outputGain = 0.5f;
    p.strumSpreadSeconds = 0;
    s.parameters.fxEnabled = amp;
    s.parameters.effects.amp = amp ? 0.48f : 0;
    s.parameters.effects.ampModel = electry::AmpModel::ModernHighGain;
    if (probe == "palm")
    {
        s.event(0, Kind::mask, 1);
        for (int phrase = 0; phrase < 3; ++phrase)
        {
            const double t = 0.2 + 2.8 * phrase;
            s.style(t, electry::PlayStyle::PalmMute);
            s.note(t, 40 + 2 * phrase, 0.95f, 2.3);
            for (int n = 1; n <= 8; ++n)
            {
                const double hit = t + 0.15 * n;
                s.style(hit, n % 2 ? electry::PlayStyle::Sustain : electry::PlayStyle::PalmMute);
                s.event(hit, Kind::on, 100, 0.85f);
                s.event(hit + 0.003, Kind::on, 100, 0.75f);
            }
            s.event(t + 1.5, Kind::palm, 0, 1);
            s.event(t + 1.7, Kind::palm, 0, 0);
            s.event(t + 1.7, Kind::on, 100, 0.85f);
        }
    }
    else if (probe == "repick")
    {
        p.sympatheticAmount = 1;
        p.pickNoise = p.fingerNoise = p.releaseNoise = p.artifactAmount = 0;
        for (int n = 0; n < 3; ++n)
        {
            const double t = 0.25 + 3.0 * n;
            s.event(t, Kind::mask, 4); // sixth string, sounding E2
            s.note(t, 52, 1.0f, 0.8);
            s.event(t + 0.8, Kind::mask, 1);
            s.note(t + 0.8, 40, 0.12f + 0.06f * n, 1.8);
        }
    }
    else if (probe == "release")
    {
        p.sympatheticAmount = 0;
        p.releaseNoise = 1;
        s.event(0, Kind::mask, 1);
        for (int n = 0; n < 8; ++n)
        {
            const double t = 0.25 + 1.1 * n;
            s.style(t, n % 2 ? electry::PlayStyle::Sustain : electry::PlayStyle::PalmMute);
            s.note(t, 45 + 7 * (n % 2), 0.95f, 0.152);
            s.style(t + 0.15, n % 2 ? electry::PlayStyle::PalmMute : electry::PlayStyle::Sustain);
            s.event(t + 0.15, Kind::on, 100, 0.7f);
        }
    }
    else if (probe == "material" || probe == "sustain" || probe == "balance")
    {
        p.sympatheticAmount = 1;
        p.pickNoise = p.fingerNoise = p.releaseNoise = p.artifactAmount = 0;
        p.pickupType = 1;
        if (probe == "balance") p.pickupSelector = electry::PickupSelector::Neck;
        s.seconds = 12;
        // A short higher E excites the untouched lowest E; repeated excitation
        // builds its ring. The last nine seconds expose only the natural tail.
        s.event(0, Kind::mask, 4);
        for (int n = 0; n < 4; ++n) s.note(0.2 + 0.5 * n, 52, 1, 0.35);
    }
    else if (probe == "pickup")
    {
        p.pickupType = 0;
        p.pickupSelector = electry::PickupSelector::Neck;
        p.sympatheticAmount = 0;
        p.pickPosition = 0;
        p.pickHardness = p.toneKnob = 1;
        p.stringAge = 0;
        s.event(0, Kind::mask, 1);
        // High frets on the same thick low string keep the coil-spacing
        // response in the audible low-mid band. Automatic allocation onto
        // thinner strings would move it above the loaded coil's passband.
        const std::array<int, 4> notes {52, 57, 59, 62};
        for (std::size_t n = 0; n < notes.size(); ++n)
            s.note(0.2 + 2.4 * n, notes[n], 0.85f, 2.1);
    }
    else if (probe == "aged")
    {
        p.stringAge = 1;
        p.sympatheticAmount = 0;
        s.seconds = 15;
        for (int n = 0; n < 3; ++n)
        {
            s.event(0.2 + 4.8 * n, Kind::mask, 1 << n);
            s.note(0.2 + 4.8 * n, std::array<int, 3>{40, 47, 52}[n], 0.9f, 4.5);
        }
    }
    else if (probe == "combined")
    {
        s.seconds = 22;
        p.pickupSelector = electry::PickupSelector::Both;
        p.sympatheticAmount = 0.55f;
        s.event(0, Kind::mask, 1);
        for (int n = 0; n < 16; ++n)
        {
            const double t = 0.2 + 0.25 * n;
            s.style(t, n % 4 == 3 ? electry::PlayStyle::Sustain : electry::PlayStyle::PalmMute);
            s.note(t, 40 + (n % 8 == 7 ? 3 : 0), 0.72f + 0.08f * (n % 3), 0.20);
        }
        s.event(4.5, Kind::mask, 0);
        s.style(4.5, electry::PlayStyle::Sustain);
        for (int n : {40, 47, 52, 59}) s.note(4.5, n, 0.9f, 3.5);
        s.event(9, Kind::mask, 4);
        s.note(9, 52, 1, 0.8);
        s.event(9.8, Kind::mask, 1);
        s.note(9.8, 40, 0.16f, 2.0);
        s.event(12.3, Kind::mask, 0);
        for (int n = 0; n < 6; ++n) s.note(12.3 + 0.45 * n, 64 + 2 * n, 0.85f, 0.4);
        for (int n : {40, 47, 52, 59, 64}) s.note(15.4, n, 0.95f, 5.0);
    }
    else throw std::runtime_error("Unknown probe: " + probe);
    std::stable_sort(s.events.begin(), s.events.end(), [](const Event& a, const Event& b) {
        return a.frame < b.frame;
    });
    return s;
}

void u16(std::ostream& out, std::uint16_t n)
{ for (int b = 0; b < 2; ++b) out.put(static_cast<char>((n >> (8 * b)) & 255)); }
void u32(std::ostream& out, std::uint32_t n)
{ for (int b = 0; b < 4; ++b) out.put(static_cast<char>((n >> (8 * b)) & 255)); }
void writeWav(const std::filesystem::path& path, const std::vector<float>& audio)
{
    std::ofstream out(path, std::ios::binary);
    const auto bytes = static_cast<std::uint32_t>(audio.size() * sizeof(float));
    out.write("RIFF", 4); u32(out, bytes + 36); out.write("WAVEfmt ", 8);
    u32(out, 16); u16(out, 3); u16(out, 2); u32(out, rate);
    u32(out, rate * 8); u16(out, 8); u16(out, 32);
    out.write("data", 4); u32(out, bytes);
    out.write(reinterpret_cast<const char*>(audio.data()), bytes);
    if (!out) throw std::runtime_error("Could not write WAV");
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        if (argc < 3 || argc > 4) throw std::runtime_error("Usage: renderer OUTPUT_DIR PROBE [amp]");
        const std::filesystem::path out = argv[1];
        const std::string probe = argv[2];
        const bool amp = argc == 4 && std::string(argv[3]) == "amp";
        const auto score = makeScore(probe, amp);
        std::filesystem::create_directories(out);
        auto performer = std::make_unique<electry::ElectryPerformer>();
        performer->setParameters(score.parameters);
        performer->getEngine().setVariationSeed(seed);
        performer->getDoubleEngine().setVariationSeed(seed ^ 0x9e3779b9u);
        performer->prepare(rate, block);
        std::vector<float> audio(static_cast<std::size_t>(score.seconds * rate) * 2);
        std::array<float, block> l {}, r {};
        std::size_t next = 0;
        const int frames = static_cast<int>(audio.size() / 2);
        const auto started = std::chrono::steady_clock::now();
        for (int at = 0; at < frames;)
        {
            while (next < score.events.size() && score.events[next].frame == at)
            {
                if (score.events[next].kind == Kind::on
                    && Engine::isMidiPlayableNote(score.events[next].note))
                {
                    std::array<Engine::NoteOnEvent, Engine::maximumChordEvents> chord {};
                    std::size_t count = 0;
                    while (next < score.events.size() && score.events[next].frame == at
                           && score.events[next].kind == Kind::on
                           && Engine::isMidiPlayableNote(score.events[next].note)
                           && count < chord.size())
                    {
                        const auto& e = score.events[next++];
                        chord[count++] = {e.note, e.value};
                    }
                    if (count == 1) performer->noteOn(chord[0].midiNote, chord[0].velocity);
                    else performer->noteOnChord(chord.data(), count);
                    continue;
                }
                const auto& e = score.events[next++];
                switch (e.kind)
                {
                    case Kind::on: performer->noteOn(e.note, e.value); break;
                    case Kind::off: performer->noteOff(e.note); break;
                    case Kind::style: performer->selectArticulation(3 + e.note); break;
                    case Kind::palm: performer->setPalmMutePressure(e.value); break;
                    case Kind::bend: performer->setPitchBend(e.value); break;
                    case Kind::mask: performer->getEngine().setSoloStringMask(static_cast<std::uint8_t>(e.note)); break;
                    case Kind::stop: performer->allSoundOff(); break;
                }
            }
            const int untilEvent = next < score.events.size() ? score.events[next].frame - at : frames - at;
            const int count = std::min({block, untilEvent, frames - at});
            if (count <= 0) throw std::runtime_error("Invalid score order");
            performer->render(l.data(), r.data(), count);
            for (int i = 0; i < count; ++i)
            {
                if (!std::isfinite(l[i]) || !std::isfinite(r[i])) throw std::runtime_error("Nonfinite audio");
                audio[2 * (at + i)] = l[i]; audio[2 * (at + i) + 1] = r[i];
            }
            at += count;
        }
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        double energy = 0, peak = 0;
        for (float x : audio) { energy += x * x; peak = std::max(peak, std::abs(static_cast<double>(x))); }
        writeWav(out / "raw.wav", audio);
        std::ofstream json(out / "render.json");
        json << std::setprecision(12) << "{\n  \"probe\": \"" << probe << "\", \"amp\": " << (amp ? "true" : "false")
             << ",\n  \"rate\": " << rate << ", \"block\": " << block << ", \"seed\": " << seed
             << ", \"frames\": " << frames << ",\n  \"rms\": " << std::sqrt(energy / audio.size())
             << ", \"peak\": " << peak << ", \"render_seconds\": " << elapsed << ",\n  \"events\": [";
        for (std::size_t i = 0; i < score.events.size(); ++i)
        {
            const auto& e = score.events[i];
            json << (i ? "," : "") << "[" << e.frame << "," << static_cast<int>(e.kind) << "," << e.note << "," << e.value << "]";
        }
        json << "]\n}\n";
        std::cout << probe << ": " << score.seconds << " s rendered in " << elapsed << " s, peak=" << peak << '\n';
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
