#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>

// The transfer-function observer below is independent of the production
// fractional reader. It evaluates the actual Lagrange FIR at a complex
// frequency, including the allpasses and the two loss biquads. A linearised
// frozen-loop resonance diagnoses an unintended pitch follower; it does not
// pretend to measure instantaneous audio pitch during a nonlinear attack.
namespace electry
{
struct ElectryEngineTestAccess
{
    struct State
    {
        bool active = false;
        bool held = false;
        int phase = 0;
        int note = -1;
        int pending = 0;
        double rate = 0;
        double targetFrequency = 0;
        double delay = 0;
        double oldDelay = 0;
        double newWeight = 1;
        int transitionRemaining = 0;
        int transitionLength = 0;
        bool queued = false;
        bool locked = false;
        bool sustained = false;
        double legatoBlend = 1;
        double loopEnergy = 0;
        double attackTension = 0;
        std::array<double, 2> ap {};
        double pole = 0;
        double gain = 0;
        std::array<double, 5> material {1, 0, 0, 0, 0};
        std::array<double, 5> hand {1, 0, 0, 0, 0};
    };

    static State state(const ElectryEngine& engine, int stringIndex)
    {
        const auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        const auto& loop = voice.vertical;
        State result;
        result.active = voice.active;
        result.held = voice.keyDown;
        result.phase = static_cast<int>(voice.excitationPhase);
        result.note = voice.midiNote;
        result.sustained = voice.sustained;
        result.legatoBlend = voice.legatoBlend;
        for (const float sample : voice.vertical.line)
            result.loopEnergy += static_cast<double>(sample) * sample;
#if ELECTRY_ENERGY_ATTACK_PITCH
        result.attackTension = voice.attackPitchTensionRatio;
#endif
        result.pending = voice.startDelaySamples;
        result.rate = engine.sampleRate_;
        result.targetFrequency = engine.sampleRate_ / voice.lastCompensatedPeriod;
        result.delay = loop.currentDelay;
        result.oldDelay = result.delay;
        // The opt-in baseline build links this same observer to frozen 1.2.3
        // headers/library to prove the old pitch-lag failures. Normal CI
        // always observes the complete actual two-tap transfer below.
#ifndef ELECTRY_REFRET_TEST_BASELINE
        result.oldDelay += voice.refretVerticalOffset;
        result.transitionRemaining = voice.refretRemainingSamples;
        result.transitionLength = voice.refretLengthSamples;
        result.queued = voice.refretQueued;
        result.locked = voice.refretDelayLocked;
        if (voice.refretRemainingSamples > 0)
        {
            const double progress = static_cast<double>(voice.refretLengthSamples
                                                        - voice.refretRemainingSamples)
                                  / (voice.refretLengthSamples - 1);
            result.newWeight = progress * progress * (3 - 2 * progress);
        }
#endif
        result.ap = {loop.dispersionLowCoefficient, loop.dispersionHighCoefficient};
        result.pole = loop.loopDampingCoefficient;
        result.gain = loop.loopGain;
        const auto coefficients = [] (const auto& biquad)
        {
            return std::array<double, 5> {biquad.b0, biquad.b1, biquad.b2,
                                          biquad.a1, biquad.a2};
        };
        if (loop.materialLossDepth > 0)
            result.material = coefficients(loop.materialLossDip);
        if (loop.handDipActive)
            result.hand = coefficients(loop.handDip);
        return result;
    }

    static bool finiteString(const ElectryEngine& engine, int stringIndex)
    {
        const auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        for (const auto* loop : {&voice.vertical, &voice.horizontal})
        {
            if (!std::isfinite(loop->currentDelay) || !std::isfinite(loop->targetDelay))
                return false;
            for (const float sample : loop->line)
                if (!std::isfinite(sample))
                    return false;
        }
        return true;
    }
};
}

