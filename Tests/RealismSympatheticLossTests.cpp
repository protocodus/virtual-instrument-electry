// The idle bass material section must bend intrinsic loss between the existing
// two anchors, not buy a darker tail by shortening the fundamental. Compare
// its complete response with an independent double-precision passive one-pole
// solve, then exercise the production idle renderer with the section ablated.
// The ablation is an activation witness, not an alternative instrument voicing.
#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <memory>
#include <string>

namespace electry
{
struct ElectryEngineTestAccess
{
    struct Response
    {
        double rate, frequency, coefficient, gain, depth;
        std::array<double, 5> material;
        bool handActive;
        std::array<double, 5> hand;
        std::array<double, 5> loss {1, 0, 0, 0, 0};
        double lossGain = 1, lossZero = 0;
    };

    static Response inspect(ElectryEngine& engine, int stringIndex, double frequency)
    {
        const auto& loop = engine.voices_[static_cast<std::size_t>(stringIndex)].vertical;
        const auto& b = loop.materialLossDip;
        const auto& h = loop.handDip;
        Response result { engine.sampleRate_, frequency,
                 loop.loopDampingCoefficient, loop.loopGain,
                 loop.materialLossDepth, { b.b0, b.b1, b.b2, b.a1, b.a2 },
                 loop.handDipActive, { h.b0, h.b1, h.b2, h.a1, h.a2 } };
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
        if (loop.lossShelfActive)
        {
            result.lossGain = loop.lossShelfGain;
            result.lossZero = loop.lossShelfZeroCoefficient;
            const auto& loss = loop.lossShelf;
            result.loss = {loss.b0, loss.b1, loss.b2, loss.a1, loss.a2};
        }
#endif
        return result;
    }

    static Response configure(ElectryEngine& engine, int stringIndex)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        engine.configureSympatheticString(voice);
        voice.sympatheticReady = true;
        return inspect(engine, stringIndex, voice.lastConfiguredFrequency);
    }

    static Response playedOpen(ElectryEngine& engine, int stringIndex, double frequency)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        engine.configureVoiceDamping(voice, PlayStyle::Sustain,
                                     static_cast<float>(frequency), 0.0f);
        return inspect(engine, stringIndex, frequency);
    }

    static bool configurationRetainsRunningState(ElectryEngine& engine)
    {
        auto& voice = engine.voices_[0];
        configure(engine, 0);
        if (!(voice.vertical.materialLossDepth > 0)) return false;
        voice.vertical.materialLossDip.z1 = 0.000125;
        voice.vertical.materialLossDip.z2 = -0.0000625;
        voice.horizontal.materialLossDip.z1 = 0.25;
        voice.horizontal.materialLossDip.z2 = -0.125;
        const float delay = voice.vertical.currentDelay;
        for (int repeat = 0; repeat < 4; ++repeat)
        {
            configure(engine, 0);
            if (voice.vertical.materialLossDip.z1 != 0.000125
                || voice.vertical.materialLossDip.z2 != -0.0000625
                || voice.horizontal.materialLossDip.z1 != 0
                || voice.horizontal.materialLossDip.z2 != 0
                // An unchanged refit is exactly idempotent. Adding then
                // subtracting the same phase moved this raw delay by one
                // float ULP at 48 kHz even while every history stayed intact.
                || voice.vertical.currentDelay != delay)
                return false;
        }
        // A retired played voice has different-pitch history; unlike a live
        // refit, its first idle configuration must not revive that section.
        voice.sympatheticReady = false;
        configure(engine, 0);
        if (voice.vertical.materialLossDip.z1 != 0
            || voice.vertical.materialLossDip.z2 != 0) return false;
        voice.vertical.materialLossDip.z1 = 0.000125;
        engine.reset();
        return voice.vertical.materialLossDip.z1 == 0
            && voice.vertical.materialLossDip.z2 == 0;
    }

    static double renderSample(ElectryEngine& engine, int stringIndex,
                               float drive, bool ablate)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        if (ablate) voice.vertical.materialLossDepth = 0;
        engine.sympatheticInjection_ = 0.001f;
        engine.feedbackDrive_ = 0;
        ElectryEngine::RenderSums sums {};
        engine.renderSympatheticString(voice, sums, drive);
        return voice.vertical.line[static_cast<std::size_t>(
            (voice.vertical.writeIndex - 1) & (ElectryEngine::delayLineSize - 1))];
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
constexpr double pi = 3.14159265358979323846;
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}

