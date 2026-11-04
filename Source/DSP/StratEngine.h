#pragma once

#include "StratCircuit.h"
#include "StratString.h"

#include <array>

namespace electry
{

// The guitar from the strings to the jack: six strings, the three single
// coils reading them, and the passive circuit. It plays what it is told,
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
    // Induced volts per m/s of the string under a coil.
    static double coilCoupling(int stringIndex) noexcept;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setControls(const StratCircuit::Controls& controls) noexcept;
    const StratCircuit::Controls& controls() const noexcept { return circuit.controls(); }

    // A fret change without glide stops the string there at once, so a pluck
    // in the same sample sounds the new note. A glide, and the tension
    // (finger bends, the tremolo), follow at the next control period; call
    // them at least that often for a smooth gesture.
    void setFret(int stringIndex, double fret, bool glide = false) noexcept;
    void setTension(int stringIndex, double tensionRatio) noexcept;
    void setDamper(int stringIndex, int damper, double metresFromBridge,
                   double kilogramsPerSecond) noexcept;
    void pluck(int stringIndex, double forceNewtons, double metresFromBridge,
               double releaseSeconds) noexcept;

    double fundamentalHz(int stringIndex) const noexcept;
    bool isSounding(int stringIndex) const noexcept;

    // Writes `samples` of the voltage at the jack.
    void process(float* jackVolts, int samples) noexcept;

private:
    struct Pending
    {
        double fret = 0.0;
        double tension = 1.0;
        bool glide = false;
        bool changed = false;
    };
    void applyPending() noexcept;

    std::array<StratString, stringCount> strings {};
    std::array<Pending, stringCount> pending {};
    std::array<int, stringCount> quietSamples {};
    std::array<int, stringCount> quietLimit {};
    std::array<float, stringCount> coupling {};
    StratCircuit circuit;
    int phase = 0;
};

} // namespace electry
