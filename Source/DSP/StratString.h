#pragma once

#include <array>
#include <complex>
#include <cstdint>

namespace electry
{

// The neck under the frets, toward the poles, as a sum of resonances: each
// mode's admittance Y(s) = G (w/Q) s / (s^2 + (w/Q) s + w^2) (StratString's
// neckModes) through the bilinear transform warped to its w. Every string on
// it pushes on it at its own fret, the nut for an open string, and the neck
// moves under all of them alike (StratString.cpp).
class StratNeck
{
public:
    static constexpr int modeCount = 2;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    // One sample: `push` is the sum over the strings on it of 2 Z y, each
    // string's impedance times twice the velocity arriving at its fret, and
    // `load` the sum of their impedances. Returns the velocity the neck moves
    // at under them; each string's fret sends back that less what arrived.
    float move(float push, float load) noexcept;
    float velocity() const noexcept { return velocity1; }

private:
    // Each mode (b, 0, -b) / (1, a1, a2), from the force they share to the
    // velocity it adds.
    struct Mode
    {
        float numerator = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
        float velocity1 = 0.0f;
        float velocity2 = 0.0f;
    };
    std::array<Mode, modeCount> modes {};
    float numeratorSum = 0.0f;
    float force1 = 0.0f;
    float force2 = 0.0f;
    float velocity1 = 0.0f;
};

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
// under the fret, through the neck's two modes (StratString.cpp), and the
// strings that share it reach one another through it (StratNeck).
class StratString
{
public:
    static constexpr int stringCount = 6;
    static constexpr int tapCount = 3;
    // 25.5 in.
    static constexpr double scaleMetres = 0.6477;
    // Tuning updates arrive this often, in the host's samples; the fret-end
    // delay ramps across one period so a bend never steps.
    static constexpr int controlPeriod = 32;

    struct Construction
    {
        double openHz;
        double kilogramsPerMetre;
        // Inharmonicity B of the open string at standard tension:
        // f_n = n f sqrt(1 + B n^2).
        double stiffness;
        // Open-string decay of the fundamental, the eighth harmonic and
        // one high frequency.
        double t60Fundamental;
        double t60EighthHarmonic;
        double highHz;
        double t60High;
        // A wound string's wraps: their own loss on the open string at
        // wrapHz, dB/s, on top of the rest (StratString.cpp); 0 for a plain
        // string.
        double wrapLoss;
        // Wrapped strings lose their upper partials faster than solid ones.
        bool wound;
        // The wrap wire's diameter, the pitch of the ridges a finger
        // sliding along the string rides over; 0 for a plain string.
        double wrapMetres;
    };
    static const Construction& construction(int stringIndex) noexcept;

    // A string run faster than its host (StratEngine) takes its tuning
    // updates every updateSamples of its own.
    void prepare(double sampleRate, int stringIndex, int updateSamples = controlPeriod) noexcept;
    void reset() noexcept;

    // fret: 0 open, fractional while sliding. tensionRatio: tension over
    // the tuned string's at rest (the tremolo and finger bends). A fret change
    // without glide stops the string there at once; with glide the fret end
    // travels, as a slide does.
    void setTuning(double fret, double tensionRatio, bool glide = false) noexcept;
    double fundamentalHz() const noexcept { return fundamental; }

    // The setup that plays the fretted notes in tune (StratEngine::prepare):
    // the open string's speaking length from nut to saddle, and how far the
    // saddle sits behind the scale's end, metres. Call it before prepare.
    // Every position on the string (setTaps, pluck, setDamper, rub) is metres
    // from this saddle.
    void setIntonation(double openMetres, double saddleSetbackMetres) noexcept;
    // Where the player's tuner left the open string at rest: its pitch over
    // standard pitch (StratEngine::restPitch). Call it before prepare.
    void setRestPitch(double ratio) noexcept { restPitch = ratio; }
    // From the saddle to where `fret` stops the string, the nut at fret 0.
    double fretMetres(double fret) const noexcept;

    void setTaps(const std::array<double, tapCount>& metresFromBridge) noexcept;

    // The string lifted off its fret under the finger: the flesh, of this
    // resistance in kg/s, ends it instead of the fret, which the neck then no
    // longer feels, until the string is put back on a fret (0).
    void setFingerEnd(double kilogramsPerSecond) noexcept;
    // The nut of an open string is a rigid termination too; false only
    // while the released string ends on the fingertip's flesh.
    bool onFret() const noexcept { return !lifted; }

