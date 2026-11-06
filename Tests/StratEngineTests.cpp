// The guitar from strings to jack: that each switch position hears its own
// coils where they sit on the strings, that a hard swing reads through the
// coils' flux law, that rendering does not depend on the host's block size,
// that silent strings drop out and come back cleanly, and that the controls
// can move under a sounding chord.
#include "DSP/StratEngine.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <memory>
#include <vector>

namespace
{
using electry::StratCircuit;
using electry::StratEngine;
using electry::StratString;
using Position = StratCircuit::Position;

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition)
    {
        ++failures;
        std::printf("FAIL: %s\n", what);
    }
}

constexpr double pi = 3.14159265358979323846;
constexpr double rate = 48000.0;

std::unique_ptr<StratEngine> makeEngine(Position position)
{
    auto engine = std::make_unique<StratEngine>();
    engine->prepare(rate);
    StratCircuit::Controls controls;
    controls.position = position;
    engine->setControls(controls);
    return engine;
}

std::vector<float> render(StratEngine& engine, double seconds)
{
    std::vector<float> out(static_cast<size_t>(seconds * rate));
    engine.process(out.data(), static_cast<int>(out.size()));
    return out;
}

double level(const std::vector<float>& x, double startSeconds, double lengthSeconds,
             double f)
{
    const int start = static_cast<int>(startSeconds * rate);
    const int length = static_cast<int>(lengthSeconds * rate);
    std::complex<double> sum = 0.0;
    for (int i = 0; i < length; ++i)
    {
        const double window = 0.5 - 0.5 * std::cos(2.0 * pi * (i + 0.5) / length);
        sum += static_cast<double>(x[static_cast<size_t>(start + i)]) * window
            * std::polar(1.0, -2.0 * pi * f * (start + i) / rate);
    }
    return 20.0 * std::log10(std::abs(sum) + 1e-30);
}

// Light enough that the coils read the string linearly: at an ordinary
// stroke the flux law fills the neck coil's notches to 14 to 25 dB under
// the partials around them (testFluxLaw), which hides where the coils sit.
constexpr double lightNewtons = 0.05;

void testCoilPlacement()
{
    // The open low E plucked at L/5. The neck coil sits at L/4 and so hears
    // no 4th harmonic; the middle and bridge coils do.
    const auto& spec = StratString::construction(0);
    const auto partial = [&](int n) {
        return n * spec.openHz
            * std::sqrt((1.0 + spec.stiffness * n * n) / (1.0 + spec.stiffness));
    };
    for (Position position : {Position::neck, Position::middle, Position::bridge})
    {
        auto engine = makeEngine(position);
        engine->pluck(0, lightNewtons, StratString::scaleMetres / 5.0, 0.0002);
        const auto y = render(*engine, 0.6);
        const double around = 0.5 * (level(y, 0.05, 0.5, partial(3))
                                     + level(y, 0.05, 0.5, partial(6)));
        const double fourth = level(y, 0.05, 0.5, partial(4));
        if (position == Position::neck)
            check(fourth < around - 30.0, "the neck coil at L/4 hears no 4th harmonic");
        else
            check(fourth > around - 15.0, "the middle and bridge coils hear the 4th harmonic");
    }
}

void testCoilPositions()
{
    // A coil at x hears harmonic n in proportion to sin(n pi x / L), so the
    // octave over the fundamental reads 2 cos(pi x / L) higher in each
    // position: everything else in the path is common to all three, so the
    // differences between positions place the coils.
    const auto& spec = StratString::construction(0);
    const auto partial = [&](int n) {
        return n * spec.openHz
            * std::sqrt((1.0 + spec.stiffness * n * n) / (1.0 + spec.stiffness));
    };
    const auto octaveOverFundamental = [&](Position position) {
        auto engine = makeEngine(position);
        engine->pluck(0, lightNewtons, 0.13, 0.0002);
        const auto y = render(*engine, 0.6);
        return level(y, 0.05, 0.5, partial(2)) - level(y, 0.05, 0.5, partial(1));
    };
    const auto expected = [&](int coil) {
        return 20.0 * std::log10(2.0 * std::cos(pi * StratEngine::coilMetres(coil, 0)
                                                / StratString::scaleMetres));
    };
    const double neck = octaveOverFundamental(Position::neck);
    const double middle = octaveOverFundamental(Position::middle);
    const double bridge = octaveOverFundamental(Position::bridge);
    std::printf("octave over fundamental, middle less neck %.3f dB (coils say "
                "%.3f), bridge less middle %.3f dB (%.3f)\n",
                middle - neck, expected(1) - expected(0), bridge - middle,
                expected(2) - expected(1));
    check(std::abs((middle - neck) - (expected(1) - expected(0))) < 0.05,
          "the middle position hears the middle coil");
    check(std::abs((bridge - middle) - (expected(2) - expected(1))) < 0.05,
          "the bridge position hears the bridge coil where it slants under the low E");
}

