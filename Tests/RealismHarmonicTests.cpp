#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    struct Geometry { float pick, finger, width, inverseLength, spread, period, offset; };
    static Geometry geometry(const ElectryEngine& e, int string = 0)
    {
        const auto& v = e.voices_[static_cast<std::size_t>(string)];
        return { v.excitationCombDelay / v.lastCompensatedPeriod,
                 v.touchFraction, v.touchHalfWidthMetres,
                 v.touchInverseSpeakingLength,
                 ElectryEngine::touchReadSpread(v, v.vertical,
                                                v.compensatedPeriodVertical),
                 v.lastCompensatedPeriod, v.strokeContactOffsetMetres };
    }
    static float scale(const ElectryEngine& e) { return e.scaleLengthMetres(); }
    static std::complex<double> contactResponse(ElectryEngine& e, double omega,
                                                float halfWidth, float depth)
    {
        auto& v = e.voices_[0]; auto& loop = v.vertical;
        loop.currentDelay = 240.375f; loop.writeIndex = 2048;
        v.lastCompensatedPeriod = 256.0f; v.compensatedPeriodVertical = loop.currentDelay;
        v.touchFraction = 0.5f; v.touchHalfWidthMetres = halfWidth;
        v.touchInverseSpeakingLength = 1.0f / 0.2f;
        std::array<double, 2> quadrature {};
        for (int component = 0; component < 2; ++component)
        {
            // Populate every cell the actual cubic stencil can reach.
            for (int at = 1024; at < 2048; ++at)
            {
                const double phase = omega * (at - loop.writeIndex);
                loop.line[static_cast<std::size_t>(at)] = static_cast<float>(
                    component == 0 ? std::cos(phase) : std::sin(phase));
            }
            const float base = loop.readFractional(loop.currentDelay);
            const float touched = ElectryEngine::readTouchedString(
                v, loop, v.compensatedPeriodVertical);
            quadrature[component] = base + 0.5f * depth * (touched - base);
        }
        return { quadrature[0], quadrature[1] };
    }
    static void zeroWidth(ElectryEngine& e) { e.voices_[0].touchHalfWidthMetres = 0; }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
