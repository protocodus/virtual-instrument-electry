#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace electry
{
// Observe the existing hand coordinates and the actual boundary reader. No
// restoration fields are required, so this test also builds against the old
// engine and detects its immediate hand removal.
struct ElectryEngineTestAccess
{
    struct State
    {
        bool active, releasing;
        int phase, remaining, delay;
        float gain, depth;
        double rate;
        int style, dampingStyle, restoring;
        bool flatRelease;
    };

    static State state(const ElectryEngine& engine, int stringIndex)
    {
        const auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        return {voice.active, voice.releasing, static_cast<int>(voice.excitationPhase),
                voice.excitationRemaining, voice.startDelaySamples,
                voice.releaseGain, voice.releaseSpectralDepth, engine.sampleRate_,
                static_cast<int>(voice.playStyle), static_cast<int>(voice.dampingStyle),
                voice.releaseRestoreLength, voice.excitationModalShaper.flatRelease};
    }

    static void renderVoice(ElectryEngine& engine, int stringIndex)
    {
        ElectryEngine::RenderSums sums {};
        engine.renderVoice(engine.voices_[static_cast<std::size_t>(stringIndex)], sums);
    }

    static void deferRepick(ElectryEngine& engine, int stringIndex, int samples,
                            PlayStyle style = PlayStyle::Sustain)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        engine.startVoice(voice, voice.midiNote, .85f, style,
                          voice.strokeIsUp, samples, 0);
    }

    static void cancelPending(ElectryEngine& engine, int stringIndex)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        voice.keyDown = false;
        voice.keyDownCount = 0;
        engine.beginVoiceRelease(voice);
    }

    static void applyLegacyPalmOpening(ElectryEngine& engine, int stringIndex)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        // Historical immediate Palm endpoint, applied independently before
        // the new contact. This is the retained control, not a new ramp law.
        voice.releasing = false;
        voice.releaseGain = voice.releaseGainTarget = 1;
        voice.releaseSpectralDepth = voice.releaseSpectralTarget = 0;
        voice.releaseGainCoefficient = 0;
    }

    static bool samePalmHistory(const ElectryEngine& a, const ElectryEngine& b,
                                int stringIndex)
    {
        const auto& x = a.voices_[static_cast<std::size_t>(stringIndex)];
        const auto& y = b.voices_[static_cast<std::size_t>(stringIndex)];
        return x.vertical.line == y.vertical.line
            && x.horizontal.line == y.horizontal.line
            && x.previousFluxNeck == y.previousFluxNeck
            && x.previousFluxBridge == y.previousFluxBridge
            && x.releaseGain == y.releaseGain
            && x.releaseSpectralDepth == y.releaseSpectralDepth
            && x.excitationModalShaper.contactShaper1.state
                == y.excitationModalShaper.contactShaper1.state
            && x.excitationModalShaper.contactShaper2.state
                == y.excitationModalShaper.contactShaper2.state;
    }

    static float observedStoppedBoundary(ElectryEngine& engine, int stringIndex,
                                        bool releasing)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        auto& loop = voice.vertical;
        // A varying line makes the symmetric spatial stop differ measurably
        // from the center tap. The zero-state first allpass stores its actual
        // production input, before output gain or later filters can hide it.
        for (std::size_t index = 0; index < loop.line.size(); ++index)
            loop.line[index] = static_cast<float>(std::sin(.31 * index)
                                                  + .3 * std::cos(.071 * index));
        loop.dispersion1.state = 0;
        voice.releasing = releasing;
        voice.releaseGain = .5f;
        voice.releaseSpectralDepth = .6f;
        voice.excitationPhase = ElectryEngine::ExcitationPhase::Idle;
        voice.noiseRemaining = 0;
        voice.touchDepth = 0;
        voice.refretRemainingSamples = 0;
        engine.feedbackDrive_ = engine.bridgeCouplingInjection_ = 0;
        engine.sympatheticBus_ = engine.sympatheticBusDelayed_ = 0;
        voice.busContribution = 0;
        renderVoice(engine, stringIndex);
        return loop.dispersion1.state;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
using State = Access::State;
int assertions = 0, failures = 0;

void expect(bool condition, const std::string& message)
{
    ++assertions;
    if (!condition)
    {
        ++failures;
        if (failures <= 30)
            std::cerr << message << '\n';
    }
}