namespace
{
constexpr double pi = 3.1415926535897932384626433832795;
using State = electry::ElectryEngineTestAccess::State;
int failures = 0;
int assertions = 0;
double largestReleaseError = 0;
double largestEnergyPitch = 0;
double largestUnexplainedReleaseError = 0;

void expect(bool condition, const std::string& message)
{
    ++assertions;
    if (!condition)
    {
        ++failures;
        if (failures < 30)
            std::cerr << message << '\n';
    }
}

std::complex<double> readTransfer(double delay, double omega)
{
    const double whole = std::ceil(delay);
    const double t = whole - delay;
    const std::array<double, 4> taps {
        -t * (t - 1) * (t - 2) / 6,
        (t + 1) * (t - 1) * (t - 2) / 2,
        -(t + 1) * t * (t - 2) / 2,
        (t + 1) * t * (t - 1) / 6
    };
    std::complex<double> result {};
    for (int index = 0; index < 4; ++index)
        result += taps[static_cast<std::size_t>(index)]
            * std::polar(1.0, omega * (index - 1 - whole));
    return result;
}

std::complex<double> biquadTransfer(const std::array<double, 5>& b,
                                   std::complex<double> z)
{
    return (b[0] + b[1] * z + b[2] * z * z)
        / (1.0 + b[3] * z + b[4] * z * z);
}

std::complex<double> loopTransfer(const State& state, double frequency)
{
    const double omega = 2 * pi * frequency / state.rate;
    const auto z = std::polar(1.0, -omega);
    auto transfer = (1 - state.newWeight) * readTransfer(state.oldDelay, omega)
                  + state.newWeight * readTransfer(state.delay, omega);
    for (const double coefficient : state.ap)
    {
        const auto stage = (coefficient + z) / (1.0 + coefficient * z);
        transfer *= stage * stage * stage * stage;
    }
    transfer *= state.gain * (1 - state.pole) / (1.0 - state.pole * z);
    transfer *= biquadTransfer(state.material, z);
    transfer *= biquadTransfer(state.hand, z);
    return transfer;
}

// Lowest nonzero positive-real phase closure in the low-frequency region.
// The two-tap mixture is evaluated as a complex sum, not an interpolated
// scalar delay. The first root is unambiguous for the small refret intervals
// used in this property; a large leap can create additional comb modes.
double frozenFundamental(const State& state)
{
    const double high = state.targetFrequency * 2;
    constexpr int bins = 2048;
    double previousFrequency = high / bins;
    auto previous = loopTransfer(state, previousFrequency);
    for (int index = 2; index <= bins; ++index)
    {
        const double frequency = high * index / bins;
        const auto current = loopTransfer(state, frequency);
        if (previous.imag() >= 0 && current.imag() <= 0)
        {
            double lo = previousFrequency;
            double hi = frequency;
            for (int iteration = 0; iteration < 48; ++iteration)
            {
                const double mid = (lo + hi) / 2;
                if (loopTransfer(state, mid).imag() > 0)
                    lo = mid;
                else
                    hi = mid;
            }
            const double root = (lo + hi) / 2;
            if (loopTransfer(state, root).real() > 0)
                return root;
        }
        previous = current;
        previousFrequency = frequency;
    }
    return 0;
}

double phaseErrorCents(const State& state)
{
    const double fundamental = frozenFundamental(state);
    return fundamental > 0 && state.targetFrequency > 0
        ? 1200 * std::log2(fundamental / state.targetFrequency) : 1e9;
}

bool destinationReachedWithOnlyEnergyAttackMotion(const State& state)
{
    // The production tension coordinate bounds the deliberate attack bend.
    // A new physical release can seed up to the existing +6-cent span while
    // its original follower catches up. This independent allowance derives
    // from the observed live q; it does not excuse an interval-sized refret
    // delay or assign a perceptual click/pitch threshold. 0.05 cent covers
    // floating-point/real-axis compensation error in the frozen observer.
    const double energyPitch = 600 * std::log2(1 + std::max(0.0, state.attackTension));
    const double error = phaseErrorCents(state);
    return error >= -energyPitch - .05 && error <= .05;
}

std::unique_ptr<electry::ElectryEngine> makeEngine(
    double rate, int stringIndex, electry::PlayStyle style = electry::PlayStyle::Sustain,
    double strum = 0, std::uint32_t seed = 0x160c2026u)
{
    auto engine = std::make_unique<electry::ElectryEngine>();
    electry::EngineParameters parameters;
    electry::applyGuitarBuild(parameters, electry::defaultGuitarBuild);
    parameters.strumSpreadSeconds = static_cast<float>(strum);
    parameters.sympatheticAmount = 0;
    parameters.artifactAmount = 0;
    parameters.pickNoise = parameters.fingerNoise = parameters.releaseNoise = 0;
    parameters.bodyResonance = 0;
    engine->setParameters(parameters);
    engine->setVariationSeed(seed);
    engine->prepare(rate, 64);
    engine->setSoloStringMask(static_cast<std::uint8_t>(1u << stringIndex));
    engine->noteOn(electry::ElectryEngine::firstPlayStyleKeyswitchNote
                       + static_cast<int>(style), 1.0f);
    return engine;
}

void render(electry::ElectryEngine& engine, int frames, int block = 64)
{
    std::array<float, 64> left {}, right {};
    while (frames > 0)
    {
        const int count = std::min({frames, block, 64});
        engine.process(left.data(), right.data(), count);
        bool finite = true;
        for (int frame = 0; frame < count; ++frame)
            finite = finite && std::isfinite(left[static_cast<std::size_t>(frame)])
                && std::isfinite(right[static_cast<std::size_t>(frame)]);
        expect(finite, "Refret render produced non-finite output");
        frames -= count;
    }
}

State firstRelease(electry::ElectryEngine& engine, int stringIndex,
                   int maximumFrames)
{
    bool contacted = false;
    for (int frame = 0; frame < maximumFrames; ++frame)
    {
        const auto state = electry::ElectryEngineTestAccess::state(engine, stringIndex);
        contacted = contacted || (state.phase == 1 && state.pending == 0);
        if (contacted && state.phase == 2 && state.pending == 0)
            return state;
        render(engine, 1);
    }
    expect(false, "Picked contact never reached its physical Release");
    return electry::ElectryEngineTestAccess::state(engine, stringIndex);
}

void pickedPitchSettlesBeforeRelease()
{
    for (const double rate : {44100., 48000., 96000., 384000.})
        for (const auto [stringIndex, note] :
             {std::pair<int, int> {0, 40}, {2, 47}, {7, 74}})
            for (const int interval : {-3, -1, 1, 3})
            {
                auto engine = makeEngine(rate, stringIndex);
                engine->noteOn(note, .85f);
                render(*engine, static_cast<int>(rate / 5));
                engine->noteOn(note + interval, .85f);
                const auto state = firstRelease(*engine, stringIndex,
                                                static_cast<int>(rate / 20));
                const double error = phaseErrorCents(state);
                const double energyPitch = 600 * std::log2(1 + std::max(0.0, state.attackTension));
                largestReleaseError = std::max(largestReleaseError, std::abs(error));
                largestEnergyPitch = std::max(largestEnergyPitch, energyPitch);
                largestUnexplainedReleaseError = std::max(largestUnexplainedReleaseError,
                    std::max(error, -error - energyPitch));
                // The defect is an interval-sized finger lag in addition to
                // the existing, explicitly bounded energy-derived attack bend.
                expect(destinationReachedWithOnlyEnergyAttackMotion(state),
                    "Picked refret retains interval-sized delay lag at Release: rate "
                    + std::to_string(rate) + ", string " + std::to_string(stringIndex)
                    + ", interval " + std::to_string(interval)
                    + ", cents " + std::to_string(error));
                expect(state.transitionRemaining == 0 && !state.queued && !state.locked,
                       "Picked contact released before its fret handoff completed");
            }
}

void handoffHasStableEndpointTaps()
{
    for (const int interval : {-3, 1, 3})
    {
        auto engine = makeEngine(48000, 2);
        engine->noteOn(47, .85f);
        render(*engine, 9600);
        const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
        const double beforePitch = frozenFundamental(before);
        engine->noteOn(47 + interval, .85f);
        auto state = electry::ElectryEngineTestAccess::state(*engine, 2);
        auto source = state;
        source.targetFrequency = beforePitch;
        expect(std::abs(phaseErrorCents(source)) < 5,
               "A fret NoteOn snapped the audible source before the handoff");
        expect(state.transitionRemaining > 0 && state.newWeight == 0,
               "A picked refret does not begin at the source-tap endpoint");
        auto oldOnly = state;
        oldOnly.newWeight = 0;
        auto newOnly = state;
        newOnly.newWeight = 1;
        const double oldEndpoint = frozenFundamental(oldOnly);
        const double newEndpoint = frozenFundamental(newOnly);
        double previousWeight = -1;
        const int expectedFrames = (state.transitionRemaining + 1) / 2;
        for (int frame = 0; frame <= expectedFrames; ++frame)
        {
            state = electry::ElectryEngineTestAccess::state(*engine, 2);
            if (state.transitionRemaining == 0)
                break;
            expect(state.newWeight >= previousWeight && state.newWeight <= 1,
                   "The picked handoff left its convex source/destination interval");
            previousWeight = state.newWeight;
            // Observe only three interior host samples; the independent
            // complex scan is intentionally separate from the realtime DSP.
            if (frame == 0 || frame == expectedFrames / 2
                           || frame + 1 == expectedFrames)
            {
                oldOnly = state;
                oldOnly.newWeight = 0;
                newOnly = state;
                newOnly.newWeight = 1;
                const double oldChange = 1200 * std::log2(
                    frozenFundamental(oldOnly) / oldEndpoint);
                const double newChange = 1200 * std::log2(
                    frozenFundamental(newOnly) / newEndpoint);
                expect(std::abs(oldChange) < 1 && std::abs(newChange) < 1,
                       "A picked handoff moved an endpoint tap through intermediate frets");
                expect(frozenFundamental(state) > 0,
                       "The complex mixed loop lost its fundamental phase closure");
            }
            render(*engine, 1);
        }
        state = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(state.transitionRemaining == 0 && state.newWeight == 1,
               "The picked handoff did not finish at the destination-tap endpoint");
    }
}

void repicksKeepTheExistingSmallAttackPitch()
{
    for (const double rate : {44100., 48000., 384000.})
        for (const auto [stringIndex, note] :
             {std::pair<int, int> {0, 40}, {2, 47}, {7, 74}})
        {
            auto engine = makeEngine(rate, stringIndex);
            engine->noteOn(note, .85f);
            render(*engine, static_cast<int>(rate / 5));
            engine->noteOn(note, .85f);
            const auto state = firstRelease(*engine, stringIndex,
                                            static_cast<int>(rate / 20));
            expect(destinationReachedWithOnlyEnergyAttackMotion(state),
                   "Same-note repick changed the existing small attack-pitch motion");
        }
}

std::vector<float> samples(electry::ElectryEngine& engine, int frames, int block)
{
    std::vector<float> result(static_cast<std::size_t>(frames));
    std::array<float, 64> left {}, right {};
    for (int at = 0; at < frames;)
    {
        const int count = std::min({frames - at, block, 64});
        engine.process(left.data(), right.data(), count);
        std::copy_n(left.begin(), count, result.begin() + at);
        at += count;
    }
    return result;
}

void interruptionsAreIndependentOfHostBlocks()
{
    for (const double rate : {48000., 384000.})
    {
        auto scalar = makeEngine(rate, 2);
        auto blocked = std::make_unique<electry::ElectryEngine>(*scalar);
        scalar->noteOn(47, .8f);
        blocked->noteOn(47, .8f);
        expect(samples(*scalar, static_cast<int>(rate / 5), 1)
                   == samples(*blocked, static_cast<int>(rate / 5), 64),
               "Fresh-note output depends on host block partition");
        for (int index = 0; index < 36; ++index)
        {
            const int note = 47 + (index % 3 == 0 ? 0 : index % 3 == 1 ? 3 : -1);
            for (auto* engine : {scalar.get(), blocked.get()})
            {
                if (index % 4 == 0)
                    engine->setSustainPedal(index % 8 == 0);
                engine->noteOn(note, .5f + .1f * (index % 4));
                if (index > 0)
                    engine->noteOff(47 + ((index - 1) % 3 == 0 ? 0
                                           : (index - 1) % 3 == 1 ? 3 : -1));
            }
            // Includes several changes within one Contact and others during
            // Release/Lift. Event boundaries are identical; only render-call
            // partitioning differs.
            const int frames = static_cast<int>(rate * (index % 3 == 0 ? .0005
                                                       : index % 3 == 1 ? .003 : .009));
            expect(samples(*scalar, frames, 1) == samples(*blocked, frames, 64),
                   "Interrupted refret depends on host block partition");
            expect(electry::ElectryEngineTestAccess::finiteString(*scalar, 2),
                   "Interrupted refret left non-finite string state");
        }
        scalar->reset();
        const auto silence = samples(*scalar, 256, 64);
        expect(std::all_of(silence.begin(), silence.end(),
                           [] (float value) { return value == 0; }),
               "Reset did not clear an interrupted refret");
    }
}

void intentionalFingerGesturesRemainContinuous()
{
    for (const auto style : {electry::PlayStyle::Hammer, electry::PlayStyle::Slide})
    {
        auto engine = makeEngine(48000, 2, style);
        engine->noteOn(47, .85f);
        render(*engine, 9600);
        const double oldPitch = frozenFundamental(
            electry::ElectryEngineTestAccess::state(*engine, 2));
        engine->noteOn(50, .85f);
        auto state = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(state.legatoBlend < 1 && !state.locked && !state.queued
                   && state.transitionRemaining == 0,
               "Intentional Hammer/Slide finger motion became a picked handoff");
        expect(std::abs(1200 * std::log2(frozenFundamental(state) / oldPitch)) < 5,
               "Intentional Hammer/Slide snapped to its new fret at MIDI arrival");
        render(*engine, 9600);
        state = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(state.legatoBlend == 1 && !state.locked && !state.queued,
               "Intentional finger motion did not finish normally");
    }
}

void everyPickedStyleCompletesItsBoundaryBeforeRelease()
{
    for (const auto style : {electry::PlayStyle::Sustain, electry::PlayStyle::PalmMute,
                            electry::PlayStyle::Harmonics, electry::PlayStyle::Pinch,
                            electry::PlayStyle::Dead})
    {
        auto engine = makeEngine(48000, 2, style);
        engine->noteOn(47, .85f);
        (void) firstRelease(*engine, 2, 4800);
        render(*engine, 48);
        engine->noteOn(48, .85f);
        const auto arrived = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(arrived.transitionRemaining > 0,
               "A picked style failed to hand off an already ringing string");
        const auto released = firstRelease(*engine, 2, 4800);
        expect(released.transitionRemaining == 0 && !released.queued && !released.locked,
               "A picked style released before its boundary handoff finished");
        expect(electry::ElectryEngineTestAccess::finiteString(*engine, 2),
               "A picked style left non-finite state after its boundary handoff");
    }
}

void unperformedAndQueuedDestinationsKeepTheSoundingBoundary()
{
    auto engine = makeEngine(48000, 2);
    engine->noteOn(47, .85f);
    render(*engine, 9600);
    engine->noteOn(48, .85f);
    const auto initial = electry::ElectryEngineTestAccess::state(*engine, 2);
    engine->noteOn(50, .85f); // No rendered sample of 48 was performed.
    auto replacement = electry::ElectryEngineTestAccess::state(*engine, 2);
    expect(replacement.newWeight == 0 && !replacement.queued,
           "Same-clock replacement queued an unheard destination");
    auto initialSource = initial;
    initialSource.newWeight = 0;
    auto replacementSource = replacement;
    replacementSource.newWeight = 0;
    const double sourceChange = 1200 * std::log2(
        frozenFundamental(replacementSource) / frozenFundamental(initialSource));
    expect(std::abs(sourceChange) < 1,
           "Same-clock replacement lost the sounding source boundary");
    render(*engine, std::max(1, replacement.transitionRemaining / 8));
    const auto performed = electry::ElectryEngineTestAccess::state(*engine, 2);
    engine->noteOn(46, .8f);
    const auto queued = electry::ElectryEngineTestAccess::state(*engine, 2);
    expect(queued.queued && queued.transitionRemaining == performed.transitionRemaining,
           "An interrupted audible handoff discarded its existing boundary mixture");
    expect(queued.newWeight == performed.newWeight,
           "An interrupted audible handoff restarted its blend coordinate");
    const auto released = firstRelease(*engine, 2, 4800);
    expect(released.note == 46 && released.transitionRemaining == 0 && !released.queued
               && !released.locked && released.newWeight == 1,
           "Latest queued fret destination did not reach its own picked Release");
    expect(destinationReachedWithOnlyEnergyAttackMotion(released),
           "Queued fret destination released with residual interval lag: "
               + std::to_string(phaseErrorCents(released)) + " cents");
}

void delayedCancellationPedalAndResetClearPendingOwnership()
{
    for (const bool pedal : {false, true})
    {
        auto engine = makeEngine(48000, 2, electry::PlayStyle::Sustain, .04);
        engine->noteOn(47, .85f);
        render(*engine, 9600);
        engine->setSustainPedal(pedal);
        engine->noteOn(48, .85f);
        auto state = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(state.pending > 0,
               "Delayed-refret lifecycle fixture did not schedule a travelling contact");
        engine->noteOff(47);
        engine->noteOff(48);
        state = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(!state.held, "Cancelled delayed refret retained a MIDI key owner");
        expect(state.sustained == pedal,
               "Delayed refret cancellation lost its sustain-pedal ownership");
        render(*engine, 4800);
        state = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(state.transitionRemaining == 0 && !state.queued && !state.locked,
               "Cancelled delayed refret left a stale handoff clock");
        engine->setSustainPedal(false);
        engine->reset();
        state = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(state.transitionRemaining == 0 && !state.queued && !state.locked,
               "Reset retained delayed refret state");
        engine->prepare(96000, 64);
        state = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(state.transitionRemaining == 0 && !state.queued && !state.locked,
               "Sample-rate reprepare retained delayed refret state");
    }
}

void incomingLargeJumpRetainsTheHeardMixtureAndLegacyChoke()
{
    // The default seed has no performance pre-roll. This explicit immediate
    // picking-hand command must exercise the in-flight guard, rather than
    // merely reserving a delayed repick that arrives after the queue clears.
    auto engine = makeEngine(48000, 2, electry::PlayStyle::Sustain, 0, 0);
    engine->noteOn(47, .85f);
    render(*engine, 9600);
    engine->noteOn(48, .85f);
    render(*engine, 24);
    const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
    expect(before.transitionRemaining > 0 && before.newWeight > 0 && before.newWeight < 1,
           "Large-jump lifecycle fixture missed the audible boundary mixture");
    engine->noteOn(59, .85f);
    const auto after = electry::ElectryEngineTestAccess::state(*engine, 2);
    expect(after.queued && after.transitionRemaining == before.transitionRemaining
               && after.newWeight == before.newWeight,
           "An incoming large jump replaced a partially heard boundary mixture");
    // This is the established production choke, not a newly fitted amplitude
    // or perceptual acceptance threshold. It must be retained when the jump
    // arrives while an older handoff is still audible.
    constexpr double retainedEnergy = .28 * .28;
    expect(before.loopEnergy > 0
               && std::abs(after.loopEnergy / before.loopEnergy - retainedEnergy) < 1e-6,
           "An incoming large jump changed the established whole-string choke");
#if ELECTRY_ENERGY_ATTACK_PITCH
    expect(before.attackTension > 0
               && std::abs(after.attackTension / before.attackTension - retainedEnergy) < 1e-6,
           "An incoming large jump lost the established attack-tension choke");
#endif
    engine->noteOn(electry::ElectryEngine::firstRepickNote + 2, .85f);
    const auto repicked = electry::ElectryEngineTestAccess::state(*engine, 2);
    expect(repicked.loopEnergy == after.loopEnergy
               && repicked.attackTension == after.attackTension,
           "A picking-hand repick applied another fret-change choke to the queued note");
    expect(repicked.queued && repicked.transitionRemaining == after.transitionRemaining
               && repicked.newWeight == after.newWeight,
           "A picking-hand repick replaced the queued fret's audible mixture");
    const auto released = firstRelease(*engine, 2, 4800);
    expect(released.note == 59 && released.transitionRemaining == 0 && !released.queued
               && !released.locked && released.newWeight == 1,
           "Queued large destination did not reach its physical picked Release");
    expect(destinationReachedWithOnlyEnergyAttackMotion(released),
           "Queued large destination retained interval-sized delay lag: "
               + std::to_string(phaseErrorCents(released)) + " cents");
    expect(electry::ElectryEngineTestAccess::finiteString(*engine, 2),
           "Queued large destination left non-finite waveguide state");
}

void memberBendCannotRetuneTheReleasedRefret()
{
    auto engine = makeEngine(48000, 2);
    constexpr electry::ElectryEngine::ExpressionId member = 7;
    engine->setExpressionPitchBend(member, .7f);
    engine->noteOn(47, .85f, member);
    render(*engine, 9600);
    engine->noteOn(48, .85f, member);
    (void) firstRelease(*engine, 2, 4800);
    engine->noteOff(47, member);
    engine->noteOff(48, member);
    auto unchanged = std::make_unique<electry::ElectryEngine>(*engine);
    engine->setExpressionPitchBend(member, 3.0f);
    expect(samples(*engine, 9600, 64) == samples(*unchanged, 9600, 64),
           "Reused MPE member bend retuned a released refret tail");
}
}

int main()
{
    pickedPitchSettlesBeforeRelease();
    handoffHasStableEndpointTaps();
    repicksKeepTheExistingSmallAttackPitch();
    interruptionsAreIndependentOfHostBlocks();
    intentionalFingerGesturesRemainContinuous();
    everyPickedStyleCompletesItsBoundaryBeforeRelease();
    unperformedAndQueuedDestinationsKeepTheSoundingBoundary();
    delayedCancellationPedalAndResetClearPendingOwnership();
    incomingLargeJumpRetainsTheHeardMixtureAndLegacyChoke();
    memberBendCannotRetuneTheReleasedRefret();
    std::cout << std::setprecision(9) << "48 picked refrets at physical Release: max absolute "
              << largestReleaseError << " cents; live energy-pitch span "
              << largestEnergyPitch << " cents; max unexplained residue "
              << largestUnexplainedReleaseError << " cents\n";
    std::cout << assertions << " assertions, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
