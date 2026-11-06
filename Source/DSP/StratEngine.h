#pragma once

#include "StratBridge.h"
#include "StratCircuit.h"
#include "StratString.h"

#include <array>

namespace electry
{

// The guitar from the strings to the jack: six strings on the floating
// tremolo, the three single coils reading them, and the passive circuit. It plays what it is told,
// string by string; MIDI, fingering and articulations belong to the
// performer. Large (the string rails): construct it on the heap.
class StratEngine
{
public:
    static constexpr int stringCount = StratString::stringCount;
    static constexpr int coilCount = 3; // neck, middle, bridge
    static constexpr int controlPeriod = StratString::controlPeriod;

    // Coil centre under a string, metres from that string's saddle.
    static double coilMetres(int coil, int stringIndex) noexcept;
    // Induced volts per m/s of the string under a coil, for a small swing.
    static double coilCoupling(int stringIndex) noexcept;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setControls(const StratCircuit::Controls& controls) noexcept;
    const StratCircuit::Controls& controls() const noexcept { return circuit.controls(); }

    // A fret change without glide stops the string there at once, so a pluck
    // in the same sample sounds the new note. A glide, the arm and finger
    // bends follow at the next control period; restate them at least that
    // often for a smooth gesture. Any fret above 0 is pressed, which
    // stretches the string by fretPressMetres (StratBridge::setPress).
    void setFret(int stringIndex, double fret, bool glide = false) noexcept;
    // The fretting finger lets the string up off its fret: until the next
    // setFret the flesh, of this resistance in kg/s, ends the string there.
    void liftFromFret(int stringIndex, double kilogramsPerSecond) noexcept;
    // How far a fretting finger stretches a string (StratEngine.cpp has the
    // measurement).
    static const double fretPressMetres;
    // The string's height over a fret's crown, the action, at fret 1 to 21.
    static double actionMetres(double fret) noexcept;
    // How much longer the string is taken down onto that fret's crown than
    // on its straight line from nut to saddle.
    static double actionStretchMetres(double fret) noexcept;
    // The tremolo arm, -1 full dive to +1 full pull-up (StratBridge).
    void setArm(double position) noexcept;
    // A finger bend to `pitchRatio` over the string's unbent pitch.
    void setBend(int stringIndex, double pitchRatio) noexcept;
    const StratBridge& bridge() const noexcept { return tremolo; }
    void setDamper(int stringIndex, int damper, double metresFromBridge,
                   double kilogramsPerSecond, double widthMetres = 0.0) noexcept;
    // StratString::rub: the fretting finger's own force.
    void rub(int stringIndex, double metresFromBridge, double speed, double newtons) noexcept;
    // A finger taking the string down to `fret` (setFret, then
    // StratString::press).
    void press(int stringIndex, double fret, double depthMetres, double seconds) noexcept;
    // StratString::pluck.
    void pluck(int stringIndex, double forceNewtons, double metresFromBridge,
               double releaseSeconds, double strokeMetresPerSecond = 0.0,
               bool opposedPlanes = false, double heldShare = 0.0) noexcept;

    double fundamentalHz(int stringIndex) const noexcept;
    // Moving under a pickup, from its own stroke or another's through the
    // neck, above 200 dB under a pluck.
    bool isSounding(int stringIndex) const noexcept;
    // A pick or a finger is driving it (StratString::isDriven).
    bool isDriven(int stringIndex) const noexcept;
    // StratString::planesOpposed.
    bool planesOpposed(int stringIndex) const noexcept;

    // Writes `samples` of the voltage at the jack.
    void process(float* jackVolts, int samples) noexcept;

private:
    struct Pending
    {
        double fret = 0.0;
        bool glide = false;
        bool changed = false;
    };
    void applyPending() noexcept;
    // One sample of every sounding string, and of every string while the
    // neck moves, summed into each coil's EMF.
    void stepStrings(float (&emf)[coilCount]) noexcept;

    std::array<StratString, stringCount> strings {};
    StratNeck neck;
    std::array<Pending, stringCount> pending {};
    std::array<int, stringCount> quietSamples {};
    std::array<int, stringCount> quietLimit {};
    // The coil reads velocity times the flux law's slope at the string's
    // distance from the poles: steel[s] times fluxTable at that distance in
    // table steps, which is the gap less the swing across the coil.
    static constexpr int fluxTableSize = 257;
    std::array<float, fluxTableSize> fluxTable {};
    std::array<float, stringCount> steel {};
    std::array<float, stringCount> gapSteps {};
    std::array<std::array<float, coilCount>, stringCount> swing {};
    float swingLeak = 1.0f;
    float swingSteps = 0.0f; // table steps per (m/s) sample of swing
    double stringRate = 96000.0; // the strings', coils' and circuit's rate
    StratBridge tremolo;
    bool bridgeMoved = false;
    StratCircuit circuit;
    int phase = 0;

    // Under 64 kHz the guitar runs at twice the host's rate (StratEngine.cpp)
    // and a halfband lowpass takes the jack back down to it: its centre tap
    // is 1/2 and every other tap off the centre is zero, so halfband[j] is
    // the pair 2j + 1 either side.
    int oversampling = 1;
    static constexpr int halfbandPairs = 16;
    static constexpr int halfbandHistory = 64; // past 4 * halfbandPairs - 1
    std::array<float, halfbandPairs> halfband {};
    std::array<float, halfbandHistory> jackHistory {};
    int jackWrite = 0;
};

} // namespace electry