std::unique_ptr<Engine> makeEngine(double rate, float age = .3f,
                                  float gauge = 1.0f, float pressure = 0)
{
    auto engine = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.stringAge = age;
    p.stringGauge = gauge;
    p.palmMute = pressure;
    p.bodyResonance = 0;
    p.pickNoise = p.fingerNoise = p.releaseNoise = p.artifactAmount = 0;
    engine->setParameters(p);
    engine->prepare(rate, 512);
    engine->reset();
    return engine;
}

double poleMagnitude(double a, double hz, double rate)
{
    const double sine = std::sin(pi * hz / rate), gap = 1.0 - a;
    return gap / std::sqrt(gap * gap + 4.0 * a * sine * sine);
}

double magnitude(const Access::Response& r, double hz)
{
    double result = r.gain * poleMagnitude(r.coefficient, hz, r.rate);
    const auto section = [&](const std::array<double, 5>& b)
    {
        const auto z = std::polar(1.0, -2.0 * pi * hz / r.rate);
        return std::abs((b[0] + b[1] * z + b[2] * z * z)
                        / (1.0 + b[3] * z + b[4] * z * z));
    };
    result *= r.lossGain * std::abs(1.0 - r.lossZero
        * std::polar(1.0, -2.0 * pi * hz / r.rate)) * section(r.loss);
    if (r.depth > 0) result *= section(r.material);
    if (r.handActive) result *= section(r.hand);
    return result;
}

double t60(double gain, double frequency)
{
    return -3.0 / (frequency * std::log10(gain));
}

struct Baseline { double low, high, middle, requestedGain; };
Baseline reference(double frequency, double rate, double lowT60, double highT60)
{
    const double lowGain = std::pow(10.0, -3.0 / (frequency * lowT60));
    const auto solve = [&](double target)
    {
        const double highGain = std::pow(10.0, -3.0 / (frequency * target));
        const double ratio = highGain / lowGain;
        double lo = 0, hi = .9995;
        for (int i = 0; i < 64; ++i)
        {
            const double a = .5 * (lo + hi);
            if (poleMagnitude(a, 3600, rate) / poleMagnitude(a, frequency, rate) > ratio)
                lo = a;
            else hi = a;
        }
        const double a = .5 * (lo + hi);
        return std::array<double, 2> { a, lowGain / poleMagnitude(a, frequency, rate) };
    };
    auto fit = solve(highT60);
    const double requestedGain = fit[1];
    if (fit[1] > .9999)
    {
        double lo = highT60, hi = lowT60;
        for (int i = 0; i < 64; ++i)
        {
            const double target = .5 * (lo + hi);
            if (solve(target)[1] > .9999) lo = target;
            else hi = target;
        }
        fit = solve(hi);
    }
    return { lowT60,
             t60(fit[1] * poleMagnitude(fit[0], 3600, rate), frequency),
             t60(fit[1] * poleMagnitude(fit[0], std::sqrt(frequency * 3600), rate), frequency),
             requestedGain };
}

