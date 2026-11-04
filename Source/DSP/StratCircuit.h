#pragma once

#include <array>

namespace electry
{

// The passive Stratocaster output network: the three coils' induced voltages
// in, the voltage at the amplifier input out.
//
// Wiring (vintage 5-way): switch pole A joins the selected coils to node X,
// the top of the Volume pot; pole B joins Tone 1 to X with the neck coil and
// Tone 2 with the middle coil, so position 4 carries both tones and the
// bridge coil none. The Volume wiper drives the jack, the cable and the amp
// input. Each coil is its induced voltage in series with its resistance and
// inductance, with its winding capacitance across the terminals.
//
// One network serves all five positions: an open contact is a zero
// conductance and a closed one a small resistance, so every state is a
// physical current or voltage and a switch or knob move needs no state
// mapping. The network is discretised exactly for an input that is linear
// between samples (ramp invariance): no frequency warping, and the very fast
// modes the near-zero contact and pot-end resistances create decay at once
// instead of ringing at Nyquist as they would under the bilinear transform.
class StratCircuit
{
public:
    enum class Position
    {
        bridge,       // 1
        bridgeMiddle, // 2, in parallel
        middle,       // 3
        middleNeck,   // 4, in parallel
        neck          // 5
    };

    struct Controls
    {
        Position position = Position::neck;
        // Knob rotations, 0 (knob at 0) to 1 (knob at 10).
        double volume = 1.0;
        double toneNeck = 1.0;   // Tone 1
        double toneMiddle = 1.0; // Tone 2

        bool operator==(const Controls& other) const noexcept
        {
            return position == other.position && volume == other.volume
                && toneNeck == other.toneNeck
                && toneMiddle == other.toneMiddle;
        }
    };

    // Fender's Pure Vintage '57 Stratocaster set: 5.95 kOhm and 2.47 H for
    // each of neck, middle and bridge, Alnico 5, Formvar wire (Fender product
    // listing 0992385000). All three coils are alike and wound the same way,
    // with no reverse-wound middle.
    static constexpr double coilOhms = 5950.0;
    static constexpr double coilHenries = 2.47;
    // Reported, not measured on this design: Lemme gives 80 to 200 pF of
    // winding capacitance for pickups in general, a set of early-1970s Strat
    // pickups measured about 100 pF (Music Electronics Forum), and with
    // 2.47 H this puts the unloaded resonance at 10.1 kHz, inside the 10 to
    // 13 kHz reported for vintage-correct Strat coils (Jungmann 1994, via
    // arXiv:2409.19782).
    static constexpr double coilFarads = 100e-12;
    // 250 kOhm audio-taper Volume and Tones and 0.1 uF tone capacitors, the
    // vintage Stratocaster values; no treble bleed.
    static constexpr double potOhms = 250e3;
    static constexpr double toneFarads = 0.1e-6;
    // Voiced within the reported range: 3 m of cable at about 100 pF/m.
    // Lemme gives 300 to 1000 pF for ordinary guitar cables.
    static constexpr double cableFarads = 300e-12;
    // The standard guitar-amplifier input.
    static constexpr double ampInputOhms = 1e6;
    // Numerical conventions, not voicing: a closed contact and each end of a
    // pot track keep 1 Ohm, so the network keeps one shape at every knob and
    // switch setting. Against the ideal 0 Ohm this moves the response by at
    // most 0.005 dB (both Tones at 0 in position 4, near their 320 Hz
    // resonance) and leaves Volume at 0 about 108 dB down.
    static constexpr double contactOhms = 1.0;
    static constexpr double potEndOhms = 1.0;

    // Fraction of the track below the wiper at a knob rotation: the usual
    // audio ("A") taper, 10 % of the track at half rotation, as the
    // exponential law (81^r - 1) / 80 through both ends and that point. Real
    // carbon tracks approximate it in two or three linear segments.
    static double audioTaper(double rotation) noexcept;

    void prepare(double sampleRate) noexcept;
    void setControls(const Controls& controls) noexcept;
    const Controls& controls() const noexcept { return current; }
    void reset() noexcept;

    // Induced voltages of the neck, middle and bridge coils at this sample;
    // returns the amplifier input voltage at the same instant.
    double process(double neck, double middle, double bridge) noexcept;

    static constexpr int stateCount = 9;
    static constexpr int inputCount = 3;

private:
    void updateCoefficients() noexcept;

    double samplePeriod = 1.0 / 48000.0;
    Controls current;
    // x[n+1] = phi x[n] + previousInput u[n] + currentInput u[n+1]
    std::array<double, stateCount * stateCount> phi {};
    std::array<double, stateCount * inputCount> previousInput {};
    std::array<double, stateCount * inputCount> currentInput {};
    std::array<double, stateCount> state {};
    std::array<double, inputCount> lastInput {};
};

} // namespace electry
