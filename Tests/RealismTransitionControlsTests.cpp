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
    struct LegatoEndpoint
    {
        float blend, fittedFret, dampedFret;
        std::array<float, 2> coefficients;
    };

    static LegatoEndpoint legatoEndpoint(const ElectryEngine& engine)
    {
        // The fixture explicitly owns physical E2, so an allocator change
        // cannot silently turn the endpoint assertion into a different string.
        const auto& voice = engine.voices_[2];
        return { voice.legatoBlend, voice.lastConfiguredLiveFret,
                 voice.lastDampedLiveFret,
                 { voice.vertical.dispersionLowCoefficient,
                   voice.vertical.dispersionHighCoefficient } };
    }

    static std::array<float, 4> attackHandState(const ElectryEngine& engine)
    {
        const auto& voice = engine.voices_[0];
        return { voice.livePalmContact, voice.livePalmPressure,
                 voice.excitationReleaseCoefficient, voice.noiseBandCoefficient };
    }

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

void testExcitationFollowsMovingHand()
{
    using Engine = electry::ElectryEngine;
    using Access = electry::ElectryEngineTestAccess;
    using Style = electry::PlayStyle;
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
      for (bool pressureOnly : { false, true })
      {
        for (bool lifting : { false, true })
        {
            auto moving = std::make_unique<Engine>();
            auto settled = std::make_unique<Engine>();
            electry::EngineParameters p;
            p.sympatheticAmount = p.artifactAmount = 0.0f;
            p.strumSpreadSeconds = 0.0f;
            const Style target = lifting ? Style::Sustain : Style::PalmMute;
            const Style origin = lifting ? Style::PalmMute : Style::Sustain;
            for (auto* e : {moving.get(), settled.get()})
            {
                e->setParameters(p);
                e->prepare(rate, 127);
                e->setSoloStringMask(1);
                const Style initial = e == moving.get() ? origin : target;
                e->noteOn(Engine::firstPlayStyleKeyswitchNote
                              + static_cast<int>(pressureOnly ? Style::Sustain : initial), 1.0f);
                if (pressureOnly)
                    e->setPalmMutePressure(initial == Style::PalmMute ? 1.0f : 0.0f);
                e->noteOn(33, 0.85f); // fret 5, so a stopping finger can also be tested
                render(*e, rate, 0.15);
                e->noteOn(Engine::firstPlayStyleKeyswitchNote
                              + static_cast<int>(pressureOnly ? Style::Sustain : target), 1.0f);
                if (pressureOnly)
                    e->setPalmMutePressure(target == Style::PalmMute ? 1.0f : 0.0f);
                e->noteOn(Engine::firstRepickNote, 0.85f);
            }
            const auto m = Access::attackHandState(*moving);
            const auto s = Access::attackHandState(*settled);
            expect(m[pressureOnly ? 1 : 0] == (lifting ? 1.0f : 0.0f),
                   "style request preserves heel position at physical contact");
            expect(lifting ? m[2] > s[2] : m[2] < s[2],
                   "release spectrum follows the present heel while it moves");
            expect(lifting ? m[3] > s[3] : m[3] < s[3],
                   "scrape spectrum follows the same present heel");
            for (auto* e : {moving.get(), settled.get()})
            {
                render(*e, rate, 0.002);
                e->noteOn(Engine::firstRepickNote, 0.85f);
            }
            const auto mid = Access::attackHandState(*moving);
            expect(mid[pressureOnly ? 1 : 0] > 0.0f
                       && mid[pressureOnly ? 1 : 0] < 1.0f,
                   "second pick actually meets an intermediate heel position");
            expect(mid[2] > std::min(m[2], s[2])
                       && mid[2] < std::max(m[2], s[2]),
                   "intermediate heel gives an intermediate release spectrum");
            // Allow the established physical
            // follower to settle. No permanent articulation retuning is allowed.
            for (auto* e : {moving.get(), settled.get()})
            {
                render(*e, rate, 0.20);
                e->noteOn(Engine::firstRepickNote, 0.85f);
            }
            const auto me = Access::attackHandState(*moving);
            const auto se = Access::attackHandState(*settled);
            expect(me == se, "settled attack endpoint is independent of hand history");
        }
      }
    }
}

