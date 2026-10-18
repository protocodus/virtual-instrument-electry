#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <memory>
#include <limits>
#include <string>
#include <vector>

namespace
{
double expectedBoundaryWeight(double progress)
{
    // Independently integrate the fixed 3/2 peak-speed construction: direct
    // double polynomials at each shoulder, with a constant-speed center.
    const auto shoulder = [] (double t)
    {
        return 13.5 * t * t * t - 20.25 * t * t * t * t;
    };
    if (progress < 1.0 / 3)
        return shoulder(progress);
    if (progress > 2.0 / 3)
        return 1 - shoulder(1 - progress);
    return 1.5 * progress - .25;
}
}

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
        int dampingStyle = 0;
        int playStyle = 0;
        bool strokeIsUp = false;
        float velocity = 0;
        bool pendingStroke = false;
        int pendingStyle = 0;
        bool pendingUp = false;
        float pendingVelocity = 0;
        std::uint64_t strokeOrder = 0;
        std::uint64_t pendingOrder = 0;
        std::uint32_t noiseState = 0;
        int excitationRemaining = 0;
        double pendingAttackEnergy = 0;
        double pendingAttackScale = 0;
        double rate = 0;
        double targetFrequency = 0;
        double physicalPeriod = 0;
        double targetDelay = 0;
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
        std::array<double, 5> loss {1, 0, 0, 0, 0};
        std::array<double, 5> material {1, 0, 0, 0, 0};
        std::array<double, 5> hand {1, 0, 0, 0, 0};
    };

#ifndef ELECTRY_REFRET_TEST_BASELINE
    static void isolateBoundaryReadProbe(ElectryEngine& engine, int stringIndex,
                                         int duration)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        engine.beginRefretHandoff(voice, duration, true);
        voice.excitationPhase = ElectryEngine::ExcitationPhase::Idle;
        voice.noiseRemaining = 0;
        voice.touchDepth = 0;
        voice.releasing = false;
    }

    static double renderObservedBoundaryWeight(ElectryEngine& engine, int stringIndex)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        auto& loop = voice.vertical;
        // Constant disjoint interpolation neighborhoods make the two actual
        // cubic readers independent known inputs, regardless of their fraction.
        loop.line.fill(.25f);
        const int destination = loop.writeIndex
                              - static_cast<int>(std::ceil(loop.currentDelay));
        for (int offset = -1; offset <= 2; ++offset)
            loop.line[static_cast<std::size_t>((destination + offset)
                                              & (ElectryEngine::delayLineSize - 1))] = .75f;
        const float source = loop.readFractional(
            loop.currentDelay + voice.refretVerticalOffset);
        const float target = loop.readFractional(loop.currentDelay);
        // A DF-II first allpass beginning at zero stores its actual input in
        // state. Observe that production sample, before subsequent recursive
        // filters, rather than repeating the production fade polynomial.
        loop.dispersion1.state = 0;
        engine.sympatheticBus_ = engine.sympatheticBusDelayed_ = 0;
        voice.busContribution = 0;
        engine.feedbackDrive_ = 0;
        ElectryEngine::RenderSums sums {};
        engine.renderVoice(voice, sums);
        return (static_cast<double>(loop.dispersion1.state) - source)
             / (static_cast<double>(target) - source);
    }
#endif

    static void setLiveBend(ElectryEngine& engine, int stringIndex, bool member, float semitones)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        if (member)
        {
            engine.expressionPitchBendTargets_[7] = semitones;
            engine.snapExpressionPitchBendToTarget(7);
        }
        else
            engine.pitchBendSemitones_ = engine.pitchBendTarget_ = semitones;
        if (voice.active) engine.configureVoicePitch(voice, false);
    }

    static void setLiveVibrato(ElectryEngine& engine, int stringIndex, float semitones)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        voice.vibratoSemitones = semitones;
        engine.configureVoicePitch(voice, false);
    }

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
        result.pendingAttackEnergy = voice.pendingAttackPitchEnergyJoules;
        result.pendingAttackScale = voice.pendingAttackPitchElasticScalePerJoule;
