// Independent spatial-field oracle for the default-off pickup-time experiment.
// Paiva et al. JAES60(10), Eq.12--15: remove only a COMMON source delay before
// mixing anchors. A softened reflection r=.6 has position-dependent phase;
// neither equal phase nor equal sign is assumed. The ideal r=1 identity is a
// separate witness. The continuous field and finite cubic-interpolation error
// are reported separately from exact discrete production-path agreement.
#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <iomanip>
#include <memory>
#include <limits>

namespace electry
{
struct ElectryEngineTestAccess
{
    using Engine = ElectryEngine;
    using Tap = Engine::DelayTap;
    struct Geometry
    {
        double rate = 0, period = 0;
        double neckLegacy = 0, bridgeLegacy = 0;
        double neckDirect = 0, neckReflected = 0;
        double bridgeDirect = 0, bridgeReflected = 0;
        double coilDelay = 0, coilBalance = 0, window = 0;
        bool commonValid = false;
    };
    static double delay(const Tap& t)
    { return t.offset - (-static_cast<double>(t.c0) + t.c2 + 2. * t.c3); }
    static Geometry geometry(Engine& e, int string, int fret, float bend)
    {
        auto& v = e.voices_[static_cast<std::size_t>(string)];
        const auto& spec = Engine::stringSpecs()[static_cast<std::size_t>(string)];
        v.fret = fret;
        v.baseFrequency = Engine::midiToHz(static_cast<float>(spec.openMidiNote + fret));
        v.pickupWaveSpeedRatio = std::exp2(bend / 12.f);
        e.configureVoicePickups(v);
        Geometry g;
        g.rate = e.sampleRate_;
        g.period = e.sampleRate_ / (v.baseFrequency * v.pickupWaveSpeedRatio);
        g.neckLegacy = delay(v.pickupTapNeck);
        g.bridgeLegacy = delay(v.pickupTapBridge);
#if ELECTRY_COMMON_TIME_PICKUP
        g.commonValid = v.pickupCommonTimeValid;
        g.neckDirect = delay(v.pickupDirectTapNeck);
        g.neckReflected = delay(v.pickupReflectedTapNeck);
        g.bridgeDirect = delay(v.pickupDirectTapBridge);
        g.bridgeReflected = delay(v.pickupReflectedTapBridge);
#endif
        g.coilDelay = v.coilPairNeck.spacingWhole + v.coilPairNeck.spacingFraction;
        g.coilBalance = v.coilPairNeck.balance;
        g.window = v.apertureNeck.windowWhole + v.apertureNeck.windowFraction;
        return g;
    }
    static std::complex<double> readMode(Engine& e, int string,
                                          const Tap& tap, double omega)
    {
        auto& loop = e.voices_[static_cast<std::size_t>(string)].vertical;
        constexpr int mask = Engine::delayLineSize - 1;
        const auto read = [&](bool imaginary)
        {
            for (int offset = -1; offset <= 2; ++offset)
            {
                const int history = tap.offset - offset;
                const double phase = -omega * history;
                loop.line[static_cast<std::size_t>((loop.writeIndex - history) & mask)]
                    = static_cast<float>(imaginary ? std::sin(phase) : std::cos(phase));
            }
            return loop.readTap(tap);
        };
        const double real = read(false), imag = read(true);
        return {real, imag};
    }
    static std::array<std::complex<double>, 2> field(Engine& e, int string,
                                                     double omega, double r)
    {
        auto& v = e.voices_[static_cast<std::size_t>(string)];
#if ELECTRY_COMMON_TIME_PICKUP
        if (v.pickupCommonTimeValid)
            return {readMode(e, string, v.pickupDirectTapNeck, omega)
                      - r * readMode(e, string, v.pickupReflectedTapNeck, omega),
                    readMode(e, string, v.pickupDirectTapBridge, omega)
                      - r * readMode(e, string, v.pickupReflectedTapBridge, omega)};
#endif
        return {1. - r * readMode(e, string, v.pickupTapNeck, omega),
                1. - r * readMode(e, string, v.pickupTapBridge, omega)};
    }
    static std::complex<double> discreteRay(const Tap& tap, double omega)
    {
        std::complex<double> h{};
        const std::array<double, 4> coefficients{tap.c0, tap.c1, tap.c2, tap.c3};
        for (int k = 0; k < 4; ++k)
            h += coefficients[static_cast<std::size_t>(k)]
                * std::polar(1., -omega * (tap.offset + 1 - k));
        return h;
    }
    static std::array<std::complex<double>, 2> productionIdle(
        Engine& e, int string, double omega, const Geometry& g,
        std::array<std::complex<double>, 2>& exactDiscrete)
    {
        auto& v = e.voices_[static_cast<std::size_t>(string)];
        e.resetVoicePickupState(v);
        e.neckPathActive_ = e.bridgePathActive_ = true;
        e.feedbackDrive_ = 0;
        e.sympatheticInjection_ = 1;
        v.sympatheticReady = true;
        v.sympatheticHandLoopGain = 1;
        v.fluxScale = 1;
        v.vertical.loopGain = 0;
        v.vertical.writeIndex = 997;
        constexpr int mask = Engine::delayLineSize - 1;
        for (int history = 0; history < Engine::delayLineSize; ++history)
            v.vertical.line[static_cast<std::size_t>((v.vertical.writeIndex - history) & mask)]
                = static_cast<float>(std::cos(omega * history));
        const auto transfer = [&](bool neck)
        {
            std::complex<double> d{1, 0}, r;
            const Tap& legacy = neck ? v.pickupTapNeck : v.pickupTapBridge;
#if ELECTRY_COMMON_TIME_PICKUP
            if (v.pickupCommonTimeValid)
            {
                d = discreteRay(neck ? v.pickupDirectTapNeck : v.pickupDirectTapBridge, omega);
                r = .6 * discreteRay(neck ? v.pickupReflectedTapNeck : v.pickupReflectedTapBridge, omega);
            }
            else
#endif
                r = .6 * discreteRay(legacy, omega);
            // Independent finite FIR sum for the two coils and rectangular
            // aperture, including fractional interpolation of their memories.
            const int whole = static_cast<int>(g.coilDelay);
            const double fraction = g.coilDelay - whole;
            const auto spacing = (1 - fraction) * std::polar(1., -omega * whole)
                               + fraction * std::polar(1., -omega * (whole + 1));
            auto h = (d - g.coilBalance * r
                    + spacing * (g.coilBalance * d - r)) / (1 + g.coilBalance);
            const int count = static_cast<int>(g.window);
            std::complex<double> window{};
            for (int sample = 0; sample < count; ++sample)
                window += std::polar(1., -omega * sample);
            window += (g.window - count) * std::polar(1., -omega * count);
            return h * window / g.window;
        };
        exactDiscrete = {transfer(true), transfer(false)};
        const int warm = static_cast<int>(std::ceil(g.window + g.coilDelay)) + 8;
        constexpr int count = 256;
        double cc = 0, ss = 0, cs = 0;
        std::array<double, 2> yc{}, ys{};
        for (int sample = 0; sample < warm + count; ++sample)
        {
            Engine::RenderSums sums{};
            e.renderSympatheticString(v, sums, static_cast<float>(std::cos(omega * sample)));
            if (sample >= warm)
            {
                const double c = std::cos(omega * sample), s = std::sin(omega * sample);
                cc += c * c; ss += s * s; cs += c * s;
                const std::array<double, 2> output{v.previousFluxNeck, v.previousFluxBridge};
                for (int pickup = 0; pickup < 2; ++pickup)
                { yc[pickup] += output[pickup] * c; ys[pickup] += output[pickup] * s; }
            }
        }
        const double determinant = cc * ss - cs * cs;
        std::array<std::complex<double>, 2> result;
        for (int pickup = 0; pickup < 2; ++pickup)
            result[pickup] = {(yc[pickup] * ss - ys[pickup] * cs) / determinant,
                             -(ys[pickup] * cc - yc[pickup] * cs) / determinant};
        return result;
    }
    static std::array<std::complex<double>, 2> productionPlayed(
        Engine& e, int string, double omega, const Geometry& g,
        const std::array<std::complex<double>, 2>& expected)
    {
        auto& v = e.voices_[static_cast<std::size_t>(string)];
        e.resetVoicePickupState(v);
        e.neckPathActive_ = e.bridgePathActive_ = true;
        e.artifactsActive_ = false;
        e.bridgeCouplingInjection_ = 0;
        e.feedbackHandScale_ = 1;
        v.active = true;
        v.excitationPhase = Engine::ExcitationPhase::Idle;
        v.noiseRemaining = 0;
        v.slideNoiseLevel = 0;
        v.touchDepth = 0;
        v.fluxScale = 1;
        v.vertical.loopGain = v.horizontal.loopGain = 0;
        v.vertical.writeIndex = v.horizontal.writeIndex = 997;
        constexpr int mask = Engine::delayLineSize - 1;
        constexpr double amplitude = 1e-4;
        for (int history = 0; history < Engine::delayLineSize; ++history)
        {
            const int index = (v.vertical.writeIndex - history) & mask;
            v.vertical.line[static_cast<std::size_t>(index)]
                = static_cast<float>(amplitude * std::cos(omega * history));
            v.horizontal.line[static_cast<std::size_t>(index)]
                = .35f * v.vertical.line[static_cast<std::size_t>(index)];
        }
        const int warm = static_cast<int>(std::ceil(g.window + g.coilDelay)) + 8;
        constexpr int count = 256;
        double cc = 0, ss = 0, cs = 0;
        std::array<double, 2> yc{}, ys{};
        for (int sample = 0; sample < warm + count; ++sample)
        {
            e.feedbackDrive_ = static_cast<float>(amplitude * std::cos(omega * sample));
            Engine::RenderSums sums{};
            e.renderVoice(v, sums);
            if (sample >= warm)
            {
                const double c = std::cos(omega * sample), z = std::sin(omega * sample);
                cc += c * c; ss += z * z; cs += c * z;
                // Two physical polarization weights in the production reader;
                // the tiny fixed drive is a small-signal magnetic witness.
                const std::array<double, 2> output{
                    v.previousFluxNeck / (amplitude * (.85 + .35 * .35)),
                    v.previousFluxBridge / (amplitude * (.85 + .35 * .35))};
                for (int anchor = 0; anchor < 2; ++anchor)
                { yc[anchor] += output[anchor] * c; ys[anchor] += output[anchor] * z; }
            }
        }
        e.feedbackDrive_ = 0;
        const double determinant = cc * ss - cs * cs;
        std::array<std::complex<double>, 2> result;
        for (int anchor = 0; anchor < 2; ++anchor)
            result[anchor] = {(yc[anchor] * ss - ys[anchor] * cs) / determinant,
                              -(ys[anchor] * cc - yc[anchor] * cs) / determinant};
        (void) expected;
        return result;
    }
    static bool refitRetainsHistory(Engine& e, int string)
    {
        auto& v = e.voices_[static_cast<std::size_t>(string)];
        const auto neck = v.apertureNeck, bridge = v.apertureBridge;
        const auto nc = v.coilPairNeck, bc = v.coilPairBridge;
        const float pn = v.previousFluxNeck, pb = v.previousFluxBridge;
        const float en = v.emfLowpassNeck.state, eb = v.emfLowpassBridge.state;
        v.pickupWaveSpeedRatio *= 1.01f;
        e.configureVoicePickups(v);
        return neck.cumulativeHistory == v.apertureNeck.cumulativeHistory
            && neck.cumulative == v.apertureNeck.cumulative
            && neck.writeIndex == v.apertureNeck.writeIndex
            && bridge.cumulativeHistory == v.apertureBridge.cumulativeHistory
            && bridge.cumulative == v.apertureBridge.cumulative
            && bridge.writeIndex == v.apertureBridge.writeIndex
            && nc.history == v.coilPairNeck.history && nc.writeIndex == v.coilPairNeck.writeIndex
            && bc.history == v.coilPairBridge.history && bc.writeIndex == v.coilPairBridge.writeIndex
            && pn == v.previousFluxNeck && pb == v.previousFluxBridge
            && en == v.emfLowpassNeck.state && eb == v.emfLowpassBridge.state;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
constexpr double pi = 3.14159265358979323846;
int assertions = 0, failures = 0;
void expect(bool value, const char* reason)
{
    ++assertions;
    if (!value && ++failures <= 16) std::cerr << reason << '\n';
}
}

int main()
{
    int cases = 0, outsideDomain = 0;
    double worstFieldError = 0, worstMagnitudeDelta = 0, legacyPhaseError = 0;
    double worstDiscretePipelineError = 0, worstPlayedPipelineError = 0;
    for (double hostRate : {16000., 44100., 48000., 88200., 96000., 192000., 384000.})
        for (float scale : {0.f, .85f, 1.f})
            for (float gauge : {0.f, .5f, 1.f})
                for (float type : {0.f, .32f, 1.f})
                {
                    auto e = std::make_unique<Engine>();
                    electry::EngineParameters p;
                    electry::applyGuitarBuild(p, electry::defaultGuitarBuild);
                    p.scaleLength = scale; p.stringGauge = gauge; p.pickupType = type;
                    p.pickupSelector = electry::PickupSelector::Both;
                    e->setParameters(p); e->prepare(hostRate, 64);
                    for (int string : {0, 1, 2, 3, 4, 5, 6, 7})
                        for (int fret : {0, 7, 12, 22, 24})
                            for (float bend : {-72.f, -24.f, -2.f, 0.f, 2.f, 24.f, 72.f, 192.f})
                            {
                                const auto g = Access::geometry(*e, string, fret, bend);
                                ++cases;
                                const double tauN = g.neckLegacy / 2, tauB = g.bridgeLegacy / 2;
                                const double time = std::max(tauN, tauB) + 4;
                                const double rounding = 12 * std::numeric_limits<float>::epsilon()
                                    * std::max(1., time + std::max(tauN, tauB));
#if ELECTRY_COMMON_TIME_PICKUP
                                const bool representable = time + std::max(tauN, tauB) <= 16376. + 2e-6;
                                expect(g.commonValid == representable, "Common-time domain disagrees with existing ring capacity");
                                if (g.commonValid)
                                {
                                    expect(std::abs((g.neckReflected + g.neckDirect) / 2 - time) < rounding
                                           && std::abs((g.bridgeReflected + g.bridgeDirect) / 2 - time) < rounding,
                                           "Anchors do not share one effective field time");
                                    expect(std::abs(g.neckReflected - g.neckDirect - g.neckLegacy) < rounding
                                           && std::abs(g.bridgeReflected - g.bridgeDirect - g.bridgeLegacy) < rounding,
                                           "Common time changed existing guarded spatial geometry");
                                    expect(g.neckDirect >= 4 - rounding && g.bridgeDirect >= 4 - rounding
                                           && g.neckReflected <= 16376. + rounding
                                           && g.bridgeReflected <= 16376. + rounding,
                                           "A common-time ray escaped the causal cubic ring domain");
                                }
                                else ++outsideDomain;
#endif
                                for (int mode : {1, 3, 9})
                                {
                                    const double omega = std::min(.3, 2 * pi * mode / std::max(g.period, 1.));
                                    for (double reflection : {.6, 1.})
                                    {
                                        const auto field = Access::field(*e, string, omega, reflection);
                                        const auto ideal = [&](double tau)
                                        { return std::polar(1., omega * tau) - reflection * std::polar(1., -omega * tau); };
                                        const auto reference = [&](double tau)
                                        {
#if ELECTRY_COMMON_TIME_PICKUP
                                            if (g.commonValid) return std::polar(1., -omega * time) * ideal(tau);
#endif
                                            return 1. - reflection * std::polar(1., -2 * omega * tau);
                                        };
                                        // Cubic Lagrange remainder <=9*w^4/384 per
                                        // ray; add explicit float-coordinate/coeff roundoff.
                                        const double bound = (1 + reflection) *
                                            (9 * std::pow(omega, 4) / 384 + omega * rounding + 3e-6);
                                        const double error = std::max(std::abs(field[0] - reference(tauN)),
                                                                      std::abs(field[1] - reference(tauB)));
                                        worstFieldError = std::max(worstFieldError, error);
                                        expect(error <= bound, "Production spatial read disagrees with independent modal field");
                                        expect(std::abs(field[0] + field[1] - reference(tauN) - reference(tauB)) <= 2 * bound,
                                               "Two-anchor sum lost the common physical phase");
                                        if (reflection == 1)
                                        {
                                            const auto identity = 4. * std::complex<double>(0, 1)
                                                * std::sin(omega * (tauN + tauB) / 2)
                                                * std::cos(omega * (tauN - tauB) / 2);
                                            expect(std::abs(ideal(tauN) + ideal(tauB) - identity) < 1e-8,
                                                   "Independent ideal standing-wave sum identity failed");
                                        }
                                        if (reflection == .6)
                                        {
                                            const double delta = std::max(std::abs(std::abs(field[0]) - std::abs(ideal(tauN))),
                                                                          std::abs(std::abs(field[1]) - std::abs(ideal(tauB))));
                                            worstMagnitudeDelta = std::max(worstMagnitudeDelta, delta);
                                            legacyPhaseError = std::max(legacyPhaseError,
                                                std::abs(1. - reflection * std::polar(1., -2 * omega * tauN)
                                                    + 1. - reflection * std::polar(1., -2 * omega * tauB)
                                                    - ideal(tauN) - ideal(tauB)));
                                        }
                                    }
                                }
                            }
                    // Exercise the actual sympathetic renderer, two-coil and
                    // aperture memories on a subset spanning all host rates,
                    // material endpoints, scale lengths and pickup anchors.
                    const auto g = Access::geometry(*e, 2, 12, 2.f);
                    const double omega = std::min(.15, 2 * pi * 3 / g.period);
                    std::array<std::complex<double>, 2> expected;
                    const auto observed = Access::productionIdle(*e, 2, omega, g, expected);
                    for (int anchor = 0; anchor < 2; ++anchor)
                    {
                        const double error = std::abs(observed[anchor] - expected[anchor]);
                        worstDiscretePipelineError = std::max(worstDiscretePipelineError, error);
                        expect(error < 1e-5, "Actual idle pickup path differs from independent complete discrete transfer");
                    }
                    const auto played = Access::productionPlayed(*e, 2, omega, g, expected);
                    for (int anchor = 0; anchor < 2; ++anchor)
                    {
                        const double error = std::abs(played[anchor] - expected[anchor]);
                        worstPlayedPipelineError = std::max(worstPlayedPipelineError, error);
                        expect(error < 1e-4, "Actual played pickup path differs from independent small-signal discrete transfer");
                    }
                    expect(Access::refitRetainsHistory(*e, 2), "Pickup time refit erased populated observation histories");
                }
    expect(legacyPhaseError > .5, "The legacy-phase negative control did not expose the missing anchor phase");
#if ELECTRY_COMMON_TIME_PICKUP
    expect(outsideDomain > 0, "The extreme-expression domain witness was not exercised");
#endif
    std::cout << std::setprecision(12) << assertions << " assertions, " << failures << " failures; "
              << cases << " geometry cases, " << outsideDomain << " explicit legacy fallbacks; "
              << "max field interpolation error " << worstFieldError
              << ", max single-anchor magnitude delta " << worstMagnitudeDelta
              << ", max discrete pipeline error " << worstDiscretePipelineError
              << ", max played pipeline error " << worstPlayedPipelineError
              << ", legacy missing-phase negative control " << legacyPhaseError << '\n';
    return failures == 0 ? 0 : 1;
}