using Style = electry::PlayStyle;
int failures = 0;
void expect(bool ok, const std::string& message)
{
    if (! ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
void style(Engine& e, Style value)
{ e.noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(value), 1); }
std::unique_ptr<Engine> make(double rate, float pick = 0.18f, float age = 0.1f)
{
    auto e = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.pickPosition = pick; p.stringAge = age;
    p.pickNoise = p.fingerNoise = p.releaseNoise = p.artifactAmount = 0;
    p.sympatheticAmount = p.bodyResonance = 0;
    e->setParameters(p); e->prepare(rate, 256); e->setSoloStringMask(1);
    e->noteOn(Engine::firstKeyswitchNote + static_cast<int>(electry::PickStyle::Down), 1);
    return e;
}
std::vector<float> render(Engine& e, int frames, int block = 256)
{
    std::vector<float> result(static_cast<std::size_t>(frames));
    std::array<float, 511> right {};
    for (int at = 0; at < frames; at += block)
        e.process(result.data() + at, right.data(), std::min(block, frames - at));
    expect(std::all_of(result.begin(), result.end(), [] (float x) {
        return std::isfinite(x) && std::abs(x) < 4;
    }), "finite bounded harmonic render");
    return result;
}
double rmsDifference(const std::vector<float>& a, const std::vector<float>& b)
{
    double signal = 0, difference = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
    { signal += a[i] * a[i]; difference += (a[i] - b[i]) * (a[i] - b[i]); }
    return std::sqrt(difference / std::max(signal, 1.0e-20));
}

void pickAndPadGeometry(double rate)
{
    for (int fret : { 0, 6, 12, 22 })
    {
        float previous = -1;
        for (float position : { 0.0f, 0.2f, 0.5f, 0.8f, 1.0f })
        {
            auto e = make(rate, position); style(*e, Style::Harmonics);
            e->noteOn(28 + fret, 0.85f);
            const auto g = Access::geometry(*e);
            const float expectedPick = std::clamp(
                (0.025f + 0.455f * position + g.offset / Access::scale(*e))
                    * std::exp2(fret / 12.0f), 0.02f, 0.98f);
            expect(std::abs(g.pick - expectedPick) < 2.0e-6f,
                   "natural harmonic honours actual plectrum position on fretted length");
            expect(g.pick >= previous, "harmonic pick position moves monotonically");
            previous = g.pick;
            expect(g.finger == 0.5f && g.width == 0.004f,
                   "moving pick preserves midpoint finger with an eight-mm pad");
            const float length = Access::scale(*e) * std::exp2(-fret / 12.0f);
            expect(std::abs(g.inverseLength * length - 1) < 2.0e-6f
                       && std::abs(g.spread / g.period
                            - 0.7745966692f * 0.004f / length) < 2.0e-6f,
                   "contact width follows physical length, independent of delay-filter phase");
        }
    }
    auto fresh = make(rate, 0.18f, 0.0f), old = make(rate, 0.18f, 1.0f);
    style(*fresh, Style::Harmonics); style(*old, Style::Harmonics);
    fresh->noteOn(34, 0.85f); old->noteOn(34, 0.85f);
    expect(std::abs(Access::geometry(*fresh).spread - Access::geometry(*old).spread) < 2.0e-5f,
           "string age does not widen the finger pad");
    const auto before = Access::geometry(*fresh);
    fresh->setPitchBend(1.0f); render(*fresh, static_cast<int>(rate * 0.15));
    const auto bent = Access::geometry(*fresh);
    expect(std::abs(bent.inverseLength - before.inverseLength) < 1.0e-6f
               && bent.spread < before.spread,
           "bend raises wave speed while retaining pad width and speaking length");

    auto near = make(rate, 0.1f), far = make(rate, 0.65f);
    style(*near, Style::Harmonics); style(*far, Style::Harmonics);
    near->noteOn(28, 0.85f); far->noteOn(28, 0.85f);
    const auto a = render(*near, static_cast<int>(rate * 0.25));
    const auto b = render(*far, static_cast<int>(rate * 0.25));
    expect(rmsDifference(a, b) > 0.1, "harmonic Pick Position makes an audible waveform change");
}

void passivePad(double rate)
{
    auto e = make(rate);
    double maximum = 0;
    for (float width : { 0.0f, 0.001f, 0.004f, 0.02f })
        for (int k = 0; k <= 128; ++k)
            for (float depth : { 0.0f, 0.5f, 0.92f, 1.0f })
                maximum = std::max(maximum, std::abs(Access::contactResponse(
                    *e, 3.141592653589793 * k / 128.0, width, depth)));
    expect(maximum <= 1.000001, "actual cubic finite-pad operator never exceeds unity gain");
    double lowLoss = 0, highLoss = 0;
    for (int harmonic : { 2, 12 })
    {
        const double omega = 6.283185307179586 * harmonic / 256;
        const double point = std::abs(Access::contactResponse(*e, omega, 0, 0.92f));
        const double pad = std::abs(Access::contactResponse(*e, omega, 0.004f, 0.92f));
        const double expected = 1 - 0.46 + 0.46 * (4.0 / 9.0 + 5.0 / 9.0
            * std::cos(6.283185307179586 * harmonic * 0.02 * std::sqrt(0.6)));
        expect(pad < point && std::abs(pad - expected) < 0.0003,
               "pad removes node-adjacent vibration with independently predicted mode response");
        if (harmonic == 2) lowLoss = 1 - pad; else highLoss = 1 - pad;
    }
    expect(highLoss > 20 * lowLoss, "shorter surviving wavelengths feel much stronger contact loss");
    std::cout << "PROBE " << rate << " Hz pad max gain " << maximum
              << ", H2/H12 loss " << lowLoss << '/' << highLoss << '\n';

    auto point = make(rate), pad = make(rate);
    style(*point, Style::Harmonics); style(*pad, Style::Harmonics);
    point->noteOn(40, 0.8f); pad->noteOn(40, 0.8f); Access::zeroWidth(*point);
    const auto a = render(*point, static_cast<int>(rate * 0.3));
    const auto b = render(*pad, static_cast<int>(rate * 0.3));
    expect(rmsDifference(a, b) > 0.01, "finite pad reaches rendered harmonic audio");
}

void callbacks(double rate)
{
    const auto score = [rate] (int block) {
        auto e = make(rate); std::vector<float> out;
        const auto wait = [&] (double seconds) {
            auto x = render(*e, static_cast<int>(seconds * rate), block);
            out.insert(out.end(), x.begin(), x.end());
        };
        style(*e, Style::Harmonics); e->noteOn(30, 0.8f); wait(0.03);
        style(*e, Style::Pinch); e->noteOn(Engine::firstRepickNote, 0.8f); wait(0.015);
        style(*e, Style::PalmMute); e->noteOn(Engine::firstRepickNote, 0.7f); wait(0.04);
        style(*e, Style::Harmonics); e->noteOn(Engine::firstRepickNote, 0.8f); wait(0.2);
        e->noteOff(30); wait(0.2); e->reset();
        const auto quiet = render(*e, 1024, block);
        expect(std::all_of(quiet.begin(), quiet.end(), [] (float x) { return x == 0; }),
               "reset clears contact history to exact silence");
        return out;
    };
    const auto reference = score(1);
    for (int block : { 17, 128, 511 })
        expect(score(block) == reference, "harmonic and thumb contact are callback-invariant");
}
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    { pickAndPadGeometry(rate); passivePad(rate); callbacks(rate); }
    if (failures) std::cerr << failures << " harmonic realism checks failed\n";
    else std::cout << "Harmonic geometry and finite-pad checks passed\n";
    return failures ? 1 : 0;
}