#endif
        result.pending = voice.startDelaySamples;
        result.dampingStyle = static_cast<int>(voice.dampingStyle);
        result.playStyle = static_cast<int>(voice.playStyle);
        result.strokeIsUp = voice.strokeIsUp;
        result.velocity = voice.velocity;
        result.pendingStroke = voice.pendingRepick.active;
        result.pendingStyle = static_cast<int>(voice.pendingRepick.playStyle);
        result.pendingUp = voice.pendingRepick.strokeIsUp;
        result.pendingVelocity = voice.pendingRepick.velocity;
        result.strokeOrder = voice.startOrder;
        result.pendingOrder = voice.pendingRepick.startOrder;
        result.noiseState = voice.noiseState;
        result.excitationRemaining = voice.excitationRemaining;
        result.rate = engine.sampleRate_;
        result.targetFrequency = engine.sampleRate_ / voice.lastCompensatedPeriod;
        result.physicalPeriod = loop.currentDelay + voice.lastCompensatedPeriod
                              - voice.compensatedPeriodVertical;
        result.delay = loop.currentDelay;
        result.targetDelay = loop.targetDelay;
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
#if defined(ELECTRY_REFRET_TEST_CUBIC_HANDOFF)
            result.newWeight = progress * progress * (3 - 2 * progress);
#elif defined(ELECTRY_REFRET_TEST_QUINTIC_HANDOFF)
            result.newWeight = progress * progress * progress
                * (6 * progress * progress - 15 * progress + 10);
#else
            // Direct double shoulders are independent of the production
            // mirrored float evaluation; use them in the full loop observer.
            result.newWeight = expectedBoundaryWeight(progress);
#endif
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
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
        if (loop.lossShelfActive)
        {
            result.pole = 0;
            result.loss = coefficients(loop.lossShelf);
        }
#endif
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
    transfer *= biquadTransfer(state.loss, z);
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

void liveExpressionMovesBothHandoffEndpoints()
{
    for (const bool member : {false, true})
        for (const bool alreadyMoving : {false, true})
        {
            auto engine = makeEngine(48000, 2);
            electry::EngineParameters parameters;
            parameters.pickHardness = 0;
            parameters.bendTimeSeconds = .04f;
            parameters.pickNoise = parameters.fingerNoise = parameters.releaseNoise = 0;
            parameters.artifactAmount = parameters.sympatheticAmount = parameters.bodyResonance = 0;
            engine->setParameters(parameters);
            const auto id = static_cast<electry::ElectryEngine::ExpressionId>(member ? 7 : 0);
            engine->noteOn(47, .85f, id);
            render(*engine, 9600);
            const auto bend = [&] {
                if (member) engine->setExpressionPitchBend(id, 24);
                else engine->setPitchBendSemitones(24);
            };
            if (alreadyMoving) { bend(); render(*engine, 32); }
            engine->noteOn(48, .85f, id);
            const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
            expect(before.locked && before.transitionRemaining > 96,
                   "Expression fixture did not enter a sufficiently long handoff");
            if (alreadyMoving)
                expect(before.physicalPeriod - before.rate / before.targetFrequency > 1,
                       "Starting a fret instantly consumed an already moving bend");
            else bend();
            render(*engine, 48);
            const auto after = electry::ElectryEngineTestAccess::state(*engine, 2);
            expect(after.locked && after.transitionRemaining > 0,
                   "Expression fixture escaped its active handoff");
            expect(after.physicalPeriod < before.physicalPeriod - 1,
                   "An active fret handoff froze global/member bend at its destination");
            const double sourceBefore = before.physicalPeriod + before.oldDelay - before.delay;
            const double sourceAfter = after.physicalPeriod + after.oldDelay - after.delay;
            expect(sourceAfter < sourceBefore - 1,
                   "An active fret handoff froze global/member bend at its source");
            expect(std::abs(sourceAfter / sourceBefore
                          - after.physicalPeriod / before.physicalPeriod) < .001,
                   "A tension bend moved the two boundary periods inconsistently");
        }

    // Isolate the finger-vibrato coordinate at a control boundary. This uses
    // the real pitch solve and original per-sample follower; it avoids making
    // the test depend on the random finger phase or shared onset ramp.
    auto vibrato = makeEngine(48000, 2);
    vibrato->noteOn(47, .85f);
    render(*vibrato, 9600);
    vibrato->noteOn(48, .85f);
    render(*vibrato, 1); // Begin just after a control tick.
    auto control = std::make_unique<electry::ElectryEngine>(*vibrato);
    const auto before = electry::ElectryEngineTestAccess::state(*vibrato, 2);
    electry::ElectryEngineTestAccess::setLiveVibrato(*vibrato, 2, .5f);
    const auto configured = electry::ElectryEngineTestAccess::state(*vibrato, 2);
    render(*vibrato, 4);
    render(*control, 4);
    const auto after = electry::ElectryEngineTestAccess::state(*vibrato, 2);
    const auto unchanged = electry::ElectryEngineTestAccess::state(*control, 2);
    expect(after.locked && after.physicalPeriod < unchanged.physicalPeriod - .01,
           "An active fret handoff froze its live finger-vibrato coordinate");
    expect(after.physicalPeriod + after.oldDelay - after.delay
               < unchanged.physicalPeriod + unchanged.oldDelay - unchanged.delay - .01,
           "Finger vibrato failed to move the source boundary period");
    expect(after.newWeight == unchanged.newWeight
               && after.transitionRemaining == before.transitionRemaining - 8,
           "Tension expression restarted the fret handoff clock");
    const double requestedDelta = configured.rate / configured.targetFrequency
                               - before.rate / before.targetFrequency;
    const double performedFraction = 1 - std::exp(-8 / (.006 * before.rate));
    const double expected = configured.physicalPeriod + requestedDelta * performedFraction;
    expect(std::abs(after.physicalPeriod - expected) < .002,
           "Finger vibrato lost the established six-ms tension follower: measured "
               + std::to_string(after.physicalPeriod) + ", expected " + std::to_string(expected));
}