void anchorGrid(double rate)
{
    constexpr std::array<double, 8> openTargets { 20, 19, 18, 16.5, 15, 12, 10, 8.5 };
    constexpr std::array<double, 8> deadSpotCentres { 8, 9, 9.5, 11, 12.5, 6, 5, 7.5 };
    for (float age : { 0.0f, .3f, 1.0f })
        for (float gauge : { 0.0f, .65f, 1.0f })
            for (float pressure : { 0.0f, .5f, 1.0f })
            {
                auto engine = makeEngine(rate, age, gauge, pressure);
                auto played = pressure == 0 ? makeEngine(rate, age, gauge) : nullptr;
                for (int stringIndex = 0; stringIndex < 8; ++stringIndex)
                {
                    const auto r = Access::configure(*engine, stringIndex);
                    // Independently reconstruct the established played-open
                    // intrinsic target. Which source excites a free string
                    // must not change these multipliers or its 26 s ceiling.
                    const auto index = static_cast<std::size_t>(stringIndex);
                    const double centre = deadSpotCentres[index];
                    const double termination = 1.0 / (1.0 + .03
                        * std::exp(-centre * centre / (2 * 1.65 * 1.65)));
                    double low = std::clamp(openTargets[index]
                        * (1.08 - .70 * age) * 1.16 * (.88 + .30 * gauge)
                        * 1.05 * 1.04 * termination, .03, 26.0);
                    if (pressure > 0)
                        low = std::exp((1.0 - pressure) * std::log(low)
                                       + pressure * std::log(.080));
                    low = std::clamp(low, .03, 26.0);
                    double ratio = (.036 - .026 * age) * (1.15 - .37 * gauge);
                    if (stringIndex >= 5) ratio *= 7.5;
                    // The measured-body model and default legacy-body fixture
                    // both use these neutral wood/construction factors.
                    ratio *= .86 * .58;
                    ratio = std::clamp(ratio * (1.0 - .38 * pressure), .0015, .9);
                    const auto old = reference(r.frequency, r.rate, low,
                        std::clamp(low * ratio, .008, low));
                    const std::string label = " rate=" + std::to_string(rate)
                        + " string=" + std::to_string(stringIndex)
                        + " age=" + std::to_string(age) + " gauge=" + std::to_string(gauge)
                        + " pressure=" + std::to_string(pressure);
                    expect(std::abs(t60(magnitude(r, r.frequency), r.frequency) / old.low - 1) < .005,
                           "material loss preserves fundamental" + label);
                    expect(std::abs(t60(magnitude(r, 3600), r.frequency) / old.high - 1) < .01,
                           "material loss preserves feasible upper anchor" + label);
                    if (pressure == 0)
                    {
                        const auto picked = Access::playedOpen(*played, stringIndex, r.frequency);
                        const double idleT60 = t60(magnitude(r, r.frequency), r.frequency);
                        const double pickedT60 = t60(magnitude(picked, picked.frequency), picked.frequency);
                        expect(!r.handActive && !picked.handActive,
                               "no-hand comparison has no hand-loss section" + label);
                        // Eligibility comes from the independent unconstrained
                        // pole solve, never from whichever output is under test.
                        // The shipping no-hand played path still clamps scalar
                        // gain when old wound strings request an infeasible
                        // tilt. That separate defect shortens its fundamental
                        // (age 1/gauge 1 E1 is about 1.16 s against 11.36 s
                        // at 48 kHz).
                        // Keep the corrected idle law pinned to its target in
                        // that case; do not reproduce the played clamp here.
                        if (old.requestedGain <= .99999)
                            expect(std::abs(pickedT60 / low - 1) < .005
                                   && std::abs(idleT60 / pickedT60 - 1) < .005,
                                   "feasible idle and played open strings share the realized fundamental decay" + label
                                   + " target=" + std::to_string(low)
                                   + " idle=" + std::to_string(idleT60)
                                   + " played=" + std::to_string(pickedT60));
                        // The idle .9999 scalar ceiling can relax the upper
                        // target more than the played .99999 ceiling. Test its
                        // own passive solution above, not false exact parity.
                    }
                    if (pressure == 1)
                        expect(std::abs(t60(magnitude(r, r.frequency), r.frequency) / .080 - 1) < .005,
                               "full bridge-hand pressure keeps the 80 ms endpoint" + label);
                    if (stringIndex < 2 && age == .3f && gauge == 1 && pressure == 0)
                        expect(r.depth > 0
                               && t60(magnitude(r, std::sqrt(r.frequency * 3600)), r.frequency)
                                   < old.middle * .95,
                               "idle bass material curvature has a mid-band witness" + label);
                    if (stringIndex >= 2)
                        expect(r.depth == 0, "six-string surface bypasses bass curvature" + label);
                    double maximum = 0;
                    // Log spacing resolves the narrow bass region; a linear
                    // grid separately covers the rest through Nyquist.
                    for (int k = 0; k <= 512; ++k)
                    {
                        maximum = std::max(maximum, magnitude(r, .5 * r.rate * k / 512));
                        maximum = std::max(maximum, magnitude(r, std::exp(
                            std::log(10.0) + k / 512.0 * std::log(.05 * r.rate))));
                    }
                    expect(std::isfinite(maximum) && maximum < 1.0,
                           "complete idle loss cascade is passive" + label);
                }
            }
    auto engine = makeEngine(rate);
    for (int stringIndex = 2; stringIndex < 8; ++stringIndex)
        expect(Access::configure(*engine, stringIndex).depth == 0,
               "six-string surface bypasses the bass correction");
    expect(Access::configurationRetainsRunningState(*engine),
           "live refits retain history; fresh handoff and reset clear it at " + std::to_string(rate));
}

