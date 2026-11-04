// The Stratocaster circuit against independent circuit math. The expected
// response is the analog network solved by complex impedances (each coil as a
// Thevenin source, node X by Millman's theorem, the Volume pot as a loaded
// divider), not by the engine's state-space form. Ramp invariance makes the
// discrete response the analog response seen through the linear-interpolation
// kernel, summed over its spectral images:
//
//   H(e^jwT) = sum_k Ha(j(w + k ws)) sinc^2((w + k ws) T / 2).
#include "DSP/StratCircuit.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>

namespace
{
using electry::StratCircuit;
using Complex = std::complex<double>;
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

// The wiring as the guitar has it, written out independently of the engine:
// which coils and which tones each switch position joins to the Volume pot.
struct Selection
{
    bool coil[3]; // neck, middle, bridge
    bool tone[2]; // Tone 1, Tone 2
};

Selection selectionFor(Position position)
{
    switch (position)
    {
        case Position::bridge: return {{false, false, true}, {false, false}};
        case Position::bridgeMiddle: return {{false, true, true}, {false, true}};
        case Position::middle: return {{false, true, false}, {false, true}};
        case Position::middleNeck: return {{true, true, false}, {true, true}};
        case Position::neck: return {{true, false, false}, {true, false}};
    }
    return {};
}

// Analog transfer from coil `input`'s induced voltage to the jack, with the
// contact and pot-end resistances given explicitly so the ideal 0 Ohm network
// can be compared.
Complex analogResponse(const StratCircuit::Controls& controls, int input,
                       Complex s,
                       double contactOhms = StratCircuit::contactOhms,
                       double potEndOhms = StratCircuit::potEndOhms)
{
    const Selection selection = selectionFor(controls.position);
    if (!selection.coil[input])
        return 0.0;

    const Complex coilSeries = StratCircuit::coilOhms
        + s * StratCircuit::coilHenries;
    const Complex winding = 1.0 / (s * StratCircuit::coilFarads);
    const Complex theveninImpedance
        = coilSeries * winding / (coilSeries + winding);
    const Complex theveninGain = winding / (coilSeries + winding);

    const double volume = StratCircuit::audioTaper(controls.volume);
    const double top = (1.0 - volume) * StratCircuit::potOhms + potEndOhms;
    const double bottom = volume * StratCircuit::potOhms + potEndOhms;
    const Complex jack = 1.0
        / (s * StratCircuit::cableFarads + 1.0 / bottom
           + 1.0 / StratCircuit::ampInputOhms);

    // Millman: node X = sum(source admittance * source) / sum(admittance).
    Complex admittance = 1.0 / (top + jack);
    Complex sourceCurrent = 0.0;
    for (int k = 0; k < 3; ++k)
        if (selection.coil[k])
        {
            const Complex branch
                = 1.0 / (theveninImpedance + contactOhms);
            admittance += branch;
            if (k == input)
                sourceCurrent += branch * theveninGain;
        }
    const double toneRotation[2] = {controls.toneNeck, controls.toneMiddle};
    for (int t = 0; t < 2; ++t)
        if (selection.tone[t])
            admittance += 1.0
                / (contactOhms
                   + StratCircuit::audioTaper(toneRotation[t])
                       * StratCircuit::potOhms
                   + 1.0 / (s * StratCircuit::toneFarads));
    const Complex nodeX = sourceCurrent / admittance;
    return nodeX * jack / (top + jack);
}

Complex expectedDiscrete(const StratCircuit::Controls& controls, int input,
                         double frequency, double sampleRate)
{
    const double omega = 2.0 * pi * frequency;
    const double omegaS = 2.0 * pi * sampleRate;
    Complex sum = 0.0;
    for (int k = -400; k <= 400; ++k)
    {
        const double w = omega + k * omegaS;
        const double x = w / (2.0 * sampleRate);
        const double kernel = std::sin(x) / x;
        sum += analogResponse(controls, input, Complex(0.0, w)) * kernel
            * kernel;
    }
    return sum;
}

std::vector<double> impulseResponse(const StratCircuit::Controls& controls,
                                    int input, double sampleRate, int length)
{
    StratCircuit circuit;
    circuit.setControls(controls);
    circuit.prepare(sampleRate);
    std::vector<double> response(static_cast<size_t>(length));
    for (int i = 0; i < length; ++i)
    {
        double e[3] = {0.0, 0.0, 0.0};
        e[input] = i == 0 ? 1.0 : 0.0;
        response[static_cast<size_t>(i)] = circuit.process(e[0], e[1], e[2]);
    }
    return response;
}

Complex dtft(const std::vector<double>& x, double frequency, double sampleRate)
{
    Complex sum = 0.0;
    const double step = -2.0 * pi * frequency / sampleRate;
    for (size_t i = 0; i < x.size(); ++i)
        sum += x[i] * std::polar(1.0, step * static_cast<double>(i));
    return sum;
}

void testTaper()
{
    check(StratCircuit::audioTaper(0.0) == 0.0, "taper is 0 at knob 0");
    check(std::abs(StratCircuit::audioTaper(1.0) - 1.0) < 1e-15,
          "taper is the whole track at knob 10");
    check(std::abs(StratCircuit::audioTaper(0.5) - 0.1) < 1e-12,
          "audio taper is 10 % at half rotation");
    double previous = -1.0;
    bool monotonic = true;
    for (int i = 0; i <= 100; ++i)
    {
        const double value = StratCircuit::audioTaper(i / 100.0);
        monotonic = monotonic && value > previous;
        previous = value;
    }
    check(monotonic, "taper rises with rotation");
}

void testAgainstAnalogMath(double sampleRate)
{
    const Position positions[] = {Position::bridge, Position::bridgeMiddle,
                                  Position::middle, Position::middleNeck,
                                  Position::neck};
    struct Knobs
    {
        double volume, toneNeck, toneMiddle;
    };
    const Knobs knobs[] = {{1.0, 1.0, 1.0}, {0.7, 0.5, 1.0},
                           {0.4, 1.0, 0.0}, {1.0, 0.0, 0.3},
                           {0.05, 0.8, 0.6}};
    const double frequencies[] = {40.0,   120.0,  300.0,  700.0,
                                  1500.0, 3000.0, 4500.0, 6000.0,
                                  8000.0, 11000.0, 16000.0};
    // Slowest pole: a Tone pot at 10 with its 0.1 uF, 25 ms. Two seconds
    // leaves its tail below double precision.
    const int length = static_cast<int>(2.0 * sampleRate);

    double worst = 0.0;
    for (Position position : positions)
        for (const Knobs& k : knobs)
        {
            StratCircuit::Controls controls;
            controls.position = position;
            controls.volume = k.volume;
            controls.toneNeck = k.toneNeck;
            controls.toneMiddle = k.toneMiddle;
            for (int input = 0; input < 3; ++input)
            {
                const auto response
                    = impulseResponse(controls, input, sampleRate, length);
                const bool connected = selectionFor(position).coil[input];
                for (double f : frequencies)
                {
                    if (f >= 0.45 * sampleRate)
                        continue;
                    const Complex got = dtft(response, f, sampleRate);
                    if (!connected)
                    {
                        check(std::abs(got) == 0.0,
                              "a coil the switch leaves out reaches nothing");
                        continue;
                    }
                    const Complex want
                        = expectedDiscrete(controls, input, f, sampleRate);
                    const double error = std::abs(got - want) / std::abs(want);
                    worst = error > worst ? error : worst;
                }
            }
        }
    std::printf("%.0f Hz: worst relative error against the analog network "
                "%.3g\n",
                sampleRate, worst);
    check(worst < 1e-6, "discrete circuit matches the analog network");
}

// The 1 Ohm contacts and pot ends are a numerical convention; against an
// ideal 0 Ohm network they must not be audible anywhere in the band, at the
// knob ends where they matter most included. They measure 0.0043 dB at worst;
// 0.01 dB is two orders under any audible level difference.
void testConventionsAreInaudible()
{
    double worstDb = 0.0;
    for (int p = 0; p < 5; ++p)
        for (double tone : {1.0, 0.0})
        {
            StratCircuit::Controls controls;
            controls.position = static_cast<Position>(p);
            controls.volume = 1.0;
            controls.toneNeck = tone;
            controls.toneMiddle = tone;
            const Selection selection = selectionFor(controls.position);
            for (int input = 0; input < 3; ++input)
            {
                if (!selection.coil[input])
                    continue;
                for (double f = 20.0; f < 20000.0; f *= 1.25)
                {
                    const Complex s(0.0, 2.0 * pi * f);
                    const Complex real = analogResponse(controls, input, s);
                    const Complex ideal
                        = analogResponse(controls, input, s, 0.0, 0.0);
                    const double db
                        = std::abs(20.0 * std::log10(std::abs(real / ideal)));
                    worstDb = db > worstDb ? db : worstDb;
                }
            }
        }
    std::printf("convention resistances move the response by at most "
                "%.2g dB\n",
                worstDb);
    check(worstDb < 0.01, "convention resistances are inaudible");
}

// Knob and switch moves keep every state physical: the output stays finite
// and bounded while all controls are swept under a full-scale input.
void testMovingControls()
{
    StratCircuit circuit;
    circuit.prepare(44100.0);
    StratCircuit::Controls controls;
    double peak = 0.0;
    bool finite = true;
    unsigned seed = 12345u;
    const auto next = [&seed] {
        seed = seed * 1664525u + 1013904223u;
        return (seed >> 8) * (1.0 / 16777216.0);
    };
    for (int i = 0; i < 44100 * 4; ++i)
    {
        if (i % 64 == 0)
        {
            controls.volume = next();
            controls.toneNeck = next();
            controls.toneMiddle = next();
            if (i % 4096 == 0)
                controls.position = static_cast<Position>(
                    static_cast<int>(next() * 5.0) % 5);
            circuit.setControls(controls);
        }
        const double e = std::sin(2.0 * pi * 330.0 * i / 44100.0);
        const double y = circuit.process(e, -0.7 * e, 0.4 * e);
        finite = finite && std::isfinite(y);
        peak = std::abs(y) > peak ? std::abs(y) : peak;
    }
    check(finite, "output stays finite while controls move");
    // A passive network cannot gain more than its resonance allows; a 1 V
    // drive staying under 4 V rules out runaway.
    check(peak < 4.0, "output stays bounded while controls move");
}

void testDeterminism()
{
    StratCircuit::Controls controls;
    controls.position = Position::middleNeck;
    controls.volume = 0.8;
    const auto first = impulseResponse(controls, 1, 48000.0, 4096);
    const auto second = impulseResponse(controls, 1, 48000.0, 4096);
    check(first == second, "the same input gives the same bits");
}

} // namespace

int main()
{
    testTaper();
    testAgainstAnalogMath(44100.0);
    testAgainstAnalogMath(48000.0);
    testAgainstAnalogMath(96000.0);
    testConventionsAreInaudible();
    testMovingControls();
    testDeterminism();
    if (failures == 0)
        std::printf("StratCircuit: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
