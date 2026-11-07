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

double levelAt(const std::vector<float>& x, double sampleRate, double startSeconds,
               double lengthSeconds, double f)
{
    const int start = static_cast<int>(startSeconds * sampleRate);
    const int length = static_cast<int>(lengthSeconds * sampleRate);
    std::complex<double> sum = 0.0;
    for (int i = 0; i < length; ++i)
    {
        const double window = 0.5 - 0.5 * std::cos(2.0 * pi * (i + 0.5) / length);
        sum += static_cast<double>(x[static_cast<size_t>(start + i)]) * window
            * std::polar(1.0, -2.0 * pi * f * (start + i) / sampleRate);
    }
    // Per second of window, so windows at different rates compare.
    return 20.0 * std::log10(std::abs(sum) / sampleRate + 1e-30);
}

double level(const std::vector<float>& x, double startSeconds, double lengthSeconds,
             double f)
{
    return levelAt(x, rate, startSeconds, lengthSeconds, f);
}

// Light enough that the coils read the string linearly: at an ordinary
// stroke the flux law fills the neck coil's notches to 14 to 25 dB under
// the partials around them (testFluxLaw), which hides where the coils sit.
constexpr double lightNewtons = 0.05;

void testCoilPlacement()
{
    // The open low E plucked at L/5. The neck coil sits at L/4 and so hears
    // no 4th harmonic; the middle and bridge coils do. The palm holds the
    // other strings: the open high E is tuned to that harmonic and, free,
    // rings with it through the neck where the neck coil hears it.
    const auto& spec = StratString::construction(0);
    const auto partial = [&](int n) {
        return n * spec.openHz
            * std::sqrt((1.0 + spec.stiffness * n * n) / (1.0 + spec.stiffness));
    };
    for (Position position : {Position::neck, Position::middle, Position::bridge})
    {
        auto engine = makeEngine(position);
        for (int s = 1; s < 6; ++s)
            engine->setDamper(s, 0, 0.02, 2.0);
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
        engine->pluck(0, newtons, 0.13, impedance * 0.0006 / force + 90.7e-6,
                      2.0 * force / (2.0 * impedance), false, 0.5);
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
    // 1.88 N is StratPerformer's ordinary stroke, which the recorded Strat's
    // pitch glide gives with the pick holding half of it at rest; 1.4 N was
    // the ordinary stroke when the push started from nothing, and 0.7 and
    // 1.4 N stay here as a doubling.
    const double half = twelfthFretOctave(0.7, false);
    const double full = twelfthFretOctave(1.4, false);
    const double ordinary = twelfthFretOctave(1.88, true);
    const double up = twelfthFretOctave(-1.88, true);
    std::printf("12th-fret 2nd partial at the neck coil: %.1f dB light, %.1f at "
                "0.7 N, %.1f at 1.4 N; %.1f an ordinary stroke, %.1f upstroke\n",
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
        // StratPerformer's ordinary stroke.
        engine->pluck(s, 1.88, 0.13, 0.0002, 2.0 * 1.88 / (2.0 * z), false, 0.5);
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
    // The palm on all six: a free string would ring on with the plucked one
    // through the neck for as long as its own decay takes.
    const auto palmed = [] {
        auto engine = makeEngine(Position::neck);
        for (int s = 0; s < 6; ++s)
            engine->setDamper(s, 0, 0.02, 2.0);
        return engine;
    };
    const auto anySounding = [](const StratEngine& engine) {
        bool any = false;
        for (int s = 0; s < 6; ++s)
            any = any || engine.isSounding(s);
        return any;
    };
    auto engine = palmed();
    check(!anySounding(*engine), "an unplayed guitar is silent");
    engine->pluck(5, 1.5, 0.13, 0.0002);
    check(engine->isSounding(5), "a plucked string sounds");
    std::vector<float> y(static_cast<size_t>(rate));
    int seconds = 0;
    while (anySounding(*engine) && seconds < 30)
    {
        engine->process(y.data(), static_cast<int>(y.size()));
        ++seconds;
    }
    check(!anySounding(*engine), "a damped guitar falls silent");

    // Played again, it sounds as a fresh string would: what was left is
    // 200 dB down.
    engine->setDamper(5, 0, 0.02, 0.0);
    engine->pluck(5, 1.5, 0.13, 0.0002);
    const auto again = render(*engine, 0.3);
    auto fresh = palmed();
    fresh->setDamper(5, 0, 0.02, 0.0);
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

void testSympathy()
{
    // The high E at fret 1, an F, with the open G free or under the palm.
    // Free, the G rings at its own partials, which none of the F's reaches,
    // through the neck both share; its second, 392 Hz, sits nearest the
    // neck's resonance. The recorded Strat's free open strings stand 40 to
    // 60 dB under the note's fundamental (StratString.cpp).
    const double b = StratString::construction(3).stiffness;
    const auto ringing = [b](bool palmOnG) {
        auto engine = makeEngine(Position::neck);
        if (palmOnG)
            engine->setDamper(3, 0, 0.02, 2.0);
        engine->setFret(5, 1.0);
        engine->pluck(5, 1.5, 0.13, 0.0002);
        const auto y = render(*engine, 1.8);
        const double second
            = 2.0 * engine->fundamentalHz(3) * std::sqrt((1.0 + 4.0 * b) / (1.0 + b));
        return level(y, 0.3, 1.5, second) - level(y, 0.3, 1.5, engine->fundamentalHz(5));
    };
    const double free = ringing(false), palmed = ringing(true);
    std::printf("open G under a fretted high E: %.1f dB re the note free, %.1f under the palm\n",
                free, palmed);
    check(free > -70.0 && free < -30.0, "a free open string rings along through the neck");
    check(palmed < free - 20.0, "the palm stops it");
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
        // The intonation's ratio for the fret (testIntonation).
        const auto [open, setback] = StratEngine::intonation(s);
        const double ratio = fret > 0.0
            ? open / (StratString::scaleMetres * std::pow(2.0, -fret / 12.0) + setback)
            : 1.0;
        const double want = StratString::construction(s).openHz * StratEngine::restPitch(s) * ratio
            * std::sqrt(engine->bridge().tensionRatio(s));
        tuned = tuned && std::abs(engine->fundamentalHz(s) / want - 1.0) < 1e-12;
    }
    check(tuned, "the arm and a bend retune every string through the bridge");
}

// Every fretted note within a cent of equal temperament against its open
// string: the saddles and the nut (StratEngine::intonation) take back what
// the fretting finger's stretch adds, which left the low E 13 to 18 cents
// sharp and the G 11 to 15 (Electry 2.0.1). A chord's notes play as they do
// alone: the stabilizer holds the plate (StratBridge), where the floating
// plate let an F barre's fingers pull every string 1 to 6.5 cents flat
// (Electry 2.0.2).
void testIntonation()
{
    auto engine = makeEngine(Position::neck);
    double worst = 0.0;
    for (int s = 0; s < 6; ++s)
    {
        for (int fret = 1; fret <= 21; ++fret)
        {
            engine->setFret(s, fret);
            const double tempered
                = StratString::construction(s).openHz * StratEngine::restPitch(s) * std::pow(2.0, fret / 12.0);
            worst = std::max(worst, std::abs(1200.0 * std::log2(engine->fundamentalHz(s) / tempered)));
        }
        engine->setFret(s, 0.0);
    }
    std::printf("fretted notes against equal temperament: %.2f cents at worst\n", worst);
    check(worst < 1.0, "every fretted note plays in tune with the open strings");

    const int barre[6] = {1, 3, 3, 2, 1, 1};
    double chordWorst = 0.0;
    for (int s = 0; s < 6; ++s)
        engine->setFret(s, barre[s]);
    std::vector<float> y(StratEngine::controlPeriod);
    engine->process(y.data(), static_cast<int>(y.size()));
    for (int s = 0; s < 6; ++s)
    {
        const double tempered = StratString::construction(s).openHz * StratEngine::restPitch(s)
            * std::pow(2.0, barre[s] / 12.0);
        chordWorst = std::max(chordWorst, std::abs(1200.0 * std::log2(engine->fundamentalHz(s) / tempered)));
    }
    std::printf("an F barre against equal temperament: %.2f cents at worst\n", chordWorst);
    check(chordWorst < 1.0, "a chord's notes play in tune together");
}

// The top octave keeps the string's own loss whatever the host's rate: the
// loop's read between samples would drain it on every round trip at 44.1
// kHz, unless the strings run faster than the host (StratEngine.cpp). The
// open high E's partials at 10 to 16 kHz over 0.3 to 0.5 s, against its
// first three, at 44.1 kHz and at 192 kHz.

void testTopOctaveAtEveryRate()
{
    const auto& spec = StratString::construction(5);
    const auto topOverBody = [&](double hostRate) {
        auto engine = std::make_unique<StratEngine>();
        engine->prepare(hostRate);
        StratCircuit::Controls controls;
        controls.position = Position::neck;
        engine->setControls(controls);
        engine->pluck(5, 1.0, 0.13, 0.0001);
        std::vector<float> y(static_cast<size_t>(0.5 * hostRate));
        engine->process(y.data(), static_cast<int>(y.size()));
        const auto energy = [&](int first, int last) {
            double sum = 0.0;
            for (int n = first; n <= last; ++n)
            {
                const double f = n * spec.openHz
                    * std::sqrt((1.0 + spec.stiffness * n * n) / (1.0 + spec.stiffness));
                sum += std::pow(10.0, levelAt(y, hostRate, 0.3, 0.2, f) / 10.0);
            }
            return 10.0 * std::log10(sum);
        };
        return energy(31, 48) - energy(1, 3);
    };
    const double host = topOverBody(44100.0), fast = topOverBody(192000.0);
    std::printf("high E 10-16 kHz at 0.3-0.5 s, re its first three partials: "
                "%.1f dB at 44.1 kHz, %.1f at 192 kHz\n", host, fast);
    check(std::abs(host - fast) < 1.5, "the top octave decays alike at 44.1 and 192 kHz");
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
    testSympathy();
    testMovingControls();
    testTremoloRetunes();
    testIntonation();
    testTopOctaveAtEveryRate();
    testDeterminism();
    if (failures == 0)
        std::printf("StratEngine: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