// The low E stopped at the 12th fret and plucked with `newtons`: the neck
// coil sits halfway along it, so a linear coil hears no 2nd partial.
// Returns the 2nd against the mean of the fundamental and the 3rd, over 20
// to 170 ms. As a stroke it releases as StratPerformer's ordinary stroke
// does; otherwise in the same time whatever the force, so the string's own
// response is linear in it.
double twelfthFretOctave(double newtons, bool stroke)
{
    auto engine = makeEngine(Position::neck);
    engine->setFret(0, 12.0);
    const auto& spec = StratString::construction(0);
    const double impedance = spec.kilogramsPerMetre * 2.0 * StratString::scaleMetres * spec.openHz;
    const double force = std::abs(newtons);
    if (stroke)
        engine->pluck(0, newtons, 0.13, impedance * 0.00015 / force + 90.7e-6,
                      1.5 * force / (2.0 * impedance));
    else
        engine->pluck(0, newtons, 0.13, 0.0002);
    const auto y = render(*engine, 0.2);
    const double f0 = engine->fundamentalHz(0);
    const auto partial = [&](int n) {
        const double b = spec.stiffness * 4.0; // stiffness grows as 1/L^2
        return n * f0 * std::sqrt((1.0 + b * n * n) / (1.0 + b));
    };
    // The 2nd partial's own peak is a few hundredths of a hertz from 2 f0:
    // the window is 150 ms, its main lobe 13 Hz wide.
    return level(y, 0.02, 0.15, partial(2))
        - 0.5 * (level(y, 0.02, 0.15, partial(1)) + level(y, 0.02, 0.15, partial(3)));
}

void testFluxLaw()
{
    const double light = twelfthFretOctave(lightNewtons, false);
    const double half = twelfthFretOctave(1.25, false);
    const double full = twelfthFretOctave(2.5, false);
    const double ordinary = twelfthFretOctave(2.5, true);
    const double up = twelfthFretOctave(-2.5, true);
    std::printf("12th-fret 2nd partial at the neck coil: %.1f dB light, %.1f at "
                "1.25 N, %.1f at 2.5 N; %.1f an ordinary stroke, %.1f upstroke\n",
                light, half, full, ordinary, up);
    check(light < -28.0, "a light pluck leaves the neck coil's notch empty");
    check(full - half > 5.0 && full - half < 7.0, "the fill grows as the swing squared");
    // The recorded Strat (IDMT-SMT-Guitar, neck pickup) has it 17 dB under
    // on its six 12th-fret notes, 12 to 22, and 15 on this one: StratEngine's
    // acrossShare.
    check(ordinary > -21.0 && ordinary < -13.0,
          "an ordinary stroke fills it as far as the recorded Strat's");
    check(std::abs(up - ordinary) < 2.0, "an upstroke fills it as much");
}

