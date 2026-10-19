#include "DSP/ElectryPerformer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>

namespace
{
bool allocationForbidden = false;
int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

using Buffer = std::array<float, 1024>;

electry::PerformerParameters defaultParameters()
{
    electry::PerformerParameters result;
    electry::applyGuitarBuild(result.engine, electry::defaultGuitarBuild);
    result.engine.outputGain = std::pow(10.0f, -6.0f / 20.0f);
    return result;
}

void writtenNotesMatchCanonicalEngine(double rate)
{
    auto performer = std::make_unique<electry::ElectryPerformer>();
    auto reference = std::make_unique<electry::ElectryEngine>();
    const auto parameters = defaultParameters();
    performer->prepare(rate, 64);
    performer->setParameters(parameters);
    reference->prepare(rate, 64);
    reference->setParameters(parameters.engine);
    Buffer actualLeft {}, actualRight {}, expectedLeft {}, expectedRight {};
    const std::array<electry::ElectryEngine::NoteOnEvent, 3> written {{
        {40, 0.84f, 0}, {47, 0.73f, 0}, {52, 0.65f, 0}
    }};
    auto sounding = written;
    for (auto& note : sounding) note.midiNote -= 12;
    allocationForbidden = true;
    performer->noteOnChord(written.data(), written.size());
    reference->noteOnChord(sounding.data(), sounding.size());
    performer->render(actualLeft.data(), actualRight.data(), actualLeft.size());
    reference->process(expectedLeft.data(), expectedRight.data(), expectedLeft.size());
    allocationForbidden = false;
    expect(actualLeft == expectedLeft && actualRight == expectedRight,
           "written chord maps exactly onto the canonical sounding engine");
    expect(std::any_of(actualLeft.begin(), actualLeft.end(), [](float x) { return x != 0.0f; }),
           "written low E chord is audible");

    allocationForbidden = true;
    performer->setPitchBend(1.0f);
    performer->setMasterTuneCents(50.0f);
    reference->setPitchBendSemitones(2.5f);
    performer->render(actualLeft.data(), actualRight.data(), actualLeft.size());
    reference->process(expectedLeft.data(), expectedRight.data(), expectedLeft.size());
    allocationForbidden = false;
    expect(actualLeft == expectedLeft && actualRight == expectedRight,
           "master tuning adds to the full pitch wheel range");

    allocationForbidden = true;
    for (const auto& note : written) performer->noteOff(note.midiNote);
    for (const auto& note : sounding) reference->noteOff(note.midiNote);
    performer->render(actualLeft.data(), actualRight.data(), actualLeft.size());
    reference->process(expectedLeft.data(), expectedRight.data(), expectedLeft.size());
    allocationForbidden = false;
    expect(actualLeft == expectedLeft && actualRight == expectedRight,
           "written releases preserve the canonical damped tail");
}

void performanceKeysAndDoubleRig()
{
    auto performer = std::make_unique<electry::ElectryPerformer>();
    auto parameters = defaultParameters();
    performer->prepare(48000.0, 64);
    performer->setParameters(parameters);
    Buffer left {}, right {};
    allocationForbidden = true;
    performer->selectArticulation(3); // base Sustain
    performer->setPlayStyleKeysHold(true);
    performer->noteOn(16, 0.8f); // Palm Mute
    expect(performer->getPlayStyle() == 0 && performer->getEffectivePlayStyle() == 1,
           "HOLD selects a temporary style without replacing the base");
    performer->noteOn(21, 0.9f); // Dead takes precedence
    performer->noteOff(21);
    expect(performer->getEffectivePlayStyle() == 1,
           "releasing the newest HOLD key restores the previous held style");
    performer->noteOff(16);
    expect(performer->getEffectivePlayStyle() == 0,
           "releasing all HOLD keys restores the base style");
    performer->noteOn(24, 1.0f);
    performer->noteOn(26, 1.0f);
    expect(performer->getSoloStringMask() == 5, "written solo keys map to physical strings");
    performer->noteOff(24);
    expect(performer->getSoloStringMask() == 4, "HOLD solo release preserves the remaining string");
    performer->noteOn(22, 0.7f);
    performer->noteOn(23, 0.8f);
    expect(performer->getVibratoGesture() > 0 && performer->getTremoloGesture() > 0,
           "gesture note attacks engage both performance controls");
    performer->allNotesOff();
    expect(performer->getSoloStringMask() == 0 && performer->getVibratoGesture() == 0
           && performer->getTremoloGesture() == 0,
           "all notes off releases temporary solo and gesture owners");

    parameters.outputMode = 2;
    parameters.fxEnabled = true;
    parameters.effects.amp = 0.4f;
    parameters.effects.room = 0.2f;
    performer->setParameters(parameters);
    performer->noteOn(52, 0.9f);
    performer->render(left.data(), right.data(), left.size());
    expect(left != right, "Double renders independent deterministic guitar performances");
    expect(std::all_of(left.begin(), left.end(), [](float x) { return std::isfinite(x); })
           && std::all_of(right.begin(), right.end(), [](float x) { return std::isfinite(x); }),
           "Double and FX produce finite audio");
    parameters.fxEnabled = false;
    performer->setParameters(parameters);
    performer->render(left.data(), right.data(), left.size());
    performer->reset();
    performer->render(left.data(), right.data(), left.size());
    expect(std::all_of(left.begin(), left.end(), [](float x) { return x == 0.0f; })
           && std::all_of(right.begin(), right.end(), [](float x) { return x == 0.0f; }),
           "reset clears the complete guitar and effect tail");
    allocationForbidden = false;
}

void ignoredKeysCannotReleasePlayableNotes()
{
    auto actual = std::make_unique<electry::ElectryPerformer>();
    auto reference = std::make_unique<electry::ElectryPerformer>();
    const auto parameters = defaultParameters();
    for (auto* performer : {actual.get(), reference.get()})
    {
        performer->prepare(48000.0, 64);
        performer->setParameters(parameters);
        performer->noteOn(47, 0.9f); // written B2, sounding MIDI 35
    }
    Buffer left {}, right {}, referenceLeft {}, referenceRight {};
    allocationForbidden = true;
    actual->render(left.data(), right.data(), left.size());
    reference->render(referenceLeft.data(), referenceRight.data(), referenceLeft.size());
    actual->noteOn(35, 0.9f); // unassigned gap below written playable range
    actual->noteOff(35);
    actual->render(left.data(), right.data(), left.size());
    reference->render(referenceLeft.data(), referenceRight.data(), referenceLeft.size());
    allocationForbidden = false;
    expect(left == referenceLeft && right == referenceRight,
           "an ignored written key cannot release a different sounding note");
}

struct PitchRangeFixture
{
    std::unique_ptr<electry::ElectryPerformer> performer =
        std::make_unique<electry::ElectryPerformer>();
    std::unique_ptr<electry::ElectryEngine> leadReference, doubleReference;
    Buffer left {}, right {}, expectedLeft {}, expectedRight {};

