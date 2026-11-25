// The six-string guitar's level on Reason's channel meter, for one setting of
// its controls.
//
//   ElectryStratLevels [--tele] [--pickup 1-5] [--volume 0-1] [--tone-neck 0-1]
//                      [--tone-middle 0-1] [--pick-position METRES] [--pick 0-1]
//                      [--strum SECONDS] [--compressor 0-1] [--output DB]
//                      [--typical VU --tolerance DB]
//
// The meter: the Main Mixer's channel meter is a VU meter, RMS with 300 ms
// ballistics read against the Big Meter's VU Offset (Reason 13 manual, "The
// Main Mixer" and "Hardware Interface"). The manual gives no default offset;
// Reason users report 12 dB, so 0 VU here is -12 dBFS RMS, and a reading is
// the RMS over a sliding 300 ms window plus 12 dB. Full scale is 10 V at the
// jack (StratPerformer).
//
// The playing, as the portfolio's other plucked instruments are measured: one
// MIDI channel, so the performer chooses the strings, at 48 kHz in 256-sample
// blocks, each event alone in a 1.5 s slot and held for 0.8 s.
//  - Typical playing: single notes every fourth semitone from the low E to
//    an octave above the open high E, and eight chords (E, A, D, G, C and Am
//    open, two barre chords up the neck), at velocities 70, 90 and 110. Its
//    reading is the median over events of each event's highest reading.
//  - The loudest chords: the same eight chords at velocity 127. Their
//    reading is the highest of them.
// Both are read 20 dB under the Output asked for, where the output limiter
// does nothing, and the 20 dB added back: "before the limiter" is what the
// limiter is handed, and it works on every sample of that over -0.5 dBFS.
// The loudest chords are then played at the Output asked for, for the peak
// and the reading that leave the device.
//
// With --typical it fails unless typical playing reads that many VU within
// the tolerance and none of it reaches the limiter, and the loudest chords
// stay under -1 VU, out of the red, with the limiter taking less than 3 dB
// off their hardest attack. A direct guitar's pick attacks stand 9 to 11 dB
// over its reading, so the level asked for decides how often the limiter
// works: -12 VU is the highest at which ordinary playing never meets it.

#include "../Source/DSP/StratPerformer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace
{
constexpr double rate = 48000.0;
constexpr int block = 256;
constexpr double slotSeconds = 1.5;
constexpr double holdSeconds = 0.8;
constexpr double vuOffsetDb = 12.0;
constexpr double voltsAtFullScale = 10.0;
constexpr double belowLimiterDb = 20.0;
constexpr double redVu = -1.0;
constexpr double limiterReachDb = 3.0;

struct Event
{
    std::vector<int> notes;
    int velocity;
};

const std::vector<std::vector<int>> chords {
    {40, 47, 52, 56, 59, 64}, {45, 52, 57, 61, 64}, {50, 57, 62, 66}, {43, 47, 50, 55, 59, 67},
    {48, 52, 55, 60, 64},     {45, 52, 57, 60, 64}, {45, 52, 57, 61, 64, 69}, {48, 55, 60, 64, 67, 72}};

std::vector<Event> typicalPlaying()
{
    std::vector<Event> events;
    for (const int velocity : {70, 90, 110})
    {
        for (int note = 40; note <= 76; note += 4)
            events.push_back({{note}, velocity});
        for (const auto& chord : chords)
            events.push_back({chord, velocity});
    }
    return events;
}

std::vector<Event> loudestChords()
{
    std::vector<Event> events;
    for (const auto& chord : chords)
        events.push_back({chord, 127});
    return events;
}

struct Reading
{
    std::vector<double> events; // each event's highest reading, VU
    double peakDbfs = -300.0;
};

Reading play(const electry::StratControls& controls, const std::vector<Event>& events)
{
    const auto slot = static_cast<long long>(slotSeconds * rate);
    const auto hold = static_cast<long long>(holdSeconds * rate);
    auto performer = std::make_unique<electry::StratPerformer>();
    performer->prepare(rate);
    performer->setControls(controls);
    // The performer's latency is left in: each slot's tail takes the event
    // that started that much before it.
    const long long total = slot * static_cast<long long>(events.size()) + performer->latencySamples();
    std::vector<float> out(static_cast<size_t>(total + block));
    for (long long done = 0; done < total;)
    {
        const long long index = done / slot, into = done % slot;
        if (index < static_cast<long long>(events.size()))
        {
            const Event& event = events[static_cast<size_t>(index)];
            if (into == 0)
                for (const int note : event.notes)
                    performer->noteOn(0, note, event.velocity);
            else if (into == hold)
                for (const int note : event.notes)
                    performer->noteOff(0, note);
        }
        const long long next = into < hold ? hold - into : slot - into;
        const int n = static_cast<int>(std::min<long long>({block, next, total - done}));
        performer->render(out.data() + done, n);
        done += n;
    }

    Reading reading;
    const long long latency = performer->latencySamples();
    const auto window = static_cast<long long>(0.3 * rate);
    double sum = 0.0, peak = 0.0, highest = -300.0;
    for (long long i = 0; i + latency < total; ++i)
    {
        const double v = out[static_cast<size_t>(i + latency)] / voltsAtFullScale;
        sum += v * v;
        if (i >= window)
        {
            const double old = out[static_cast<size_t>(i + latency - window)] / voltsAtFullScale;
            sum -= old * old;
        }
        highest = std::max(highest, 10.0 * std::log10(std::max(sum / static_cast<double>(window), 1e-30)) + vuOffsetDb);
        peak = std::max(peak, std::abs(v));
        if ((i + 1) % slot == 0)
        {
            reading.events.push_back(highest);
            highest = -300.0;
        }
    }
    reading.peakDbfs = 20.0 * std::log10(std::max(peak, 1e-15));
    return reading;
}

double median(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    const size_t n = values.size();
    return n % 2 ? values[n / 2] : 0.5 * (values[n / 2 - 1] + values[n / 2]);
}
} // namespace

