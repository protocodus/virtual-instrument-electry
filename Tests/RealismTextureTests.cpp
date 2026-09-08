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
    static std::array<float, 2> makeup(const ElectryEngine& e)
    {
        return { e.voices_[0].articulationMakeupCurrent,
                 e.voices_[0].articulationMakeup };
    }
    static double phase(const ElectryEngine& e) { return e.voices_[0].slideRidgePhase; }
    static double internalRate(const ElectryEngine& e) { return e.sampleRate_; }
    static std::vector<float> friction(ElectryEngine& e, int frames,
                                       bool wound, bool legacy = false)
    {
        auto& v = e.voices_[0];
        v.slideNoiseLevel = 1.0f;
        v.slideHasWinding = wound;
        v.slideRidgePhase = 0.0;
        v.slideFrictionEnergy = 0.0f;
        v.slideRidgeCyclesPerSample = 1000.0f / static_cast<float>(e.sampleRate_);
        v.slideBandHigh = std::exp(-6.28318530718f * 1600.0f / e.sampleRate_);
        v.slideBandLow = std::exp(-6.28318530718f * 600.0f / e.sampleRate_);
        v.slideShaperHigh.reset(); v.slideShaperLow.reset();
        v.noiseState = 0x12345678;
        std::vector<float> result(static_cast<std::size_t>(frames));
        for (auto& sample : result)
        {
            if (legacy)
            {
                const float raw = ElectryEngine::bipolarNoise(v.noiseState);
                const float high = v.slideShaperHigh.process(raw, v.slideBandHigh);
                sample = high - v.slideShaperLow.process(high, v.slideBandLow);
            }
            else sample = e.renderSlideFriction(v);
        }
        return result;
    }
    static bool stoppedFriction(ElectryEngine& e)
    {
        auto& v = e.voices_[0];
        v.slideNoiseLevel = 0.0f;
        const double oldPhase = v.slideRidgePhase;
        const auto oldNoise = v.noiseState;
        const float oldEnergy = v.slideFrictionEnergy;
        for (int i = 0; i < 1000; ++i)
            if (e.renderSlideFriction(v) != 0.0f) return false;
        return v.slideRidgePhase == oldPhase && v.noiseState == oldNoise
            && v.slideFrictionEnergy == oldEnergy;
    }
    static bool reversal(ElectryEngine& e)
    {
        auto& v = e.voices_[0];
        v.slideNoiseLevel = 1.0f;
        v.slideRidgePhase = 0.375;
        // Exact binary increment: reversal must retrace the same ridges.
        v.slideRidgeCyclesPerSample = 0.03125f;
        for (int i = 0; i < 51; ++i) e.renderSlideFriction(v);
        v.slideRidgeCyclesPerSample = -v.slideRidgeCyclesPerSample;
        for (int i = 0; i < 51; ++i) e.renderSlideFriction(v);
        return v.slideRidgePhase == 0.375;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
using Style = electry::PlayStyle;
int failures = 0;
void expect(bool value, const std::string& what)
{
    if (! value) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}

std::unique_ptr<Engine> makeEngine(double rate)
{
    auto e = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.pickNoise = p.releaseNoise = p.sympatheticAmount = p.artifactAmount = 0.0f;
    p.bodyResonance = 0.0f;
    p.fingerNoise = 0.7f;
    p.bendTimeSeconds = 0.8f;
    e->setParameters(p); e->prepare(rate, 256); e->setSoloStringMask(1);
    e->noteOn(Engine::firstKeyswitchNote + static_cast<int>(electry::PickStyle::Down), 1.0f);
    return e;
}

void style(Engine& e, Style s)
{
    e.noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(s), 1.0f);
}

void render(Engine& e, int count)
{
    std::array<float, 256> l {}, r {};
    for (int at = 0; at < count; at += 256)
    {
        const int frames = std::min(256, count - at);
        e.process(l.data(), r.data(), frames);
        expect(std::all_of(l.begin(), l.begin() + frames, [] (float x) {
            return std::isfinite(x) && std::abs(x) < 2.0f;
        }), "finite bounded transition audio");
    }
}

