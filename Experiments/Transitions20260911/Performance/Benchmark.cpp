#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef ELECTRY_PERFORMANCE_BLOCK
#define ELECTRY_PERFORMANCE_BLOCK 256
#endif
namespace {
constexpr int rate = 96000, block = ELECTRY_PERFORMANCE_BLOCK, frames = rate * 2;
double cpuSeconds() {
    timespec value {};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &value) != 0)
        throw std::runtime_error("Thread CPU clock unavailable");
    return value.tv_sec + value.tv_nsec * 1.e-9;
}
using Clock = std::chrono::steady_clock;
struct Scenario { const char* id; int changesPerSecond; bool repick; };
constexpr std::array scenarios {
    Scenario {"held", 0, false},
    Scenario {"style_4hz", 4, true}, Scenario {"cc2_4hz", 4, false},
    Scenario {"style_16hz", 16, true}, Scenario {"cc2_16hz", 16, false}
};
}
int main() {
    std::cout << std::setprecision(10) << "[";
    bool first = true;
    for (const auto scenario : scenarios) {
        auto engine = std::make_unique<electry::ElectryEngine>();
        electry::EngineParameters p;
        electry::applyGuitarBuild(p, electry::defaultGuitarBuild);
        p.pickupSelector = electry::PickupSelector::Bridge;
        p.outputMode = electry::OutputMode::Mono;
        p.strumSpreadSeconds = 0.0f;
        engine->prepare(rate, block);
        engine->setParameters(p);
        engine->setVariationSeed(0);
        engine->setAcousticReturnLevel(0.0f);
        engine->reset();
        std::array<electry::ElectryEngine::NoteOnEvent, 8> chord {{
            {28,.9f}, {35,.9f}, {40,.9f}, {45,.9f},
            {50,.9f}, {55,.9f}, {59,.9f}, {64,.9f}
        }};
        engine->noteOnChord(chord);
        std::array<float, block> left {}, right {};
        for (int n = 0; n < rate / 5; n += block)
            engine->process(left.data(), right.data(), std::min(block, rate / 5 - n));
        if (engine->getActiveVoiceCount() != 8)
            throw std::runtime_error("CPU fixture did not establish eight strings");
        const int interval = scenario.changesPerSecond > 0
            ? rate / scenario.changesPerSecond : frames;
        int nextEvent = interval, lastEvent = -rate, transitions = 0;
        int minimumVoices = 8;
        double checksum = 0, peak = 0, renderCpu = 0, eventCpu = 0;
        double movingCpu = 0, settledCpu = 0;
        std::int64_t movingFrames = 0, settledFrames = 0;
        std::vector<double> callbackRatios;
        const double beganCpu = cpuSeconds();
        const auto beganWall = Clock::now();
        for (int n = 0; n < frames;) {
            const double callbackBegan = cpuSeconds();
            const bool event = scenario.changesPerSecond > 0 && n == nextEvent;
            if (event) {
                const double beganEvent = cpuSeconds();
                ++transitions;
                const bool palm = transitions % 2 != 0;
                if (scenario.repick) {
                    engine->noteOn(electry::ElectryEngine::firstPlayStyleKeyswitchNote
                        + static_cast<int>(palm ? electry::PlayStyle::PalmMute
                                               : electry::PlayStyle::Sustain), 1.0f);
                    // Release/reacquire MIDI ownership at the same sample. The
                    // physical strings keep ringing; no reset/empty audio gap.
                    for (const auto note : chord) engine->noteOff(note.midiNote);
                    engine->noteOnChord(chord);
                } else engine->setPalmMutePressure(palm ? .85f : 0.0f);
                eventCpu += cpuSeconds() - beganEvent;
                lastEvent = n;
                nextEvent += interval;
            }
            const int count = std::min({block, frames - n,
                scenario.changesPerSecond > 0 ? nextEvent - n : block});
            const double beganRender = cpuSeconds();
            engine->process(left.data(), right.data(), count);
            const double elapsed = cpuSeconds() - beganRender;
            renderCpu += elapsed;
            const double callbackElapsed = cpuSeconds() - callbackBegan;
            // Exclude event-split fragments from callback percentile estimates;
            // aggregate times above still include every sample and event.
            if (count == block)
                callbackRatios.push_back(callbackElapsed * rate / count);
            if (n - lastEvent < static_cast<int>(.080 * rate)) {
                movingCpu += elapsed; movingFrames += count;
            } else { settledCpu += elapsed; settledFrames += count; }
            minimumVoices = std::min(minimumVoices, engine->getActiveVoiceCount());
            for (int i = 0; i < count; ++i) {
                if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
                    throw std::runtime_error("Invalid CPU benchmark audio");
                checksum += static_cast<double>(left[i]) * left[i];
                peak = std::max(peak, static_cast<double>(std::abs(left[i])));
            }
            n += count;
        }
        const double totalCpu = cpuSeconds() - beganCpu;
        const double totalWall = std::chrono::duration<double>(Clock::now()-beganWall).count();
        std::sort(callbackRatios.begin(), callbackRatios.end());
        const auto percentile = [&](double p) {
            return callbackRatios[static_cast<std::size_t>(p * (callbackRatios.size()-1))];
        };
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"scenario\":\"" << scenario.id << "\",\"audio_seconds\":2"
            << ",\"thread_cpu_seconds\":" << totalCpu << ",\"wall_seconds\":" << totalWall
            << ",\"render_cpu_seconds\":" << renderCpu << ",\"event_cpu_seconds\":" << eventCpu
            << ",\"cpu_realtime_ratio\":" << totalCpu / 2
            << ",\"moving_render_ratio\":" << (movingFrames ? movingCpu * rate / movingFrames : 0)
            << ",\"settled_render_ratio\":" << (settledFrames ? settledCpu * rate / settledFrames : 0)
            << ",\"callback_p50_ratio\":" << percentile(.50)
            << ",\"callback_p95_ratio\":" << percentile(.95)
            << ",\"callback_p99_ratio\":" << percentile(.99)
            << ",\"callback_max_ratio\":" << callbackRatios.back()
            << ",\"transitions\":" << transitions << ",\"minimum_voices\":" << minimumVoices
            << ",\"peak\":" << peak << ",\"sum_squares\":" << checksum << '}';
        if (minimumVoices != 8) throw std::runtime_error("A held string retired during CPU timing");
    }
    std::cout << "]\n";
}
