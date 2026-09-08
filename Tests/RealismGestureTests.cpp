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
    struct Gesture
    {
        float fret, frequency, seconds, blend, noise, friction, length;
        bool physicalSlide, pendingFinger;
        int delay;
    };
    static Gesture gesture(const ElectryEngine& e)
    {
        const auto& v = e.voices_[0];
        return { ElectryEngine::performedFret(v),
                 v.baseFrequency * std::exp2(ElectryEngine::legatoPitchOffset(v) / 12.0f),
                 v.legatoIncrement > 0.0f
                     ? static_cast<float>(ElectryEngine::controlPeriod)
                           / (v.legatoIncrement * static_cast<float>(e.sampleRate_))
                     : 0.0f,
                 v.legatoBlend, v.noiseAmplitude, v.slideNoiseLevel,
                 e.scaleLengthMetres(), v.legatoUsesLengthTrajectory,
                 v.frettingContactPending, v.startDelaySamples };
    }
    static void atBlend(ElectryEngine& e, float blend)
    {
        e.voices_[0].legatoBlend = blend;
        e.configureVoicePitch(e.voices_[0], false);
    }
    static void retire(ElectryEngine& e) { e.silenceVoice(e.voices_[0]); }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
int failures = 0;
void expect(bool value, const std::string& what)
{
    if (! value) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}

std::unique_ptr<Engine> makeEngine(double rate, float bend = 0.8f,
                                 float scale = 0.85f, float spread = 0.0f)
{
    auto e = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.pickNoise = p.releaseNoise = p.sympatheticAmount = p.artifactAmount = 0.0f;
    p.bodyResonance = 0.0f;
    p.fingerNoise = 1.0f;
    p.bendTimeSeconds = bend;
    p.scaleLength = scale;
    p.strumSpreadSeconds = spread;
    e->setParameters(p);
    e->prepare(rate, 256);
    e->setSoloStringMask(1);
    return e;
}

std::vector<float> render(Engine& e, int count, int block = 256)
{
    std::vector<float> out(static_cast<std::size_t>(count));
    std::array<float, 512> right {};
    for (int at = 0; at < count; at += block)
        e.process(out.data() + at, right.data(), std::min(block, count - at));
    expect(std::all_of(out.begin(), out.end(), [](float x) {
        return std::isfinite(x) && std::abs(x) < 2.0f;
    }), "finite bounded gesture render");
    return out;
}

void style(Engine& e, electry::PlayStyle s)
{
    e.noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(s), 1.0f);
}

void beginSlide(Engine& e, int from, int to, double rate)
{
    e.noteOn(28 + from, 0.85f);
    render(e, static_cast<int>(0.12 * rate));
    style(e, electry::PlayStyle::Slide);
    e.noteOn(28 + to, 0.8f);
}

void physicalSlides(double rate)
{
    auto low = makeEngine(rate), high = makeEngine(rate), reverse = makeEngine(rate);
    beginSlide(*low, 0, 2, rate);
    beginSlide(*high, 12, 14, rate);
    beginSlide(*reverse, 4, 2, rate);
    const auto a = Access::gesture(*low), b = Access::gesture(*high),
               c = Access::gesture(*reverse);
    // The same interval an octave higher travels exactly half the distance.
    // These slow fixtures remain above the explicit 30 ms minimum.
    expect(std::abs(b.seconds / a.seconds - 0.5f) < 2.0e-5f,
           "equal interval above octave takes half the physical travel time");
    expect(std::abs(a.seconds - 0.128f) < 2.0e-5f,
           "default-scale nut-to-second-fret timing retains calibration");
    expect(std::abs(c.seconds / a.seconds - std::exp2(-2.0f / 12.0f)) < 2.0e-5f,
           "reverse physical slide follows its actual neck distance");

    auto shortScale = makeEngine(rate, 0.8f, 0.0f);
    auto longScale = makeEngine(rate, 0.8f, 1.0f);
    beginSlide(*shortScale, 2, 7, rate);
    beginSlide(*longScale, 2, 7, rate);
    expect(std::abs(Access::gesture(*longScale).seconds
                       / Access::gesture(*shortScale).seconds - 28.0f / 25.5f)
               < 2.0e-5f,
           "longer scale takes proportionally longer at the same hand speed");

    auto octave = makeEngine(rate);
    beginSlide(*octave, 0, 12, rate);
    const float length = Access::gesture(*octave).length;
    for (float blend : { 0.25f, 0.5f, 0.75f })
    {
        Access::atBlend(*octave, blend);
        const auto g = Access::gesture(*octave);
        const float progress = blend * blend * (3.0f - 2.0f * blend);
        const float expectedLength = length * (1.0f - 0.5f * progress);
        const float observedLength = length * std::exp2(-g.fret / 12.0f);
        expect(std::abs(observedLength - expectedLength) < 2.0e-6f,
               "pitch and fret describe a smoothstep in physical metres");
        const float openFrequency = 440.0f * std::exp2((28.0f - 69.0f) / 12.0f);
        expect(std::abs(g.frequency * observedLength / (openFrequency * length)
                            - 1.0f) < 2.0e-6f,
               "slide conserves transverse wave speed");
    }
    Access::atBlend(*octave, 0.35f);
    const auto before = Access::gesture(*octave);
    octave->noteOn(35, 0.75f);
    const auto after = Access::gesture(*octave);
    expect(std::abs(after.fret - before.fret) < 2.0e-5f
               && std::abs(after.frequency - before.frequency) < 1.0e-4f,
           "interrupted slide starts at the actual travelling finger");
    style(*octave, electry::PlayStyle::Sustain);
    octave->noteOn(Engine::firstRepickNote, 0.85f);
    expect(Access::gesture(*octave).physicalSlide,
           "picking-hand repick preserves the unfinished physical trajectory");
    render(*octave, static_cast<int>(0.01 * rate));
    const auto release = Access::gesture(*octave);
    octave->noteOff(35);
    render(*octave, static_cast<int>(0.01 * rate));
    expect(Access::gesture(*octave).fret == release.fret
               && Access::gesture(*octave).friction == 0.0f,
           "lifted slide finger freezes its actual position and friction");
}