    PitchRangeFixture()
    {
        performer->prepare(48000.0, 64);
        auto parameters = defaultParameters();
        parameters.outputMode = 2;
        performer->setParameters(parameters);
        strikeAndCopy();
    }

    void strikeAndCopy()
    {
        // Sounding E4 keeps even a two-octave dive above the existing 20 Hz
        // pitch floor; this tests the controller interval, not that limiter.
        performer->noteOn(76, 0.85f);
        leadReference = std::make_unique<electry::ElectryEngine>(performer->getEngine());
        doubleReference = std::make_unique<electry::ElectryEngine>(performer->getDoubleEngine());
    }

    void compare(float semitones, const char* message)
    {
        leadReference->setPitchBendSemitones(semitones);
        doubleReference->setPitchBendSemitones(semitones);
        // Process both owned engines independently so neither half of Double
        // can hide a missing controller update behind the other channel.
        performer->getEngine().process(left.data(), right.data(), left.size());
        leadReference->process(expectedLeft.data(), expectedRight.data(), expectedLeft.size());
        expect(left == expectedLeft && right == expectedRight, message);
        expect(std::any_of(left.begin(), left.end(), [](float v) { return v != 0.0f; }),
               "pitch range fixture has an audible lead string");
        performer->getDoubleEngine().process(left.data(), right.data(), left.size());
        doubleReference->process(expectedLeft.data(), expectedRight.data(), expectedLeft.size());
        expect(left == expectedLeft && right == expectedRight, message);
        expect(std::any_of(left.begin(), left.end(), [](float v) { return v != 0.0f; }),
               "pitch range fixture has an audible Double string");
    }
};

void pitchBendRangeAndBounds()
{
    PitchRangeFixture fixture;
    struct Case { float range, wheel, expected; };
    const std::array<Case, 10> cases {{
        {0.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 2.0f},
        {12.0f, -1.0f, -12.0f}, {24.0f, -1.0f, -24.0f},
        {24.0f, 1.0f, 24.0f}, {-8.0f, 1.0f, 0.0f},
        {40.0f, 1.0f, 24.0f},
        {std::numeric_limits<float>::quiet_NaN(), 1.0f, 2.0f},
        {std::numeric_limits<float>::infinity(), 1.0f, 2.0f},
        {-std::numeric_limits<float>::infinity(), -1.0f, -2.0f},
    }};
    allocationForbidden = true;
    for (const auto& value : cases)
    {
        fixture.performer->setPitchBendRangeSemitones(value.range);
        fixture.performer->setPitchBend(value.wheel);
        fixture.compare(value.expected, "range and hostile inputs preserve the expected interval on both engines");
    }
    allocationForbidden = false;
}

void heldPitchBendRangeRetainsTuningAndReset()
{
    PitchRangeFixture fixture;
    allocationForbidden = true;
    fixture.performer->setPitchBend(0.5f);
    fixture.performer->setMasterTuneCents(50.0f);
    fixture.compare(1.5f, "default range remains two semitones plus master tuning");
    // Do not resend the wheel: range automation must affect a held bend now.
    fixture.performer->setPitchBendRangeSemitones(12.0f);
    fixture.compare(6.5f, "a live range change updates both held engines");
    fixture.performer->setPitchBendRangeSemitones(24.0f);
    fixture.compare(12.5f, "a second live range change preserves master tuning");
    fixture.performer->setMasterTuneCents(-100.0f);
    fixture.compare(11.0f, "master tuning remains independent at an extended range");
    fixture.performer->setPitchBendRangeSemitones(0.0f);
    fixture.compare(-1.0f, "zero bend range preserves master tuning");
    fixture.performer->setPitchBendRangeSemitones(24.0f);
    fixture.performer->resetControllers();
    fixture.compare(-1.0f, "controller reset recenters the wheel without clearing tuning");
    fixture.performer->setPitchBend(0.5f);
    fixture.compare(11.0f, "controller reset retains the configured bend range");
    fixture.performer->reset();
    allocationForbidden = false;
    fixture.strikeAndCopy();
    allocationForbidden = true;
    fixture.performer->setPitchBend(-0.5f);
    fixture.compare(-13.0f, "instrument reset retains range and master tuning");
    allocationForbidden = false;
}
}

void* operator new(std::size_t size)
{
    if (allocationForbidden)
    {
        std::fputs("Realtime performer allocated memory\n", stderr);
        std::abort();
    }
    if (void* result = std::malloc(size == 0 ? 1 : size)) return result;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

int main()
{
    for (const double rate : {44100.0, 48000.0, 96000.0, 192000.0})
        writtenNotesMatchCanonicalEngine(rate);
    performanceKeysAndDoubleRig();
    ignoredKeysCannotReleasePlayableNotes();
    pitchBendRangeAndBounds();
    heldPitchBendRangeRetainsTuningAndReset();
    if (failures == 0) std::puts("Electry performer contracts passed");
    return failures == 0 ? 0 : 1;
}
