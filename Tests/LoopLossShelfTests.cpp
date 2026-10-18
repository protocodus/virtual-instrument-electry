// The candidate keeps each pre-existing feasible traversal anchor. Compare
// against the independent complex response of that original one-pole/scalar,
// not against a duplicate of the shelf coefficient formula. A physical T60
// additionally depends on the complete dispersive loop's group delay.
#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <memory>

namespace electry
{
struct ElectryEngineTestAccess
{
    struct Result
    {
        std::array<double, 5> coefficients;
        double gain;
        double pole, b3, depth;
        bool active;
        bool historiesRetained;
        bool invalidCorrectionCleared;
    };
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
    static Result transform(float coefficient, float gain, float omega0, float high,
                            float totalPole = -1, float totalGain = -1)
    {
        auto loop = std::make_unique<ElectryEngine::PolarisationLoop>();
        loop->loopDampingCoefficient = totalPole < 0 ? coefficient : totalPole;
        loop->loopGain = totalGain < 0 ? gain : totalGain;
        loop->damping.state = .03125f;
        loop->lossShelf.z1 = .000125;
        loop->lossShelf.z2 = -.0000625;
        loop->lossShelfZ3 = .00003125;
        ElectryEngine::configureLoopLossShelf(*loop, omega0, high, coefficient, gain);
        const auto &b = loop->lossShelf;
        return {{b.b0, b.b1, b.b2, b.a1, b.a2},
                loop->loopGain,
                loop->loopDampingCoefficient,
                loop->lossShelfB3,
                loop->lossShelfDepth,
                loop->lossShelfActive,
                b.z1 == .000125 && b.z2 == -.0000625 &&
                    loop->lossShelfZ3 == .00003125 && loop->damping.state == .03125f,
                b.z1 == 0 && b.z2 == 0 && loop->lossShelfZ3 == 0 &&
                    loop->damping.state == .03125f};
    }
    static bool invalidDomainReturn()
    {
        auto loop = std::make_unique<ElectryEngine::PolarisationLoop>();
        loop->loopDampingCoefficient = .3f;
        loop->loopGain = .9999f;
        loop->damping.state = .03125f;
        ElectryEngine::configureLoopLossShelf(*loop, .001f, 1.f, .3f, .9999f);
        if (!loop->lossShelfActive)
            return false;
        loop->lossShelf.z1 = .000125;
        loop->lossShelf.z2 = -.0000625;
        loop->lossShelfZ3 = .00003125;
        ElectryEngine::configureLoopLossShelf(*loop, .001f, 4.f, .3f, .9999f);
        if (loop->lossShelfActive || loop->lossShelf.z1 != 0 ||
            loop->lossShelf.z2 != 0 || loop->lossShelfZ3 != 0 ||
            loop->damping.state != .03125f)
            return false;
        ElectryEngine::configureLoopLossShelf(*loop, .001f, 1.f, .3f, .9999f);
        return loop->lossShelfActive && loop->lossShelf.z1 == 0 &&
               loop->lossShelf.z2 == 0 && loop->lossShelfZ3 == 0 &&
               loop->damping.state == .03125f;
    }
    static double largestPressureStep(int resolution, int stringIndex, int fret,
                                      PlayStyle style, float age, float gauge)
    {
        auto e = std::make_unique<ElectryEngine>();
        EngineParameters p;
        applyGuitarBuild(p, defaultGuitarBuild);
        p.strumSpreadSeconds = 0;
        p.stringAge = age;
        p.stringGauge = gauge;
        p.bodyResonance = p.sympatheticAmount = p.artifactAmount = 0;
        e->setParameters(p);
        e->prepare(384000, 64);
        e->setSoloStringMask(1u << stringIndex);
        e->noteOn(ElectryEngine::firstPlayStyleKeyswitchNote + static_cast<int>(style),
                  1);
        e->noteOn(e->stringSpecs()[stringIndex].openMidiNote + fret, .85f);
        auto &v = e->voices_[stringIndex];
        v.attackPitchTensionRatio = 0;
        e->configureVoicePitch(v, true);
        std::array<std::complex<double>, 32> previous{}, previousLegacy{};
        double largest = 0, legacyAtLargest = 0, priorDepth = 1, depthBefore = 1,
               depthAfter = 1;
        int stepAtLargest = 0;
        for (int step = 0; step <= resolution; ++step)
        {
            v.livePalmPressure = static_cast<float>(step) / resolution;
            e->configureVoiceDamping(v, style, v.lastConfiguredFrequency, float(fret));
            const auto &l = v.vertical;
            const auto &b = l.lossShelf;
            for (int bin = 0; bin < 32; ++bin)
            {
                const double hz = 20 * std::pow(1000., bin / 31.);
                const auto z =
                    std::polar(1., -6.2831853071795864769 * hz / e->sampleRate_);
                const double a = l.loopDampingCoefficient;
                auto legacy = double(l.loopGain) * (1 - a) / (1. - a * z);
                if (l.handDipActive)
                {
                    const auto &d = l.handDip;
                    legacy *= (d.b0 + d.b1 * z + d.b2 * z * z) /
                              (1. + d.a1 * z + d.a2 * z * z);
                }
                std::complex<double> h = 1;
                if (l.lossShelfActive)
                    h = (b.b0 + b.b1 * z + b.b2 * z * z + l.lossShelfB3 * z * z * z) /
                        (1. + b.a1 * z + b.a2 * z * z);
                if (step && std::abs(h - previous[bin]) > largest)
                {
                    largest = std::abs(h - previous[bin]);
                    legacyAtLargest = std::abs(legacy - previousLegacy[bin]);
                    stepAtLargest = step;
                    depthBefore = priorDepth;
                    depthAfter = l.lossShelfDepth;
                }
                previous[bin] = h;
                previousLegacy[bin] = legacy;
            }
            priorDepth = l.lossShelfDepth;
        }
        std::cout << "DETAIL pressure=" << double(stepAtLargest) / resolution
                  << " depth=" << depthBefore << "->" << depthAfter
                  << " originaldelta=" << legacyAtLargest << '\n';
        return largest;
    }
    struct Relaxation
    {
        bool established = false, relaxed = false, correctionRetained = false;
        double maximumRelaxedRatio = 0, maximumBypassedRatio = 0;
    };
    static Relaxation laterHandRelaxation(double rate)
    {
        auto e = std::make_unique<ElectryEngine>();
        EngineParameters p;
        applyGuitarBuild(p, defaultGuitarBuild);
        p.stringAge = 0;
        p.stringGauge = 1;
        p.palmMute = .5f;
        p.strumSpreadSeconds = 0;
        p.bodyResonance = p.sympatheticAmount = p.artifactAmount = 0;
        e->setParameters(p);
        e->prepare(rate, 64);
        e->setSoloStringMask(1u << 2);
        e->noteOn(ElectryEngine::firstPlayStyleKeyswitchNote + int(PlayStyle::Dead), 1);
        e->noteOn(e->stringSpecs()[2].openMidiNote + 22, .85f);
        auto &v = e->voices_[2];
        v.attackPitchTensionRatio = 0;
        e->configureVoicePitch(v, true);
        auto &l = v.vertical;
        const auto before = l.lossShelf;
        const double beforeB3 = l.lossShelfB3;
        const float beforeCorrectionDepth = l.lossShelfDepth;
        const float solvedHandDepth = l.handLossSolvedDepth;
        Relaxation result;
        result.established =
            l.lossShelfActive && l.handDipActive && solvedHandDepth > 0;
        // Exercise the actual envelope path without requesting new parameters
        // or another damping fit. It deliberately weakens the solved hand dip.
        v.palmDampingPending = false;
        v.palmDampingCountdown = 100;
        l.handEnvelopePeak = 1;
        l.handEnvelope = 0;
        e->updateVoiceControl(v);
        result.relaxed = l.handLossDepth == .4f * solvedHandDepth;
        result.correctionRetained =
            l.lossShelf.b0 == before.b0 && l.lossShelf.b1 == before.b1 &&
            l.lossShelf.b2 == before.b2 && l.lossShelf.a1 == before.a1 &&
            l.lossShelf.a2 == before.a2 && l.lossShelfB3 == beforeB3 &&
            l.lossShelfDepth == beforeCorrectionDepth;
        const auto maximumRatio = [&]()
        {
            double maximum = 0;
            for (int bin = 0; bin <= 2048; ++bin)
            {
                const auto z = std::polar(1., -3.14159265358979323846 * bin / 2048);
                const auto &b = l.lossShelf;
                const double a = l.loopDampingCoefficient;
                auto h = (1 - a) / (1. - a * z) *
                         (b.b0 + b.b1 * z + b.b2 * z * z + l.lossShelfB3 * z * z * z) /
                         (1. + b.a1 * z + b.a2 * z * z);
                if (l.handDipActive)
                {
                    const auto &hand = l.handDip;
                    h *= (hand.b0 + hand.b1 * z + hand.b2 * z * z) /
                         (1. + hand.a1 * z + hand.a2 * z * z);
                }
                maximum = std::max(maximum, std::abs(h));
            }
            return maximum;
        };
        result.maximumRelaxedRatio = maximumRatio();
        // A bypass is a passive future hand configuration too. Prove the
        // correction's bound does not depend on any amount of hand attenuation.
        e->applyDipDepth(l, 0);
        result.maximumBypassedRatio = maximumRatio();
        return result;
    }
    static Relaxation syntheticHandRelaxation()
    {
        // Filter-domain negative control from the independent review. This is
        // not a claim of instability in the observed public parameter grid.
        // Hand-assisted admission used the full dip to authorize a correction
        // that exceeded unity after the ordinary 0.4 relaxation.
        auto l = std::make_unique<ElectryEngine::PolarisationLoop>();
        l->loopDampingCoefficient = .01f;
        l->loopGain = .9999f;
        l->handLossShape = {.08f, .7f, 12.f};
        ElectryEngine::applyDipDepth(*l, 1);
        ElectryEngine::configureLoopLossShelf(*l, .001f, 1.f, .6f, .9999f);
        const auto b = l->lossShelf;
        const double b3 = l->lossShelfB3;
        const float correctionDepth = l->lossShelfDepth;
        Relaxation result;
        result.established = l->lossShelfActive && l->handDipActive;
        const auto maximumRatio = [&]()
        {
            double maximum = 0;
            for (int bin = 0; bin <= 2048; ++bin)
            {
                const auto z = std::polar(1., -3.14159265358979323846 * bin / 2048);
                const double a = l->loopDampingCoefficient;
                auto h = (1 - a) / (1. - a * z) *
                         (b.b0 + b.b1 * z + b.b2 * z * z + b3 * z * z * z) /
                         (1. + b.a1 * z + b.a2 * z * z);
                if (l->handDipActive)
                {
                    const auto &hand = l->handDip;
                    h *= (hand.b0 + hand.b1 * z + hand.b2 * z * z) /
                         (1. + hand.a1 * z + hand.a2 * z * z);
                }
                maximum = std::max(maximum, std::abs(h));
            }
            return maximum;
        };
        ElectryEngine::applyDipDepth(*l, .4f);
        result.relaxed = l->handLossDepth == .4f;
        result.maximumRelaxedRatio = maximumRatio();
        ElectryEngine::applyDipDepth(*l, 0);
        result.maximumBypassedRatio = maximumRatio();
        result.correctionRetained =
            l->lossShelf.b0 == b.b0 && l->lossShelf.b1 == b.b1 &&
            l->lossShelf.b2 == b.b2 && l->lossShelf.a1 == b.a1 &&
            l->lossShelf.a2 == b.a2 && l->lossShelfB3 == b3 &&
            l->lossShelfDepth == correctionDepth;
        return result;
    }
    struct WideBend
    {
        int configured = 0, invalidActive = 0, unstable = 0;
        bool finite = true;
    };
    static WideBend wideSympatheticBends(double rate)
    {
        auto e = std::make_unique<ElectryEngine>();
        EngineParameters p;
        applyGuitarBuild(p, defaultGuitarBuild);
        p.sympatheticAmount = 1;
        p.bodyResonance = p.artifactAmount = p.pickNoise = p.fingerNoise =
            p.releaseNoise = 0;
        p.strumSpreadSeconds = 0;
        e->setParameters(p);
        e->prepare(rate, 64);
        WideBend result;
        std::array<float, 64> left{}, right{};
        const auto renderAndInspect = [&](bool fixedBend)
        {
            const int frames = static_cast<int>(.08 * rate);
            for (int frame = 0; frame < frames; frame += 64)
            {
                const int count = std::min(64, frames - frame);
                e->process(left.data(), right.data(), count);
                for (int i = 0; i < count; ++i)
                    result.finite = result.finite && std::isfinite(left[i]) &&
                                    std::isfinite(right[i]);
                for (int stringIndex = 2; stringIndex < 8; ++stringIndex)
                {
                    const auto &v = e->voices_[stringIndex];
                    if (!v.sympatheticReady)
                        continue;
                    ++result.configured;
                    const auto &l = v.vertical;
                    const auto &b = l.lossShelf;
                    result.finite = result.finite && std::isfinite(b.z1) &&
                                    std::isfinite(b.z2) && std::isfinite(l.lossShelfZ3);
                    const double omega0 = 6.2831853071795864769 *
                                          v.lastConfiguredFrequency / e->sampleRate_;
                    const double high = std::max(std::min(3600., .32 * e->sampleRate_),
                                                 1.5 * v.lastConfiguredFrequency);
                    const double omegaHigh =
                        6.2831853071795864769 * high / e->sampleRate_;
                    if (l.lossShelfActive)
                    {
                        // AP fit metadata is quantized during a moving wheel.
                        // Check input domain at a reset, fixed-bend solve and
                        // actual denominator stability throughout the return.
                        if (fixedBend && !(omega0 > 0 && omega0 < omegaHigh &&
                                           omegaHigh < 3.14159265358979323846))
                            ++result.invalidActive;
                        const auto discriminant =
                            std::sqrt(std::complex<double>(b.a1 * b.a1 - 4 * b.a2, 0));
                        if (!(std::abs((-b.a1 + discriminant) / 2.) < 1 &&
                              std::abs((-b.a1 - discriminant) / 2.) < 1))
                            ++result.unstable;
                    }
                }
            }
        };
        for (float bend : {84.f, 96.f, 120.f, 144.f, 168.f, 192.f})
        {
            e->setPitchBendSemitones(bend);
            e->reset(); // Public reset snaps the already-authorized global bend.
            e->setSoloStringMask(1);
            // An unbent member voice audibly drives idle strings whose legacy
            // global wheel is much wider. No private frequency injection.
            e->noteOn(28, .85f, 7);
            renderAndInspect(true);
            e->setPitchBendSemitones(0);
            renderAndInspect(false); // Return from fallback with running histories.
        }
        e->reset();
        for (const auto &v : e->voices_)
            result.finite = result.finite && v.vertical.lossShelf.z1 == 0 &&
                            v.vertical.lossShelf.z2 == 0 && v.vertical.lossShelfZ3 == 0;
        return result;
    }
    struct Grid
    {
        int configurations = 0, active = 0, fallbacks = 0, violations = 0,
            historyChanges = 0;
        int preDipViolations = 0;
        double maximumPreDipRatio = 0;
        double maximum = 0, minimumDepth = 1, maximumFundamentalDecayChange = 0,
               maximumUpperDecayChange = 0;
        int limited = 0;
        int worstString = -1, worstFret = -1, worstStyle = -1;
        float worstAge = 0, worstGauge = 0, worstPressure = 0;
        double worstRate = 0;
        std::array<int, 21> fallbackByStylePressure{};
    };
    static Grid configurationGrid()
    {
        Grid result;
        auto e = std::make_unique<ElectryEngine>();
        for (double rate : {48000., 96000., 384000.})
            for (float age : {0.f, .3f, 1.f})
                for (float gauge : {0.f, 1.f})
                    for (float pressure : {0.f, .5f, 1.f})
                        for (auto style :
                             {PlayStyle::Sustain, PlayStyle::PalmMute, PlayStyle::Dead})
                        {
                            EngineParameters p;
                            applyGuitarBuild(p, defaultGuitarBuild);
                            p.stringAge = age;
                            p.stringGauge = gauge;
                            p.palmMute = pressure;
                            p.strumSpreadSeconds = 0;
                            p.bodyResonance = p.sympatheticAmount = p.artifactAmount =
                                0;
                            e->setParameters(p);
                            e->prepare(rate, 64);
                            for (int stringIndex = 0; stringIndex < 8; ++stringIndex)
                                for (int fret : {0, 12, 22})
                                {
                                    e->reset();
                                    e->setSoloStringMask(1u << stringIndex);
                                    e->noteOn(
                                        ElectryEngine::firstPlayStyleKeyswitchNote +
                                            static_cast<int>(style),
                                        1);
                                    e->noteOn(
                                        e->stringSpecs()[stringIndex].openMidiNote +
                                            fret,
                                        .85f);
                                    auto &v = e->voices_[stringIndex];
                                    v.attackPitchTensionRatio = 0;
                                    e->configureVoicePitch(v, true);
                                    for (auto *loop : {&v.vertical, &v.horizontal})
                                    {
                                        ++result.configurations;
                                        if (loop->lossShelfActive)
                                        {
                                            ++result.active;
                                            if (loop->lossShelfDepth <
                                                result.minimumDepth)
                                            {
                                                result.minimumDepth =
                                                    loop->lossShelfDepth;
                                                result.worstString = stringIndex;
                                                result.worstFret = fret;
                                                result.worstStyle = int(style);
                                                result.worstAge = age;
                                                result.worstGauge = gauge;
                                                result.worstPressure = pressure;
                                                result.worstRate = rate;
                                            }
                                            if (loop->lossShelfDepth < 1)
                                                ++result.limited;
                                        }
                                        else if (stringIndex >= 2 &&
                                                 loop->loopDampingCoefficient > 0)
                                        {
                                            ++result.fallbacks;
                                            ++result.fallbackByStylePressure
                                                  [3 * static_cast<int>(style) +
                                                   static_cast<int>(pressure * 2)];
                                        }
                                        const auto section = [](const auto &b,
                                                                std::complex<double> z,
                                                                double b3 = 0)
                                        {
                                            return (b.b0 + b.b1 * z + b.b2 * z * z +
                                                    b3 * z * z * z) /
                                                   (1.0 + b.a1 * z + b.a2 * z * z);
                                        };
                                        if (loop->lossShelfActive)
                                            for (int anchor = 0; anchor < 2; ++anchor)
                                            {
                                                const double f0 =
                                                    v.lastConfiguredFrequency;
                                                const double high =
                                                    style == PlayStyle::Dead
                                                        ? std::min(8 * f0,
                                                                   .32 * e->sampleRate_)
                                                        : std::min(3600.,
                                                                   .32 *
                                                                       e->sampleRate_);
                                                const double hz =
                                                    anchor ? std::max(high, 1.5 * f0)
                                                           : f0;
                                                const auto z = std::polar(
                                                    1., -6.2831853071795864769 * hz /
                                                            e->sampleRate_);
                                                const double a =
                                                    loop->loopDampingCoefficient;
                                                auto old = double(loop->loopGain) *
                                                           (1 - a) / (1. - a * z);
                                                if (loop->handDipActive)
                                                    old *= section(loop->handDip, z);
                                                const auto &b = loop->lossShelf;
                                                const auto correction =
                                                    (b.b0 + b.b1 * z + b.b2 * z * z +
                                                     loop->lossShelfB3 * z * z * z) /
                                                    (1. + b.a1 * z + b.a2 * z * z);
                                                const double change = std::abs(
                                                    std::log(std::abs(old)) /
                                                        std::log(std::abs(old *
                                                                          correction)) -
                                                    1);
                                                auto &maximum =
                                                    anchor
                                                        ? result.maximumUpperDecayChange
                                                        : result
                                                              .maximumFundamentalDecayChange;
                                                maximum = std::max(maximum, change);
                                            }
                                        for (int bin = 0; bin <= 128; ++bin)
                                        {
                                            const auto z = std::polar(
                                                1.0,
                                                -3.14159265358979323846 * bin / 128);
                                            const double a =
                                                loop->loopDampingCoefficient;
                                            auto h = double(loop->loopGain) * (1 - a) /
                                                     (1.0 - a * z);
                                            if (loop->lossShelfActive)
                                                h *= section(loop->lossShelf, z,
                                                             loop->lossShelfB3);
                                            // The admitted correction must retain the
                                            // original scalar bound before the hand
                                            // dip. Its later envelope relaxation is
                                            // deliberately not followed by a refit.
                                            const double preDipRatio =
                                                std::abs(h) / loop->loopGain;
                                            result.maximumPreDipRatio = std::max(
                                                result.maximumPreDipRatio, preDipRatio);
                                            if (!std::isfinite(preDipRatio) ||
                                                preDipRatio > 1 + 1e-10)
                                                ++result.preDipViolations;
                                            if (loop->handDipActive)
                                                h *= section(loop->handDip, z);
                                            if (loop->materialLossDepth > 0)
                                                h *= section(loop->materialLossDip, z);
                                            const double magnitude = std::abs(h);
                                            result.maximum =
                                                std::max(result.maximum, magnitude);
                                            if (!std::isfinite(magnitude) ||
                                                magnitude > 1 + 1e-10)
                                                ++result.violations;
                                        }
                                    }
                                    // A repeated same-coordinate solve must retain
                                    // every new running state and must not compound its
                                    // correction gain.
                                    auto &l = v.vertical;
                                    l.lossShelf.z1 = .000125;
                                    l.lossShelf.z2 = -.0000625;
                                    l.lossShelfZ3 = .00025;
                                    const auto before = l.lossShelf;
                                    const double beforeGain = l.lossShelfB3;
                                    const float beforeLoopGain = l.loopGain;
                                    e->configureVoiceDamping(v, style,
                                                             v.lastConfiguredFrequency,
                                                             float(fret));
                                    if (l.lossShelf.z1 != .000125 ||
                                        l.lossShelf.z2 != -.0000625 ||
                                        l.lossShelfZ3 != .00025 ||
                                        l.loopGain != beforeLoopGain ||
                                        l.lossShelfB3 != beforeGain ||
                                        l.lossShelf.b0 != before.b0 ||
                                        l.lossShelf.a1 != before.a1)
                                        ++result.historyChanges;
                                }
                        }
        return result;
    }
#endif
};
} // namespace electry
namespace
{
int failures = 0, checks = 0;
void expect(bool value, const char *text)
{
    ++checks;
    if (!value)
    {
        ++failures;
        if (failures < 12)
            std::cerr << text << '\n';
    }
}
constexpr double pi = 3.14159265358979323846;
std::complex<double> response(const electry::ElectryEngineTestAccess::Result &r,
                              double omega)
{
    const auto z = std::polar(1.0, -omega);
    const auto &b = r.coefficients;
    return r.gain * (1 - r.pole) / (1.0 - r.pole * z) *
           (b[0] + b[1] * z + b[2] * z * z + r.b3 * z * z * z) /
           (1.0 + b[3] * z + b[4] * z * z);
}
} // namespace
int main()
{
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
    using Access = electry::ElectryEngineTestAccess;
    double maxAnchorRelative = 0, maxMagnitude = 0;
    int fallback = 0;
    // Cover all host/internal rates, ratio extremes and scalar headroom.
    // These are filter-domain coordinates, independent of the corpus.
    for (double rate : {16000., 88200., 96000., 192000., 384000., 768000.})
        for (double f0 : {20., 41.203444614, 82.406889228, 195.99771799, 783.99087196,
                          2637.0204553})
        {
            f0 = std::min(f0, .24 * rate);
            const float w0 = static_cast<float>(2 * pi * f0 / rate);
            const float wh = static_cast<float>(
                2 * pi * std::max(std::min(3600., .32 * rate), 1.5 * f0) / rate);
            for (float pole : {0.f, .00001f, .01f, .1f, .3f, .6f, .9f, .99f, .9995f})
                for (float gain : {.1f, .9f, .9999f, .99999f})
                {
                    const auto r = Access::transform(pole, gain, w0, wh);
                    expect(r.historiesRetained,
                           "live transform resets running history");
                    expect(r.gain <= gain && r.gain > 0,
                           "scalar exceeds original passive scalar");
                    if (!r.active)
                    {
                        ++fallback;
                        if (pole != 0 && fallback < 20)
                            std::cerr << "fallback rate=" << rate << " f0=" << f0
                                      << " pole=" << pole << " gain=" << gain << '\n';
                        expect(pole == 0, "nonflat transform unexpectedly fell back");
                        continue;
                    }
                    const auto &b = r.coefficients;
                    const auto discriminant =
                        std::sqrt(std::complex<double>(b[3] * b[3] - 4 * b[4], 0));
                    expect(std::abs((-b[3] + discriminant) / 2.0) < 1 &&
                               std::abs((-b[3] - discriminant) / 2.0) < 1,
                           "shelf pole not strictly stable");
                    for (double w : {double(w0), double(wh)})
                    {
                        const auto z = std::polar(1.0, -w);
                        const double old = std::abs(double(gain) * (1 - double(pole)) /
                                                    (1.0 - double(pole) * z));
                        const double error =
                            std::abs(std::abs(response(r, w)) / old - 1);
                        maxAnchorRelative = std::max(maxAnchorRelative, error);
                        expect(error < 8e-8, "existing traversal anchor changed");
                    }
                    double previous = 2;
                    for (int index = 0; index <= 128; ++index)
                    {
                        const double magnitude =
                            std::abs(response(r, pi * index / 128));
                        maxMagnitude = std::max(maxMagnitude, magnitude);
                        expect(std::isfinite(magnitude) && magnitude <= gain + 2e-10,
                               "shelf amplifies a frequency beyond original scalar");
                        expect(magnitude <= previous + 2e-10, "shelf is not monotonic");
                        previous = magnitude;
                    }
                }
        }
    const auto grid = Access::configurationGrid();
    expect(grid.preDipViolations == 0,
           "correction exceeds the original scalar before mutable hand attenuation");
    expect(grid.violations == 0,
           "actual style/age/gauge/pressure family has gain above unity");
    expect(grid.historyChanges == 0,
           "same-coordinate refit changes shelf history or compounds gain");
    std::cout << "Actual family " << grid.configurations << " loops, active "
              << grid.active << ", fallback " << grid.fallbacks << ", max magnitude "
              << grid.maximum << ", history changes " << grid.historyChanges
              << ", limited " << grid.limited << ", minimum depth " << grid.minimumDepth
              << ", max fundamental/upper decay change "
              << grid.maximumFundamentalDecayChange << '/'
              << grid.maximumUpperDecayChange << ", pre-dip maximum normalized gain "
              << grid.maximumPreDipRatio << ", pre-dip violations "
              << grid.preDipViolations << '\n';
    for (int style : {0, 1, 6})
        for (int pressure = 0; pressure < 3; ++pressure)
            std::cout << "Fallback style " << style << " pressure " << pressure * .5
                      << ": " << grid.fallbackByStylePressure[style * 3 + pressure]
                      << '\n';
    // Audit the added correction separately from the pre-existing hand
    // solve: the legacy curve itself has high-rate hand-dip solve steps. A
    // new hard correction fallback would not shrink as the pressure increment
    // is halved. Include the broad grid's actual constrained construction.
    std::cout << "Worst constrained cell string " << grid.worstString << " fret "
              << grid.worstFret << " style " << grid.worstStyle << " age "
              << grid.worstAge << " gauge " << grid.worstGauge << " pressure "
              << grid.worstPressure << " rate " << grid.worstRate << '\n';
    for (auto style : {electry::PlayStyle::Sustain, electry::PlayStyle::PalmMute,
                       electry::PlayStyle::Dead})
        for (int stringIndex : {2, 7})
        {
            const double coarse = Access::largestPressureStep(
                128, stringIndex, 22, style, grid.worstAge, grid.worstGauge);
            const double fine = Access::largestPressureStep(
                256, stringIndex, 22, style, grid.worstAge, grid.worstGauge);
            expect((coarse == 0 && fine == 0) || fine < .75 * coarse,
                   "added correction retains a resolution-independent pressure step");
            std::cout << "Correction convergence style " << int(style) << " string "
                      << stringIndex << ": " << coarse << " -> " << fine << '\n';
        }
    for (double rate : {48000., 96000., 384000.})
    {
        const auto relaxation = Access::laterHandRelaxation(rate);
        expect(relaxation.established && relaxation.relaxed,
               "dynamic hand witness did not weaken the configured hand dip");
        expect(relaxation.correctionRetained,
               "dynamic hand witness unexpectedly refitted the intrinsic correction");
        expect(relaxation.maximumRelaxedRatio <= 1 + 1e-10 &&
                   relaxation.maximumBypassedRatio <= 1 + 1e-10,
               "weakening a later hand dip invalidates the original scalar bound");
        std::cout << "Later hand relaxation " << rate << " Hz: max normalized gain "
                  << relaxation.maximumRelaxedRatio << ", bypass "
                  << relaxation.maximumBypassedRatio << '\n';
    }
    const auto syntheticRelaxation = Access::syntheticHandRelaxation();
    expect(syntheticRelaxation.established && syntheticRelaxation.relaxed &&
               syntheticRelaxation.correctionRetained,
           "synthetic hand-relaxation witness did not retain its admitted correction");
    expect(syntheticRelaxation.maximumRelaxedRatio <= 1 + 1e-10 &&
               syntheticRelaxation.maximumBypassedRatio <= 1 + 1e-10,
           "hand-assisted admission permits gain after later dip relaxation");
    std::cout << "Synthetic later hand relaxation: max normalized gain "
              << syntheticRelaxation.maximumRelaxedRatio << ", bypass "
              << syntheticRelaxation.maximumBypassedRatio << '\n';
    const float floatPi = 3.14159265358979323846f;
    for (const auto coordinates : {std::array<float, 2>{0, 1},
                                   {-1, 1},
                                   {.001f, floatPi},
                                   {.001f, 4},
                                   {.001f, 2 * floatPi + 1},
                                   {2.7614f, 4.1421f},
                                   {.1f, .1f}})
    {
        const auto invalid =
            Access::transform(.6f, .9999f, coordinates[0], coordinates[1]);
        expect(!invalid.active && invalid.invalidCorrectionCleared &&
                   invalid.gain == .9999f,
               "invalid bilinear coordinates replace the stable legacy fallback");
    }
    expect(Access::invalidDomainReturn(),
           "invalid-domain bypass replays stale correction history on valid return");
    for (double rate : {48000., 96000., 384000.})
    {
        const auto wide = Access::wideSympatheticBends(rate);
        expect(wide.configured > 0,
               "public wide-bend witness did not wake the idle bank");
        expect(wide.invalidActive == 0 && wide.unstable == 0,
               "wide global bend installs an invalid or unstable idle correction");
        expect(wide.finite, "wide sympathetic bend or return corrupts running state");
        std::cout << "Wide sympathetic bend " << rate << " Hz: observations "
                  << wide.configured << ", invalid active " << wide.invalidActive
                  << ", unstable " << wide.unstable << '\n';
    }
    // Deliberately active total filters must be rejected if their corrected
    // response remains above unity; the certificate is not an unconditional
    // claim that every algebraic correction is passive.
    const auto unsafe = Access::transform(.3f, .9999f, .01f, .4f, .3f, 1.1f);
    expect(!unsafe.active, "passivity certificate accepts an active total filter");
    std::cout << "Shelf anchor maximum relative error " << maxAnchorRelative
              << ", maximum magnitude " << maxMagnitude << ", flat bypasses "
              << fallback << '\n';
#else
    expect(true, "default bypass builds");
#endif
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
