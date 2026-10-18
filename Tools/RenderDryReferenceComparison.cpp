// Descriptive real/model audition driver, not a calibrated capture emulator.
// Same host-neutral production Performer as the plug-in; WRITTEN MIDI is
// sounding MIDI + 12. One isolated held note, no effects, no postprocessing.
// A real player's force, construction, release and pickup loading are unknown.
// Build the identical driver against immutable source snapshots for A and B.
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
constexpr int block = 64;
constexpr std::uint32_t seed = 0x160c2026u;
int integer(const char* text)
{
    std::size_t used = 0;
    const std::string value(text);
    const int result = std::stoi(value, &used);
    if (used != value.size()) throw std::runtime_error("Invalid integer");
    return result;
}
float unit(const char* text)
{
    std::size_t used = 0;
    const std::string value(text);
    const float result = std::stof(value, &used);
    if (used != value.size() || !std::isfinite(result) || result < 0 || result > 1)
        throw std::runtime_error("Controls must be finite values from 0 to 1");
    return result;
}
void u16(std::ostream& out, std::uint16_t n)
{ for (int b = 0; b < 2; ++b) out.put(static_cast<char>((n >> (8 * b)) & 255)); }
void u32(std::ostream& out, std::uint32_t n)
{ for (int b = 0; b < 4; ++b) out.put(static_cast<char>((n >> (8 * b)) & 255)); }
void writeWav(const std::filesystem::path& path, int rate, const std::vector<float>& audio)
{
    std::ofstream out(path, std::ios::binary);
    const auto bytes = static_cast<std::uint32_t>(audio.size() * sizeof(float));
    out.write("RIFF", 4); u32(out, bytes + 36); out.write("WAVEfmt ", 8);
    u32(out, 16); u16(out, 3); u16(out, 1); u32(out, rate);
    u32(out, rate * 4); u16(out, 4); u16(out, 32);
    out.write("data", 4); u32(out, bytes);
    out.write(reinterpret_cast<const char*>(audio.data()), bytes);
    if (!out) throw std::runtime_error("Could not write WAV");
}
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 13)
            throw std::runtime_error("Usage: renderer OUTPUT_DIR RATE FRAMES NOTE_ON_FRAME SOUNDING_MIDI STRING_INDEX PICKUP STYLE GAUGE SCALE VELOCITY PALM_TIGHTNESS");
        const std::filesystem::path out = argv[1];
        const int rate = integer(argv[2]), frames = integer(argv[3]), onset = integer(argv[4]);
        const int sounding = integer(argv[5]), stringIndex = integer(argv[6]);
        const std::string pickup = argv[7], style = argv[8];
        const float gauge = unit(argv[9]), scale = unit(argv[10]);
        const float velocity = unit(argv[11]), tightness = unit(argv[12]);
        if (rate < 44100 || rate > 192000 || frames < 1 || frames > rate * 60
            || onset < 0 || onset >= frames || sounding < 28 || sounding > 86
            || stringIndex < 0 || stringIndex > 7 || velocity == 0)
            throw std::runtime_error("Rate, length, MIDI note, string or velocity outside supported range");
        electry::PerformerParameters p;
        electry::applyGuitarBuild(p.engine, electry::defaultGuitarBuild);
        p.fxEnabled = false;
        p.outputMode = 0;
        p.engine.outputGain = 0.5f;
        p.engine.strumSpreadSeconds = 0;
        p.engine.stringGauge = gauge;
        p.engine.scaleLength = scale;
        p.engine.muteDamping = tightness;
        // Common declared hypotheses, not fits to a reference waveform.
        p.engine.pickupType = 0; // humbucker, nearest named type for these sources
        p.engine.toneKnob = 1;
        if (pickup == "bridge") p.engine.pickupSelector = electry::PickupSelector::Bridge;
        else if (pickup == "neck") p.engine.pickupSelector = electry::PickupSelector::Neck;
        else if (pickup == "both") p.engine.pickupSelector = electry::PickupSelector::Both;
        else throw std::runtime_error("Pickup must be bridge, neck or both");
        electry::PlayStyle playStyle;
        if (style == "sustain") playStyle = electry::PlayStyle::Sustain;
        else if (style == "palm") playStyle = electry::PlayStyle::PalmMute;
        else throw std::runtime_error("Style must be sustain or palm");

        auto performer = std::make_unique<electry::ElectryPerformer>();
        performer->setParameters(p);
        performer->getEngine().setVariationSeed(seed);
        performer->getDoubleEngine().setVariationSeed(seed ^ 0x9e3779b9u);
        performer->prepare(rate, block);
        performer->selectArticulation(3 + static_cast<int>(playStyle));
        // Use the public written solo-string keys so the Performer owns the
        // mask; bypassing only its engine mask would not preserve host state.
        performer->noteOn(24 + stringIndex, 1);
        std::array<float, block> left {}, right {};
        std::vector<float> audio(static_cast<std::size_t>(frames));
        const auto started = std::chrono::steady_clock::now();
        for (int at = 0; at < frames;)
        {
            if (at == onset) performer->noteOn(sounding + 12, velocity);
            const int until = at < onset ? onset - at : frames - at;
            const int count = std::min({block, until, frames - at});
            if (count <= 0) throw std::runtime_error("Invalid event order");
            performer->render(left.data(), right.data(), count);
            for (int i = 0; i < count; ++i)
            {
                if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
                    throw std::runtime_error("Nonfinite audio");
                if (std::abs(left[i] - right[i]) > 1e-7f)
                    throw std::runtime_error("Expected production mono output");
                audio[static_cast<std::size_t>(at + i)] = left[i];
            }
            at += count;
        }
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        std::filesystem::create_directories(out);
        writeWav(out / "raw.wav", rate, audio);
        std::ofstream json(out / "render.json");
        json << std::setprecision(12)
             << "{\n\"schema\":\"electry-isolated-dry-render/v1\",\"rate\":" << rate
             << ",\"frames\":" << frames << ",\"block\":" << block << ",\"seed\":" << seed
             << ",\"sounding_midi\":" << sounding << ",\"written_midi\":" << sounding + 12
             << ",\"physical_string_index\":" << stringIndex << ",\"pickup\":\"" << pickup
             << "\",\"style\":\"" << style << "\",\"gauge\":" << gauge << ",\"scale\":" << scale
             << ",\"velocity\":" << velocity << ",\"palm_tightness\":" << tightness
             << ",\"fx_enabled\":false,\"output_mode\":\"mono\",\"pickup_type\":0,\"tone_knob\":1"
             << ",\"default_guitar_build\":" << electry::defaultGuitarBuild
             << ",\"events\":[[0,\"solo-string\"," << 24 + stringIndex
             << "],[0,\"articulation\"," << 3 + static_cast<int>(playStyle)
             << "],[" << onset << ",\"note-on-written\"," << sounding + 12 << "," << velocity
             << "]],\"note_off\":\"after audition crop; held for full render\",\"render_seconds\":" << elapsed << "\n}\n";
        if (!json) throw std::runtime_error("Could not write render receipt");
        std::cout << "Rendered " << frames << " frames at " << rate << " Hz in " << elapsed << " s\n";
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