std::unique_ptr<Engine> makeEngine(double rate, int stringIndex,
                                 electry::PlayStyle style, float hardness = .58f)
{
    auto engine = std::make_unique<Engine>();
    electry::EngineParameters parameters;
    electry::applyGuitarBuild(parameters, electry::defaultGuitarBuild);
    parameters.strumSpreadSeconds = 0;
    parameters.pickNoise = parameters.fingerNoise = parameters.releaseNoise = 0;
    parameters.artifactAmount = parameters.sympatheticAmount = parameters.bodyResonance = 0;
    parameters.pickHardness = hardness;
    engine->setParameters(parameters);
    // Zero seed has no performer pre-roll; delayed travel is tested below
    // independently so the Contact endpoint can be observed exactly here.
    engine->setVariationSeed(0);
    engine->prepare(rate, 64);
    engine->setSoloStringMask(static_cast<std::uint8_t>(1u << stringIndex));
    engine->noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(style), 1);
    return engine;
}

void pick(Engine& engine, int note)
{
    const Engine::NoteOnEvent event {note, .85f};
    engine.noteOnChord(&event, 1);
}

void render(Engine& engine, int frames)
{
    std::array<float, 64> left {}, right {};
    bool finite = true;
    while (frames > 0)
    {
        const int count = std::min(frames, 64);
        engine.process(left.data(), right.data(), count);
        for (int frame = 0; frame < count; ++frame)
            finite = finite && std::isfinite(left[static_cast<std::size_t>(frame)])
                && std::isfinite(right[static_cast<std::size_t>(frame)]);
        frames -= count;
    }
    expect(finite, "A hand-restoration render was non-finite");
}

void stopRinging(Engine& engine, int note, int rate, double wait = .018)
{
    pick(engine, note);
    render(engine, static_cast<int>(.08 * rate));
    engine.noteOff(note);
    render(engine, static_cast<int>(wait * rate));
}

double openingWeight(double progress)
{
    // Independent double polynomial for the standard minimum-jerk path.
    return progress * progress * progress
         * (10 - 15 * progress + 6 * progress * progress);
}

void stoppedRingOpensOnlyThroughPhysicalContact()
{
    for (const int rate : {48000, 384000})
        for (const auto stringNote : {std::array<int, 2>{0, 28}, {2, 47}})
            for (const auto style : {electry::PlayStyle::Sustain})
                for (const float hardness : {0.f, .58f, 1.f})
                {
                    auto engine = makeEngine(rate, stringNote[0], style, hardness);
                    stopRinging(*engine, stringNote[1], rate);
                    const auto old = Access::state(*engine, stringNote[0]);
                    expect(old.active && old.releasing && old.gain < .99f,
                           "The stopped-ring witness did not contain a finite hand stop");
                    pick(*engine, stringNote[1]);
                    auto state = Access::state(*engine, stringNote[0]);
                    expect(state.phase == 1 && state.remaining > 1 && state.delay == 0,
                           "A complete public pick did not begin physical Contact");
                    expect(state.gain == old.gain && state.depth == old.depth,
                           "Note On removed the old hand before physical Contact");
                    const int length = state.remaining;
                    float previousGain = old.gain, previousDepth = old.depth;
                    for (int sample = 0; sample < length; ++sample)
                    {
                        Access::renderVoice(*engine, stringNote[0]);
                        state = Access::state(*engine, stringNote[0]);
                        const double weight = openingWeight(static_cast<double>(sample) / (length - 1));
                        expect(std::abs(state.gain - (old.gain + (1 - old.gain) * weight)) < 2e-6,
                               "The actual gain did not follow the existing Contact clock");
                        expect(std::abs(state.depth - old.depth * (1 - weight)) < 2e-6,
                               "The actual spectral stop did not share the Contact clock");
                        expect(std::isfinite(state.gain) && std::isfinite(state.depth)
                                   && state.gain >= previousGain && state.gain <= 1
                                   && state.depth <= previousDepth && state.depth >= 0,
                               "The old hand opened non-monotonically or outside its endpoints");
                        expect(state.phase == (sample == length - 1 ? 2 : 1),
                               "Hand opening changed the physical source Contact duration");
                        previousGain = state.gain;
                        previousDepth = state.depth;
                    }
                    expect(state.gain == 1 && state.depth == 0 && !state.releasing,
                           "A stopped hand remained after the new pick's physical Release");
                }
}