void testStringBalance()
{
    // One stroke on each open string, the neck coil alone: its level below
    // 1.2 kHz over the first 250 ms. A Strat recorded direct has every
    // string within 6 dB of its low E (IDMT-SMT-Guitar, neck pickup, notes
    // at frets 0-2: 0 to -5.6 dB in dataset 1, +1.1 to -1.9 in dataset 2),
    // where the steel and the gap alone leave the high E 10 dB under.
    double low = 0.0, worst = 0.0;
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        auto engine = makeEngine(Position::neck);
        const auto& spec = StratString::construction(s);
        const double z = spec.kilogramsPerMetre * 2.0 * StratString::scaleMetres * spec.openHz;
        engine->pluck(s, 2.5, 0.13, 0.0002, 1.5 * 2.5 / (2.0 * z));
        const auto y = render(*engine, 0.3);
        double sum = 0.0;
        for (int k = 1; k * spec.openHz < 1200.0; ++k)
            sum += std::pow(10.0, level(y, 0.01, 0.25, k * spec.openHz) / 10.0);
        const double db = 10.0 * std::log10(sum);
        if (s == 0)
            low = db;
        std::printf("open string %d at one stroke: %+.1f dB against the low E\n", s, db - low);
        worst = std::max(worst, std::abs(db - low));
    }
    check(worst < 6.0, "one stroke on every string comes out as evenly as the recorded Strat's");
}

// Renders the same performance with the host splitting it into blocks of
// `block` samples; events land at the same absolute samples every time.
std::vector<float> performance(int block)
{
    auto engine = makeEngine(Position::middleNeck);
    const int length = static_cast<int>(1.5 * rate);
    std::vector<float> out(static_cast<size_t>(length));
    const int events[] = {0, 3001, 5000, 9000, 20011, length};
    int done = 0;
    for (int e = 0; e + 1 < 6; ++e)
    {
        switch (e)
        {
            case 0:
                for (int s = 0; s < 6; ++s)
                    engine->pluck(s, 1.0 + 0.1 * s, 0.13, 0.0002);
                break;
            case 1: engine->setFret(2, 5.0); engine->pluck(2, 1.2, 0.11, 0.0003); break;
            case 2: engine->setDamper(0, 0, 0.02, 0.5); break;
            case 3: engine->setFret(4, 3.0, true); break;
            case 4: engine->setBend(5, 1.06); engine->setArm(-0.2); break;
        }
        // Continuous gestures are restated each control period, as a
        // performer does; here a slide on string 4 that lasts to the end.
        while (done < events[e + 1])
        {
            int n = events[e + 1] - done;
            n = n < block ? n : block;
            const int toControl = StratEngine::controlPeriod
                - done % StratEngine::controlPeriod;
            n = n < toControl ? n : toControl;
            if (e >= 3 && done % StratEngine::controlPeriod == 0)
                engine->setFret(4, 3.0 + 4.0 * done / length, true);
            engine->process(out.data() + done, n);
            done += n;
        }
    }
    return out;
}

void testBlockPartition()
{
    const auto reference = performance(1);
    bool same = true;
    for (int block : {7, 64, 256, 4096})
        same = same && performance(block) == reference;
    check(same, "any block size gives the same bits");
}

void testQuietStrings()
{
    auto engine = makeEngine(Position::neck);
    engine->setDamper(5, 0, 0.02, 2.0);
    engine->pluck(5, 1.5, 0.13, 0.0002);
    check(engine->isSounding(5), "a plucked string sounds");
    std::vector<float> y(static_cast<size_t>(rate));
    int seconds = 0;
    while (engine->isSounding(5) && seconds < 30)
    {
        engine->process(y.data(), static_cast<int>(y.size()));
        ++seconds;
    }
    check(!engine->isSounding(5), "a damped string falls silent");
    check(!engine->isSounding(0), "an unplayed string is silent");

    // Played again, it sounds as a fresh string would: what was left is
    // 200 dB down.
    engine->setDamper(5, 0, 0.02, 0.0);
    engine->pluck(5, 1.5, 0.13, 0.0002);
    const auto again = render(*engine, 0.3);
    auto fresh = makeEngine(Position::neck);
    fresh->pluck(5, 1.5, 0.13, 0.0002);
    const auto first = render(*fresh, 0.3);
    double difference = 0.0, energy = 0.0;
    for (size_t i = 0; i < first.size(); ++i)
    {
        difference += (again[i] - first[i]) * (again[i] - first[i]);
        energy += first[i] * first[i];
    }
    check(difference < 1e-12 * energy, "a string played after silence sounds like new");
}

