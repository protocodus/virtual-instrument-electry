// Renders a MIDI file through the Strat performer to a mono 32-bit float WAV,
// for listening and for measuring the model against recordings. Every render
// is local material: write it outside the checkout.
//
//   RenderStrat [--rate HZ] [--pickup 1-5] [--pick METRES] [--lookahead]
//               [--tab] [--hum 0-1] [--tail SECONDS] <in.mid> <out.wav>
//
// --tab plays each note on the string its MIDI channel names, channel 1 the
// low E to 6 the high E, as a transcription with string numbers asks.

#include "../Source/DSP/StratPerformer.h"
#include "PresetDemoRenderer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace
{
void writeWav(const char* path, const std::vector<float>& samples, int rate)
{
    std::ofstream file(path, std::ios::binary);
    const auto put = [&](std::uint32_t v, int bytes) {
        for (int i = 0; i < bytes; ++i)
            file.put(static_cast<char>((v >> (8 * i)) & 0xFF));
    };
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(samples.size() * 4);
    file.write("RIFF", 4);
    put(36 + dataBytes, 4);
    file.write("WAVEfmt ", 8);
    put(16, 4);
    put(3, 2); // IEEE float
    put(1, 2);
    put(static_cast<std::uint32_t>(rate), 4);
    put(static_cast<std::uint32_t>(rate * 4), 4);
    put(4, 2);
    put(32, 2);
    file.write("data", 4);
    put(dataBytes, 4);
    file.write(reinterpret_cast<const char*>(samples.data()), static_cast<std::streamsize>(dataBytes));
}
} // namespace

int main(int argc, char** argv)
{
    int rate = 48000;
    int pickup = 5;
    double pick = -1.0, tail = 2.0, hum = 0.0;
    bool lookahead = false, tab = false;
    std::vector<const char*> paths;
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--rate" && i + 1 < argc)
            rate = std::atoi(argv[++i]);
        else if (a == "--pickup" && i + 1 < argc)
            pickup = std::atoi(argv[++i]);
        else if (a == "--pick" && i + 1 < argc)
            pick = std::atof(argv[++i]);
        else if (a == "--hum" && i + 1 < argc)
            hum = std::atof(argv[++i]);
        else if (a == "--tail" && i + 1 < argc)
            tail = std::atof(argv[++i]);
        else if (a == "--lookahead")
            lookahead = true;
        else if (a == "--tab")
            tab = true;
        else
            paths.push_back(argv[i]);
    }
    if (paths.size() != 2 || pickup < 1 || pickup > 5)
    {
        std::fprintf(stderr, "usage: RenderStrat [--rate HZ] [--pickup 1-5] [--pick METRES] "
                             "[--lookahead] [--tab] [--hum 0-1] [--tail SECONDS] <in.mid> <out.wav>\n");
        return 2;
    }
    const auto midi = protocodus::demos::ReadMidi(paths[0]);

    electry::StratPerformer performer;
    performer.prepare(rate);
    electry::StratControls controls;
    controls.circuit.position = static_cast<electry::StratCircuit::Position>(pickup - 1);
    if (pick > 0.0)
        controls.pickMetres = pick;
    controls.lookahead = lookahead;
    controls.hum = hum;
    performer.setControls(controls);

    const double end = (midi.empty() ? 0.0 : midi.back().seconds) + tail;
    const auto total = static_cast<std::int64_t>(end * rate) + performer.latencySamples();
    std::vector<float> out(static_cast<size_t>(total));
    std::int64_t done = 0;
    size_t next = 0;
    constexpr int block = 256;
    while (done < total)
    {
        while (next < midi.size() && static_cast<std::int64_t>(midi[next].seconds * rate) <= done)
        {
            const auto& m = midi[next++];
            const int status = m.bytes[0] & 0xF0, channel = m.bytes[0] & 0x0F;
            if (status == 0x90)
                performer.noteOn(channel, m.bytes[1], m.bytes[2], tab && channel < 6 ? channel : -1);
            else if (status == 0x80)
                performer.noteOff(channel, m.bytes[1]);
            else if (status == 0xE0)
                performer.pitchBend(channel, ((m.bytes[2] << 7 | m.bytes[1]) - 8192) / 8192.0);
            else if (status == 0xB0 && m.bytes[1] == 1)
                performer.modWheel(m.bytes[2] / 127.0);
            else if (status == 0xB0 && m.bytes[1] == 64)
                performer.sustain(m.bytes[2] >= 64);
        }
        std::int64_t n = total - done < block ? total - done : block;
        if (next < midi.size())
        {
            const auto at = static_cast<std::int64_t>(midi[next].seconds * rate);
            if (at > done && at - done < n)
                n = at - done;
        }
        performer.render(out.data() + done, static_cast<int>(n));
        done += n;
    }
    // The lookahead's latency comes off the front, as a host compensates it.
    out.erase(out.begin(), out.begin() + performer.latencySamples());
    writeWav(paths[1], out, rate);
    return 0;
}
