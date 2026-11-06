#pragma once

#include "StratEngine.h"

#include <array>
#include <cstdint>
#include <memory>

namespace electry
{

// The player's settings: the guitar's own switch and knobs, then the hands.
struct StratControls
{
    StratCircuit::Controls circuit {};
    double pickMetres = 0.130;    // pick position from the bridge, 0.025 to 0.250
    double pick = 0.5;            // plectrum, 0 thin to 1 heavy
    double palmMute = 0.5;        // palm pressure while Palm mute is on, 0 to 1
    double strumSeconds = 0.008;  // pick travel from one string to the next, 0 to 0.040
    double outputDecibels = 0.0;  // -24 to +12

    bool operator==(const StratControls& o) const noexcept
    {
        return circuit == o.circuit && pickMetres == o.pickMetres && pick == o.pick
            && palmMute == o.palmMute && strumSeconds == o.strumSeconds
            && outputDecibels == o.outputDecibels;
    }
};

// MIDI to a guitarist's hands. Notes 40 to 85 sound at concert pitch on the
// six strings and 21 frets; C1 to F1 (24 to 29) latch Open, Palm mute, Dead
// note, Legato, Slide and Harmonic. Channel 1's pitch bend is the tremolo
// arm; pitch bend on channels 2 to 16 bends that channel's string at MPE's
// 48-semitone range. The mod wheel adds fretting-hand vibrato and the
// sustain pedal lets released notes ring.
//
// Events take effect where they are called: render up to an event's sample,
// then send it. prepare() allocates; nothing after it does.
class StratPerformer
{
public:
    static constexpr int lowestNote = 40;
    static constexpr int highestNote = 85;
    static constexpr int frets = 21;

    enum class Articulation : std::uint8_t
    {
        open,
        palmMute,
        deadNote,
        legato,
        slide,
        harmonic
    };
    static constexpr int firstArticulationKey = 24;
    static constexpr int articulationCount = 6;

    StratPerformer();
    void prepare(double sampleRate);
    void reset() noexcept;
    void setControls(const StratControls& controls) noexcept;
    const StratControls& controls() const noexcept { return current; }

    // channel 0 to 15; velocity 1 to 127 (0 is a note-off).
    void noteOn(int channel, int note, int velocity) noexcept;
    void noteOff(int channel, int note) noexcept;
    // -1 to +1.
    void pitchBend(int channel, double position) noexcept;
    // 0 to 1.
    void modWheel(double position) noexcept;
    void sustain(bool down) noexcept;
    void allNotesOff() noexcept;

    void render(float* out, int samples) noexcept;

    Articulation articulation() const noexcept { return latched; }
    const StratEngine& engine() const noexcept { return *guitar; }
    // The note a string is playing, or -1.
    int stringNote(int stringIndex) const noexcept;
    int stringFret(int stringIndex) const noexcept;

private:
    struct StringState
    {
        int note = -1;          // key that owns the string, -1 none
        int channel = 0;
        bool keyDown = false;
        bool sustained = false; // key up while the pedal is down
        double fret = 0.0;      // where the fretting hand is
        bool fretted = false;   // pressed to the fret (not touching)
        double bend = 1.0;      // finger bend and vibrato, pitch ratio
        bool slide = false;
        double slideFrom = 0.0, slideTo = 0.0;
        std::int64_t slideStart = 0, slideLength = 0;
        std::uint64_t order = 0;
    };

    enum class EventKind : std::uint8_t
    {
        strike,
        liftTouch
    };
    struct Event
    {
        std::int64_t time = 0;
        EventKind kind = EventKind::strike;
        int string = 0;
        double fret = 0.0;
        double force = 0.0;
        double touchMetres = 0.0;
        double touchResistance = 0.0;
        double touchWidth = 0.0;
    };

    struct Pending
    {
        int note = 0;
        int channel = 0;
        int velocity = 0;
    };

    double velocityForce(int velocity) const noexcept;
    double releaseSeconds(int stringIndex, double force) const noexcept;
    void startStroke() noexcept;
    bool playOnSoundingString(const Pending& p) noexcept;
    bool playHarmonic(const Pending& p, double force) noexcept;
    void assign(const Pending* notes, int count, std::array<int, 6>& stringFor) const noexcept;
    void schedule(const Event& e) noexcept;
    void runEvent(const Event& e) noexcept;
    void release(int stringIndex) noexcept;
    bool nextTiltOpposed() noexcept;
    static constexpr std::uint32_t tiltSeed = 0x9E3779B9u;
    void applyPalm() noexcept;
    void controlUpdate() noexcept;

    std::unique_ptr<StratEngine> guitar;
    StratControls current;
    double sampleRate = 48000.0;
    double outputGain = 1.0;
    Articulation latched = Articulation::open;

    std::array<StringState, StratEngine::stringCount> strings {};
    std::array<double, 16> channelBend {};
    double vibratoDepth = 0.0;
    double vibratoPhase = 0.0;
    bool pedal = false;
    double hand = 2.0;
    bool nextStrokeUp = false;
    std::uint32_t tilt = tiltSeed; // the picking hand's tilt from stroke to stroke
    std::int64_t lastStroke = -1000000;
    std::uint64_t orderCounter = 0;

    static constexpr int maxPending = 16;
    std::array<Pending, maxPending> pending {};
    int pendingCount = 0;

    static constexpr int maxEvents = 64;
    std::array<Event, maxEvents> events {};
    int eventCount = 0;

    std::int64_t now = 0;
};

} // namespace electry
