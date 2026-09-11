#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace electry
{
struct ElectryEngineTestAccess
{
    static bool handIsOpen(const ElectryEngine& engine)
    {
        const auto& voice = engine.voices_[2];
        return engine.palmMuteBlend_ == 0.0f
            && engine.appliedPalmMute_ == 0.0f
            && voice.vertical.handLossSolvedDepth == 0.0f
            && voice.horizontal.handLossSolvedDepth == 0.0f
            && ! voice.vertical.handDipActive && ! voice.horizontal.handDipActive;
    }

    static bool handIsEngaged(const ElectryEngine& engine)
    {
        return engine.voices_[2].vertical.handLossSolvedDepth > 0.0f;
    }

    static float lowStringContactScale(const ElectryEngine& engine)
    {
        return engine.voices_[0].handContactScale;
    }

    static bool lowStringKeepsItsOwner(const ElectryEngine& engine)
    {
        return engine.voices_[0].active && engine.voices_[0].midiNote == 28
            && engine.heldNoteCounts_[0] == 1;
    }

    static bool lowStringLossMatchesCurrentContact(ElectryEngine& engine)
    {
        auto& voice = engine.voices_[0];
        const auto solvedLoss = [&voice]
        {
            return std::array<float, 6> {
                voice.vertical.loopGain,
                voice.vertical.loopDampingCoefficient,
                voice.vertical.handLossSolvedDepth,
                voice.horizontal.loopGain,
                voice.horizontal.loopDampingCoefficient,
                voice.horizontal.handLossSolvedDepth
            };
        };
        const auto before = solvedLoss();
        // Hold the exact fitted pitch/fret fixed: this oracle checks whether
        // the cached coefficients describe the current hand force/position,
        // independently of the separate sub-cent pitch refresh quantum.
        engine.configureVoiceDamping(voice, voice.dampingStyle,
                                     voice.lastDampedFrequency,
                                     voice.lastDampedLiveFret);
        const auto after = solvedLoss();
        for (std::size_t i = 0; i < before.size(); ++i)
            if (std::abs(before[i] - after[i]) > 1.0e-5f)
                return false;
        return true;
    }
};
}

namespace
{
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void render(electry::ElectryEngine& engine, double rate, double seconds)
{
    int remaining = static_cast<int>(std::lround(rate * seconds));
    std::array<float, 127> left {}, right {};
    while (remaining > 0)
    {
        const int count = std::min(remaining, static_cast<int>(left.size()));
        engine.process(left.data(), right.data(), count);
        expect(std::all_of(left.begin(), left.begin() + count, [] (float sample)
        {
            return std::isfinite(sample) && std::abs(sample) < 2.0f;
        }), "pressure transition stays finite and bounded");
        remaining -= count;
    }
}

void testStyleChangesUnderHeldPressure()
{
    using Engine = electry::ElectryEngine;
    using Access = electry::ElectryEngineTestAccess;
    using Style = electry::PlayStyle;
    for (double rate : { 44100.0, 48000.0, 96000.0 })
    {
        for (bool sameSampleReversal : { false, true })
        {
            auto engine = std::make_unique<Engine>();
            electry::EngineParameters parameters;
            parameters.sympatheticAmount = parameters.artifactAmount = 0.0f;
            parameters.pickNoise = parameters.fingerNoise = parameters.releaseNoise = 0.0f;
            engine->setParameters(parameters);
            engine->prepare(rate, 127);
            engine->setSoloStringMask(1);
            engine->setPalmMutePressure(0.8f);
            const std::array<Engine::NoteOnEvent, 1> first {{{ 28, 0.95f }}};
            engine->noteOnChord(first);
            render(*engine, rate, 0.10);
            const float loudContact = Access::lowStringContactScale(*engine);
            const auto selectAndRepick = [&] (Style style, float force)
            {
                engine->noteOn(Engine::firstPlayStyleKeyswitchNote
                                   + static_cast<int>(style), 1.0f);
                engine->noteOn(Engine::firstRepickNote, force);
            };
            if (sameSampleReversal)
            {
                // Both physical contacts arrive before a hand update. The
                // heel ends where it began, but the final stroke's force is
                // different and still requires a pressure-loss refresh.
                selectAndRepick(Style::PalmMute, 0.95f);
                selectAndRepick(Style::Sustain, 0.20f);
            }
            else
            {
                // Neither style asks for a bridge-heel position change. The
                // new softer contact must still refit the existing CC2 loss.
                selectAndRepick(Style::Harmonics, 0.20f);
            }
            render(*engine, rate, 0.10);
            const std::string context = std::to_string(static_cast<int>(rate))
                + (sameSampleReversal ? " Hz same-sample palm reversal"
                                      : " Hz non-palm style change");
            expect(Access::lowStringKeepsItsOwner(*engine),
                   context + " retains one physical fretting owner");
            expect(loudContact - Access::lowStringContactScale(*engine) > 0.20f,
                   context + " actually changes the stroke's hand force");
            expect(Access::lowStringLossMatchesCurrentContact(*engine),
                   context + " refreshes loss for its changed force under held CC2");
            engine->noteOff(28);
            render(*engine, rate, 0.10);
        }
    }
}
}

int main()
{
    testStyleChangesUnderHeldPressure();
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        for (bool pressureBeforeNote : { false, true })
        {
            auto engine = std::make_unique<electry::ElectryEngine>();
            electry::EngineParameters parameters;
            parameters.palmMute = 0.0f;
            parameters.sympatheticAmount = parameters.artifactAmount = 0.0f;
            engine->setParameters(parameters);
            engine->prepare(rate, 127);
            engine->setSoloStringMask(1u << 2);
            if (pressureBeforeNote)
                engine->setPalmMutePressure(0.001f);
            engine->noteOn(40, 0.7f);
            render(*engine, rate, 0.10);
            if (! pressureBeforeNote)
            {
                engine->setPalmMutePressure(0.7f);
                render(*engine, rate, 0.10);
                engine->setPalmMutePressure(0.001f);
                render(*engine, rate, 0.10);
            }
            expect(electry::ElectryEngineTestAccess::handIsEngaged(*engine),
                   "tiny-pressure fixture must contain actual bridge-hand loss");
            engine->setPalmMutePressure(0.0f);
            render(*engine, rate, 0.15);
            expect(electry::ElectryEngineTestAccess::handIsOpen(*engine),
                   "CC2 zero clears the hand after a sub-quantum pressure change");
        }
    }
    if (failures == 0)
        std::cout << "All transition control checks passed.\n";
    return failures == 0 ? 0 : 1;
}