struct Bandpass
{
    double b0, b2, a1, a2, z1 = 0, z2 = 0;
    Bandpass(double rate, double hz)
    {
        const double w = 2 * pi * hz / rate, alpha = std::sin(w) / 4;
        b0 = alpha / (1 + alpha); b2 = -b0;
        a1 = -2 * std::cos(w) / (1 + alpha); a2 = (1 - alpha) / (1 + alpha);
    }
    double process(double x)
    {
        const double y = b0 * x + z1;
        z1 = -a1 * y + z2; z2 = b2 * x - a2 * y;
        return y;
    }
};

void renderedWitness(double hostRate)
{
    for (int stringIndex = 0; stringIndex < 2; ++stringIndex)
    {
        auto intact = makeEngine(hostRate), ablated = makeEngine(hostRate);
        const auto r = Access::configure(*intact, stringIndex);
        Access::configure(*ablated, stringIndex);
        Bandpass a(r.rate, std::sqrt(r.frequency * 3600));
        Bandpass b(r.rate, std::sqrt(r.frequency * 3600));
        double early[2] {}, late[2] {};
        bool finite = true;
        for (int i = 0; i < static_cast<int>(.65 * r.rate); ++i)
        {
            const double t = i / r.rate;
            const double rawA = Access::renderSample(*intact, stringIndex, i == 0 ? 1.0f : 0.0f, false);
            const double rawB = Access::renderSample(*ablated, stringIndex, i == 0 ? 1.0f : 0.0f, true);
            finite = finite && std::isfinite(rawA) && std::isfinite(rawB)
                            && std::abs(rawA) < .01 && std::abs(rawB) < .01;
            const double filtered[2] { a.process(rawA), b.process(rawB) };
            for (int version = 0; version < 2; ++version)
            {
                if (t >= .10 && t < .20) early[version] += filtered[version] * filtered[version];
                if (t >= .50 && t < .60) late[version] += filtered[version] * filtered[version];
            }
        }
        const double intactCooling = 10 * std::log10(late[0] / early[0]);
        const double ablatedCooling = 10 * std::log10(late[1] / early[1]);
        std::cout << "PROBE idle material cooling " << hostRate << " Hz string " << stringIndex
                  << ": intact=" << intactCooling << " dB ablated=" << ablatedCooling << " dB\n";
        expect(finite && early[0] > 1e-15 && early[1] > 1e-15,
               "real idle renderer is finite and measurably excited");
        expect(intactCooling < ablatedCooling - .5,
               "real idle renderer applies the material section at " + std::to_string(hostRate));
    }
}
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0, 384000.0 })
    {
        anchorGrid(rate);
        renderedWitness(rate);
    }
    if (failures) return 1;
    std::cout << "PASS: shared open decay, idle material anchors, passivity, lifecycle and rendered cooling\n";
    return 0;
}
