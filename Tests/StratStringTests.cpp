// The Stratocaster string against the physics it claims. Pitches are checked
// against equal temperament from A4 = 440 Hz, partials against the stiff
// string's f_n = n f sqrt(1 + B n^2), decays against the construction table,
// and the pick, pickup and damper positions against the nodes a point on an
// ideal string has: a pluck or a pickup at 1/k of the length neither excites
// nor hears harmonic k.
#include "DSP/StratString.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <memory>
#include <utility>
#include <vector>

namespace
{
using electry::StratString;

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
constexpr double neckMetres = 0.162;   // L/4 from the bridge
constexpr double bridgeMetres = 0.041;
constexpr double pickMetres = 0.13;
// The string's pitch rises with its energy (testTensionModulation), as the
// square of the pluck force: 1.5 N starts the open low E 6 cents sharp.
// Tuning, decay and node checks are of the small-amplitude string, plucked
// where that rise is under a hundredth of a cent.
constexpr double lightNewtons = 0.05;

std::unique_ptr<StratString> makeString(double rate, int index, double fret)
{
    auto string = std::make_unique<StratString>();
    string->prepare(rate, index);
    string->setTaps({neckMetres, 0.098, bridgeMetres});
    string->setTuning(fret, 1.0);
    return string;
}

// Velocity under each tap for `seconds`, from tap `tap`.
std::vector<float> run(StratString& string, double rate, double seconds, int tap)
{
    std::vector<float> out(static_cast<size_t>(seconds * rate));
    std::array<float, StratString::tapCount> taps {};
    std::array<float, StratString::tapCount> alongTop {};
    for (float& sample : out)
    {
        string.process(taps, alongTop);
        sample = taps[static_cast<size_t>(tap)];
    }
    return out;
}

// Hann-windowed DTFT magnitude of x[start, start + length) at f.
double magnitude(const std::vector<float>& x, double rate, double startSeconds,
                 double lengthSeconds, double f)
{
    const int start = static_cast<int>(startSeconds * rate);
    const int length = static_cast<int>(lengthSeconds * rate);
    std::complex<double> sum = 0.0;
    const std::complex<double> step = std::polar(1.0, -2.0 * pi * f / rate);
    std::complex<double> phasor = std::polar(1.0, -2.0 * pi * f * start / rate);
    for (int i = 0; i < length; ++i)
    {
        const double window = 0.5 - 0.5 * std::cos(2.0 * pi * (i + 0.5) / length);
        sum += static_cast<double>(x[static_cast<size_t>(start + i)]) * window * phasor;
        phasor *= step;
    }
    return std::abs(sum);
}

double decibels(double value) { return 20.0 * std::log10(value + 1e-30); }

// The spectral peak of x[start, start + length) within `span` Hz of
// `guess`, by golden-section search.
double peakFrequency(const std::vector<float>& x, double rate, double startSeconds,
                     double lengthSeconds, double guess, double span)
{
    double low = guess - span, high = guess + span;
    for (int i = 0; i < 32; ++i)
    {
        const double a = low + 0.382 * (high - low);
        const double b = low + 0.618 * (high - low);
        if (magnitude(x, rate, startSeconds, lengthSeconds, a)
            < magnitude(x, rate, startSeconds, lengthSeconds, b))
            low = a;
        else
            high = b;
    }
    return 0.5 * (low + high);
}

double peakFrequency(const std::vector<float>& x, double rate, double guess,
                     double span)
{
    return peakFrequency(x, rate, 0.2, 0.5, guess, span);
}

double cents(double measured, double expected)
{
    return 1200.0 * std::log2(measured / expected);
}

// MIDI notes of standard tuning, low E to high E.
constexpr int openNotes[StratString::stringCount] = {40, 45, 50, 55, 59, 64};

double equalTempered(int note) { return 440.0 * std::pow(2.0, (note - 69) / 12.0); }

void testTuningAndStiffness(double rate)
{
    double worstFundamental = 0.0, worstPartial = 0.0;
    for (int s = 0; s < StratString::stringCount; ++s)
        for (int fret : {0, 5, 12, 19, 21})
        {
            auto string = makeString(rate, s, fret);
            string->pluck(lightNewtons, pickMetres, 0.0002);
            const auto y = run(*string, rate, 0.7, 0);
            const double f0 = equalTempered(openNotes[s] + fret);
            const double measured = peakFrequency(y, rate, f0, 2.0);
            const double error = std::abs(cents(measured, f0));
            worstFundamental = error > worstFundamental ? error : worstFundamental;

            // B grows with the inverse square of the speaking length.
            const double b = StratString::construction(s).stiffness
                * std::pow(2.0, fret / 6.0);
            for (int n = 2; n <= 15; ++n)
            {
                const double fn = n * f0 * std::sqrt((1.0 + b * n * n) / (1.0 + b));
                if (fn > 4000.0)
                    break;
                // The neck tap sits at L/4 of the open string only.
                if (fret == 0 && n % 4 == 0)
                    continue;
                // The pick at L/5 leaves harmonic 5 and 10 too weak to place.
                if (fret == 0 && n % 5 == 0)
                    continue;
                const double partial = peakFrequency(y, rate, fn, 3.0);
                const double e = std::abs(cents(partial, fn));
                worstPartial = e > worstPartial ? e : worstPartial;
            }
        }
    std::printf("%.0f Hz: fundamentals within %.3f cents, partials within "
                "%.2f cents\n",
                rate, worstFundamental, worstPartial);
    check(worstFundamental < 0.5, "every fret is in tune");
    // Four identical first-order allpass sections cannot follow the stiffest
    // string across the band: the low E from fret 11 up keeps some low
    // partials 5 to 7.4 cents sharp of the stiff string (fret 18, partial 3:
    // 16.6 against 9.2 cents over harmonic), at a least-squares and at a
    // minimax fit alike; the other five strings stay within 2.8 cents. That
    // is under the 1 % (17 cents) listeners need to hear one low partial
    // mistuned (Moore, Peters and Glasberg, JASA 77, 1985).
    check(worstPartial < 8.0, "partials follow the stiff string");
}

void testDecay()
{
    const double rate = 48000.0;
    double worst = 0.0, worstTop = 0.0;
    for (int s = 0; s < StratString::stringCount; ++s)
    {
        const auto& spec = StratString::construction(s);
        auto string = makeString(rate, s, 0);
        string->pluck(lightNewtons, pickMetres, 0.0002);
        const auto y = run(*string, rate, 2.4, 2);
        const double b = spec.stiffness;
        auto partialHz = [&](int n) { return n * spec.openHz * std::sqrt((1.0 + b * n * n) / (1.0 + b)); };
        for (int n : {1, 8})
        {
            const double early = magnitude(y, rate, 0.3, 0.5, partialHz(n));
            const double late = magnitude(y, rate, 1.8, 0.5, partialHz(n));
            const double t60 = 60.0 * 1.5 / (decibels(early) - decibels(late));
            const double want = n == 1 ? spec.t60Fundamental : spec.t60EighthHarmonic;
            const double error = std::abs(t60 / want - 1.0);
            worst = error > worst ? error : worst;
        }
        // The top: the strongest of the four partials under highHz, since the
        // pick and the tap leave some up there 70 dB down. The dispersion is fitted
        // under 4 kHz, so it is found where it sounds; it falls 50 dB/s or
        // more, so it is read over 0.6 s before it sinks into the float's
        // floor. It may sit a few hundred hertz off the design point and the
        // fret-end interpolator adds its own few dB/s there (StratString.h),
        // so it is held to 4 dB/s of the table.
        int high = 1;
        while (partialHz(high + 1) < spec.highHz)
            ++high;
        double topHz = 0.0, topEarly = 0.0;
        for (int n = high - 3; n <= high; ++n)
        {
            const double fn = peakFrequency(y, rate, 0.1, 0.2, partialHz(n), 0.3 * spec.openHz);
            const double early = magnitude(y, rate, 0.1, 0.2, fn);
            if (early > topEarly)
            {
                topEarly = early;
                topHz = fn;
            }
        }
        const double topRate = (decibels(topEarly) - decibels(magnitude(y, rate, 0.7, 0.2, topHz))) / 0.6;
        const double topError = std::abs(topRate - 60.0 / spec.t60High);
        worstTop = topError > worstTop ? topError : worstTop;
    }
    std::printf("open-string T60s within %.1f %% of the table at the fundamental and eighth harmonic, "
                "the top within %.1f dB/s\n", 100.0 * worst, worstTop);
    check(worst < 0.1, "open strings decay as the table says");
    check(worstTop < 4.0, "the open strings' top decays as the table says");
}

// The two planes of the swing: the one along the top reflects as a string
// inPlaneShortening shorter, so every partial of it sits that share of the
// speaking length sharp, and it loses what the other loses each trip.
// The fret seen by a partial at f on a string of impedance z: the neck under
// it yields toward the poles, Y = G (w/Q) s / (s^2 + (w/Q) s + w^2) at
// s = j 2 pi f, and the end reflects -(1 - zY) / (1 + zY). Returned without
// the sign.
std::complex<double> fretReflection(double z, double f)
{
    const double w = 2.0 * pi * StratString::neckHz;
    const double band = w / StratString::neckQ;
    const std::complex<double> s(0.0, 2.0 * pi * f);
    const std::complex<double> y = StratString::neckConductance * band * s / (s * s + band * s + w * w);
    return (1.0 - z * y) / (1.0 + z * y);
}

// The yielding fret's pull on partial f of a note at f0, as a fraction of its
// frequency: the loop takes out the fret's phase delay at the fundamental, so
// only the difference is left, once a round trip.
double neckPull(double z, double f, double f0)
{
    const double residual = -std::arg(fretReflection(z, f)) / (2.0 * pi * f)
        + std::arg(fretReflection(z, f0)) / (2.0 * pi * f0);
    return 1.0 / (1.0 + residual * f0) - 1.0;
}

// What the yielding fret takes from partial f over the given seconds, dB.
double neckLoss(double z, double f, double f0, double seconds)
{
    return -decibels(std::abs(fretReflection(z, f))) * f0 * seconds;
}

void testSecondPlane()
{
    const double rate = 48000.0;
    double worstSplit = 0.0, worstDecay = 0.0;
    for (int s : {0, 3, 5})
        for (int fret : {0, 7, 12})
        {
            auto string = makeString(rate, s, fret);
            string->pluck(lightNewtons, pickMetres, 0.0002);
            std::vector<float> poles, top;
            std::array<float, StratString::tapCount> taps {};
            std::array<float, StratString::tapCount> alongTop {};
            for (int i = 0; i < static_cast<int>(2.4 * rate); ++i)
            {
                string->process(taps, alongTop);
                poles.push_back(taps[2]);
                top.push_back(alongTop[2]);
            }
            const double speaking = StratString::scaleMetres * std::pow(2.0, -fret / 12.0);
            const double want = StratString::inPlaneShortening / speaking;
            const double f0 = equalTempered(openNotes[s] + fret);
            const double b = StratString::construction(s).stiffness * std::pow(2.0, fret / 6.0);
            const double z = string->impedance();
            for (int n : {1, 3})
            {
                const double fn = n * f0 * std::sqrt((1.0 + b * n * n) / (1.0 + b));
                const double fp = peakFrequency(poles, rate, 0.2, 2.0, fn, 3.0);
                // Searched about where it should be: 4 Hz up at the 12th
                // fret's third partial on the high E. The fret yields only
                // under the swing toward the poles, which it pulls.
                const double split = (1.0 + want) * (1.0 - neckPull(z, fn, f0)) - 1.0;
                const double ft = peakFrequency(top, rate, 0.2, 2.0, fp * (1.0 + split), 1.0);
                const double error = std::abs((ft / fp - 1.0) / split - 1.0);
                worstSplit = error > worstSplit ? error : worstSplit;
                const double lostPoles = decibels(magnitude(poles, rate, 0.3, 0.5, fp))
                    - decibels(magnitude(poles, rate, 1.8, 0.5, fp));
                const double lostTop = decibels(magnitude(top, rate, 0.3, 0.5, ft))
                    - decibels(magnitude(top, rate, 1.8, 0.5, ft));
                const double d = std::abs(lostTop / (lostPoles - neckLoss(z, fp, f0, 1.5)) - 1.0);
                worstDecay = d > worstDecay ? d : worstDecay;
            }
        }
    std::printf("the in-plane swing: split within %.1f %% of its shortfall, decay within "
                "%.1f %%\n", 100.0 * worstSplit, 100.0 * worstDecay);
    check(worstSplit < 0.05, "the swing along the top sits sharp by its shortfall");
    check(worstDecay < 0.02, "the swing along the top decays as the other does, less the neck");
}

void testNeck()
{
    // Partials at the neck's resonance: what the poles' swing loses over
    // the top's is what the string equation gives a fret that yields.
    const double rate = 48000.0;
    struct Case { int string, fret, n; };
    double worst = 0.0, least = 1e9;
    for (const Case c : {Case {5, 5, 1}, Case {4, 9, 1}, Case {2, 7, 2}, Case {0, 9, 3}})
    {
        auto string = makeString(rate, c.string, c.fret);
        string->pluck(lightNewtons, pickMetres, 0.0002);
        std::vector<float> poles, top;
        std::array<float, StratString::tapCount> taps {};
        std::array<float, StratString::tapCount> alongTop {};
        for (int i = 0; i < static_cast<int>(2.4 * rate); ++i)
        {
            string->process(taps, alongTop);
            poles.push_back(taps[2]);
            top.push_back(alongTop[2]);
        }
        const double f0 = equalTempered(openNotes[c.string] + c.fret);
        const double b = StratString::construction(c.string).stiffness * std::pow(2.0, c.fret / 6.0);
        const double fn = c.n * f0 * std::sqrt((1.0 + b * c.n * c.n) / (1.0 + b));
        const double fp = peakFrequency(poles, rate, 0.2, 2.0, fn, 3.0);
        const double ft = peakFrequency(top, rate, 0.2, 2.0, fn, 3.0);
        const double extra = (decibels(magnitude(poles, rate, 0.3, 0.5, fp))
                              - decibels(magnitude(poles, rate, 1.8, 0.5, fp)))
            - (decibels(magnitude(top, rate, 0.3, 0.5, ft))
               - decibels(magnitude(top, rate, 1.8, 0.5, ft)));
        const double theory = neckLoss(string->impedance(), fp, f0, 1.5);
        std::printf("neck: string %d fret %d partial %d at %.0f Hz loses %.2f dB/s more toward "
                    "the poles, string theory %.2f\n",
                    c.string, c.fret, c.n, fp, extra / 1.5, theory / 1.5);
        const double e = std::abs(extra / theory - 1.0);
        worst = e > worst ? e : worst;
        least = theory < least ? theory : least;
    }
    check(least > 1.5 * 1.5, "the neck's resonance takes a dead note's sustain");
    check(worst < 0.05, "the fret yields as the string equation says");
}

// A share of each string's loss goes per second rather than per round trip,
// so an octave up the neck a partial loses 2 - a times what the open
// string's partial at the same frequency loses, not twice: open partial 10
// against partial 5 at fret 12, less what the yielding fret takes per trip.
void testPerSecondLoss()
{
    const double rate = 48000.0;
    double worst = 0.0;
    for (int s : {0, 2, 3, 5})
    {
        const auto& spec = StratString::construction(s);
        const double a = spec.wound ? StratString::woundPerSecondShare : StratString::plainPerSecondShare;
        double decay[2] {}, f = 0.0;
        for (int fret : {0, 12})
        {
            auto string = makeString(rate, s, fret);
            // 90 mm keeps both notes clear of a pick node at this partial.
            string->pluck(lightNewtons, 0.09, 0.0002);
            const auto y = run(*string, rate, 2.4, 2);
            const double f0 = equalTempered(openNotes[s] + fret);
            const int n = fret == 0 ? 10 : 5;
            const double b = spec.stiffness * std::pow(2.0, fret / 6.0);
            const double fn = peakFrequency(y, rate, n * f0 * std::sqrt((1.0 + b * n * n) / (1.0 + b)), 3.0);
            f = fn;
            const double lost = decibels(magnitude(y, rate, 0.3, 0.5, fn)) - decibels(magnitude(y, rate, 1.8, 0.5, fn));
            decay[fret == 0 ? 0 : 1] = lost / 1.5 - neckLoss(string->impedance(), fn, f0, 1.0);
        }
        const double ratio = decay[1] / decay[0];
        const double e = std::abs(ratio / (2.0 - a) - 1.0);
        std::printf("string %d at %.0f Hz: fret 12 loses %.2f times the open string, want %.2f\n", s, f, ratio, 2.0 - a);
        worst = e > worst ? e : worst;
    }
    check(worst < 0.05, "a fretted note loses the per-second share for as long as its trip");
}

void testNodes()
{
    const double rate = 48000.0;
    for (int s : {0, 5})
    {
        const double f0 = StratString::construction(s).openHz;
        const double b = StratString::construction(s).stiffness;
        const auto partial = [&](int n) {
            return n * f0 * std::sqrt((1.0 + b * n * n) / (1.0 + b));
        };
        auto string = makeString(rate, s, 0);
        string->pluck(lightNewtons, StratString::scaleMetres / 5.0, 0.00005);
        std::vector<float> neck, bridge;
        std::array<float, StratString::tapCount> taps {};
        std::array<float, StratString::tapCount> alongTop {};
        for (int i = 0; i < static_cast<int>(0.6 * rate); ++i)
        {
            string->process(taps, alongTop);
            neck.push_back(taps[0]);
            bridge.push_back(taps[2]);
        }
        const auto level = [&](const std::vector<float>& x, int n) {
            return decibels(magnitude(x, rate, 0.05, 0.5, partial(n)));
        };
        const double around5 = 0.5 * (level(bridge, 4) + level(bridge, 6));
        check(level(bridge, 5) < around5 - 30.0,
              "a pluck at L/5 leaves harmonic 5 out");
        const double around4 = 0.5 * (level(neck, 3) + level(neck, 6));
        check(level(neck, 4) < around4 - 30.0,
              "a pickup at L/4 does not hear harmonic 4");
    }
}

void testTouchHarmonic()
{
    // A finger resting at the 12th fret, lightly enough to let the string
    // ring on either side of it, is a node for the even harmonics only.
    const double rate = 48000.0;
    auto string = makeString(rate, 0, 0);
    string->pluck(1.5, pickMetres, 0.0002);
    string->setDamper(1, StratString::scaleMetres / 2.0, 50.0);
    const auto y = run(*string, rate, 1.2, 2);
    const double f0 = StratString::construction(0).openHz;
    const double b = StratString::construction(0).stiffness;
    const auto level = [&](int n) {
        return decibels(magnitude(y, rate, 0.6, 0.5,
                                  n * f0 * std::sqrt((1.0 + b * n * n) / (1.0 + b))));
    };
    check(level(1) < level(2) - 40.0 && level(3) < level(2) - 40.0,
          "a touch at the 12th fret leaves the octave harmonic");
}

// Expected decay rate of the fundamental from a dashpot alone: the complex
// root k near pi/L of Z sin(kL) + jR sin(k x) sin(k (L - x)) = 0, the ideal
// string fixed at both ends with the dashpot's force jump at x. The amplitude
// falls as exp(-Im(k) c t).
double dashpotDecayPerSecond(double impedance, double resistance, double x,
                             double length, double waveSpeed)
{
    using Complex = std::complex<double>;
    const Complex j(0.0, 1.0);
    Complex k = pi / length;
    for (int i = 0; i < 40; ++i)
    {
        const Complex f = impedance * std::sin(k * length)
            + j * resistance * std::sin(k * x) * std::sin(k * (length - x));
        const Complex df = impedance * length * std::cos(k * length)
            + j * resistance
                * (x * std::cos(k * x) * std::sin(k * (length - x))
                   + (length - x) * std::sin(k * x) * std::cos(k * (length - x)));
        k -= f / df;
    }
    return k.imag() * waveSpeed;
}

void testPalm()
{
    const double rate = 48000.0;
    const auto& spec = StratString::construction(0);
    const double waveSpeed = 2.0 * StratString::scaleMetres * spec.openHz;
    const double impedance = spec.kilogramsPerMetre * waveSpeed, sample = waveSpeed / rate;
    const auto decayDbPerSecond = [&](double x, double resistance, double width) {
        auto string = makeString(rate, 0, 0);
        string->setDamper(0, x, resistance, width);
        string->pluck(lightNewtons, pickMetres, 0.0002);
        const auto y = run(*string, rate, 2.4, 2);
        return (decibels(magnitude(y, rate, 0.3, 0.5, spec.openHz))
                - decibels(magnitude(y, rate, 1.8, 0.5, spec.openHz)))
            / 1.5;
    };
    const double undamped = decayDbPerSecond(0.0, 0.0, 0.0);
    // A dashpot near the bridge, on a sample point and between two.
    for (double samples : {7.0, 7.4})
        for (double resistance : {0.1, 0.3, 1.0})
        {
            const double x = samples * sample;
            const double extra = decayDbPerSecond(x, resistance, 0.0) - undamped;
            const double want = 20.0 / std::log(10.0)
                * dashpotDecayPerSecond(impedance, resistance, x, StratString::scaleMetres,
                                        waveSpeed);
            std::printf("dashpot %.1f kg/s at %.1f mm: %.3f dB/s, string theory %.3f dB/s\n",
                        resistance, 1000.0 * x, extra, want);
            check(std::abs(extra / want - 1.0) < 0.01,
                  "a dashpot on the string damps it as the string equation says");
        }
    // A light contact nine samples wide damps as its nine points would, each
    // with a ninth of its resistance.
    const double resistance = 0.3;
    double want = 0.0;
    for (int k = 16; k <= 24; ++k)
        want += 20.0 / std::log(10.0)
            * dashpotDecayPerSecond(impedance, resistance / 9.0, k * sample,
                                    StratString::scaleMetres, waveSpeed);
    const double extra = decayDbPerSecond(20.0 * sample, resistance, 9.0 * sample) - undamped;
    std::printf("contact %.1f kg/s over %.1f mm: %.3f dB/s, its points %.3f dB/s\n", resistance,
                9000.0 * sample, extra, want);
    check(std::abs(extra / want - 1.0) < 0.01, "a contact damps as the points it covers");
}

void testTensionModulation()
{
    // A swinging string is stretched: with energy E its tension rises by
    // EA E / (2 L T) and its pitch by EA E / (4 L T^2), so a hard pluck
    // starts sharp and settles as it decays (Legge and Fletcher, JASA 76,
    // 1984). Here EA is the steel's 207 GPa over the area pi d^2 / 4 of a
    // 0.010 in high E and of the low E's 0.0185 in core (its wrap carries no
    // tension), and E the energy of the triangle a pluck of force F at x
    // releases, F^2 x (L - x) / (2 T L), let go at once so all of it is in
    // the string.
    const double rate = 48000.0, x = pickMetres, length = StratString::scaleMetres;
    const std::pair<int, double> strings[] = {{0, 0.0185}, {5, 0.010}};
    for (const auto& [s, inches] : strings)
    {
        const auto& spec = StratString::construction(s);
        const double tension = spec.kilogramsPerMetre * std::pow(2.0 * length * spec.openHz, 2.0);
        const double diameter = inches * 0.0254;
        const double stretchStiffness = 207e9 * pi * diameter * diameter / 4.0;
        const auto sharpening = [&](double newtons, double startSeconds) {
            const auto pitch = [&](double force) {
                auto string = makeString(rate, s, 0);
                string->pluck(force, x, 0.0);
                const auto y = run(*string, rate, startSeconds + 0.1, 2);
                return peakFrequency(y, rate, startSeconds, 0.1, spec.openHz, 3.0);
            };
            return cents(pitch(newtons), pitch(lightNewtons));
        };
        const auto theory = [&](double newtons) {
            const double energy = newtons * newtons * x * (length - x) / (2.0 * tension * length);
            return cents(1.0 + stretchStiffness * energy / (4.0 * length * tension * tension), 1.0);
        };
        const double hard = sharpening(2.0, 0.02), soft = sharpening(1.0, 0.02);
        const double late = sharpening(2.0, 1.5);
        std::printf("string %d at 2 N: %.2f cents sharp over 20-120 ms (theory %.2f at release), "
                    "%.2f at 1 N, %.2f after 1.5 s\n",
                    s, hard, theory(2.0), soft, late);
        // By the window's middle the string has lost 5 to 12 % of its energy,
        // the high E's upper partials fastest, so it is a little under.
        check(hard < theory(2.0) && hard > 0.85 * theory(2.0),
              "a hard pluck starts as sharp as its stretch says");
        check(std::abs(hard / soft - 4.0) < 0.2, "the sharpening goes as the energy");
        check(late < 0.5 * hard, "the pitch settles as the string decays");
    }
}

double peakAbs(const std::vector<float>& x)
{
    double peak = 0.0;
    for (float v : x)
        peak = std::abs(v) > peak ? std::abs(v) : peak;
    return peak;
}

void testBendAndGlide()
{
    const double rate = 48000.0;
    for (int s : {0, 3, 5})
    {
        // A whole-tone bend: tension up by 2^(4/12).
        auto string = makeString(rate, s, 7);
        string->pluck(lightNewtons, pickMetres, 0.0002);
        std::array<float, StratString::tapCount> taps {};
        std::array<float, StratString::tapCount> alongTop {};
        const double ratio = std::pow(2.0, 4.0 / 12.0);
        for (int i = 0; i < 4800; ++i)
        {
            if (i % StratString::controlPeriod == 0)
                string->setTuning(7, 1.0 + (ratio - 1.0) * i / 4800.0);
            string->process(taps, alongTop);
        }
        string->setTuning(7, ratio);
        const auto bent = run(*string, rate, 0.7, 2);
        const double want = equalTempered(openNotes[s] + 9);
        check(std::abs(cents(peakFrequency(bent, rate, want, 3.0), want)) < 0.5,
              "a whole-tone bend lands a whole tone up");

        // A slide from the 2nd to the 9th fret over a quarter second.
        string = makeString(rate, s, 2);
        string->pluck(lightNewtons, pickMetres, 0.0002);
        double before = 0.0;
        std::vector<float> slid;
        for (int i = 0; i < 12000; ++i)
        {
            if (i % StratString::controlPeriod == 0)
                string->setTuning(2.0 + 7.0 * i / 12000.0, 1.0, true);
            string->process(taps, alongTop);
            if (i < 2400)
                before = std::abs(taps[2]) > before ? std::abs(taps[2]) : before;
            slid.push_back(taps[2]);
        }
        string->setTuning(9.0, 1.0, true);
        const auto after = run(*string, rate, 0.7, 2);
        check(std::abs(cents(peakFrequency(after, rate, want, 3.0), want)) < 0.5,
              "a slide to the 9th fret is in tune there");
        check(peakAbs(slid) < 1.5 * before, "a slide adds no energy");
    }
}

void testFretJumpsStayBounded()
{
    const double rate = 44100.0;
    unsigned seed = 2024u;
    const auto next = [&seed] {
        seed = seed * 1664525u + 1013904223u;
        return (seed >> 8) * (1.0 / 16777216.0);
    };
    bool finite = true;
    double peak = 0.0;
    for (int s = 0; s < StratString::stringCount; ++s)
    {
        auto string = makeString(rate, s, 0);
        std::array<float, StratString::tapCount> taps {};
        std::array<float, StratString::tapCount> alongTop {};
        double fret = 0.0, tension = 1.0;
        for (int i = 0; i < static_cast<int>(6.0 * rate); ++i)
        {
            if (i % StratString::controlPeriod == 0)
            {
                const double r = next();
                if (r < 0.02)
                    fret = std::floor(next() * 22.0);
                if (r > 0.97)
                    tension = 0.5 + next() * 0.9;
                string->setTuning(fret, tension, r < 0.01);
                if (r > 0.995)
                    string->pluck(1.5, 0.05 + next() * 0.15, 0.0002);
                if (r > 0.4 && r < 0.402)
                    string->setDamper(0, 0.01 + next() * 0.05, next() * 2.0);
            }
            string->process(taps, alongTop);
            for (int k = 0; k < StratString::tapCount; ++k)
                for (float v : {taps[static_cast<size_t>(k)], alongTop[static_cast<size_t>(k)]})
                {
                    finite = finite && std::isfinite(v);
                    peak = std::abs(v) > peak ? std::abs(v) : peak;
                }
        }
    }
    check(finite, "frets, bends and plucks keep the string finite");
    // A 1.5 N release sends at most about 3 m/s on the thinnest string, and
    // overlapping plucks add; 40 m/s rules out runaway.
    check(peak < 40.0, "frets, bends and plucks keep the string bounded");
}

void testPickContact()
{
    // Until its reflections return, the string meets the pick as 2Z, so the
    // hand driving it at V through the grip's spring k moves the pick point at
    // V (1 - exp(-t / tau)), tau = 2Z / k. The force's 5 Hz DC blocker
    // (tau_b) leaks a little of it: V tau' / tau (1 - exp(-t / tau')) with
    // 1 / tau' = 1 / tau + 1 / tau_b. The neck tap hears that once the wave
    // has crossed to it. A slip force far above the push keeps it held.
    const double rate = 48000.0, speed = 1.0;
    auto string = makeString(rate, 0, 0.0);
    const double tau = 2.0 * string->impedance() / StratString::contactStiffness;
    const double leaky = 1.0 / (1.0 / tau + 2.0 * pi * 5.0);
    const double waveSpeed = 2.0 * StratString::scaleMetres * StratString::construction(0).openHz;
    const double travel = (neckMetres - pickMetres) / waveSpeed;
    const double returns = 2.0 * pickMetres / waveSpeed + travel;
    string->pluck(100.0, pickMetres, 0.0002, speed);
    const std::vector<float> held = run(*string, rate, returns, 0);
    double worst = 0.0;
    for (int i = static_cast<int>((travel + 0.0002) * rate); i < static_cast<int>(held.size()); ++i)
    {
        const double t = (i + 1) / rate - travel;
        const double want = speed * leaky / tau * (1.0 - std::exp(-t / leaky));
        worst = std::max(worst, std::abs(held[static_cast<size_t>(i)] - want));
    }
    check(worst < 0.005 * speed, "the pick drives the string through its grip");

    // The pick lets go once the string pushes back with the slip force, and
    // the string then sounds as a pluck of that force does: within 3 dB over
    // 20 to 60 ms, for a slow stroke and one as fast as the performer's, on
    // the heaviest and the lightest string. The push comes first and has the
    // stroke's sign.
    const auto rms = [rate](const std::vector<float>& x) {
        double sum = 0.0;
        for (size_t i = static_cast<size_t>(0.02 * rate); i < x.size(); ++i)
            sum += static_cast<double>(x[i]) * x[i];
        return std::sqrt(sum / (x.size() - static_cast<size_t>(0.02 * rate)));
    };
    for (int s : {0, 5})
    {
        auto plain = makeString(rate, s, 0.0);
        plain->pluck(2.5, pickMetres, 0.0002);
        const double reference = rms(run(*plain, rate, 0.06, 0));
        for (double stroke : {0.5, 1.5 * 2.5 / (2.0 * plain->impedance())})
        {
            auto pushed = makeString(rate, s, 0.0);
            pushed->pluck(2.5, pickMetres, 0.0002, stroke);
            const std::vector<float> out = run(*pushed, rate, 0.06, 0);
            // A tenth of the stroke's speed clears the interpolators' ripple.
            const auto first = std::find_if(out.begin(), out.end(), [stroke](float v) {
                return std::abs(v) > 0.1 * stroke;
            });
            check(first != out.end() && *first > 0.0f, "the push has the stroke's sign");
            check(std::abs(decibels(rms(out) / reference)) < 3.0,
                  "after the push the string sounds as plucked at the slip force");
        }
    }
}

void testDeterminism()
{
    const auto render = [] {
        auto string = makeString(48000.0, 2, 3);
        string->pluck(1.2, pickMetres, 0.0003);
        return run(*string, 48000.0, 0.5, 1);
    };
    check(render() == render(), "the same input gives the same bits");
}

} // namespace

int main()
{
    testTuningAndStiffness(44100.0);
    testTuningAndStiffness(48000.0);
    testTuningAndStiffness(96000.0);
    testDecay();
    testSecondPlane();
    testNeck();
    testPerSecondLoss();
    testNodes();
    testTouchHarmonic();
    testPalm();
    testTensionModulation();
    testBendAndGlide();
    testFretJumpsStayBounded();
    testPickContact();
    testDeterminism();
    if (failures == 0)
        std::printf("StratString: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
