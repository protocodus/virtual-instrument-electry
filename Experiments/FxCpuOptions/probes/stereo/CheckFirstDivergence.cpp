/*
First-divergence diagnostic for the integrated shipping Source/DSP/ElectryFx:
prepared Standard/High selector banks, PI/diode tables and per-bank mono reuse.
This does not test the original standalone stereo experiment patch.

Compile from the repository root (the output directory must already exist):
c++ -std=c++20 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -ISource \
  Experiments/FxCpuOptions/probes/stereo/CheckFirstDivergence.cpp \
  Source/DSP/ElectryFx.cpp -o build-fx-mono-reuse/first-divergence
c++ -std=c++20 -O3 -DNDEBUG -Wall -Wextra -Wpedantic \
  -DELECTRY_MEASURED_MODERN_CABINET=1 -ISource \
  Experiments/FxCpuOptions/probes/stereo/CheckFirstDivergence.cpp \
  Source/DSP/ElectryFx.cpp -o build-fx-mono-reuse/first-divergence-cabinet

Run only in a serialized timing window. Fixed protocol: 48 kHz, 64 trials
for each Standard/High and American/British/Modern combination, both steady
quality and during a quality crossfade. Each trial resets and warms the gain
history with 256 + (trial % 127) identical stereo frames. Crossfade cases then
activate the other prepared bank and render 48 more identical frames. Record
a mono frame, the first divergent frame, the next divergent frame, and (after
fresh warmup) a 128-frame callback whose channels diverge at frame 13. Varying
the warmup length samples several cabinet partition positions. Report median,
empirical 95th percentile and maximum elapsed microseconds for observed
warm-cache events; these are not cold-cache or worst-case deadline guarantees.

Allocation instrumentation intercepts global ordinary, array and aligned C++
new during process(), including warmup/quality activation. Preparation and
explicit reset happen outside that scope. Direct malloc/calloc/realloc,
system allocation, deallocation and locking are not intercepted. This is a
single-thread diagnostic, not a proof of every real-time property. A nonzero
intercepted allocation count makes the diagnostic fail; output must be finite.
*/
#include "DSP/ElectryFx.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>
#include <vector>

namespace {
bool countAllocations = false;
std::size_t allocationCount = 0;
}
void* operator new(std::size_t bytes)
{
    if (countAllocations) ++allocationCount;
    if (void* p = std::malloc(bytes == 0 ? 1 : bytes)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void* operator new(std::size_t bytes, std::align_val_t alignment)
{
    if (countAllocations) ++allocationCount;
    void* p = nullptr;
    const auto align = std::max(static_cast<std::size_t>(alignment), sizeof(void*));
    if (posix_memalign(&p, align, bytes == 0 ? 1 : bytes) == 0) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes, std::align_val_t alignment)
{ return ::operator new(bytes, alignment); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

namespace electry {
struct ElectryFxTestAccess {
    static void reportSizes()
    {
        std::cerr << "gain_channel_bytes=" << sizeof(ElectryFx::GainChannel);
#if ELECTRY_MEASURED_MODERN_CABINET
        std::cerr << ", cabinet_history_bytes=" << sizeof(ElectryFx::CabinetConvolver);
#endif
        std::cerr << '\n';
    }
};
}

int main()
{
    using namespace electry;
    constexpr std::size_t trials = 64;
    constexpr std::array<const char*, 4> events {
        "mono-frame", "first-divergent-frame", "second-divergent-frame",
        "128-frame-block-diverging-at-13" };
    ElectryFxTestAccess::reportSizes();
    std::cout << std::setprecision(9)
              << "cabinet,quality,model,crossfading,event,trials,median_us,p95_us,max_us,audio_allocations\n";
    std::size_t totalAllocations = 0;
    for (auto quality : { FxOversampling::Standard, FxOversampling::High })
    for (auto model : { AmpModel::AmericanClean, AmpModel::BritishCrunch,
                        AmpModel::ModernHighGain })
    for (bool crossfading : { false, true })
    {
        ElectryFx fx;
        fx.prepare(48000.0);
        FxParameters parameters { .8f, .95f, model, .65f, .45f, .55f, quality };
        std::array<std::array<double, trials>, events.size()> times {};
        std::array<std::size_t, events.size()> eventAllocations {};
        std::size_t warmAudioAllocations = 0;
        const auto warmProcess = [&](float* l, float* r, int count)
        {
            allocationCount = 0;
            countAllocations = true;
            fx.process(l, r, count);
            countAllocations = false;
            warmAudioAllocations += allocationCount;
        };
        const auto warm = [&](std::size_t trial)
        {
            fx.setParameters(parameters);
            fx.reset();
            std::array<float, 384> l {}, r {};
            const auto count = 256 + static_cast<int>(trial % 127);
            for (int i = 0; i < count; ++i)
                l[static_cast<std::size_t>(i)] = r[static_cast<std::size_t>(i)]
                    = .2f * std::sin(.071f * static_cast<float>(i));
            warmProcess(l.data(), r.data(), count);
            if (crossfading)
            {
                auto next = parameters;
                next.oversampling = quality == FxOversampling::Standard
                    ? FxOversampling::High : FxOversampling::Standard;
                fx.setParameters(next);
                l.fill(.1f); r.fill(.1f);
                warmProcess(l.data(), r.data(), 48);
            }
        };
        const auto measure = [&](std::size_t trial, std::size_t event,
                                 float* l, float* r, int count)
        {
            allocationCount = 0;
            countAllocations = true;
            const auto start = std::chrono::steady_clock::now();
            fx.process(l, r, count);
            const auto end = std::chrono::steady_clock::now();
            countAllocations = false;
            times[event][trial] = std::chrono::duration<double, std::micro>(end - start).count();
            eventAllocations[event] += allocationCount;
            for (int i = 0; i < count; ++i)
                if (!std::isfinite(l[i]) || !std::isfinite(r[i])) std::abort();
        };
        for (std::size_t trial = 0; trial < trials; ++trial)
        {
            warm(trial);
            float l = .13f, r = .13f;
            measure(trial, 0, &l, &r, 1);
            l = .13f; r = .131f;
            measure(trial, 1, &l, &r, 1);
            l = .13f; r = .131f;
            measure(trial, 2, &l, &r, 1);
            warm(trial);
            std::array<float, 128> blockL {}, blockR {};
            blockL.fill(.13f); blockR.fill(.13f);
            for (std::size_t i = 13; i < blockR.size(); ++i) blockR[i] = .131f;
            measure(trial, 3, blockL.data(), blockR.data(), 128);
        }
        totalAllocations += warmAudioAllocations;
        if (warmAudioAllocations != 0)
            std::cerr << "processing warmup/quality activation allocated "
                      << warmAudioAllocations << " times\n";
        for (std::size_t event = 0; event < events.size(); ++event)
        {
            auto sorted = times[event];
            std::sort(sorted.begin(), sorted.end());
            totalAllocations += eventAllocations[event];
            std::cout << ELECTRY_MEASURED_MODERN_CABINET << ','
                      << static_cast<int>(quality) << ',' << static_cast<int>(model) << ','
                      << crossfading << ',' << events[event] << ',' << trials << ','
                      << .5 * (sorted[trials / 2 - 1] + sorted[trials / 2]) << ','
                      << sorted[60] << ',' << sorted.back() << ',' << eventAllocations[event] << '\n';
        }
    }
    return totalAllocations == 0 ? 0 : 1;
}