void queuedFretKeepsItsLengthOutOfTheExpressionFollower()
{
    auto engine = makeEngine(48000, 2);
    engine->noteOn(47, .85f);
    render(*engine, 9600);
    engine->noteOn(48, .85f);
    render(*engine, 8);
    const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
    engine->noteOn(50, .85f);
    const auto queued = electry::ElectryEngineTestAccess::state(*engine, 2);
    render(*engine, 16);
    const auto held = electry::ElectryEngineTestAccess::state(*engine, 2);
    expect(queued.queued && held.queued && held.locked,
           "Queued-expression fixture did not retain the heard boundary mixture");
    expect(std::abs(1200 * std::log2(held.physicalPeriod / before.physicalPeriod)) < 1,
           "A queued fret entered the tension-only follower");
    engine->setPitchBendSemitones(24);
    render(*engine, 16);
    const auto bent = electry::ElectryEngineTestAccess::state(*engine, 2);
    expect(bent.physicalPeriod < held.physicalPeriod - .01,
           "A queued fret froze the live tension-only follower");
    expect(electry::ElectryEngineTestAccess::finiteString(*engine, 2),
           "Queued expression left non-finite waveguide state");

    // Both MPE components can approach the documented wide safety limits.
    // Exercise them through overlapping queued destinations and rate changes.
    for (const double rate : {48000., 384000.})
    {
        auto wide = makeEngine(rate, 2);
        wide->noteOn(47, .8f, 7);
        render(*wide, static_cast<int>(rate / 5));
        for (int index = 0; index < 12; ++index)
        {
            wide->noteOn(48 + index % 3, .8f, 7);
            wide->setExpressionPitchBend(7, index % 2 ? -192 : 192);
            wide->setExpressionMasterPitchBend(7, index % 2 ? -192 : 192);
            wide->snapExpressionPitchBendToTarget(7);
            render(*wide, 8);
            const auto state = electry::ElectryEngineTestAccess::state(*wide, 2);
            expect(std::isfinite(state.delay) && std::isfinite(state.oldDelay)
                       && std::isfinite(state.physicalPeriod)
                       && electry::ElectryEngineTestAccess::finiteString(*wide, 2),
                   "Wide MPE bend through a queued fret left non-finite state");
        }
        wide->reset();
    }
}

