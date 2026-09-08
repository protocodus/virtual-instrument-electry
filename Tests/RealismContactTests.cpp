#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    struct Contact
    {
        float noise, noiseCorner, closureSeconds, motionPeak;
        float imageDelay, imageWidth, period;
        int midi;
        bool active;
    };
    static Contact contact(const ElectryEngine& engine)
    {
        const auto& v = engine.voices_[0];
        return { v.noiseAmplitude,
                 -std::log(v.noiseBandCoefficient)
                     * static_cast<float>(engine.sampleRate_) / 6.28318530718f,
                 v.releaseGainCoefficient > 0.0f
                     ? -1.0f / (std::log1p(-v.releaseGainCoefficient)
                                  * static_cast<float>(engine.sampleRate_)) : 0.0f,
                 v.releaseMotionPeak, v.excitationCombDelay,
                 v.excitationCombWidth, v.lastCompensatedPeriod,
                 v.midiNote, v.active };
    }
};
}

namespace
{
int failures = 0;
void expect(bool condition, const std::string& message)
{
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}

using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;

std::unique_ptr<Engine> makeEngine(double rate, float pressure = 0.0f,
                                  float releaseNoise = 1.0f)
{
    auto e = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.sympatheticAmount = p.artifactAmount = p.bodyResonance = 0.0f;
    p.pickNoise = p.fingerNoise = 0.0f;
    p.releaseNoise = releaseNoise;
    p.palmMute = pressure;
    e->setParameters(p);
    e->prepare(rate, 256);
    e->setSoloStringMask(1);
    return e;
}

std::vector<float> render(Engine& e, int samples, int block = 256)
{
    std::vector<float> out(static_cast<std::size_t>(samples));
    std::array<float, 512> right {};
    for (int i = 0; i < samples; i += block)
    {
        const int n = std::min(block, samples - i);
        e.process(out.data() + i, right.data(), n);
    }
    expect(std::all_of(out.begin(), out.end(), [](float x) {
        return std::isfinite(x) && std::abs(x) < 2.0f;
    }), "finite bounded contact render");
    return out;
}

void releaseMotion(double rate)
{
    auto early = makeEngine(rate);
    auto late = makeEngine(rate);
    for (auto* e : {early.get(), late.get()})
    {
        e->noteOn(Engine::firstPlayStyleKeyswitchNote + 1, 1.0f); // Palm
        e->noteOn(33, 0.9f);
    }
    render(*early, static_cast<int>(0.10 * rate));
    render(*late, static_cast<int>(3.0 * rate));
    early->noteOff(33);
    late->noteOff(33);
    const auto a = Access::contact(*early), b = Access::contact(*late);
    std::cout << "release " << rate << " Hz: early=" << a.noise
              << " late=" << b.noise << '\n';
    expect(a.noise > 0.0f, "moving muted string retains a release contact");
    expect(b.noise < 0.25f * a.noise,
           "late Palm release follows remaining motion (>12 dB quieter)");
    render(*late, static_cast<int>(0.30 * rate));
    late->reset();
    expect(Access::contact(*late).motionPeak == 0.0f,
           "reset clears release motion history");
    // A never-excited string must not emit a note-off contact.
    late->noteOff(33);
    const auto silent = render(*late, 512);
    expect(std::all_of(silent.begin(), silent.end(), [](float x) { return x == 0; }),
           "unplayed note-off remains exactly silent");
}

void stopHands(double rate)
{
    auto open = makeEngine(rate), finger = makeEngine(rate), palm = makeEngine(rate, 1);
    for (auto* e : {open.get(), finger.get(), palm.get()})
    {
        const int note = e == open.get() ? 28 : 33;
        e->noteOn(note, 0.85f);
        render(*e, static_cast<int>(0.15 * rate));
        e->noteOff(note);
    }
    const auto o = Access::contact(*open), f = Access::contact(*finger),
               p = Access::contact(*palm);
    expect(f.closureSeconds < 0.6f * o.closureSeconds,
           "finger relaxation is quicker than broad hand closure");
    expect(std::abs(p.closureSeconds - o.closureSeconds) < 1.0e-5f,
           "planted palm owns stopped-note closure");
    expect(o.noiseCorner < 0.6f * f.noiseCorner
               && p.noiseCorner < 0.6f * f.noiseCorner,
           "open/planted hand stop has a darker contact than a finger lift");
    auto half = makeEngine(rate, 0.0f, 0.5f);
    half->noteOn(33, 0.85f);
    render(*half, static_cast<int>(0.15 * rate));
    half->noteOff(33);
    expect(std::abs(Access::contact(*half).noise / f.noise
                       - std::pow(0.5f, 0.75f)) < 1.0e-5f,
           "half release control retains exact event gain under motion scaling");
    for (auto* e : {open.get(), finger.get(), palm.get()})
        render(*e, static_cast<int>(0.5 * rate));
}

void hammerGeometry(double rate)
{
    for (int interval : {2, 7, 12})
    {
        auto e = makeEngine(rate);
        e->noteOn(28, 0.85f);
        render(*e, static_cast<int>(0.12 * rate));
        e->noteOn(Engine::firstPlayStyleKeyswitchNote + 2, 1.0f); // Hammer
        e->noteOn(28 + interval, 0.8f);
        const auto c = Access::contact(*e);
        // Independently locate frets from the nut, x=L(1-2^(-q/12)).
        const double nutPosition = 1.0 - std::pow(2.0, -interval / 12.0);
        const double bridgeDistance = 1.0 - nutPosition;
        expect(c.midi == 28 + interval && c.active,
               "hammer keeps the low physical string");
        expect(std::abs(c.imageDelay / c.period - bridgeDistance) < 1.0e-5,
               "hammer image coincides with landing fret on source string");
        expect(c.imageWidth > 0.0f && c.imageWidth < 0.04f * c.period,
               "finger contact has a finite bounded pad");
        render(*e, static_cast<int>(0.08 * rate));
        e->noteOn(28, 0.8f); // existing pull-off uses old stopped boundary
        const auto pull = Access::contact(*e);
        expect(std::abs(pull.imageDelay - pull.period) < 1.0e-4f,
               "pull-off preserves old-fret release geometry");
        render(*e, static_cast<int>(0.12 * rate));
    }
}

void blockInvariant()
{
    const auto performance = [](int block) {
        auto e = makeEngine(48000);
        std::vector<float> all;
        const auto append = [&](int n) {
            auto part = render(*e, n, block);
            all.insert(all.end(), part.begin(), part.end());
        };
        e->noteOn(28, 0.9f); append(4800);
        e->noteOn(17, 1); e->noteOn(30, 0.75f); append(960);
        e->noteOn(35, 0.75f); append(960); // interrupted hammer trajectory
        e->noteOff(35); append(12000);
        return all;
    };
    expect(performance(1) == performance(257),
           "new contact behavior is sample-identical across callback sizes");
}

void movingHammerGeometry(double rate)
{
    auto e = makeEngine(rate);
    e->setPitchBend(1.0f);
    render(*e, static_cast<int>(0.5 * rate));
    e->noteOn(28, 0.8f);
    render(*e, static_cast<int>(0.1 * rate));
    const auto bentSource = Access::contact(*e);
    e->noteOn(17, 1.0f);
    e->noteOn(35, 0.8f);
    const auto first = Access::contact(*e);
    std::cout << "bent hammer " << rate << ": source period=" << bentSource.period
              << " contact period=" << first.period << " image=" << first.imageDelay << '\n';
    // A Note On can refresh the sub-cent pitch cache between these two
    // snapshots; allow that existing control quantum, not a missing bend.
    expect(std::abs(first.imageDelay / bentSource.period
                     - std::pow(2.0, -7.0 / 12.0)) < 1.0e-4,
           "hammer geometry includes the already-bent source wave speed");
    render(*e, static_cast<int>(0.003 * rate));
    e->noteOn(40, 0.8f);
    const auto second = Access::contact(*e);
    // Three milliseconds into a ten-ms hammer, the source length is between
    // the open string and fret 7, near the open end of that interval. A stale
    // written source (fret 7) would put this ratio at 0.749 instead.
    const float ratio = second.imageDelay / second.period;
    expect(ratio > 0.50f && ratio < 0.65f,
           "interrupted hammer lands relative to the travelling finger");
    render(*e, static_cast<int>(0.3 * rate));

    auto pull = makeEngine(rate);
    pull->noteOn(33, 0.8f);
    render(*pull, static_cast<int>(0.1 * rate));
    pull->noteOn(17, 1.0f);
    pull->noteOn(28, 0.8f);
    render(*pull, static_cast<int>(0.003 * rate));
    pull->noteOff(28);
    expect(std::abs(Access::contact(*pull).closureSeconds - 0.010f) < 1.0e-5f,
           "interrupted pull toward open still releases its fretting finger");
    render(*pull, static_cast<int>(0.3 * rate));
}
}

int main()
{
    for (double rate : {44100.0, 48000.0, 96000.0, 192000.0})
    {
        releaseMotion(rate); stopHands(rate); hammerGeometry(rate);
        movingHammerGeometry(rate);
    }
    blockInvariant();
    if (!failures) std::cout << "Realism contact tests passed at all four sample rates.\n";
    return failures ? 1 : 0;
}
