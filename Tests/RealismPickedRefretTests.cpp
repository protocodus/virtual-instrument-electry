#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    struct State
    {
        float frequency, targetFrequency, energy, blend;
        int pendingSamples;
        std::int64_t lastPick;
        bool active, releasing;
    };
    static State state(const ElectryEngine& engine, int string)
    {
        const auto& voice = engine.voices_[static_cast<std::size_t>(string)];
        const float period = voice.lastCompensatedPeriod
                           + voice.vertical.currentDelay - voice.vertical.targetDelay;
        return { static_cast<float>(engine.sampleRate_) / period,
                 static_cast<float>(engine.sampleRate_) / voice.lastCompensatedPeriod,
                 voice.outputEnergy, voice.legatoBlend, voice.startDelaySamples,
                 engine.lastPlectrumContactClock_, voice.active, voice.releasing };
    }
    static int internalFactor(const ElectryEngine& engine)
    {
        return engine.oversamplingFactor_;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
using Style = electry::PlayStyle;
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (! condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
float cents(float a, float b) { return 1200.0f * std::log2(a / b); }
float hz(int note) { return 440.0f * std::exp2((note - 69.0f) / 12.0f); }

std::unique_ptr<Engine> makeEngine(double rate, int string = 7, float spread = 0)
{
    auto engine = std::make_unique<Engine>();
    electry::EngineParameters parameters;
    parameters.pickupSelector = electry::PickupSelector::Neck;
    parameters.pickHardness = .72f;
    parameters.pickPosition = .27f;
    parameters.pickNoise = parameters.fingerNoise = parameters.releaseNoise = 0;
    parameters.artifactAmount = parameters.sympatheticAmount = 0;
    parameters.bodyResonance = 0;
    parameters.bendTimeSeconds = .15f;
    parameters.strumSpreadSeconds = spread;
    engine->setParameters(parameters);
    engine->prepare(rate, 127);
    engine->setSoloStringMask(static_cast<std::uint8_t>(1u << string));
    return engine;
}
void style(Engine& engine, Style value)
{
    engine.noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(value), 1);
}
void picked(Engine& engine, int note, float velocity = .79f)
{
    const std::array<Engine::NoteOnEvent, 1> notes {{{ note, velocity }}};
    engine.noteOnChord(notes);
}
std::vector<float> render(Engine& engine, int frames)
{
    std::vector<float> result(static_cast<std::size_t>(frames));
    std::array<float, 127> right {};
    for (int at = 0; at < frames; at += 127)
        engine.process(result.data() + at, right.data(), std::min(127, frames - at));
    expect(std::all_of(result.begin(), result.end(), [] (float x) {
        return std::isfinite(x) && std::abs(x) < 4;
    }), "picked fret movement remains finite and bounded");
    return result;
}
void wait(Engine& engine, double rate, double seconds)
{
    render(engine, static_cast<int>(std::lround(rate * seconds)));
}

double audioPitchCents(const std::vector<float>& audio, double rate, float target)
{
    // The first twenty milliseconds after pick contact must already describe
    // the destination note. Looking only at a settled tail misses the former
    // automatic scoop, which had almost disappeared thirty milliseconds in.
    constexpr double pi = 3.14159265358979323846;
    double best = 0, bestPower = -1;
    for (double detune = -160; detune <= 160; detune += .5)
    {
        const double angle = 2 * pi * target * std::exp2(detune / 1200) / rate;
        const double dc = std::cos(angle), ds = std::sin(angle);
        double c = 1, s = 0, real = 0, imag = 0;
        for (std::size_t i = 0; i < audio.size(); ++i)
        {
            const double weight = .5 - .5 * std::cos(2 * pi * i / (audio.size() - 1));
            real += weight * audio[i] * c;
            imag += weight * audio[i] * s;
            const double next = c * dc - s * ds;
            s = s * dc + c * ds; c = next;
        }
        const double power = real * real + imag * imag;
        if (power > bestPower) { bestPower = power; best = detune; }
    }
    return best;
}

void stoppedPickedNotes(double rate)
{
    for (int string : { 0, 7 })
        for (const auto pair : { std::array<int, 2>{0, 1}, {0, 2}, {0, 3},
                                {0, 4}, {3, 0}, {4, 0} })
            for (double gap : { -1.0, .008, .095 })
                for (Style articulation : { Style::Sustain, Style::PalmMute })
                {
                    auto engine = makeEngine(rate, string);
                    const int first = (string == 7 ? 69 : 28) + pair[0];
                    const int second = (string == 7 ? 69 : 28) + pair[1];
                    picked(*engine, first); wait(*engine, rate, .20);
                    if (gap >= 0) { engine->noteOff(first); wait(*engine, rate, gap); }
                    style(*engine, articulation); picked(*engine, second, .72f);
                    const auto contact = Access::state(*engine, string);
                    expect(std::abs(cents(contact.frequency, contact.targetFrequency)) < .1f,
                           "ordinary picked fret starts at its stopped pitch: "
                           + std::to_string(first) + "->" + std::to_string(second)
                           + " at " + std::to_string(rate) + " Hz");
                    wait(*engine, rate, .025);
                }

    auto engine = makeEngine(rate);
    picked(*engine, 69); wait(*engine, rate, .36136);
    engine->noteOff(69); wait(*engine, rate, .09545);
    picked(*engine, 72, .72f);
    wait(*engine, rate, .002);
    const auto attack = render(*engine, static_cast<int>(std::lround(rate * .018)));
    const double measured = audioPitchCents(attack, rate, hz(72));
    std::cout << "Picked A4->C5 first attack at " << rate << " Hz: "
              << measured << " cents from destination\n";
    expect(std::abs(measured) < 20,
           "actual picked attack reaches C5 instead of scooping from the old A4");
}

void preserveMovingFinger(double rate)
{
    for (bool command : { false, true })
    {
        auto engine = makeEngine(rate);
        picked(*engine, 69); wait(*engine, rate, .20);
        engine->setPitchBend(1); wait(*engine, rate, .008);
        const auto before = Access::state(*engine, 7);
        if (command) engine->noteOn(Engine::firstRepickNote + 7, .79f);
        else picked(*engine, 69);
        const auto after = Access::state(*engine, 7);
        expect(std::abs(cents(before.frequency, before.targetFrequency)) > 2,
               "same-note repick fixture has an unfinished real bend");
        expect(std::abs(cents(after.frequency, before.frequency)) < 1,
               "same-note repick preserves the moving finger's actual pitch");
        expect(std::abs(cents(after.frequency, after.targetFrequency)) > 2,
               "same-note repick does not snap an unfinished bend to its target");
    }
    for (Style articulation : { Style::Hammer, Style::Slide })
    {
        auto engine = makeEngine(rate);
        picked(*engine, 69); wait(*engine, rate, .20);
        const auto source = Access::state(*engine, 7);
        style(*engine, articulation); picked(*engine, 72);
        const auto contact = Access::state(*engine, 7);
        expect(std::abs(cents(contact.frequency, source.frequency)) < 1,
               "true Hammer/Slide starts continuously at the source pitch");
        expect(contact.blend == 0, "true fretting gesture retains its travel");
        wait(*engine, rate, .005);
        const auto moving = Access::state(*engine, 7);
        style(*engine, Style::Sustain); picked(*engine, 72);
        const auto repicked = Access::state(*engine, 7);
        expect(moving.blend < 1 && repicked.blend == moving.blend,
               "repicking a travelling finger preserves unfinished Hammer/Slide");
        expect(std::abs(cents(repicked.frequency, moving.frequency)) < 1,
               "repicking a travelling finger introduces no pitch snap");
    }
}

void delayedStrumKeepsContact(double rate)
{
    for (bool cancel : { false, true })
    {
        auto engine = makeEngine(rate, 7, .020f);
        picked(*engine, 69); wait(*engine, rate, .20);
        const auto before = Access::state(*engine, 7);
        // Scalar note streams have a real pre-roll while the chord edge is
        // unknown. The new fret is stopped now; its pick must still wait.
        engine->noteOn(72, .72f);
        const auto pending = Access::state(*engine, 7);
        expect(pending.active && pending.pendingSamples > 0,
               "changed-fret strum retains its live string and pending pick");
        expect(pending.lastPick == before.lastPick && pending.energy == before.energy,
               "stopping the new fret neither picks early nor erases the old ring");
        expect(std::abs(cents(pending.frequency, pending.targetFrequency)) < .1f,
               "delayed pick approaches the already stopped destination fret");
        const int untilContact = pending.pendingSamples / Access::internalFactor(*engine);
        render(*engine, untilContact / 2);
        expect(Access::state(*engine, 7).lastPick == before.lastPick,
               "ringing during strum travel does not create an early pick");
        if (cancel) engine->noteOff(72);
        render(*engine, untilContact + 2);
        const auto after = Access::state(*engine, 7);
        expect(cancel ? after.lastPick == before.lastPick : after.lastPick > before.lastPick,
               cancel ? "cancelled changed-fret strum never picks after release"
                      : "changed-fret strum picks only when its reservation arrives");
    }
}
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        stoppedPickedNotes(rate);
        preserveMovingFinger(rate);
        delayedStrumKeepsContact(rate);
    }
    if (failures) std::cerr << failures << " picked-refret checks failed\n";
    else std::cout << "Picked frets start in tune; moving fingers and strum contact survive\n";
    return failures ? 1 : 0;
}