void clippedExpressionTargetsReturnToTheirOwnFret()
{
    for (const bool member : {false, true})
        for (const int direction : {-1, 1})
        {
            auto engine = makeEngine(48000, 2);
            const auto id = static_cast<electry::ElectryEngine::ExpressionId>(member ? 7 : 0);
            electry::ElectryEngineTestAccess::setLiveBend(*engine, 2, member, direction * 96.f);
            engine->noteOn(47, .85f, id);
            render(*engine, 9600);
            // Moving further into an already pinned range creates no debt.
            electry::ElectryEngineTestAccess::setLiveBend(*engine, 2, member, direction * 192.f);
            render(*engine, 4);
            engine->noteOn(48, .85f, id);
            auto pinned = electry::ElectryEngineTestAccess::state(*engine, 2);
            expect(pinned.locked && pinned.delay == pinned.targetDelay,
                   "An outward already-clipped bend invented a pending pitch movement: member "
                       + std::to_string(member) + ", direction " + std::to_string(direction)
                       + ", delay " + std::to_string(pinned.delay) + ", target " + std::to_string(pinned.targetDelay));
            render(*engine, 1); // Keep the next few samples clear of control ticks.
            pinned = electry::ElectryEngineTestAccess::state(*engine, 2);
            electry::ElectryEngineTestAccess::setLiveBend(*engine, 2, member, 0);
            const auto centred = electry::ElectryEngineTestAccess::state(*engine, 2);
            render(*engine, 4);
            const auto moved = electry::ElectryEngineTestAccess::state(*engine, 2);
            const double requestedDelta = centred.rate / centred.targetFrequency
                                        - pinned.rate / pinned.targetFrequency;
            const double expected = centred.physicalPeriod + requestedDelta
                * (1 - std::exp(-8 / (.006 * centred.rate)));
            expect(std::abs(moved.physicalPeriod - expected) < .05,
                   "Leaving a pitch clamp ratcheted toward the opposite limit: measured "
                       + std::to_string(moved.physicalPeriod) + ", expected " + std::to_string(expected));
            expect(moved.locked && std::isfinite(moved.oldDelay)
                       && electry::ElectryEngineTestAccess::finiteString(*engine, 2),
                   "A clipped-expression exit disrupted the active handoff");
        }
}

void sameClockLargeReplacementChokesOnlyTheActualFretChange()
{
    for (const bool pickingHandRepick : {false, true})
    {
        // Zero seed gives an immediate public command, so no hidden
        // performance pre-roll can postpone the repick beyond the handoff.
        auto engine = makeEngine(48000, 2, electry::PlayStyle::Sustain, 0, 0);
        engine->noteOn(47, .85f);
        render(*engine, 9600);
        const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
        engine->noteOn(51, .85f);
        const auto intermediate = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(intermediate.loopEnergy == before.loopEnergy,
               "The first small same-clock move unexpectedly choked the string");
        engine->noteOn(55, .85f); // 51 was never rendered: the source is still 47.
        const auto replaced = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(replaced.transitionRemaining > 0
                   && replaced.transitionRemaining == replaced.transitionLength
                   && replaced.newWeight == 0 && !replaced.queued,
               "Same-clock large replacement lost its unheard source boundary");
        constexpr double retainedEnergy = .28 * .28;
        expect(before.loopEnergy > 0
                   && std::abs(replaced.loopEnergy / before.loopEnergy
                               - retainedEnergy) < 1e-6,
               "Same-clock small hops bypassed the established net large-jump choke");
#if ELECTRY_ENERGY_ATTACK_PITCH
        expect(before.attackTension > 0
                   && std::abs(replaced.attackTension / before.attackTension
                               - retainedEnergy) < 1e-6,
               "Same-clock net large replacement missed the attack-tension choke");
#endif
        engine->noteOn(pickingHandRepick
                           ? electry::ElectryEngine::firstRepickNote + 2 : 55,
                       .85f);
        const auto repicked = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(repicked.loopEnergy == replaced.loopEnergy
                   && repicked.attackTension == replaced.attackTension,
               "An unheard same-destination repick applied a second fret-change choke");
        expect(repicked.transitionRemaining == replaced.transitionRemaining
                   && repicked.newWeight == replaced.newWeight && !repicked.queued,
               "An unheard same-destination repick replaced the source boundary");
        const auto released = firstRelease(*engine, 2, 4800);
        expect(released.note == 55 && released.transitionRemaining == 0
                   && !released.queued && !released.locked && released.newWeight == 1,
               "Same-clock net large destination did not finish before picked Release");
        expect(destinationReachedWithOnlyEnergyAttackMotion(released),
               "Same-clock net large destination retained interval-sized pitch lag");
        expect(electry::ElectryEngineTestAccess::finiteString(*engine, 2),
               "Same-clock large replacement left non-finite waveguide state");
    }
}

