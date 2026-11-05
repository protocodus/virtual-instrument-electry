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
//
// The string swings in two planes: toward the poles and along the top. Each
// has its own pair of rails. The pick, the palm and a fret act on both, and
// their ends differ only by the in-plane speaking length (inPlaneShortening),
// so each partial is a pair a fraction of a hertz apart that the pickup hears
// beating (StratEngine reads both). Toward the poles the neck also yields
// under the fret, in one resonance near 420 Hz (StratString.cpp).
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
        // Wrapped strings lose their upper partials faster than solid ones.
        bool wound;
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

    // A damper on the string (palm, a touching finger): a point, or a
    // contact spread evenly along a width of string centred there.
    // Resistance in kg/s, in all; 0 lifts it.
    static constexpr int damperCount = 2;
    void setDamper(int damper, double metresFromBridge, double kilogramsPerSecond,
                   double widthMetres = 0.0) noexcept;

    // Plucks the string: the force at which it slips off the pick (its sign
    // the stroke's direction), the pick's distance from the bridge and the
    // time the release takes. With a stroke speed the hand first drives the
    // pick into the string at that speed, through the give of its grip, until
    // the string pushes back with the slip force; without one the string
    // starts held at that force, as a fingertip leaves it. Adds to whatever
    // is already sounding.
    void pluck(double forceNewtons, double metresFromBridge, double releaseSeconds,
               double strokeMetresPerSecond = 0.0) noexcept;
    // The spring between the hand and the string: fitted with the stroke
    // speed (StratPerformer's pushRatio) to the push IDMT-SMT-Guitar's Strat
    // neck DI shows before every pick release, a ~1 ms ramp to ~0.7 of the
    // following peak. Over +-3 ms of the release edge, 18 notes (each string,
    // frets 0/5/10) correlate 0.835 with the reference against 0.62 without a
    // push; a rigid pick reaches 0.79 but its sharp first touch leaves 4-8 kHz
    // 5-14 dB too bright. 600-1000 N/m score within 0.01 of the best.
    static constexpr double contactStiffness = 800.0;

    // The swing along the top reflects as if the string were this much
    // shorter than its swing toward the poles (StratString.cpp has the
    // measurement).
    static const double inPlaneShortening;
    // The neck under the fret yields toward the poles in one resonance: its
    // frequency, Q and conductance at the peak, s/kg (StratString.cpp).
    static const double neckHz;
    static const double neckQ;
    static const double neckConductance;

    // One sample; writes the string's velocity (m/s) under each tap, toward
    // the poles and along the top.
    void process(std::array<float, tapCount>& towardPoles,
                 std::array<float, tapCount>& alongTop) noexcept;
    // A release or its high-passed tail is still driving the string.
    bool isDriven() const noexcept
    {
        return stickRemaining > 0 || releasePosition < releaseLength || blockedForce != 0.0;
    }

    // Waveguide characteristic impedance sqrt(T mu) at the current tension.
    double impedance() const noexcept { return characteristicImpedance; }

    // The dispersion section count and its coefficient for the current
    // stiffness, exposed for the tuning check.
    static constexpr int dispersionSections = 4;
    static double allpassPhaseDelay(double omega, double coefficient) noexcept;
    // The loss filter k (1 + zero z^-1) / (1 + pole z^-1).
    static double lossPhaseDelay(double omega, double zero, double pole) noexcept;

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
    // The fret end reads once per round trip, so its interpolator's top-end
    // loss adds to the string's on every trip: seventh order keeps it under
    // 4 dB/s on the high E at 8 kHz (third order loses 75, more than the
    // string's own 37).
    struct LoopTap
    {
        int base = 3;
        float weight[8] {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    };
    static LoopTap loopTapAt(double position) noexcept;

    // One plane of the swing: its rails and its fret end's filter states.
    struct Plane
    {
        std::array<float, railSize> right {};
        std::array<float, railSize> left {};
        float lossInput = 0.0f;
        float lossState = 0.0f;
        std::array<float, dispersionSections> allpassState {};
        LoopTap loopTap;
        double loopTapPosition = -1.0;
        // The neck's resonance under the fret: the force it was driven with
        // (over the string's impedance) and the velocity it moved at, the
        // last two samples.
        float neckForce1 = 0.0f;
        float neckForce2 = 0.0f;
        float neckVelocity1 = 0.0f;
        float neckVelocity2 = 0.0f;
    };
    // Reads the right rail at the fret, through the loss and the dispersion,
    // and writes the reflection onto the left rail; a nonzero neckGain lets
    // the fret yield.
    void reflectAtFret(Plane& plane, double readAt, int skipped, float neckGain) noexcept;

    void designDispersion(double stiffnessNow, double f0) noexcept;
    double positionSamples(double metres) const noexcept;
    float readRight(const Plane& plane, const Tap& tap) const noexcept;
    float readRight(const Plane& plane, const LoopTap& tap) const noexcept;
    float readLeft(const Plane& plane, const Tap& tap) const noexcept;
    void clearLeft(int fromPosition, int toPosition) noexcept;
    void clearRight(int fromPosition, int toPosition) noexcept;
    void inject(Plane& plane, float velocity) noexcept;
    void damp(Plane& plane) noexcept;

    double sampleRate = 48000.0;
    const Construction* spec = nullptr;

    // Toward the poles: the plane the pick's contact is solved in, the
    // stretch measured on and the pickup's distance follows.
    Plane poles;
    Plane top;
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
    // The in-plane loop's shortfall, samples.
    double topShortfall = 0.0;

    // Tension modulation, the twang of a hard pluck: the swinging string
    // stretches and its tension rises with its energy, so the loop shortens by
    // stretchSamplesPerJoule times that energy, measured every control period
    // and ramped between.
    double stretchSamplesPerJoule = 0.0;
    double stretch = 0.0;
    double stretchStep = 0.0;
    int stretchCountdown = 0;
    // The last historyLength arrivals at the bridge, as running sums of the
    // samples and of their squares, recounted every so often.
    double historySum = 0.0;
    double historySquares = 0.0;
    int historyLength = 0;
    int recountCountdown = 0;
    void measureStretch() noexcept;

    float lossGain = 1.0f;
    float lossZero = 0.0f;
    float lossPole = 0.0f;
    // The neck's admittance as a bilinear biquad (b, 0, -b) / (1, a1, a2),
    // b per kg/s of the string's impedance.
    double neckNumerator = 0.0;
    float neckA1 = 0.0f;
    float neckA2 = 0.0f;
    // The yielding fret's phase delay at the fundamental, samples: the loop
    // takes it out so a fretted note stays in tune, and the plane along the
    // top, whose fret does not yield, puts it back.
    double neckDelay = 0.0;
    double neckPhaseDelay(double omega) const noexcept;
    double designedStiffness = -1.0;
    float allpassCoefficient = 0.0f;

    std::array<double, tapCount> tapMetres {};
    std::array<Tap, tapCount> taps {};

    struct Damper
    {
        double metres = 0.0;
        double resistance = 0.0;
        double width = 0.0;
        int position = 0; // the first sample point it covers
        int count = 1;
        float absorption = 0.0f; // at each point
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
    double releaseRate = 1.0;
    // While the pick carries the string: its speed, the force the string
    // slips at, the force before the contact, and the release to follow.
    int stickRemaining = 0;
    double contact = 0.0;
    double stickSpeed = 0.0;
    double stickLimit = 0.0;
    double stickBase = 0.0;
    double stickRelease = 0.0;
    void startRelease(double from, double to, double seconds) noexcept;
    std::array<double, 3> pickOverlap {};
    std::array<float, 3> injected {};
    double forceHeld = 0.0;
    double blockedForce = 0.0;
    double blockCoefficient = 0.0;
    void placeDampers() noexcept;
    double lastUsablePosition() const noexcept;
};

} // namespace electry
