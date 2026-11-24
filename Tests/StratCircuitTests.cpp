// The Stratocaster and Telecaster circuits against independent circuit math. The expected
// response is the analog network solved by complex impedances (each coil as a
// Thevenin source, node X by Millman's theorem, the Volume pot as a loaded
// divider), not by the engine's state-space form. Ramp invariance makes the
// discrete response the analog response seen through the linear-interpolation
// kernel, summed over its spectral images:
//
//   H(e^jwT) = sum_k Ha(j(w + k ws)) sinc^2((w + k ws) T / 2).
#include "DSP/StratCircuit.h"

#include <algorithm>
#include <cmath>
#include <iterator>
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

Selection selectionFor(StratCircuit::Guitar guitar, Position position)
{
    if (guitar == StratCircuit::Guitar::tele)
        switch (position)
        {
            case Position::bridge: return {{false, false, true}, {true, false}};
            case Position::bridgeNeck: return {{true, false, true}, {true, false}};
            case Position::neck: return {{true, false, false}, {true, false}};
            default: return {};
        }
    switch (position)
    {
        case Position::bridge: return {{false, false, true}, {false, false}};
        case Position::bridgeMiddle: return {{false, true, true}, {false, true}};
        case Position::middle:
        case Position::bridgeNeck: return {{false, true, false}, {false, true}};
        case Position::middleNeck: return {{true, true, false}, {true, true}};
        case Position::neck: return {{true, false, false}, {true, false}};
    }
    return {};
}

Selection selectionFor(const StratCircuit::Controls& controls)
{
    return selectionFor(controls.guitar, controls.position);
}

struct CoilValues
{
    double ohms, henries, farads, eddyOhms;
};

CoilValues coilValues(StratCircuit::Guitar guitar, int coil)
{
    if (guitar == StratCircuit::Guitar::tele && coil == 0)
        return {StratCircuit::teleNeckOhms, StratCircuit::teleNeckHenries,
                StratCircuit::teleNeckFarads, StratCircuit::teleNeckEddyOhms};
    if (guitar == StratCircuit::Guitar::tele && coil == 2)
        return {StratCircuit::teleBridgeOhms, StratCircuit::teleBridgeHenries,
                StratCircuit::teleBridgeFarads, StratCircuit::teleBridgeEddyOhms};
    return {StratCircuit::coilOhms, StratCircuit::coilHenries, StratCircuit::coilFarads,
            StratCircuit::eddyOhms};
}