void frettingOwnership(double rate)
{
    auto e = makeEngine(rate);
    e->noteOn(33, 0.85f);
    expect(Access::gesture(*e).noise > 0.0f,
           "a fresh fretted note retains its finger contact");
    render(*e, static_cast<int>(0.1 * rate));
    e->noteOn(Engine::firstRepickNote, 0.85f);
    expect(Access::gesture(*e).noise == 0.0f,
           "picking-hand repick does not invent another finger landing");
    e->noteOn(33, 0.85f);
    expect(Access::gesture(*e).noise == 0.0f,
           "overlapping Note On retains the already placed finger");
    e->noteOff(33); // release the extra owner only
    Access::retire(*e); // durable fretting-key owner deliberately remains
    e->noteOn(Engine::firstRepickNote, 0.85f);
    expect(Access::gesture(*e).noise == 0.0f,
           "repick of a retired held string retains the physical finger");
    e->noteOff(33);
    e->noteOn(33, 0.85f);
    expect(Access::gesture(*e).noise > 0.0f,
           "finger lift then replacement restores contact noise");
    e->noteOn(34, 0.85f);
    expect(Access::gesture(*e).noise > 0.0f,
           "a different fret supplies a new physical contact");

    auto delayed = makeEngine(rate, 0.8f, 0.85f, 0.01f);
    delayed->noteOn(33, 0.85f);
    expect(Access::gesture(*delayed).pendingFinger,
           "fresh strum retains the not-yet-played finger contact");
    delayed->noteOn(33, 0.85f);
    render(*delayed, static_cast<int>(0.07 * rate));
    expect(Access::gesture(*delayed).noise > 0.0f
               && ! Access::gesture(*delayed).pendingFinger,
           "rescheduling preserves and consumes exactly one finger landing");
    delayed->noteOn(Engine::firstRepickNote, 0.85f);
    render(*delayed, static_cast<int>(0.07 * rate));
    expect(Access::gesture(*delayed).noise == 0.0f,
           "delayed picking-hand arrival keeps the existing finger silent");
    delayed->noteOn(Engine::firstRepickNote, 0.85f);
    style(*delayed, electry::PlayStyle::Hammer);
    delayed->noteOn(35, 0.8f);
    expect(Access::gesture(*delayed).noise > 0.0f,
           "a hammer before the reserved pick supplies its own finger contact");
    render(*delayed, static_cast<int>(0.07 * rate));
    expect(Access::gesture(*delayed).noise == 0.0f,
           "reserved pick after a hammer does not replay the finger contact");
}

void callbackInvariance(double rate)
{
    const auto score = [rate] (int block) {
        auto e = makeEngine(rate, 0.28f);
        std::vector<float> result;
        const auto append = [&] (double seconds) {
            auto audio = render(*e, static_cast<int>(seconds * rate), block);
            result.insert(result.end(), audio.begin(), audio.end());
        };
        e->noteOn(30, 0.9f); append(0.12);
        style(*e, electry::PlayStyle::Slide);
        e->noteOn(40, 0.8f); append(0.045);
        e->noteOn(34, 0.8f); append(0.013);
        style(*e, electry::PlayStyle::Sustain);
        e->noteOn(Engine::firstRepickNote, 0.75f); append(0.055);
        e->noteOff(34); append(0.10);
        e->reset();
        const auto silence = render(*e, 512, block);
        expect(std::all_of(silence.begin(), silence.end(), [](float x) { return x == 0; }),
               "gesture reset is exactly silent");
        return result;
    };
    const auto reference = score(1);
    for (int block : { 17, 128, 511 })
        expect(score(block) == reference,
               "slide/repick/release audio is independent of callback size");
}
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        physicalSlides(rate);
        frettingOwnership(rate);
        callbackInvariance(rate);
    }
    if (failures) std::cerr << failures << " gesture checks failed\n";
    return failures ? 1 : 0;
}
