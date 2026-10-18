#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

// This suite observes the real engine rather than calling its mass helper.
// The five SI fixtures independently convert the frozen 2026-10-17 D'Addario
// String Tension Pro U1AGFNP024/032/042/060/080 Mass_UL records from lb/in.
// Source: https://embed.cartfulsolutions.com/daddario-string-tension-finder/data/strings_sample.json
// SHA256: 855d638563bb07bcb156bb359a75ff3fdaf83a15d27cfa840b7950c675c10ee3.
// Only gauge=0 identifies those actual constructions. Gauge .5/1 checks the
// candidate's explicitly synthetic diameter-squared similarity hypothesis.
namespace electry
{
struct ElectryEngineTestAccess
{
    struct State
    {
        double tension, impedance, frequency, internalRate, releaseSeconds;
        double pendingSeed, tensionRatio, frequencyFactor, handRetention;
        bool releasing, active;
    };

    static State state(const ElectryEngine& e, int stringIndex)
    {
        const auto& v = e.voices_[static_cast<std::size_t>(stringIndex)];
        State result {};
#if ELECTRY_ANALYTIC_RELEASE_IC || ELECTRY_ENERGY_ATTACK_PITCH
        result.tension = v.stringTensionNewtons;
#endif
        result.impedance = v.stringWaveImpedance;
        result.frequency = v.baseFrequency;
        result.internalRate = e.sampleRate_;
        result.releaseSeconds = v.releaseGainCoefficient > 0.0f
            ? -1.0 / (e.sampleRate_ * std::log1p(-double(v.releaseGainCoefficient)))
            : 0.0;
#if ELECTRY_ENERGY_ATTACK_PITCH
        result.pendingSeed = double(v.pendingAttackPitchEnergyJoules)
                           * v.pendingAttackPitchElasticScalePerJoule;
        result.tensionRatio = v.attackPitchTensionRatio;
        result.frequencyFactor = v.attackPitchFrequencyFactor;
        result.handRetention = v.attackPitchHandRetention;
#else
        result.frequencyFactor = result.handRetention = 1.0;
#endif
        result.releasing = v.releasing;
        result.active = v.active;
        return result;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
constexpr std::array<int, 8> openNotes { 28, 35, 40, 45, 50, 55, 59, 64 };
constexpr std::array<double, 8> proxyMass {
    .021638436187379540, .012171620355400993, .005964093974146487,
    .003462149789980727, .001947459256864159, .0010182793499943314,
    .00048129609902075805, .00032218995058414386
};
[[maybe_unused]] constexpr std::array<double, 5> referenceMass {
    .019742987485561633, .011454724543980310, .005608351798152271,
    .0033967559895793364, .0019334624432032757
};
constexpr std::array<double, 5> hostRates { 44100, 48000, 96000, 192000, 384000 };
constexpr double scaleMetres = .6477; // fixed 25.5-inch test instrument
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool relativeNear(double actual, double expected, double tolerance = 3.0e-6)
{
    return std::isfinite(actual)
        && std::abs(actual - expected) <= tolerance * std::abs(expected);
}

double expectedMass(int stringIndex, float gauge)
{
    const auto index = static_cast<std::size_t>(stringIndex);
    double mass = proxyMass[index];
#if ELECTRY_REFERENCE_STRING_MASS
    if (stringIndex < 5)
        mass = referenceMass[index];
#endif
    const double diameterRatio = 1.0 + double(gauge) * (2.0 / 9.0);
    return mass * diameterRatio * diameterRatio;
}

std::unique_ptr<Engine> makeEngine(double rate, int stringIndex, float gauge)
{
    auto e = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.scaleLength = 0.0f;
    p.stringGauge = gauge;
    p.pickNoise = p.fingerNoise = p.releaseNoise = p.artifactAmount = 0.0f;
    p.sympatheticAmount = p.bodyResonance = p.palmMute = 0.0f;
    p.outputGain = 0.5f;
    e->setParameters(p);
    e->prepare(rate, 256);
    e->setVariationSeed(0);
    e->setSoloStringMask(static_cast<std::uint8_t>(1u << stringIndex));
    e->noteOn(Engine::firstKeyswitchNote + static_cast<int>(electry::PickStyle::Down), 1);
    return e;
}

void physicalFixtures()
{
    double worstTensionError = 0.0, worstImpedanceError = 0.0;
    int cells = 0;
    for (double rate : hostRates)
        for (float gauge : { 0.0f, 0.5f, 1.0f })
            for (int stringIndex = 0; stringIndex < 8; ++stringIndex)
                for (int fret : { 0, 12 })
                {
                    auto e = makeEngine(rate, stringIndex, gauge);
                    const int open = openNotes[static_cast<std::size_t>(stringIndex)];
                    e->noteOn(open + fret, 0.8f);
                    const auto observed = Access::state(*e, stringIndex);
                    const double openHz = 440.0 * std::exp2((open - 69) / 12.0);
                    const double speed = 2.0 * scaleMetres * openHz;
                    const double mass = expectedMass(stringIndex, gauge);
                    const double tension = mass * speed * speed;
                    const double impedance = mass * speed;
                    const std::string cell = "rate=" + std::to_string(rate)
                        + " string=" + std::to_string(stringIndex)
                        + " gauge=" + std::to_string(gauge)
                        + " fret=" + std::to_string(fret);
#if ELECTRY_ANALYTIC_RELEASE_IC || ELECTRY_ENERGY_ATTACK_PITCH
                    expect(relativeNear(observed.tension, tension),
                           "live tension matches independent SI fixture " + cell);
                    worstTensionError = std::max(worstTensionError,
                        std::abs(observed.tension / tension - 1.0));
#endif
                    expect(relativeNear(observed.impedance, impedance),
                           "live impedance matches independent SI fixture " + cell);
                    worstImpedanceError = std::max(worstImpedanceError,
                        std::abs(observed.impedance / impedance - 1.0));
                    expect(relativeNear(observed.frequency, openHz * std::exp2(fret / 12.0)),
                           "construction changes preserve nominal note target " + cell);
                    ++cells;
                }
    std::cout << "MASS_FIXTURES candidate=" << ELECTRY_REFERENCE_STRING_MASS
              << " cells=" << cells << " worst_relative_T=" << worstTensionError
              << " worst_relative_Z=" << worstImpedanceError
              << " plain_controls=3 gauge_similarity=hypothesis\n";
}

struct RenderObservation
{
    double peak { 0.0 }, energy { 0.0 }, maximumRatio { 0.0 };
};

RenderObservation render(Engine& e, int stringIndex, double rate, double seconds)
{
    RenderObservation result;
    // Short unequal blocks sample the actual evolving state and exercise an
    // ordinary host partition without allocating inside the render loop.
    std::array<float, 31> left {}, right {};
    const int total = static_cast<int>(std::lround(rate * seconds));
    for (int at = 0; at < total; at += 31)
    {
        const int count = std::min(31, total - at);
        e.process(left.data(), right.data(), count);
        for (int i = 0; i < count; ++i)
        {
            expect(std::isfinite(left[static_cast<std::size_t>(i)])
                       && std::isfinite(right[static_cast<std::size_t>(i)]),
                   "finite attack, pressure and release audio");
            result.peak = std::max(result.peak,
                std::max(std::abs(double(left[static_cast<std::size_t>(i)])),
                         std::abs(double(right[static_cast<std::size_t>(i)]))));
            result.energy += double(left[static_cast<std::size_t>(i)])
                           * left[static_cast<std::size_t>(i)];
        }
        const auto observed = Access::state(e, stringIndex);
        result.maximumRatio = std::max(result.maximumRatio, observed.tensionRatio);
        expect(std::isfinite(observed.frequency) && observed.frequency > 0.0
                   && std::isfinite(observed.frequencyFactor)
                   && observed.frequencyFactor >= 1.0,
               "finite positive frequency while attack and hand state evolve");
        expect(observed.tensionRatio >= 0.0
                   && observed.frequencyFactor <= std::exp2(6.0 / 1200.0) + 2.0e-6,
               "attack pitch remains inside its existing six-cent bound");
    }
    expect(result.peak < 4.0, "bounded physical render");
    return result;
}

void attackAndStopObservations()
{
    const double sixCentRatio = std::expm1(std::log(2.0) * 6.0 / 600.0);
    double maximumSeed = 0.0, maximumCommitted = 0.0, maximumPeak = 0.0;
    int capCandidates = 0, contacts = 0;
    for (double rate : hostRates)
    {
        double rateMaximumSeed = 0.0, rateMaximumCommitted = 0.0;
        int rateCapCandidates = 0, rateContacts = 0;
        for (float gauge : { 0.0f, 0.5f, 1.0f })
            for (int stringIndex = 0; stringIndex < 8; ++stringIndex)
                for (float velocity : { 0.35f, 1.0f })
                {
                    auto e = makeEngine(rate, stringIndex, gauge);
                    const int note = openNotes[static_cast<std::size_t>(stringIndex)] + 5;
                    e->noteOn(note, velocity);
                    const auto initial = Access::state(*e, stringIndex);
                    expect(std::isfinite(initial.pendingSeed) && initial.pendingSeed >= 0.0,
                           "finite nonnegative pending attack energy product");
                    rateMaximumSeed = std::max(rateMaximumSeed, initial.pendingSeed);
                    rateCapCandidates += initial.pendingSeed >= sixCentRatio ? 1 : 0;
                    ++rateContacts;
                    const auto attack = render(*e, stringIndex, rate, .018);
                    e->noteOff(note);
                    const auto stopped = Access::state(*e, stringIndex);
                    expect(stopped.releasing, "note-off owns the sounding string release");
                    // An unmuted fret-5 note is stopped by the fretting finger.
                    // Its existing ten-ms closure is independent of string mass.
                    expect(std::abs(stopped.releaseSeconds - .010) < 2.0e-5,
                           "mass does not change the fretting-hand stop time");
                    const auto tail = render(*e, stringIndex, rate, .022);
                    rateMaximumCommitted = std::max(rateMaximumCommitted,
                        std::max(attack.maximumRatio, tail.maximumRatio));
                    maximumPeak = std::max(maximumPeak, std::max(attack.peak, tail.peak));
                }
        std::cout << "ATTACK_OBSERVATION host_rate=" << rate
                  << " contacts=" << rateContacts
                  << " seed_ge_six_cent_cap=" << rateCapCandidates
                  << " maximum_unclamped_seed=" << rateMaximumSeed
                  << " maximum_observed_q=" << rateMaximumCommitted << '\n';
        maximumSeed = std::max(maximumSeed, rateMaximumSeed);
        maximumCommitted = std::max(maximumCommitted, rateMaximumCommitted);
        capCandidates += rateCapCandidates;
        contacts += rateContacts;
    }

    double minimumPressedClosure = 1.0, maximumPressedClosure = 0.0;
    for (double rate : hostRates)
        for (float gauge : { 0.0f, 0.5f, 1.0f })
        {
            auto free = makeEngine(rate, 0, gauge), pressed = makeEngine(rate, 0, gauge);
            free->noteOn(33, 1.0f); pressed->noteOn(33, 1.0f);
            render(*free, 0, rate, .018); render(*pressed, 0, rate, .018);
            pressed->setPalmMutePressure(1.0f);
            render(*free, 0, rate, .028); render(*pressed, 0, rate, .028);
            const auto freeHeld = Access::state(*free, 0);
            const auto pressedHeld = Access::state(*pressed, 0);
            expect(pressedHeld.tensionRatio <= freeHeld.tensionRatio + 1.0e-8,
                   "planted hand does not add attack-tension energy");
            free->noteOff(33); pressed->noteOff(33);
            const auto freeStop = Access::state(*free, 0);
            const auto pressedStop = Access::state(*pressed, 0);
            expect(pressedStop.releaseSeconds > freeStop.releaseSeconds
                       && pressedStop.releaseSeconds <= .0221,
                   "registered pressure selects the bounded broad-hand stop closure");
            minimumPressedClosure = std::min(minimumPressedClosure, pressedStop.releaseSeconds);
            maximumPressedClosure = std::max(maximumPressedClosure, pressedStop.releaseSeconds);
            render(*free, 0, rate, .022); render(*pressed, 0, rate, .022);
        }
    // These are observations, not fitted expectations for how often a guitar
    // ought to reach the voiced cap. Compare OFF/ON before any promotion.
    std::cout << "MASS_ATTACK_TOTAL contacts=" << contacts
              << " seed_ge_six_cent_cap=" << capCandidates
              << " maximum_unclamped_seed=" << maximumSeed
              << " maximum_observed_q=" << maximumCommitted
              << " maximum_audio_peak=" << maximumPeak
              << " pressed_stop_seconds=" << minimumPressedClosure
              << ':' << maximumPressedClosure << '\n';
}
}

int main()
{
    std::cout << std::setprecision(10);
    physicalFixtures();
    attackAndStopObservations();
    if (failures != 0)
        std::cerr << failures << " reference string mass assertion(s) failed\n";
    return failures == 0 ? 0 : 1;
}