// Analog transfer from coil `input`'s induced voltage to the jack, with the
// contact and pot-end resistances given explicitly so the ideal 0 Ohm network
// can be compared.
Complex analogResponse(const StratCircuit::Controls& controls, int input,
                       Complex s,
                       double contactOhms = StratCircuit::contactOhms,
                       double potEndOhms = StratCircuit::potEndOhms)
{
    const Selection selection = selectionFor(controls);
    if (!selection.coil[input])
        return 0.0;
    const double toneFarads = controls.guitar == StratCircuit::Guitar::tele
        ? StratCircuit::teleToneFarads : StratCircuit::toneFarads;

    const double volume = StratCircuit::audioTaper(controls.volume);
    const double top = (1.0 - volume) * StratCircuit::potOhms + potEndOhms;
    const double bottom = volume * StratCircuit::potOhms + potEndOhms;
    const Complex jack = 1.0
        / (s * StratCircuit::cableFarads + 1.0 / bottom
           + 1.0 / StratCircuit::ampInputOhms);

    // Millman: node X = sum(source admittance * source) / sum(admittance),
    // each coil a Thevenin source.
    Complex admittance = 1.0 / (top + jack);
    Complex sourceCurrent = 0.0;
    for (int k = 0; k < 3; ++k)
        if (selection.coil[k])
        {
            const CoilValues c = coilValues(controls.guitar, k);
            const Complex coilSeries = c.ohms + s * c.henries;
            const Complex winding = 1.0 / (s * c.farads + 1.0 / c.eddyOhms);
            const Complex theveninImpedance = coilSeries * winding / (coilSeries + winding);
            const Complex theveninGain = winding / (coilSeries + winding);
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
                   + 1.0 / (s * toneFarads));
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
                                  Position::neck, Position::bridgeNeck};
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

    const Position telePositions[] = {Position::bridge, Position::bridgeNeck, Position::neck};
    double worst = 0.0;
    for (const auto guitar : {StratCircuit::Guitar::strat, StratCircuit::Guitar::tele})
    for (Position position : positions)
        for (const Knobs& k : knobs)
        {
            if (guitar == StratCircuit::Guitar::tele
                && std::find(std::begin(telePositions), std::end(telePositions), position)
                    == std::end(telePositions))
                continue;
            StratCircuit::Controls controls;
            controls.guitar = guitar;
            controls.position = position;
            controls.volume = k.volume;
            controls.toneNeck = k.toneNeck;
            controls.toneMiddle = k.toneMiddle;
            for (int input = 0; input < 3; ++input)
            {
                const auto response
                    = impulseResponse(controls, input, sampleRate, length);
                const bool connected = selectionFor(controls).coil[input];
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
            const Selection selection = selectionFor(controls);
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

// A pickup alone on the bench, as Zollner measured them ("Physics of the
// Electric Guitar", 5.15): its wire and inductance in series, and across its
// terminals its winding capacitance, its loss and whatever loads it.
struct BenchCoil
{
    double ohms, henries, farads, lossOhms;
};

// The peak of the impedance at its terminals with loadFarads across them,
// dB re 1 kOhm.
double impedancePeakDb(const BenchCoil& coil, double loadFarads)
{
    double peak = 0.0;
    for (double f = 1000.0; f < 20000.0; f += 1.0)
    {
        const Complex s(0.0, 2.0 * pi * f);
        const Complex admittance = 1.0 / coil.lossOhms + s * (coil.farads + loadFarads)
            + 1.0 / (coil.ohms + s * coil.henries);
        peak = std::max(peak, 1.0 / std::abs(admittance));
    }
    return 20.0 * std::log10(peak / 1000.0);
}

// The peak of the voltage it gives into loadOhms and loadFarads, over what
// it gives at low frequency, dB.
double loadedPeakDb(const BenchCoil& coil, double loadOhms, double loadFarads)
{
    const auto volts = [&](double f) {
        const Complex s(0.0, 2.0 * pi * f);
        return std::abs(1.0 / (1.0 + (coil.ohms + s * coil.henries)
                                         * (1.0 / coil.lossOhms + 1.0 / loadOhms
                                            + s * (coil.farads + loadFarads))));
    };
    double peak = 0.0;
    for (double f = 1000.0; f < 20000.0; f += 1.0)
        peak = std::max(peak, volts(f));
    return 20.0 * std::log10(peak / volts(20.0));
}

// The coils' losses against the pickups they were read from, each sheet's
// curves traced from its figure. An earlier check held the Strat's loss to
// 2.5 to 5 dB off the un-toned bridge's peak, which is what 220 kOhm did and
// no measurement; these hold the losses to Zollner's.
void testCoilLossesAgainstMeasurements()
{
    // The sheets load each pickup as a guitar does: both pots and the amp.
    const double loadOhms = 1.0 / (2.0 / StratCircuit::potOhms + 1.0 / StratCircuit::ampInputOhms);

    // His Strat-72 with the Strat's loss: the impedance peaks unloaded and
    // with 450 and 750 pF, and the loaded peaks with those two cables.
    const BenchCoil strat {5880.0, 2.19, 133e-12, StratCircuit::eddyOhms};
    const double stratImpedance[3] = {impedancePeakDb(strat, 0.0), impedancePeakDb(strat, 450e-12),
                                      impedancePeakDb(strat, 750e-12)};
    const double stratLoaded[2] = {loadedPeakDb(strat, loadOhms, 450e-12),
                                   loadedPeakDb(strat, loadOhms, 750e-12)};
    std::printf("Strat-72 with the Strat's loss: impedance peaks %.2f, %.2f, %.2f dB re 1 kOhm "
                "(sheet 57.38, 51.88, 49.56); loaded peaks %.2f, %.2f dB (sheet 3.76, 4.75)\n",
                stratImpedance[0], stratImpedance[1], stratImpedance[2], stratLoaded[0],
                stratLoaded[1]);
    check(std::abs(stratImpedance[0] - 57.38) < 0.15 && std::abs(stratImpedance[1] - 51.88) < 0.15
              && std::abs(stratImpedance[2] - 49.56) < 0.15,
          "the Strat coil's loss gives the measured Strat's impedance peaks");
    check(std::abs(stratLoaded[0] - 3.76) < 0.15 && std::abs(stratLoaded[1] - 4.75) < 0.15,
          "the Strat coil's loss gives the measured Strat's loaded peaks");

    // His Tele-52 neck, under its cover, with the Tele neck's loss.
    const BenchCoil teleNeck {7630.0, 2.45, StratCircuit::teleNeckFarads,
                              StratCircuit::teleNeckEddyOhms};
    const double neckImpedance[3] = {impedancePeakDb(teleNeck, 0.0),
                                     impedancePeakDb(teleNeck, 450e-12),
                                     impedancePeakDb(teleNeck, 750e-12)};
    std::printf("Tele-52 neck with the Tele neck's loss: impedance peaks %.2f, %.2f, %.2f dB re "
                "1 kOhm (sheet 49.08, 45.92, 44.49)\n",
                neckImpedance[0], neckImpedance[1], neckImpedance[2]);
    check(std::abs(neckImpedance[0] - 49.08) < 0.25 && std::abs(neckImpedance[1] - 45.92) < 0.25
              && std::abs(neckImpedance[2] - 44.49) < 0.25,
          "the Tele neck coil's loss gives the measured covered coil's impedance peaks");

    // The Tele's bridge coil: its plate takes about 1 dB off the loaded peak
    // its rods alone would leave (5.1, Fig. 5.1.4).
    for (const double cable : {450e-12, 700e-12})
    {
        BenchCoil bridge {StratCircuit::teleBridgeOhms, StratCircuit::teleBridgeHenries,
                          StratCircuit::teleBridgeFarads, StratCircuit::teleBridgeEddyOhms};
        const double plated = loadedPeakDb(bridge, loadOhms, cable);
        bridge.lossOhms = StratCircuit::eddyOhms;
        const double bare = loadedPeakDb(bridge, loadOhms, cable);
        std::printf("Tele bridge coil with %.0f pF: its plate takes %.2f dB off the loaded peak\n",
                    cable * 1e12, bare - plated);
        check(std::abs(bare - plated - 1.0) < 0.15, "the Tele bridge coil's plate takes 1 dB off its peak");
    }
}

// The Strat's positions on a Telecaster play its 3-way on their side of the
// switch, and its own both-coils position on a Strat plays the middle coil.
void testSwitchMapping()
{
    const auto same = [](StratCircuit::Guitar g, Position a, Position b) {
        StratCircuit::Controls x, y;
        x.guitar = y.guitar = g;
        x.position = a;
        y.position = b;
        for (int input = 0; input < 3; ++input)
            if (impulseResponse(x, input, 48000.0, 2048) != impulseResponse(y, input, 48000.0, 2048))
                return false;
        return true;
    };
    using G = StratCircuit::Guitar;
    check(same(G::tele, Position::bridgeMiddle, Position::bridge), "Tele: position 2 of 5 is the bridge");
    check(same(G::tele, Position::middle, Position::bridgeNeck), "Tele: the middle is both coils");
    check(same(G::tele, Position::middleNeck, Position::neck), "Tele: position 4 of 5 is the neck");
    check(same(G::strat, Position::bridgeNeck, Position::middle), "Strat: no neck-and-bridge position");
}

// The Tele's bridge coil, at 3.8 H against the neck's 2.4, is the darker of
// the two at the jack with every knob at 10: the master Tone's 250 kOhm hangs
// on it in every position, which leaves neither coil much of a peak, so
// compare where each falls 3 dB under its level at 500 Hz. Its master Tone
// at 0 cuts 4 kHz on every position.
void testTeleVoicing()
{
    StratCircuit::Controls controls;
    controls.guitar = StratCircuit::Guitar::tele;
    const auto cornerHz = [&](Position position, int input) {
        controls.position = position;
        controls.toneNeck = 1.0;
        const auto gain = [&](double f) {
            return std::abs(analogResponse(controls, input, Complex(0.0, 2.0 * pi * f)));
        };
        const double reference = gain(500.0);
        double f = 500.0;
        while (f < 20000.0 && gain(f) > reference / std::sqrt(2.0))
            f *= 1.005;
        return f;
    };
    const double bridge = cornerHz(Position::bridge, 2), neck = cornerHz(Position::neck, 0);
    std::printf("Tele -3 dB corners at the jack: bridge %.0f Hz, neck %.0f Hz\n", bridge, neck);
    check(bridge < neck, "Tele: the bridge coil is the darker");
    check(bridge > 2500.0 && neck < 12000.0, "Tele: corners where single coils of this inductance sit");
    for (Position position : {Position::bridge, Position::bridgeNeck, Position::neck})
    {
        controls.position = position;
        const int input = position == Position::neck ? 0 : 2;
        const Complex s(0.0, 2.0 * pi * 4000.0);
        controls.toneNeck = 1.0;
        const double open = std::abs(analogResponse(controls, input, s));
        controls.toneNeck = 0.0;
        const double closed = std::abs(analogResponse(controls, input, s));
        check(20.0 * std::log10(open / closed) > 15.0, "Tele: master Tone works on every position");
        controls.toneMiddle = 0.0;
        check(std::abs(analogResponse(controls, input, s)) == closed, "Tele: no second Tone");
        controls.toneMiddle = 1.0;
    }
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
    testCoilLossesAgainstMeasurements();
    testSwitchMapping();
    testTeleVoicing();
    if (failures == 0)
        std::printf("StratCircuit: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
