#pragma once

#include <array>

namespace electry
{

// One Stratocaster string as a two-rail digital waveguide of velocity waves,
// indexed from the bridge saddle. The pick, the pickups and the palm all sit
// near the bridge, so the rails are exact delays there; the losses, the
// stiffness dispersion and the tuning's fractional delay are lumped at the
// fret end, which is the only place a fret change or a bend moves.
//
//   bridge (-1) -> right-going rail -> fret: loss, dispersion, -1 -> left-going rail
//
// Positions on both rails count samples of travel from the bridge at the
// string's current wave speed, so a pickup's comb stays at the same harmonic
// numbers through a bend, as it does on the guitar.
class StratString
{
public:
    static constexpr int stringCount = 6;
    static constexpr int tapCount = 3;
    // 25.5 in.
    static constexpr double scaleMetres = 0.6477;
    // Tuning updates arrive this often; the fret-end delay ramps across one
    // period so a bend never steps.
    static constexpr int controlPeriod = 32;

    struct Construction
    {
        double openHz;
        double kilogramsPerMetre;
        // Inharmonicity B of the open string at standard tension:
        // f_n = n f sqrt(1 + B n^2).
        double stiffness;
        // Open-string decay of the fundamental and the eighth harmonic.
        double t60Fundamental;
        double t60EighthHarmonic;
    };
    static const Construction& construction(int stringIndex) noexcept;

    void prepare(double sampleRate, int stringIndex) noexcept;
    void reset() noexcept;

    // fret: 0 open, fractional while sliding. tensionRatio: tension over
    // standard-tuning tension (the tremolo and finger bends). A fret change
    // without glide stops the string there at once; with glide the fret end
    // travels, as a slide does.
    void setTuning(double fret, double tensionRatio, bool glide = false) noexcept;
    double fundamentalHz() const noexcept { return fundamental; }

    void setTaps(const std::array<double, tapCount>& metresFromBridge) noexcept;

    // A point damper on the string (palm, a touching finger). Resistance in
    // kg/s; 0 lifts it.
    static constexpr int damperCount = 2;
    void setDamper(int damper, double metresFromBridge,
                   double kilogramsPerSecond) noexcept;

    // Releases the string from the pick: the force the pick held it with,
    // the pick's distance from the bridge, and the time the release takes
    // (the pick's stiffness). Adds to whatever is already sounding.
    void pluck(double forceNewtons, double metresFromBridge,
               double releaseSeconds) noexcept;

    // One sample; writes the string velocity (m/s) under each tap.
    void process(std::array<float, tapCount>& tapVelocity) noexcept;
    // A release or its high-passed tail is still driving the string.
    bool isDriven() const noexcept
    {
        return releasePosition < releaseLength || blockedForce != 0.0;
    }

    // Waveguide characteristic impedance sqrt(T mu) at the current tension.
    double impedance() const noexcept { return characteristicImpedance; }

    // The dispersion section count and its coefficient for the current
    // stiffness, exposed for the tuning check.
    static constexpr int dispersionSections = 4;
    static double allpassPhaseDelay(double omega, double coefficient) noexcept;
    static double lossPhaseDelay(double omega, double pole) noexcept;

private:
    static constexpr int railSize = 8192;
    static constexpr int railMask = railSize - 1;
    // Rail L slot for position s is write + leftOffset + s.
    static constexpr int leftOffset = 8;

    struct Tap
    {
        int base = 1;
        float weight[4] {0.0f, 1.0f, 0.0f, 0.0f};
    };
    static Tap tapAt(double position) noexcept;

    void designDispersion(double stiffnessNow, double f0) noexcept;
    double positionSamples(double metres) const noexcept;
    float readRight(const Tap& tap) const noexcept;
    float readLeft(const Tap& tap) const noexcept;
    void clearLeft(int fromPosition, int toPosition) noexcept;
    void clearRight(int fromPosition, int toPosition) noexcept;

    double sampleRate = 48000.0;
    const Construction* spec = nullptr;

    std::array<float, railSize> right {};
    std::array<float, railSize> left {};
    int write = 0;

    // Fret end.
    double fret = 0.0;
    double tension = 1.0;
    double fundamental = 0.0;
    double samplesPerMetre = 0.0;
    double characteristicImpedance = 0.0;
    int leftLength = 0;       // rail L, integer
    int leftTarget = 0;       // where a glide is taking it
    // The whole loop in samples less the fret-end filters' phase delay;
    // rail R is read at loopDelay - leftLength.
    double loopDelay = 0.0;
    double loopDelayTarget = 0.0;
    double loopDelayStep = 0.0;
    int rampRemaining = 0;
    bool tuned = false;

    float lossGain = 1.0f;
    float lossPole = 0.0f;
    float lossState = 0.0f;
    double designedStiffness = -1.0;
    float allpassCoefficient = 0.0f;
    std::array<float, dispersionSections> allpassState {};

    std::array<double, tapCount> tapMetres {};
    std::array<Tap, tapCount> taps {};

    struct Damper
    {
        double metres = 0.0;
        double resistance = 0.0;
        int position = 0;
        float absorption = 0.0f;
    };
    std::array<Damper, damperCount> dampers {};

    // The pick's force as a step the release ramps through, injected at the
    // pick into both rails. It is high-passed far below any string's
    // fundamental so a finished release leaves nothing to carry.
    double pickMetres = 0.13;
    Tap pick {};
    double force = 0.0;
    double forceStart = 0.0;
    double forceEnd = 0.0;
    int releaseLength = 0;
    int releasePosition = 0;
    double forceHeld = 0.0;
    double blockedForce = 0.0;
    double blockCoefficient = 0.0;
    void placeDampers() noexcept;
    double lastUsablePosition() const noexcept;
};

} // namespace electry
