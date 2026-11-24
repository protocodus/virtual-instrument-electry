#pragma once

#include "StratBridge.h"
#include "StratCircuit.h"
#include "StratString.h"

#include <array>
#include <utility>

namespace electry
{

// What a Telecaster's two coils make of the EMF a Strat coil would read in
// their places: each coil's own sensitivity, and the neck coil's cover between
// it and the strings. Their losses and capacitances are the circuit's
// (StratCircuit).
class TeleCoils
{
public:
    // Each coil's sensitivity over a Strat coil's, dB. The neck's is measured:
    // Zollner marks each pickup's low-frequency sensitivity on its data sheet
    // against a Stratocaster pickup's ("Physics of the Electric Guitar", 5.15),
    // and his Tele-52 neck stands 1.8 dB under his Strat-72, the pair the
    // circuit's losses come from. The bridge's is an estimate. Its turns over
    // the Strat coil's, by the two sets' listed inductances with the 6 % the
    // base plate adds taken off, sqrt(3.58 H / 2.47 H), give 1.6 dB, and the
    // plate itself gives 0.6 dB (5.1, Fig. 5.1.4). His table of 40 pickups
    // (5.4.5; its values are withheld) ranks the Tele-52 bridge over every
    // Fender Strat coil in it and the Tele-52 neck under them all.
    static constexpr double neckDb = -1.8;
    static constexpr double bridgeDb = 2.2;
    // The neck coil's cover, 0.5 mm of nickel silver, is thin against its skin
    // depth (2.7 mm at 10 kHz), so the eddy currents the strings' field
    // drives in it oppose that field as a first-order lag, which the coil's
    // impedance does not show. Zollner read one Fender Telecaster neck
    // pickup's output by laser with its cover on and off, into 110 kOhm and
    // 700 pF (Fig. 5.10.15): on, it gives 0.5 dB less at 2 kHz, 1.4 at 3, 1.7
    // from 3.5 to 4.5 and 1.2 to 1.6 from 5.5 to 9.5 kHz. The loss the cover
    // adds to the circuit (StratCircuit) gives most of that up to the
    // resonance and almost none of it above 6 kHz; with that loss, a corner
    // at 13 kHz follows the reading within 0.4 dB at each of 18 points from
    // 1.25 to 9.5 kHz, 0.15 dB rms (StratEngineTests).
    static constexpr double coverHz = 13000.0;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    // One sample: the EMF at the neck and bridge coils' places in, the coils'
    // own out.
    void process(double& neck, double& bridge) noexcept;

private:
    double neckGain = 1.0, bridgeGain = 1.0;
    // The lag, exact for an input linear between samples as the circuit is:
    // volts[n+1] = pole volts[n] + previous in[n] + current in[n+1].
    double pole = 0.0, previous = 0.0, current = 1.0;
    double coverVolts = 0.0, coverInput = 0.0;
};

// The guitar from the strings to the jack: six strings on the stabilized
// tremolo, the three single coils reading them, and the passive circuit. As a
// Telecaster (StratCircuit::Guitar) the same strings run over its two coils
// (TeleCoils) and the bridge is fixed: the arm does nothing. It
// plays what it is told, string by string; MIDI, fingering and articulations
// belong to the performer. Large (the string rails): construct it on the
// heap.
class StratEngine
{
public:
    static constexpr int stringCount = StratString::stringCount;
    static constexpr int coilCount = 3; // neck, middle, bridge
    static constexpr int controlPeriod = StratString::controlPeriod;

    // Coil centre under a string, metres from that string's saddle.
    static double coilMetres(StratCircuit::Guitar guitar, int coil, int stringIndex) noexcept;

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
    // Excess axial length held by the fretting finger beyond the calibrated press.
    void setExtraFretPress(int stringIndex, double metres) noexcept;
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
    // The open string's speaking length and the saddle's setback, metres,
    // that play this string's fretted notes in tune (StratEngine.cpp).
    static std::pair<double, double> intonation(int stringIndex) noexcept;
    // The open string's pitch at rest over standard pitch, where the player
    // tuned it (StratEngine.cpp).
    static double restPitch(int stringIndex) noexcept;
    // The host's master tune, as a pitch ratio over standard: every string
    // retuned at the tuner. A ringing string follows within a control period.
    void setMasterTune(double pitchRatio) noexcept;
    // StratString::fretMetres: from the string's saddle to where `fret`
    // stops it, the nut at fret 0, metres.
    double fretMetres(int stringIndex, double fret) const noexcept;
    // The tremolo arm, -1 full dive to +1 full pull-up (StratBridge). A
    // Telecaster has none and ignores it.
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
    void press(int stringIndex, double fret, double depthMetres, double seconds,
               double landingScale = 1.0) noexcept;
    // StratString::pluck.
    void pluck(int stringIndex, double forceNewtons, double metresFromBridge,
               double releaseSeconds, double strokeMetresPerSecond = 0.0,
               bool opposedPlanes = false, double heldShare = 0.0,
               double contactStiffnessNewtonsPerMetre = StratString::contactStiffness,
               double wrapScrapeScale = 1.0) noexcept;

    double fundamentalHz(int stringIndex) const noexcept;
    // Moving under a pickup, from its own stroke or another's through the
    // neck, above 200 dB under a pluck.
    bool isSounding(int stringIndex) const noexcept;
    // A pick or a finger is driving it (StratString::isDriven).
    bool isDriven(int stringIndex) const noexcept;
    // A finger rub is active on the string (StratString::isRubbing).
    bool isRubbing(int stringIndex) const noexcept;
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
    // The string's place over each coil at its fret and bend.
    void updateCoilGeometry(int stringIndex) noexcept;
    // One sample of every sounding string, and of every string while the
    // neck moves, summed into each coil's EMF.
    void stepStrings(float (&emf)[coilCount]) noexcept;
    // The coils' EMF through this guitar's coils and circuit: the jack's
    // voltage at the same instant.
    double stepCircuit(const float (&emf)[coilCount]) noexcept;

    std::array<StratString, stringCount> strings {};
    double masterTune = 1.0;
    StratCircuit::Guitar guitar = StratCircuit::Guitar::strat;
    StratNeck neck;
    std::array<Pending, stringCount> pending {};
    std::array<int, stringCount> quietSamples {};
    std::array<int, stringCount> quietLimit {};
    // The coil reads velocity times the flux law's slope at the string's
    // distance from the poles: steel[s] times fluxTable at that distance in
    // table steps, which is the gap at the string's fret less the swing
    // across the coil.
    static constexpr int fluxTableSize = 257;
    std::array<float, fluxTableSize> fluxTable {};
    std::array<float, stringCount> steel {};
    std::array<std::array<float, coilCount>, stringCount> gapSteps {};
    std::array<std::array<float, coilCount>, stringCount> swing {};
    std::array<std::array<float, coilCount>, stringCount> swingTop {};
    std::array<std::array<float, coilCount>, stringCount> bendOffset {};
    float swingLeak = 1.0f;
    float swingSteps = 0.0f; // table steps per (m/s) sample of swing
    double stringRate = 96000.0; // the strings', coils' and circuit's rate
    StratBridge tremolo;
    bool bridgeMoved = false;
    TeleCoils teleCoils;
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
    std::array<float, 2 * halfbandHistory> jackHistory {};
    int jackWrite = 0;
    int jackQuiet = halfbandHistory;
};

} // namespace electry