void noteOffDuringOpeningRemainsAStop()
{
    for (const int rate : {48000, 384000})
        for (const double fraction : {.25, .75})
        {
            auto engine = makeEngine(rate, 2, electry::PlayStyle::Sustain);
            stopRinging(*engine, 47, rate);
            pick(*engine, 47);
            const int length = Access::state(*engine, 2).remaining;
            for (int sample = 0; sample < static_cast<int>(fraction * length); ++sample)
                Access::renderVoice(*engine, 2);
            const auto before = Access::state(*engine, 2);
            engine->noteOff(47);
            auto state = Access::state(*engine, 2);
            expect(state.releasing && state.gain == before.gain && state.depth == before.depth,
                   "Note Off interrupted opening with a gain or spectral jump");
            float previousGain = state.gain, previousDepth = state.depth;
            for (int sample = 0; sample < 2 * length; ++sample)
            {
                Access::renderVoice(*engine, 2);
                state = Access::state(*engine, 2);
                expect(state.releasing && state.gain <= previousGain
                           && state.depth >= previousDepth && state.gain < 1,
                       "A cancelled opening resurrected the old sustained loop");
                previousGain = state.gain;
                previousDepth = state.depth;
            }
        }
}

void delayedReservationLeavesOldReleaseUntouched()
{
    auto pending = makeEngine(48000, 2, electry::PlayStyle::Sustain);
    auto control = makeEngine(48000, 2, electry::PlayStyle::Sustain);
    stopRinging(*pending, 47, 48000, .008);
    stopRinging(*control, 47, 48000, .008);
    const auto before = Access::state(*pending, 2);
    Access::deferRepick(*pending, 2, static_cast<int>(before.rate * .02));
    expect(Access::state(*pending, 2).gain == before.gain
               && Access::state(*pending, 2).depth == before.depth,
           "A delayed reservation removed the released hand in lookahead");
    bool identical = true;
    for (int frame = 0; frame < 240; ++frame)
    {
        float left = 0, right = 0, refLeft = 0, refRight = 0;
        pending->process(&left, &right, 1);
        control->process(&refLeft, &refRight, 1);
        const auto a = Access::state(*pending, 2), b = Access::state(*control, 2);
        identical = identical && left == refLeft && right == refRight
            && a.gain == b.gain && a.depth == b.depth && a.releasing == b.releasing;
    }
    expect(identical, "A pending repick changed the old release before its contact");
    Access::cancelPending(*pending, 2);
    for (int frame = 0; frame < 240; ++frame)
    {
        float left = 0, right = 0, refLeft = 0, refRight = 0;
        pending->process(&left, &right, 1);
        control->process(&refLeft, &refRight, 1);
        identical = identical && left == refLeft && right == refRight;
    }
    expect(identical, "Cancelling a delayed repick restarted the preceding release");
}

void delayedContactLatchesTheHandThatActuallyArrives()
{
    auto pending = makeEngine(48000, 2, electry::PlayStyle::Sustain);
    auto control = makeEngine(48000, 2, electry::PlayStyle::Sustain);
    stopRinging(*pending, 47, 48000, .008);
    stopRinging(*control, 47, 48000, .008);
    const auto reservation = Access::state(*pending, 2);
    Access::deferRepick(*pending, 2, static_cast<int>(reservation.rate * .004));
    int waited = 0;
    while (Access::state(*pending, 2).delay > 0 && waited < 1000)
    {
        float left = 0, right = 0, refLeft = 0, refRight = 0;
        pending->process(&left, &right, 1);
        control->process(&refLeft, &refRight, 1);
        ++waited;
    }
    const auto arrived = Access::state(*pending, 2);
    const auto undisturbed = Access::state(*control, 2);
    expect(arrived.phase == 1 && arrived.remaining > 1 && arrived.delay == 0,
           "The delayed arrival witness did not reach its actual Contact");
    expect(arrived.gain == undisturbed.gain && arrived.depth == undisturbed.depth
               && arrived.gain < reservation.gain && arrived.depth > reservation.depth,
           "The pick reopened a stale reservation-time hand instead of the arriving hand");
    const int length = arrived.remaining;
    for (int sample = 0; sample < length; ++sample)
    {
        Access::renderVoice(*pending, 2);
        const auto state = Access::state(*pending, 2);
        const double weight = openingWeight(static_cast<double>(sample) / (length - 1));
        expect(std::abs(state.gain - (arrived.gain + (1 - arrived.gain) * weight)) < 2e-6
                   && std::abs(state.depth - arrived.depth * (1 - weight)) < 2e-6,
               "A delayed pick did not open its actual arriving hand over Contact");
    }
}