// An unheard intermediate destination must not turn a return to the original
// source into a new large physical jump.
void sameClockReturnDoesNotChokeAnUnperformedFret()
{
    auto engine = makeEngine(48000, 2, electry::PlayStyle::Sustain, 0, 0);
    engine->noteOn(47, .85f);
    render(*engine, 9600);
    const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
    engine->noteOn(51, .85f);
    engine->noteOn(47, .85f);
    const auto returned = electry::ElectryEngineTestAccess::state(*engine, 2);
    expect(returned.loopEnergy == before.loopEnergy
               && returned.attackTension == before.attackTension,
           "An unheard intermediate fret caused a spurious choke on returning to source");
    const auto released = firstRelease(*engine, 2, 4800);
    expect(released.transitionRemaining == 0 && !released.queued && !released.locked,
           "Same-clock return retained an unfinished handoff at picked Release");
    expect(electry::ElectryEngineTestAccess::finiteString(*engine, 2),
           "Same-clock return left non-finite waveguide state");
}

void queuedRepickCannotReplaceTheSoundingStrokeBeforeContact()
{
    for (const double rate : {48000.0, 96000.0, 384000.0})
        for (const int gesture : {0, 1, 2})
        {
            const auto initial = gesture == 2 ? electry::PlayStyle::PalmMute
                                              : electry::PlayStyle::Sustain;
            auto engine = makeEngine(rate, 2, initial, 0, 0);
            engine->noteOn(47, .85f);
            render(*engine, static_cast<int>(.2 * rate));
            engine->noteOn(48, .85f);
            const auto fresh = electry::ElectryEngineTestAccess::state(*engine, 2);
            const int oversampling = static_cast<int>(std::lround(fresh.rate / rate));
            render(*engine, std::max(1, fresh.transitionLength / (4 * oversampling)));
            engine->noteOn(49, .8f);
            render(*engine, 1);
            auto reference = std::make_unique<electry::ElectryEngine>(*engine);
            const auto command = gesture == 1
                ? electry::ElectryEngine::firstKeyswitchNote
                      + static_cast<int>(electry::PickStyle::Up)
                : electry::ElectryEngine::firstPlayStyleKeyswitchNote
                      + static_cast<int>(gesture == 2 ? electry::PlayStyle::Sustain
                                                    : electry::PlayStyle::PalmMute);
            engine->noteOn(command, 1);
            reference->noteOn(command, 1);
            engine->noteOn(electry::ElectryEngine::firstRepickNote + 2, .8f);
            const auto pending = electry::ElectryEngineTestAccess::state(*engine, 2);
            expect(pending.queued && pending.pending > oversampling,
                   "Queued immediate repick missed the zero-natural-delay fixture");
            // Delay counts internal samples; stop before its physical contact.
            const int frames = (pending.pending - 1) / oversampling;
            for (int frame = 0; frame < frames; ++frame)
            {
                float left[1], right[1], refLeft[1], refRight[1];
                engine->process(left, right, 1);
                reference->process(refLeft, refRight, 1);
                expect(left[0] == refLeft[0] && right[0] == refRight[0],
                       "Queued repick changed the sounding stroke before contact");
            }
            const auto released = firstRelease(*engine, 2, static_cast<int>(.1 * rate));
            expect(released.note == 49 && released.transitionRemaining == 0
                       && !released.queued && !released.locked,
                   "Deferred repick failed to reach its latest physical picked Release");
        }
}
// A new fret changes geometry immediately, but its queued bridge hand must
// remain the one physically present until the pick arrives.

void newQueuedFretRetainsThePresentHandUntilContact()
{
    for (const double rate : {48000.0, 96000.0, 384000.0})
        for (const auto initial : {electry::PlayStyle::Sustain, electry::PlayStyle::PalmMute})
        {
            auto engine = makeEngine(rate, 2, initial, 0, 0);
            engine->noteOn(47, .85f);
            render(*engine, static_cast<int>(.2 * rate));
            engine->noteOn(48, .85f);
            const auto fresh = electry::ElectryEngineTestAccess::state(*engine, 2);
            const int oversampling = static_cast<int>(std::lround(fresh.rate / rate));
            render(*engine, std::max(1, fresh.transitionLength / (4 * oversampling)));
            const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
            const auto requested = initial == electry::PlayStyle::Sustain
                ? electry::PlayStyle::PalmMute : electry::PlayStyle::Sustain;
            engine->noteOn(electry::ElectryEngine::firstPlayStyleKeyswitchNote
                              + static_cast<int>(requested), 1);
            engine->noteOn(49, .8f);
            const auto queued = electry::ElectryEngineTestAccess::state(*engine, 2);
            expect(queued.queued && queued.pending > 0,
                   "New queued refret did not wait for the existing audible boundary");
            expect(queued.dampingStyle == before.dampingStyle,
                   "New queued refret replaced the present bridge hand before contact");
            const auto released = firstRelease(*engine, 2, static_cast<int>(.1 * rate));
            expect(released.dampingStyle == static_cast<int>(requested),
                   "New queued refret failed to apply its requested bridge hand at contact");
        }
}