void testMovingControls()
{
    const int frets[6] = {0, 2, 2, 1, 0, 0};
    const auto strum = [&frets](StratEngine& engine) {
        for (int s = 0; s < 6; ++s)
        {
            engine.setFret(s, frets[s]);
            engine.pluck(s, 2.0, 0.13, 0.0002);
        }
    };
    auto still = makeEngine(Position::neck);
    strum(*still);
    double stillPeak = 0.0;
    for (float v : render(*still, 2.0))
        stillPeak = std::abs(v) > stillPeak ? std::abs(v) : stillPeak;

    auto engine = makeEngine(Position::neck);
    strum(*engine);
    unsigned seed = 7u;
    const auto next = [&seed] {
        seed = seed * 1664525u + 1013904223u;
        return (seed >> 8) * (1.0 / 16777216.0);
    };
    std::vector<float> block(64);
    bool finite = true;
    double peak = 0.0;
    for (int i = 0; i < static_cast<int>(2.0 * rate) / 64; ++i)
    {
        StratCircuit::Controls controls = engine->controls();
        controls.volume = next();
        controls.toneNeck = next();
        controls.toneMiddle = next();
        if (i % 50 == 0)
            controls.position = static_cast<Position>(static_cast<int>(next() * 5.0) % 5);
        engine->setControls(controls);
        engine->process(block.data(), 64);
        for (float v : block)
        {
            finite = finite && std::isfinite(v);
            peak = std::abs(v) > peak ? std::abs(v) : peak;
        }
    }
    check(finite, "switch and knob moves under a chord stay finite");
    // A Tone jumping off 0 between blocks lets go of the coil current its
    // 0.1 uF was drawing, and the winding's and cable's 400 pF take it: a
    // kick of I sqrt(L / C), I about the coil's EMF over its resistance at
    // most, from two coils in parallel twice that into sqrt(L / 2C). The jack
    // stays within 19 times the coils' EMF, which under this chord peaks with
    // the chord at Volume 10 (5.1 against 5.0 V). Twelve seeds of these moves
    // reach 2.4 times it; a real knob turns too slowly to kick at all. (A
    // limit of 10 V once passed on this seed but not on others.)
    const double kick = std::sqrt(2.0 * StratCircuit::coilHenries
                                  / (StratCircuit::coilFarads + StratCircuit::cableFarads))
        / StratCircuit::coilOhms;
    std::printf("moving controls peak at %.2f V, the still chord at %.2f V\n", peak, stillPeak);
    check(peak < kick * stillPeak, "switch and knob moves under a chord stay bounded");
    // Once the hand leaves the controls the network settles under what the
    // chord alone drives.
    double settled = 0.0;
    const auto after = render(*engine, 0.5);
    for (size_t i = after.size() / 2; i < after.size(); ++i)
        settled = std::abs(after[i]) > settled ? std::abs(after[i]) : settled;
    check(settled < stillPeak, "after the moves the controls settle");
}

void testTremoloRetunes()
{
    // The arm and a bend reach every string within one control period.
    auto engine = makeEngine(Position::neck);
    engine->setFret(3, 7.0);
    engine->setArm(0.7);
    engine->setBend(3, std::pow(2.0, 2.0 / 12.0));
    std::vector<float> y(StratEngine::controlPeriod);
    engine->process(y.data(), static_cast<int>(y.size()));
    bool tuned = true;
    for (int s = 0; s < 6; ++s)
    {
        const double fret = s == 3 ? 7.0 : 0.0;
        const double want = StratString::construction(s).openHz * std::pow(2.0, fret / 12.0)
            * std::sqrt(engine->bridge().tensionRatio(s));
        tuned = tuned && std::abs(engine->fundamentalHz(s) / want - 1.0) < 1e-12;
    }
    check(tuned, "the arm and a bend retune every string through the bridge");
}

void testDeterminism()
{
    check(performance(256) == performance(256), "the same performance gives the same bits");
}

} // namespace

int main()
{
    testCoilPlacement();
    testCoilPositions();
    testFluxLaw();
    testStringBalance();
    testBlockPartition();
    testQuietStrings();
    testMovingControls();
    testTremoloRetunes();
    testDeterminism();
    if (failures == 0)
        std::printf("StratEngine: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