void interruptedContactStartsAtTheCurrentHand()
{
    for (const double fraction : {.25, .75})
    {
        auto engine = makeEngine(48000, 2, electry::PlayStyle::Sustain);
        stopRinging(*engine, 47, 48000);
        pick(*engine, 47);
        const int firstLength = Access::state(*engine, 2).remaining;
        for (int sample = 0; sample < static_cast<int>(fraction * firstLength); ++sample)
            Access::renderVoice(*engine, 2);
        const auto current = Access::state(*engine, 2);
        pick(*engine, 47);
        auto state = Access::state(*engine, 2);
        expect(state.phase == 1 && state.remaining > 1
                   && state.gain == current.gain && state.depth == current.depth,
               "A second pick interrupted the actual hand with an endpoint jump");
        const int length = state.remaining;
        for (int sample = 0; sample < length; ++sample)
        {
            Access::renderVoice(*engine, 2);
            state = Access::state(*engine, 2);
            const double weight = openingWeight(static_cast<double>(sample) / (length - 1));
            expect(std::abs(state.gain - (current.gain + (1 - current.gain) * weight)) < 2e-6
                       && std::abs(state.depth - current.depth * (1 - weight)) < 2e-6,
                   "An interrupted pick lost the continuous current hand coordinate");
        }
        expect(state.phase == 2 && state.gain == 1 && state.depth == 0,
               "An interrupted contact failed to release the stopping hand");
    }
}

void aSpectralHandRemainsPresentWhileOpening()
{
    auto closing = makeEngine(48000, 2, electry::PlayStyle::Sustain);
    auto opening = makeEngine(48000, 2, electry::PlayStyle::Sustain);
    pick(*closing, 47);
    pick(*opening, 47);
    const float a = Access::observedStoppedBoundary(*closing, 2, true);
    const float b = Access::observedStoppedBoundary(*opening, 2, false);
    expect(a == b && std::isfinite(a),
           "The actual boundary reader bypassed a finite spectral stop while opening");
}

void delayedStyleBoundaryUsesTheArrivingContact()
{
    for (const int rate : {48000, 384000})
        for (const auto styles : {std::array<electry::PlayStyle, 2>{
                                    electry::PlayStyle::Sustain, electry::PlayStyle::PalmMute},
                                 {electry::PlayStyle::PalmMute, electry::PlayStyle::Sustain},
                                 {electry::PlayStyle::PalmMute, electry::PlayStyle::PalmMute}})
        {
            auto pending = makeEngine(rate, 2, styles[0]);
            auto control = makeEngine(rate, 2, styles[0]);
            stopRinging(*pending, 47, rate, .008);
            stopRinging(*control, 47, rate, .008);
            const auto old = Access::state(*pending, 2);
            Access::deferRepick(*pending, 2, static_cast<int>(old.rate) / 250, styles[1]);
            auto state = Access::state(*pending, 2);
            expect(state.gain == old.gain && state.depth == old.depth
                       && state.style == old.style && state.dampingStyle == old.dampingStyle,
                   "A reserved style boundary replaced the old hand before contact");
            bool identical = true;
            while (state.delay > 0)
            {
                float left = 0, right = 0, refLeft = 0, refRight = 0;
                pending->process(&left, &right, 1);
                control->process(&refLeft, &refRight, 1);
                state = Access::state(*pending, 2);
                const auto reference = Access::state(*control, 2);
                if (state.delay > 0)
                    identical = identical && left == refLeft && right == refRight
                        && state.gain == reference.gain && state.depth == reference.depth
                        && state.style == old.style && state.dampingStyle == old.dampingStyle;
            }
            expect(identical, "A pending Palm/ordinary boundary changed the old ringing wrist");
            expect(state.phase == 1 && state.style == static_cast<int>(styles[1]),
                   "The arriving physical contact lost its reserved style");
            const bool palm = styles[1] == electry::PlayStyle::PalmMute;
            expect(palm ? state.gain == 1 && state.depth == 0 && state.restoring == 0
                              && state.flatRelease
                        : state.gain < 1 && state.restoring > 1 && state.flatRelease,
                   "The actual contact applied the wrong Palm/ordinary opening or projection");
            const int length = state.remaining;
            const auto arriving = state;
            for (int sample = 0; sample < length; ++sample)
            {
                Access::renderVoice(*pending, 2);
                state = Access::state(*pending, 2);
                const double weight = openingWeight(static_cast<double>(sample) / (length - 1));
                expect(palm ? state.gain == 1 && state.depth == 0
                            : std::abs(state.gain - (arriving.gain
                                + (1 - arriving.gain) * weight)) < 2e-6
                                && std::abs(state.depth - arriving.depth * (1 - weight)) < 2e-6,
                       "Palm scope leaked into ordinary Contact restoration or vice versa");
            }
        }
}