void queuedNewFretCannotApplyTheFutureWristBeforeContact()
{
    for (const double rate : {48000.0, 96000.0, 384000.0})
        for (const int gesture : {0, 1, 2, 3})
            for (const bool textured : {false, true})
                for (const bool newMember : {false, true})
                {
                    const auto initial = gesture == 2 ? electry::PlayStyle::PalmMute
                                                      : electry::PlayStyle::Sustain;
                    auto engine = makeEngine(rate, 2, initial, 0, 0);
                    if (textured)
                    {
                        electry::EngineParameters parameters;
                        electry::applyGuitarBuild(parameters, electry::defaultGuitarBuild);
                        parameters.strumSpreadSeconds = 0;
                        parameters.sympatheticAmount = parameters.bodyResonance = 0;
                        engine->setParameters(parameters);
                    }
                    const electry::ElectryEngine::ExpressionId member = newMember ? 7 : 0;
                    engine->noteOn(47, .85f, member);
                    render(*engine, static_cast<int>(.2 * rate));
                    engine->noteOn(48, .85f, member);
                    const auto fresh = electry::ElectryEngineTestAccess::state(*engine, 2);
                    const int oversampling = static_cast<int>(std::lround(fresh.rate / rate));
                    render(*engine, std::max(1, fresh.transitionLength / (4 * oversampling)));
                    if (newMember)
                    {
                        engine->noteOn(49, .8f, member);
                        render(*engine, 1);
                    }
                    auto reference = std::make_unique<electry::ElectryEngine>(*engine);
                    const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
                    const auto requested = gesture == 0 ? electry::PlayStyle::PalmMute
                        : electry::PlayStyle::Sustain;
                    if (gesture != 1)
                        engine->noteOn(electry::ElectryEngine::firstPlayStyleKeyswitchNote
                                           + static_cast<int>(requested), 1);
                    else
                        engine->noteOn(electry::ElectryEngine::firstKeyswitchNote
                                           + static_cast<int>(electry::PickStyle::Up), 1);
                    const float velocity = gesture == 3 ? .45f : .8f;
                    const electry::ElectryEngine::ExpressionId incoming = newMember ? 8 : 0;
                    engine->noteOn(49, velocity, incoming);
                    reference->noteOn(49, .8f, incoming);
                    const auto queued = electry::ElectryEngineTestAccess::state(*engine, 2);
                    const auto control = electry::ElectryEngineTestAccess::state(*reference, 2);
                    expect(queued.queued && queued.pending > oversampling
                               && queued.pending == control.pending && queued.pendingStroke,
                           "New queued wrist missed the positive-wait same-geometry fixture");
                    expect(queued.playStyle == before.playStyle
                               && queued.strokeIsUp == before.strokeIsUp
                               && queued.velocity == before.velocity
                               && queued.strokeOrder == before.strokeOrder
                               && queued.noiseState == before.noiseState
                               && queued.phase == before.phase
                               && queued.excitationRemaining == before.excitationRemaining
                               && queued.pendingAttackEnergy == before.pendingAttackEnergy
                               && queued.pendingAttackScale == before.pendingAttackScale,
                           "New queued wrist replaced the previous Contact before arrival");
                    expect(queued.pendingStyle == static_cast<int>(requested)
                               && queued.pendingUp == (gesture == 1)
                               && queued.pendingVelocity == velocity,
                           "New queued wrist did not reserve its complete requested stroke");
                    // Both sides receive the same fret geometry now. Only the
                    // future wrist differs, so every pre-contact sample agrees.
                    const int frames = (std::min(queued.pending, control.pending) - 1)
                                     / oversampling;
                    const auto actual = samples(*engine, frames, 1);
                    const auto unchanged = samples(*reference, frames, 1);
                    expect(actual == unchanged,
                           "Future queued wrist changed same-geometry audio before contact");
                    const auto released = firstRelease(*engine, 2, static_cast<int>(.1 * rate));
                    expect(released.playStyle == static_cast<int>(requested)
                               && released.strokeIsUp == (gesture == 1)
                               && released.velocity == velocity
                               && released.strokeOrder == queued.pendingOrder
                               && !released.pendingStroke && released.note == 49,
                           "Queued wrist failed to install its reserved stroke at contact");
                }
}

