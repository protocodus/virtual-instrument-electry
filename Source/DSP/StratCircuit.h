#pragma once

#include <array>

namespace electry
{

// The passive output network of a Stratocaster or a Telecaster: the coils'
// induced voltages in, the voltage at the amplifier input out.
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
// mapping.
//
// The Telecaster (modern 3-way): its two coils sit in the neck and bridge
// slots, the middle slot is never connected, and one master Tone, Tone 1's
// branch, hangs on node X in every position. Position 1 is the bridge coil,
// 2 the two in parallel, 3 the neck coil; there is no blend or dark circuit.
//
// The network is discretised exactly for an input that is linear
// between samples (ramp invariance): no frequency warping, and the very fast
// modes the near-zero contact and pot-end resistances create decay at once
// instead of ringing at Nyquist as they would under the bilinear transform.
class StratCircuit
{
public:
    enum class Guitar
    {
        strat,
        tele
    };

    enum class Position
    {
        bridge,       // 1
        bridgeMiddle, // 2, in parallel
        middle,       // 3
        middleNeck,   // 4, in parallel
        neck,         // 5
        // The Telecaster's middle position, the neck and bridge coils in
        // parallel. On a Telecaster the Strat's positions play the Tele
        // position on their side of the switch (bridgeMiddle the bridge,
        // middle both, middleNeck the neck); on a Strat this one plays the
        // middle coil, as the vintage 5-way has no neck-and-bridge setting.
        bridgeNeck
    };

    struct Controls
    {
        Guitar guitar = Guitar::strat;
        Position position = Position::neck;
        // Knob rotations, 0 (knob at 0) to 1 (knob at 10).
        double volume = 1.0;
        double toneNeck = 1.0;   // Tone 1; the Telecaster's master Tone
        double toneMiddle = 1.0; // Tone 2; no Telecaster has it

        bool operator==(const Controls& other) const noexcept
        {
            return guitar == other.guitar && position == other.position && volume == other.volume
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
    // Alnico 5 pole-piece eddy-current loss resistance in parallel with each
    // coil's inductance and winding capacitance (Jungmann 1994; Horton and
    // Moore 2009; Lemme 2003): alternating flux in the conductive Alnico 5
    // rod magnets inside the bobbin damps the unloaded LC resonance (from
    // Q > 13 down to ~3.2 unloaded and ~1.85 on the un-toned bridge pickup).
    static constexpr double eddyOhms = 220e3;
    // 250 kOhm audio-taper Volume and Tones and 0.1 uF tone capacitors, the
    // vintage Stratocaster values; no treble bleed.
    static constexpr double potOhms = 250e3;
    static constexpr double toneFarads = 0.1e-6;
    // Voiced within the reported range: 3 m of cable at about 100 pF/m.
    // Lemme gives 300 to 1000 pF for ordinary guitar cables.
    static constexpr double cableFarads = 300e-12;

    // The Telecaster's coils: Fender's Pure Vintage '52 Telecaster set
    // (product listing 0992119000), Alnico 3, enamel wire, flush poles. The
    // neck: 7.2 kOhm and 2.4 H, under its chrome cover; the bridge: 6.6 kOhm
    // and 3.8 H, on its copper-plated steel base plate, which the listed
    // inductance includes.
    static constexpr double teleNeckOhms = 7200.0;
    static constexpr double teleNeckHenries = 2.4;
    static constexpr double teleBridgeOhms = 6600.0;
    static constexpr double teleBridgeHenries = 3.8;
    // Estimates, not measurements, within Lemme's 80 to 200 pF for pickups
    // in general: the neck coil's narrow bobbin winds about as the Strat's
    // (100 pF); the bridge's wider, fuller one, and its plate under it, a
    // little more.
    static constexpr double teleNeckFarads = 100e-12;
    static constexpr double teleBridgeFarads = 120e-12;
    // Eddy-current loss across each coil: the Alnico rods' alone, as for the
    // Strat (eddyOhms; Alnico 3 conducts about as Alnico 5 does). The neck's
    // cover and the bridge's plate add their own loss, which nothing here
    // measures, so it is left out (README, Known gaps): both coils ring a
    // little more than a Tele's would.
    static constexpr double teleNeckEddyOhms = eddyOhms;
    static constexpr double teleBridgeEddyOhms = eddyOhms;
    // One master Tone of 250 kOhm, audio taper, with 0.05 uF: the value
    // reported for Fender's vintage-style Telecasters (estimate; the
    // 0.047 uF of current production moves its corner 6 %).
    static constexpr double teleToneFarads = 0.05e-6;
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
    // Connected-subspace bookkeeping for fast exact state updates: only coils
    // and tone capacitors joined to node X by the 5-way switch couple to the
    // output; disconnected coils evolve as isolated 2-state tanks and
    // disconnected tone capacitors hold their charge.
    std::array<int, stateCount> activeRows {};
    int activeCount = 0;
    std::array<int, inputCount> activeCoils {};
    int activeCoilCount = 0;
    std::array<bool, inputCount> connectedCoil {};
    std::array<bool, 2> connectedTone {};
    // Contiguous column-major packed active subspace matrices (col j, row i)
    // so paired-row loads walk contiguous addresses without changing sum order.
    std::array<std::array<double, stateCount>, stateCount> activePhiCol {};
    std::array<std::array<double, stateCount>, inputCount> activePrevCol {};
    std::array<std::array<double, stateCount>, inputCount> activeCurrCol {};
    bool idle = true;
};

} // namespace electry