    // The fretting finger's own force on the string where it touches it,
    // toward the poles: rubbing, the fingertip sliding along the string at
    // `speed` m/s while pressing on it with `newtons`, sent into the rails
    // as the pick's force is, half each way. Restate it every update period
    // (it ramps between); a speed of 0 ends it.
    void rub(double metresFromBridge, double speed, double newtons) noexcept;
    bool isRubbing() const noexcept { return rubTarget != 0.0f || rubSpeed != 0.0f; }
    // The finger taking the string down onto its fret: the string's end
    // there goes `depthMetres` toward the board over `seconds` and meets the
    // fret at landingScale times the speed the recordings' ticks give
    // (StratString.cpp). The string is already stopped at that fret
    // (setTuning).
    void press(double depthMetres, double seconds, double landingScale = 1.0) noexcept;

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
    // is already sounding. With opposedPlanes the stroke sets the swing along
    // the top off in the other sense to the swing toward the poles, as a pick
    // leaving the string at the other tilt does (StratPerformer draws it).
    // With a heldShare the push starts from that share of the slip force,
    // taken up so slowly that the string stands at rest under it, as it does
    // under the whole force without a push.
    void pluck(double forceNewtons, double metresFromBridge, double releaseSeconds,
               double strokeMetresPerSecond = 0.0, bool opposedPlanes = false,
               double heldShare = 0.0) noexcept;
    // The spring between the hand and the string: fitted with the stroke
    // (StratPerformer's heldShare and pushRatio) to the attacks of two Strats
    // recorded direct. 400 and 1600 N/m leave the neck pickup's first swing
    // after the release at 0.57 and 0.75 of the largest that follows (0.71
    // here, the recordings 0.86 and 0.90) and the first 260 ms 3.0 and 3.2 dB
    // off one of them in third octaves (2.7 here).
    static constexpr double contactStiffness = 800.0;

    // Where a wound string's wraps loss is stated.
    static constexpr double wrapHz = 2800.0;

    // The swing along the top reflects as if the string were this much
    // shorter than its swing toward the poles (StratString.cpp has the
    // measurement).
    static const double inPlaneShortening;
    // The share of an open string's loss that goes per second rather than
    // per round trip, wound and plain (StratString.cpp).
    static const double woundPerSecondShare;
    static const double plainPerSecondShare;
    // What a fretted string's loss is multiplied by, the finger behind the
    // fret taking its share (StratString.cpp).
    static const double fretFingerLoss;
    // The neck under the fret yields toward the poles in resonances: each
    // one's frequency, Q and conductance at its peak, s/kg (StratString.cpp).
    struct NeckMode
    {
        double hz;
        double q;
        double conductance;
    };
    static const std::array<NeckMode, StratNeck::modeCount> neckModes;

    // One sample on a neck of its own; writes the string's velocity (m/s)
    // under each tap, toward the poles and along the top.
    void process(std::array<float, tapCount>& towardPoles,
                 std::array<float, tapCount>& alongTop) noexcept;
    // The same sample in two halves, so that strings sharing a neck meet at
    // it: arrive() returns the velocity of the wave arriving at the fret
    // toward the poles, and leave() reflects it off the fret moving at the
    // neck's velocity and finishes the sample.
    float arrive() noexcept;
    void leave(float neckVelocity, std::array<float, tapCount>& towardPoles,
               std::array<float, tapCount>& alongTop) noexcept;
    // The latest stroke set the swing along the top against its own.
    bool planesOpposed() const noexcept { return topDrive < 0.0f; }
    // A release or its high-passed tail is still driving the string.
    bool isDriven() const noexcept
    {
        return stickRemaining > 0 || releasePosition < releaseLength || blockedForce != 0.0
            || topBlockedForce != 0.0 || pendingTopForce != 0.0
            || fingerActive || pressPosition < pressLength;
    }