void renderedBoundaryWeightsReduceColorWithoutChangingTheClock()
{
#ifndef ELECTRY_REFRET_TEST_BASELINE
    for (const int length : {4, 52, 151, 288, 603, 1151})
    {
        auto engine = makeEngine(48000, 2);
        engine->noteOn(47, .85f);
        render(*engine, 9600);
        engine->noteOn(50, .85f);
        electry::ElectryEngineTestAccess::isolateBoundaryReadProbe(*engine, 2, length);
        double previous = -1;
        double measuredDeficit = 0;
        double expectedDeficit = 0;
        double cubicDeficit = 0;
        const double numericalTolerance = 16 * std::numeric_limits<float>::epsilon();
        for (int sample = 0; sample < length; ++sample)
        {
            const auto before = electry::ElectryEngineTestAccess::state(*engine, 2);
            expect(before.transitionRemaining == length - sample,
                   "Changing fade color changed the physical handoff clock");
            const double weight = electry::ElectryEngineTestAccess::renderObservedBoundaryWeight(
                *engine, 2);
            expect(std::isfinite(weight) && weight >= -numericalTolerance
                       && weight <= 1 + numericalTolerance,
                   "The actual feedback read left the convex source/destination interval");
            expect(weight + numericalTolerance >= previous,
                   "The actual feedback read reversed its boundary handoff");
            if (sample == 0 || sample + 1 == length)
                expect(std::abs(weight - (sample == 0 ? 0 : 1)) < numericalTolerance,
                       "The actual feedback read lost an exact boundary endpoint");
            const double progress = static_cast<double>(sample) / (length - 1);
            const double expected = expectedBoundaryWeight(progress);
            expect(std::abs(weight - expected) < numericalTolerance,
                   "The rendered feedback sample differs from the independent C2 transfer");
            const double cubic = progress * progress * (3 - 2 * progress);
            measuredDeficit += weight * (1 - weight);
            expectedDeficit += expected * (1 - expected);
            cubicDeficit += cubic * (1 - cubic);
            previous = weight;
        }
        const auto finished = electry::ElectryEngineTestAccess::state(*engine, 2);
        expect(finished.transitionRemaining == 0 && !finished.locked && !finished.queued,
               "The C2 boundary fade changed completion/ownership semantics");
        // For any fixed complex pair, coherent energy deficit is proportional
        // to w(1-w). This checks actual rendered weights across the entire
        // finite contact against the exact independent shoulder construction.
        // Its continuous integral is 449/3780, versus cubic's 9/70; use the
        // actual finite sum rather than a fitted reduction threshold. This is
        // not a claim of switched-network passivity.
        expect(std::abs(measuredDeficit - expectedDeficit) < length * numericalTolerance
                   && measuredDeficit < cubicDeficit,
               "The actual finite contact differs from the derived lower-deficit construction");
    }
#endif
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
    sameClockLargeReplacementChokesOnlyTheActualFretChange();
    sameClockReturnDoesNotChokeAnUnperformedFret();
    queuedRepickCannotReplaceTheSoundingStrokeBeforeContact();
    newQueuedFretRetainsThePresentHandUntilContact();
    liveExpressionMovesBothHandoffEndpoints();
    queuedFretKeepsItsLengthOutOfTheExpressionFollower();
    clippedExpressionTargetsReturnToTheirOwnFret();
    queuedNewFretCannotApplyTheFutureWristBeforeContact();
    renderedBoundaryWeightsReduceColorWithoutChangingTheClock();
    memberBendCannotRetuneTheReleasedRefret();
    std::cout << std::setprecision(9) << "48 picked refrets at physical Release: max absolute "
              << largestReleaseError << " cents; live energy-pitch span "
              << largestEnergyPitch << " cents; max unexplained residue "
              << largestUnexplainedReleaseError << " cents\n";
    std::cout << assertions << " assertions, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