void rapidPalmRetainsTheEstablishedSourceAndHistory()
{
    auto actual = makeEngine(44100, 0, electry::PlayStyle::PalmMute, .85f);
    auto legacyControl = makeEngine(44100, 0, electry::PlayStyle::PalmMute, .85f);
    bool identical = true;
    for (int hit = 0; hit < 12; ++hit)
    {
        Access::applyLegacyPalmOpening(*legacyControl, 0);
        pick(*actual, 28);
        pick(*legacyControl, 28);
        auto state = Access::state(*actual, 0);
        expect(state.gain == 1 && state.depth == 0 && state.restoring == 0
                   && state.flatRelease,
               "Rapid Palm no longer uses its established opening and qualified Bessel source");
        const int hold = static_cast<int>(.055 * 44100);
        const int gap = static_cast<int>(.028333 * 44100);
        for (int frame = 0; frame < hold + gap; ++frame)
        {
            if (frame == hold)
            {
                actual->noteOff(28);
                legacyControl->noteOff(28);
            }
            float left = 0, right = 0, refLeft = 0, refRight = 0;
            actual->process(&left, &right, 1);
            legacyControl->process(&refLeft, &refRight, 1);
            identical = identical && left == refLeft && right == refRight;
        }
        expect(Access::samePalmHistory(*actual, *legacyControl, 0),
               "A rapid Palm contact changed its established source or pickup history");
    }
    expect(identical, "The scoped Palm path changed the legacy-opening audio control");
}

void freshAndHeldPicksNeedNoRestoration()
{
    for (const auto style : {electry::PlayStyle::Sustain, electry::PlayStyle::PalmMute,
                             electry::PlayStyle::Hammer})
    {
        auto engine = makeEngine(48000, 2, style);
        pick(*engine, 47);
        auto state = Access::state(*engine, 2);
        const int length = state.phase == 1 ? state.remaining : 1;
        for (int sample = 0; sample < length; ++sample)
        {
            Access::renderVoice(*engine, 2);
            state = Access::state(*engine, 2);
            expect(state.gain == 1 && state.depth == 0,
                   "A fresh contact invented a released hand");
        }
        render(*engine, 4800);
        pick(*engine, 47);
        state = Access::state(*engine, 2);
        expect(state.gain == 1 && state.depth == 0,
               "An ordinary held repick acquired hand-restoration attenuation");
        engine->reset();
        pick(*engine, 47);
        state = Access::state(*engine, 2);
        expect(state.gain == 1 && state.depth == 0,
               "Reset retained a previous opening hand");
    }
}
}

int main()
{
    stoppedRingOpensOnlyThroughPhysicalContact();
    noteOffDuringOpeningRemainsAStop();
    delayedReservationLeavesOldReleaseUntouched();
    delayedContactLatchesTheHandThatActuallyArrives();
    interruptedContactStartsAtTheCurrentHand();
    aSpectralHandRemainsPresentWhileOpening();
    delayedStyleBoundaryUsesTheArrivingContact();
    rapidPalmRetainsTheEstablishedSourceAndHistory();
    freshAndHeldPicksNeedNoRestoration();
    std::cout << assertions << " assertions, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