void testLegatoEndpointKeepsContinuingDispersionFit()
{
    using Engine = electry::ElectryEngine;
    using Access = electry::ElectryEngineTestAccess;
    using Style = electry::PlayStyle;
    for (double rate : { 48000.0, 96000.0, 384000.0 })
        for (Style style : { Style::Hammer, Style::Slide })
            for (bool descending : { false, true })
            {
                auto engine = std::make_unique<Engine>();
                electry::EngineParameters p;
                p.sympatheticAmount = p.artifactAmount = p.bodyResonance = 0.0f;
                p.pickNoise = p.fingerNoise = p.releaseNoise = 0.0f;
                engine->setParameters(p);
                engine->prepare(rate, 127);
                engine->setSoloStringMask(1u << 2);
                const int source = descending ? 45 : 43;
                const int destination = descending ? 43 : 45;
                engine->noteOn(source, 0.7f);
                render(*engine, rate, 0.5);
                engine->noteOn(Engine::firstPlayStyleKeyswitchNote
                    + static_cast<int>(style), 1.0f);
                engine->noteOn(destination, 0.7f);
                const std::string context = std::to_string(static_cast<int>(rate))
                    + (style == Style::Hammer ? " Hz Hammer" : " Hz Slide")
                    + (descending ? " descending" : " ascending");

                bool reachedEndpoint = false;
                float endpointStep = 0.0f, largestAudioStep = 0.0f;
                float previousSample = 0.0f;
                int endpointSample = -1, audioPeakSample = -1;
                // The two-fret Slide lasts 44.8 ms at these defaults. Stop
                // after its endpoint plus 20 ms, enough to observe a false
                // coefficient-switch burst emerging through the loop/pickup.
                const int count = static_cast<int>(std::lround(0.12 * rate));
                auto before = Access::legatoEndpoint(*engine);
                for (int sample = 0; sample < count; ++sample)
                {
                    float left = 0.0f, right = 0.0f;
                    engine->process(&left, &right, 1);
                    expect(std::isfinite(left) && std::isfinite(right),
                        context + " endpoint render has finite stereo samples");
                    const auto after = Access::legatoEndpoint(*engine);
                    if (! reachedEndpoint && before.blend < 1.0f
                        && after.blend == 1.0f)
                    {
                        reachedEndpoint = true;
                        endpointSample = sample;
                        for (std::size_t band = 0; band < before.coefficients.size(); ++band)
                            endpointStep = std::max(endpointStep, std::abs(
                                after.coefficients[band] - before.coefficients[band]));
                        expect(std::abs(after.fittedFret
                            - static_cast<float>(destination - 40)) < 1.0e-5f,
                            context + " refits dispersion at the exact final fret");
                        expect(std::abs(after.dampedFret
                            - static_cast<float>(destination - 40)) < 1.0e-5f,
                            context + " refits loss at the exact final fret");
                    }
                    if (reachedEndpoint && sample <= endpointSample
                        + static_cast<int>(std::lround(0.020 * rate)))
                    {
                        const float step = std::abs(left - previousSample);
                        if (step > largestAudioStep)
                        {
                            largestAudioStep = step;
                            audioPeakSample = sample;
                        }
                    }
                    previousSample = left;
                    before = after;
                    if (reachedEndpoint && sample > endpointSample
                        + static_cast<int>(std::lround(0.020 * rate)))
                        break;
                }
                expect(reachedEndpoint, context + " reaches its physical endpoint");
                // Existing continuation policy: the first pass searches +/-
                // .02; its 17-way refinement may extend one initial grid cell
                // beyond that interval in the shipping fitter. A trajectory's
                // final tick must obey that same local bound, not invoke the
                // unrelated global fit merely to pin its exact destination.
                constexpr float maximumContinuingStep = 0.02f
                    + (2.0f * 0.02f / 17.0f) + 1.0e-6f;
                expect(endpointStep <= maximumContinuingStep,
                    context + " keeps its final dispersion step within the continuing neighborhood ("
                    + std::to_string(endpointStep) + ")");
                expect(std::isfinite(largestAudioStep), context + " endpoint audio stays finite");
                std::cout << "PROBE " << context << " endpoint AP step " << endpointStep
                          << ", largest post-endpoint audio step " << largestAudioStep
                          << " at " << (audioPeakSample - endpointSample)
                          << " host samples after endpoint\n";
            }
}
}

int main()
{
    testStyleChangesUnderHeldPressure();
    testExcitationFollowsMovingHand();
    testLegatoEndpointKeepsContinuingDispersionFit();
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
