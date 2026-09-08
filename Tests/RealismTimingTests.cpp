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
    struct State
    {
        float frequency, intendedFrequency, slideSeconds, blend;
        float thumbDepth, thumbFraction, pendingFraction, remainder;
        float delayRetention, internalRate;
        int phase, contactFrames, thumbHold, ramp;
        bool pending;
    };
    static State state(const ElectryEngine& e)
    {
        const auto& v = e.voices_[0];
        const float period = v.vertical.currentDelay + v.lastCompensatedPeriod
                           - v.compensatedPeriodVertical;
        return { static_cast<float>(e.sampleRate_) / period,
                 static_cast<float>(e.sampleRate_) / v.lastCompensatedPeriod,
                 v.legatoIncrement > 0.0f
                     ? ElectryEngine::controlPeriod / (v.legatoIncrement
                         * static_cast<float>(e.sampleRate_)) : 0.0f,
                 v.legatoBlend, v.touchDepth, v.touchFraction,
                 v.pendingPinchTouchFraction, v.slideDelayPending,
                 v.vertical.delayRetention, static_cast<float>(e.sampleRate_),
                 static_cast<int>(v.excitationPhase), v.excitationRemaining,
                 v.touchHoldRemaining, v.slideDelayRampRemaining,
                 v.pinchTouchPending };
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
using Style = electry::PlayStyle;
int failures = 0;
void expect(bool condition, const std::string& message)
{
    if (! condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
float hz(int note) { return 440.0f * std::exp2((note - 69.0f) / 12.0f); }
float cents(float a, float b) { return 1200.0f * std::log2(a / b); }

std::unique_ptr<Engine> makeEngine(double rate, float hardness = 0.7f,
                                 float spread = 0.0f)
{
    auto e = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.pickNoise = p.fingerNoise = p.releaseNoise = p.artifactAmount = 0.0f;
    p.sympatheticAmount = p.bodyResonance = 0.0f;
    p.bendTimeSeconds = 0.05f;
    p.strumSpreadSeconds = spread;
    p.pickHardness = hardness;
    e->setParameters(p); e->prepare(rate, 512); e->setSoloStringMask(1);
    return e;
}
void style(Engine& e, Style s)
{
    e.noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(s), 1.0f);
}
std::vector<float> render(Engine& e, int frames, int block = 256)
{
    std::vector<float> output(static_cast<std::size_t>(frames));
    std::array<float, 512> right {};
    for (int at = 0; at < frames; at += block)
        e.process(output.data() + at, right.data(), std::min(block, frames - at));
    expect(std::all_of(output.begin(), output.end(), [](float x) {
        return std::isfinite(x) && std::abs(x) < 4.0f;
    }), "gesture output stays finite and bounded");
    return output;
}

double audioPitch(const std::vector<float>& audio, double rate, double expected)
{
    // Locate the fundamental in the actual pickup output. A Hann window
    // excludes distant, stiff-string upper partials whose stretched periods
    // would bias a broadband autocorrelation toward a slightly sharper pitch.
    constexpr double pi = 3.14159265358979323846;
    const auto power = [&](double detune)
    {
        const double frequency = expected * std::exp2(detune / 1200.0);
        const double angle = 2.0 * pi * frequency / rate;
        const double stepCos = std::cos(angle), stepSin = std::sin(angle);
        double c = 1.0, s = 0.0, real = 0.0, imag = 0.0;
        for (std::size_t i = 0; i < audio.size(); ++i)
        {
            const double weight = 0.5 - 0.5 * std::cos(
                2.0 * pi * i / (audio.size() - 1));
            real += weight * audio[i] * c;
            imag += weight * audio[i] * s;
            const double nextCos = c * stepCos - s * stepSin;
            s = s * stepCos + c * stepSin; c = nextCos;
        }
        return real * real + imag * imag;
    };
    double best = -35.0, bestPower = power(best);
    for (double detune = -34.5; detune <= 35.0; detune += 0.5)
    {
        const double p = power(detune);
        if (p > bestPower) { best = detune; bestPower = p; }
    }
    const double left = power(best - 0.5), right = power(best + 0.5);
    const double refinement = 0.25 * (left - right)
                            / (left - 2.0 * bestPower + right);
    return expected * std::exp2((best + std::clamp(refinement, -0.5, 0.5)) / 1200.0);
}

void slideArrival(double rate)
{
    float worstArrival = 0.0f;
    for (const auto notes : { std::array<int, 2>{30, 28}, {28, 30},
                             {40, 42}, {42, 40}, {30, 40}, {40, 30} })
    {
        auto e = makeEngine(rate);
        e->noteOn(notes[0], 0.8f); render(*e, static_cast<int>(rate * 0.18));
        style(*e, Style::Slide); e->noteOn(notes[1], 0.8f);
        const auto beginning = Access::state(*e);
        expect(std::abs(cents(beginning.frequency, hz(notes[0]))) < 1.0f,
               "slide begins continuously at the source pitch");
        const int travel = static_cast<int>(std::ceil(rate * beginning.slideSeconds));
        float previous = beginning.frequency;
        float largestStep = 0.0f;
        for (int frame = 0; frame < travel; ++frame)
        {
            render(*e, 1, 1);
            const float current = Access::state(*e).frequency;
            largestStep = std::max(largestStep, std::abs(cents(current, previous)));
            previous = current;
        }
        const auto arrival = Access::state(*e);
        const float arrivalError = std::abs(cents(arrival.frequency, hz(notes[1])));
        worstArrival = std::max(worstArrival, arrivalError);
        expect(arrivalError < 2.0f,
               "physical slide arrives within two cents at the finger's arrival");
        expect(largestStep < 3.0f,
               "slide period advances continuously rather than jumping at arrival");
        render(*e, static_cast<int>(rate * 0.04));
        const auto settled = render(*e, static_cast<int>(rate * 0.20));
        const double measured = audioPitch(settled, rate, hz(notes[1]));
        expect(std::abs(cents(static_cast<float>(measured), hz(notes[1]))) < 6.0f,
               "rendered slide tail reaches the requested musical pitch ("
                   + std::to_string(notes[0]) + "->" + std::to_string(notes[1])
                   + ": " + std::to_string(measured) + " vs "
                   + std::to_string(hz(notes[1])) + ")");
    }
    std::cout << "Slide " << rate << " Hz worst arrival " << worstArrival
              << " cents\n";

    auto e = makeEngine(rate);
    e->noteOn(30, 0.8f); render(*e, static_cast<int>(rate * 0.18));
    style(*e, Style::Slide); e->noteOn(42, 0.8f);
    render(*e, static_cast<int>(rate * 0.013));
    const float before = Access::state(*e).frequency;
    e->noteOn(33, 0.8f);
    expect(std::abs(cents(Access::state(*e).frequency, before)) < 0.1f,
           "redirected slide retains the sounding period at the event");
    style(*e, Style::Sustain); e->noteOn(Engine::firstRepickNote, 0.7f);
    render(*e, static_cast<int>(rate * 0.01));
    e->noteOff(33);
    const auto stopped = Access::state(*e);
    render(*e, static_cast<int>(rate * 0.02));
    expect(Access::state(*e).blend == stopped.blend,
           "released slide finger does not continue to its abandoned fret");
    expect(Access::state(*e).ramp == 0 && Access::state(*e).remainder == 0.0f,
           "an interrupted slide leaves no stale feed-forward movement");

    auto bend = makeEngine(rate);
    bend->noteOn(30, 0.8f); render(*bend, static_cast<int>(rate * 0.18));
    bend->setPitchBend(1.0f); render(*bend, static_cast<int>(rate * 0.004));
    const auto bending = Access::state(*bend);
    expect(bending.ramp == 0 && bending.remainder == 0.0f,
           "ordinary pitch-wheel motion bypasses the slide feed-forward path");
    expect(std::abs(cents(bending.frequency, bending.intendedFrequency)) > 0.1f
               && std::abs(bending.delayRetention
                   - std::exp(-1.0f / (0.006f * bending.internalRate))) < 1.0e-7f,
           "wheel bends retain their existing six-ms delay follower");

    for (const float interval : {-192.0f, 192.0f})
    {
        auto limited = makeEngine(rate);
        limited->setExpressionPitchBend(1, interval);
        limited->snapExpressionPitchBendToTarget(1);
        limited->noteOn(30, 0.6f, 1);
        render(*limited, static_cast<int>(rate * 0.06));
        style(*limited, Style::Slide); limited->noteOn(42, 0.6f, 1);
        bool inventedMotion = false;
        for (int frame = 0; frame < static_cast<int>(rate * 0.04); ++frame)
        {
            render(*limited, 1, 1);
            inventedMotion |= Access::state(*limited).remainder != 0.0f;
        }
        expect(! inventedMotion,
               "an MPE pitch safety clamp cannot invent slide feed-forward motion");
    }
}

void pinchClock(double rate)
{
    for (const float hardness : { 0.0f, 1.0f })
    {
        auto e = makeEngine(rate, hardness);
        style(*e, Style::Pinch); e->noteOn(30, 0.85f);
        const auto armed = Access::state(*e);
        expect(armed.pending && armed.thumbDepth == 0.0f && armed.thumbHold == 0,
               "pinch thumb is pending while the pick still holds the string");
        int frames = 0;
        while (Access::state(*e).pending && frames < static_cast<int>(rate * 0.01))
        {
            const auto before = Access::state(*e);
            if (before.phase == 1)
                expect(before.thumbDepth == 0.0f && before.thumbHold == 0,
                       "the pick-contact interval does not consume thumb hold time");
            render(*e, 1, 1); ++frames;
        }
        const auto contact = Access::state(*e);
        expect(! contact.pending && contact.thumbDepth == 1.0f,
               "thumb lands on the first physically released pick samples");
        expect(contact.thumbHold >= static_cast<int>(0.090 * contact.internalRate) - 2
                   && contact.thumbHold < static_cast<int>(0.090 * contact.internalRate),
               "full ninety-ms thumb hold starts at release across sample rates");
        expect(contact.thumbFraction == armed.pendingFraction,
               "released thumb uses the reserved pick contact point");

        render(*e, static_cast<int>(rate * 0.008));
        const auto priorTouch = Access::state(*e);
        e->noteOn(Engine::firstRepickNote, 0.75f);
        expect(Access::state(*e).pending
                   && Access::state(*e).thumbDepth == priorTouch.thumbDepth
                   && Access::state(*e).thumbFraction == priorTouch.thumbFraction,
               "a pinch repick preserves the old thumb until its new pick releases");
        style(*e, Style::Hammer); e->noteOn(32, 0.8f);
        render(*e, static_cast<int>(rate * 0.01));
        expect(! Access::state(*e).pending && Access::state(*e).thumbDepth == 0.0f,
               "a replacing fretting gesture cancels the pending pinch thumb");

        auto cancelled = makeEngine(rate, hardness);
        style(*cancelled, Style::Pinch); cancelled->noteOn(30, 0.8f);
        cancelled->noteOff(30); render(*cancelled, static_cast<int>(rate * 0.01));
        expect(! Access::state(*cancelled).pending
                   && Access::state(*cancelled).thumbDepth == 0.0f,
               "a cancelled note cannot land a phantom thumb afterwards");
    }
    auto natural = makeEngine(rate);
    style(*natural, Style::Harmonics); natural->noteOn(30, 0.8f);
    const auto finger = Access::state(*natural);
    expect(! finger.pending && finger.thumbDepth == 0.92f
               && finger.thumbHold > 0 && finger.thumbFraction == 0.5f,
           "natural harmonics retain their pre-positioned fretting finger");
    style(*natural, Style::Pinch);
    natural->noteOn(Engine::firstRepickNote, 0.8f);
    const auto reserved = Access::state(*natural);
    expect(reserved.pending && reserved.thumbDepth == 0.92f
               && reserved.thumbFraction == 0.5f
               && std::abs(reserved.pendingFraction - 0.5f) > 0.1f,
           "a new thumb reservation cannot teleport the existing harmonic finger");
    render(*natural, static_cast<int>(rate * 0.004));
    expect(! Access::state(*natural).pending
               && Access::state(*natural).thumbFraction == reserved.pendingFraction,
           "releasing that pick replaces the preceding touch with its actual thumb");
    auto delayed = makeEngine(rate, 0.0f, 0.015f);
    style(*delayed, Style::Pinch); delayed->noteOn(30, 0.8f);
    delayed->noteOff(30); render(*delayed, static_cast<int>(rate * 0.08));
    expect(! Access::state(*delayed).pending && Access::state(*delayed).thumbDepth == 0.0f,
           "cancelled strum pre-roll cannot arm the thumb contact");
}

std::vector<float> phrase(double rate, int block)
{
    auto e = makeEngine(rate);
    std::vector<float> result;
    const auto wait = [&](double seconds)
    {
        auto part = render(*e, static_cast<int>(rate * seconds), block);
        result.insert(result.end(), part.begin(), part.end());
    };
    e->noteOn(30, 0.8f); wait(0.1);
    style(*e, Style::Slide); e->noteOn(42, 0.8f); wait(0.013);
    e->noteOn(33, 0.75f); e->setPitchBend(0.7f); wait(0.017);
    style(*e, Style::Pinch); e->noteOn(Engine::firstRepickNote, 0.85f); wait(0.020);
    e->setPitchBend(-0.4f); wait(0.025);
    style(*e, Style::Slide); e->noteOn(30, 0.8f); wait(0.022);
    e->noteOff(30); wait(0.080);
    return result;
}
}

int main()
{
    for (const double rate : {44100.0, 48000.0, 96000.0, 192000.0})
    {
        slideArrival(rate); pinchClock(rate);
        const auto reference = phrase(rate, 1);
        for (const int block : {17, 128, 257, 511})
            expect(phrase(rate, block) == reference,
                   "overlapping slides, wheel bends and thumb contacts are block-identical");
    }
    if (failures) std::cerr << failures << " timing failures\n";
    return failures ? 1 : 0;
}
