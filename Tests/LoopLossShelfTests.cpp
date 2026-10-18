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
        ElectryEngine::configureLoopLossShelf(*loop, omega0, high, coefficient, gain);
        const auto &b = loop->lossShelf;
        return {{b.b0, b.b1, b.b2, b.a1, b.a2},
                loop->loopGain,
                loop->loopDampingCoefficient,
                loop->lossShelfB3,
                loop->lossShelfDepth,
                loop->lossShelfActive,
                b.z1 == .000125 && b.z2 == -.0000625 && loop->damping.state == .03125f};
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
    struct Grid
    {
        int configurations = 0, active = 0, fallbacks = 0, violations = 0,
            historyChanges = 0;
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
              << grid.maximumUpperDecayChange << '\n';
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