    // Waveguide characteristic impedance sqrt(T mu) at the current tension.
    double impedance() const noexcept { return characteristicImpedance; }
    // How far a force held at the latest pluck's point bends the speaking
    // length aside at a point (metres from the bridge), at rest: the
    // deflected rest that a pluck's held force leaves out of the rails.
    double restingDeflection(double forceNewtons, double atMetres) const noexcept;

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
        float wrapState = 0.0f;
        std::array<float, dispersionSections> allpassState {};
        double historySum = 0.0;
        float dcMean = 0.0f;
        LoopTap loopTap;
        double loopTapPosition = -1.0;
    };
    // The wave arriving at the fret: the right rail read there, through the
    // loss and the dispersion.
    float arriveAtFret(Plane& plane, double readAt) noexcept;
    // Writes the fret's reflection onto the left rail.
    void leaveFret(Plane& plane, float reflected) noexcept;
    // Where the left rail grew this sample, -1 if it did not.
    int skipped = -1;
    float arrivingAtNeck = 0.0f;

    void designDispersion(double stiffnessNow, double f0) noexcept;
    double positionSamples(double metres) const noexcept;
    float readRight(const Plane& plane, const Tap& tap) const noexcept;
    float readRight(const Plane& plane, const LoopTap& tap) const noexcept;
    float readLeft(const Plane& plane, const Tap& tap) const noexcept;
    void clearLeft(int fromPosition, int toPosition) noexcept;
    void clearRight(int fromPosition, int toPosition) noexcept;
    void inject(Plane& plane, const Tap& at, float velocity) noexcept;
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
    double fundamental = 0.0;
    double intonedOpen = scaleMetres;
    double saddleSetback = 0.0;
    double restPitch = 1.0;
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
    int rampSamples = controlPeriod;
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
    // samples (Plane::historySum) and of their squares, recounted every so
    // often.
    double historySquares = 0.0;
    float dcPole = 0.0f;
    int historyLength = 0;
    int recountCountdown = 0;
    void measureStretch() noexcept;

    // The loop filter through the open string's measured losses, with the
    // per-second share scaled to the current trip's time.
    void designLoss(double share) noexcept;
    double designedLossShare = -1.0;
    float lossGain = 1.0f;
    float lossZero = 0.0f;
    float lossPole = 0.0f;
    // The wraps: a one-pole lowpass (1 - p) / (1 - p z^-1), unity at DC.
    float wrapPole = 0.0f;
    std::complex<double> wrapResponse(double omega) const noexcept;
    // The neck process() plays on.
    StratNeck ownNeck;
    // Off the fret, the finger's end: the share of an arriving wave it
    // sends back inverted, (R - Z) / (R + Z).
    bool lifted = false;
    float fingerEnd = 0.0f;
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
        // A point between two samples: its share of the next one, which it
        // reads and loads with the current one in proportion.
        float next = 0.0f;
    };
    std::array<Damper, damperCount> dampers {};

    // The pick's force as a step the release ramps through, injected at the
    // pick into both rails. It is high-passed far below any string's
    // fundamental so a finished release leaves nothing to carry.
    double pickMetres = 0.13;
    Tap pick {};
    double force = 0.0;
    float topDrive = 1.0f; // the latest stroke's sense along the top, +-1
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
    // Each stroke's tilt applies only to that stroke's force change. The
    // previous stroke's high-pass tail keeps its own direction when the
    // next pick turns; pendingTopForce completes an interrupted old contact
    // in its old direction before the next one starts.
    double topBlockedForce = 0.0;
    double pendingTopForce = 0.0;
    double blockCoefficient = 0.0;
    void placeDampers() noexcept;
    double lastUsablePosition() const noexcept;

    // The fretting finger: its rubbing force, and the fret end's press.
    float fingerForce() noexcept;
    bool fingerActive = false;
    double fingerMetres = 0.0;
    Tap fingerTap {};
    float rubSpeed = 0.0f, rubTarget = 0.0f, rubStep = 0.0f;
    float rubNewtons = 0.0f;
    int rubRemaining = 0;
    double rubPhase = 0.0;
    static constexpr std::uint32_t rubNoiseSeed = 0x2545F491u;
    std::uint32_t rubNoise = rubNoiseSeed; // xorshift32
    float rubRough = 0.0f;
    float rubbing() noexcept;
    double pressDepth = 0.0;
    double pressLanding = 0.0;
    int pressPosition = 0, pressLength = 0;
    float roughPole = 0.0f;
};

} // namespace electry