int main(int argc, char** argv)
{
    electry::StratControls controls;
    using Position = electry::StratCircuit::Position;
    bool tele = false, check = false;
    int pickup = 5;
    double target = 0.0, tolerance = 0.5;
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto value = [&] { return i + 1 < argc ? std::atof(argv[++i]) : 0.0; };
        if (a == "--tele")
            tele = true;
        else if (a == "--pickup")
            pickup = static_cast<int>(value());
        else if (a == "--volume")
            controls.circuit.volume = value();
        else if (a == "--tone-neck")
            controls.circuit.toneNeck = value();
        else if (a == "--tone-middle")
            controls.circuit.toneMiddle = value();
        else if (a == "--pick-position")
            controls.pickMetres = value();
        else if (a == "--pick")
            controls.pick = value();
        else if (a == "--strum")
            controls.strumSeconds = value();
        else if (a == "--compressor")
            controls.compressor = value();
        else if (a == "--output")
            controls.outputDecibels = value();
        else if (a == "--typical")
        {
            target = value();
            check = true;
        }
        else if (a == "--tolerance")
            tolerance = value();
        else
        {
            std::fprintf(stderr, "ElectryStratLevels: unknown argument %s\n", a.c_str());
            return 2;
        }
    }
    if (pickup < 1 || pickup > (tele ? 3 : 5) || controls.outputDecibels - belowLimiterDb < -24.0)
    {
        std::fprintf(stderr, "ElectryStratLevels: --pickup is 1 to %d and --output -4 to 12\n", tele ? 3 : 5);
        return 2;
    }
    if (tele)
    {
        constexpr Position teleSwitch[] = {Position::bridge, Position::bridgeNeck, Position::neck};
        controls.circuit.guitar = electry::StratCircuit::Guitar::tele;
        controls.circuit.position = teleSwitch[pickup - 1];
    }
    else
        controls.circuit.position = static_cast<Position>(pickup - 1);

    electry::StratControls below = controls;
    below.outputDecibels -= belowLimiterDb;
    const Reading ordinary = play(below, typicalPlaying());
    const double typical = median(ordinary.events) + belowLimiterDb;
    const Reading free = play(below, loudestChords());
    const Reading out = play(controls, loudestChords());
    const double loudest = *std::max_element(out.events.begin(), out.events.end());
    const double ceilingDbfs = 20.0 * std::log10(electry::StratPerformer::outputCeiling);
    const double reach = free.peakDbfs + belowLimiterDb - ceilingDbfs;
    std::printf("typical %+.2f VU, peak %+.2f dBFS before the limiter; loudest chords %+.2f VU, peak %+.2f dBFS, "
                "%+.2f dBFS before the limiter (%s it by %.2f dB)\n",
                typical, ordinary.peakDbfs + belowLimiterDb, loudest, out.peakDbfs, free.peakDbfs + belowLimiterDb,
                reach > 0.0 ? "over" : "under", std::abs(reach));
    if (!check)
        return 0;
    bool ok = true;
    const auto require = [&](bool condition, const char* what) {
        if (!condition)
            std::printf("FAIL: %s\n", what);
        ok = ok && condition;
    };
    require(std::abs(typical - target) <= tolerance, "typical playing reads the level asked for");
    require(loudest <= redVu, "the loudest chords stay out of the red");
    require(ordinary.peakDbfs + belowLimiterDb <= ceilingDbfs, "typical playing stays under the limiter");
    require(reach <= limiterReachDb, "the limiter takes less than 3 dB off the loudest chords");
    return ok ? 0 : 1;
}