void gainTransitions(double rate)
{
    for (auto s : { Style::Sustain, Style::PalmMute, Style::Dead,
                    Style::Harmonics, Style::Pinch, Style::Hammer, Style::Slide })
    {
        auto fresh = makeEngine(rate);
        style(*fresh, s); fresh->noteOn(33, 0.9f);
        const auto g = Access::makeup(*fresh);
        expect(g[0] == g[1], "fresh attacks retain their calibrated immediate gain");
    }

    for (auto s : { Style::PalmMute, Style::Dead, Style::Harmonics,
                    Style::Pinch, Style::Hammer, Style::Slide })
    {
        auto e = makeEngine(rate);
        e->noteOn(33, 0.9f); render(*e, static_cast<int>(rate * 0.1));
        const float old = Access::makeup(*e)[0];
        style(*e, s);
        if (s == Style::Hammer || s == Style::Slide) e->noteOn(35, 0.8f);
        else e->noteOn(Engine::firstRepickNote, 0.8f);
        const auto initial = Access::makeup(*e);
        expect(initial[0] == old && std::abs(initial[1] - old) > 0.05f,
               "ringing string retains pickup gain at articulation contact");
        render(*e, 1);
        auto g = Access::makeup(*e);
        expect(std::abs(g[0] - old) < 0.02f * std::abs(initial[1] - old),
               "first sample removes at most two percent of the gain step");
        const int settle = static_cast<int>(std::lround(rate * 0.006));
        render(*e, settle - 1);
        g = Access::makeup(*e);
        const float fraction = (g[0] - initial[1]) / (old - initial[1]);
        expect(std::abs(fraction - std::exp(-3.0f)) < 0.001f,
               "pickup gain reaches 95 percent in six milliseconds at every rate");
        render(*e, static_cast<int>(rate * 0.025));
        g = Access::makeup(*e);
        expect(std::abs(g[0] - g[1]) < 3.0e-5f, "gain converges without overshoot");
        // Return while still ringing: the downward transition follows the
        // same pole and cannot inherit a stale fresh-note initialisation.
        const float before = g[0];
        style(*e, Style::Sustain); e->noteOn(Engine::firstRepickNote, 0.8f);
        const auto down = Access::makeup(*e);
        expect(down[0] == before, "return to Sustain is continuous at contact");
        render(*e, settle);
        g = Access::makeup(*e);
        expect(g[0] <= before && g[0] >= down[1]
                   && std::abs((g[0] - down[1]) / (before - down[1])
                                - std::exp(-3.0f)) < 0.001f,
               "downward gain transition is monotonic with the same settling time");
    }
}

double rms(const std::vector<float>& x, int begin)
{
    double sum = 0;
    for (std::size_t i = begin; i < x.size(); ++i) sum += x[i] * x[i];
    return std::sqrt(sum / (x.size() - begin));
}

double line(const std::vector<float>& x, double rate, int begin)
{
    double real = 0, imaginary = 0;
    for (std::size_t i = begin; i < x.size(); ++i)
    {
        const double angle = 6.283185307179586 * 1000.0 * i / rate;
        real += x[i] * std::cos(angle); imaginary += x[i] * std::sin(angle);
    }
    return 2.0 * std::hypot(real, imaginary) / (x.size() - begin);
}

void ridgeTexture(double rate)
{
    auto e = makeEngine(rate);
    const double frictionRate = Access::internalRate(*e);
    const int count = static_cast<int>(frictionRate), begin = static_cast<int>(frictionRate * 0.1);
    const auto plain = Access::friction(*e, count, false);
    const auto legacy = Access::friction(*e, count, false, true);
    expect(plain == legacy, "plain string retains exact broadband friction path");
    const auto wound = Access::friction(*e, count, true);
    const double ratio = rms(wound, begin) / rms(plain, begin);
    const double ridgeShare = line(wound, frictionRate, begin) / rms(wound, begin);
    expect(std::isfinite(ratio) && ratio > 0.95 && ratio < 1.05,
           "winding texture preserves friction energy within half a decibel");
    expect(ridgeShare > 0.20 && ridgeShare < 0.31
               && line(wound, frictionRate, begin) > 5.0 * line(plain, frictionRate, begin),
           "moving winding ridges add a modest coherent component at speed/pitch");
    expect(Access::stoppedFriction(*e), "stationary finger freezes ridge and random state");
    expect(Access::reversal(*e), "reversing finger direction retraces winding ridge phase");
    std::cout << "PROBE " << rate << " Hz ridge RMS ratio " << ratio
              << ", coherent amplitude/RMS " << ridgeShare << '\n';

    e = makeEngine(rate);
    e->noteOn(33, 0.9f); render(*e, static_cast<int>(rate * 0.1));
    style(*e, Style::Slide); e->noteOn(40, 0.8f);
    render(*e, static_cast<int>(rate * 0.035));
    const double phase = Access::phase(*e);
    expect(phase > 0.0 && phase < 1.0, "live slide advances a bounded winding phase");
    e->noteOn(35, 0.8f);
    expect(Access::phase(*e) == phase, "redirected slide preserves the same physical ridge phase");
    style(*e, Style::Sustain); e->noteOn(Engine::firstRepickNote, 0.8f);
    expect(Access::phase(*e) == phase, "picking hand does not reset fretting-hand texture");
    e->noteOff(35); e->noteOff(40); e->noteOff(33);
    e->noteOn(34, 0.8f);
    expect(Access::phase(*e) == 0.0, "new finger placement starts a new winding contact");
    e->reset();
    expect(Access::phase(*e) == 0.0, "engine reset clears winding phase");
}
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        gainTransitions(rate); ridgeTexture(rate);
    }
    if (failures) std::cerr << failures << " realism texture checks failed\n";
    else std::cout << "Realism texture and gain tests passed\n";
    return failures ? 1 : 0;
}
