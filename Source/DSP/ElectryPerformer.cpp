#include "ElectryPerformer.h"
#include <algorithm>
#include <limits>

namespace electry
{
namespace
{
template<class T> T bounded(T low, T high, T value) noexcept
{ return value < low ? low : value > high ? high : value; }
bool finite(float value) noexcept
{ return value == value && value <= std::numeric_limits<float>::max()
                       && value >= -std::numeric_limits<float>::max(); }
int roundToInt(float value) noexcept { return static_cast<int>(value + 0.5f); }
}

ElectryPerformer::ElectryPerformer()
    : doubleEngine(std::make_unique<ElectryEngine>())
{
    doubleEngine->setVariationSeed(0x9e3779b9u);
}

void ElectryPerformer::prepare(double sampleRate, int maximumBlockSize)
{
    sampleRate = sampleRate == sampleRate ? bounded(8000.0, 384000.0, sampleRate) : 44100.0;
    engine.prepare(sampleRate, std::max(0, maximumBlockSize));
    doubleEngine->prepare(sampleRate, std::max(0, maximumBlockSize));
    effects.prepare(sampleRate);
    fxRampLength = static_cast<int>(sampleRate * 0.005);
    resetControllers();
    reset();
}

bool ElectryPerformer::isAttackConditioningNote(int note) noexcept
{
    return ElectryEngine::isKeyswitchNote(note)
        || ElectryEngine::isVibratoGestureNote(note)
        || ElectryEngine::isTremoloGestureNote(note)
        || ElectryEngine::isMidiSoloStringNote(note)
        || ElectryEngine::isMidiSoloClearNote(note);
}

void ElectryPerformer::setParameters(const PerformerParameters& parameters) noexcept
{
    setEngineParameters(parameters.engine, parameters.outputMode);
    setEffectParameters(parameters.effects, parameters.fxEnabled);
}

void ElectryPerformer::setEngineParameters(const EngineParameters& parameters,
                                          int outputMode) noexcept
{
    auto next = parameters;
    next.outputMode = outputMode == 1 ? OutputMode::Stereo : OutputMode::Mono;
    engine.setParameters(next);
    next.outputMode = OutputMode::Mono;
    doubleEngine->setParameters(next);
    if (!doubleModeActive && outputMode == 2)
    {
        doubleEngine->reset();
        doubleEngine->noteOn(ElectryEngine::firstKeyswitchNote
                            + static_cast<int>(engine.getCurrentPickStyle()), 1.0f);
        doubleEngine->noteOn(ElectryEngine::firstPlayStyleKeyswitchNote
                            + static_cast<int>(engine.getCurrentPlayStyle()), 1.0f);
        doubleEngine->setSustainPedal(sustainPedalDown);
        doubleEngine->setSoloStringMask(engine.getSoloStringMask());
    }
    doubleModeActive = outputMode == 2;
}

void ElectryPerformer::setEffectParameters(const FxParameters& next, bool enabled) noexcept
{
    effects.setParameters(next);
    if (enabled && fxWet == 0.0f && fxWetTarget == 0.0f)
        effects.reset();
    effectsEnabled = enabled;
    const float target = enabled ? 1.0f : 0.0f;
    if (target != fxWetTarget)
    {
        fxWetTarget = target;
        fxRampRemaining = fxRampLength;
        fxWetStep = (target - fxWet) / static_cast<float>(fxRampLength);
    }
    const float acousticLevel = enabled ? std::min(1.0f, next.amp + 0.6f * next.distortion) : 0.0f;
    engine.setAcousticReturnLevel(acousticLevel);
    doubleEngine->setAcousticReturnLevel(acousticLevel);
}

void ElectryPerformer::resetEffects() noexcept
{
    effects.reset();
    fxWet = fxWetTarget = effectsEnabled ? 1.0f : 0.0f;
    fxRampRemaining = 0;
}

void ElectryPerformer::reset() noexcept
{
    reset(pickStyleIndex, appliedBasePlayStyleIndex);
}

void ElectryPerformer::reset(int pickStyle, int playStyle) noexcept
{
    pickStyleIndex = bounded(0, ElectryEngine::pickStyleKeyswitchCount - 1, pickStyle);
    appliedBasePlayStyleIndex = bounded(0, ElectryEngine::playStyleKeyswitchCount - 1, playStyle);
    clearHeldPlayStyles();
    clearHeldSoloStrings();
    applySoloStringMask(0);
    clearVibratoGesture();
    clearTremoloGesture();
    setSustainPedal(false);
    resetEngineWithArticulations(pickStyleIndex, appliedBasePlayStyleIndex);
    resetEffects();
}

void ElectryPerformer::allSoundOff() noexcept
{
    resetEngineWithArticulations(static_cast<int>(engine.getCurrentPickStyle()),
                                static_cast<int>(engine.getCurrentPlayStyle()));
    setSustainPedal(sustainPedalDown);
    effects.reset();
}

void ElectryPerformer::allNotesOff() noexcept
{
    releasePerformanceKeys();
    releasePlayableNotes();
}

void ElectryPerformer::resetControllers() noexcept
{
    setPalmMutePressure(0.0f);
    setPitchBend(0.0f);
    setResonance(0.0f);
    setSustainPedal(false);
}

void ElectryPerformer::selectArticulation(int index) noexcept
{
    if (index >= 0 && index < ElectryEngine::keyswitchCount)
        noteOn(ElectryEngine::firstKeyswitchNote + index, 1.0f, true);
}

void ElectryPerformer::setPlayStyleKeysHold(bool hold) noexcept
{
    if (hold == appliedPlayStyleKeysHold)
        return;
    appliedPlayStyleKeysHold = hold;
    clearHeldPlayStyles();
    clearHeldSoloStrings();
    applySoloStringMask(0);
    applyPlayStyle(appliedBasePlayStyleIndex);
}

void ElectryPerformer::setSustainPedal(bool down) noexcept
{
    sustainPedalDown = down;
    engine.setSustainPedal(down);
    doubleEngine->setSustainPedal(down);
}

void ElectryPerformer::setResonance(float value) noexcept
{
    engine.setResonance(value);
    doubleEngine->setResonance(value);
}

void ElectryPerformer::setPalmMutePressure(float value) noexcept
{
    engine.setPalmMutePressure(value);
    doubleEngine->setPalmMutePressure(value);
}

void ElectryPerformer::setPitchBend(float value) noexcept
{
    pitchBend = finite(value) ? bounded(-1.0f, 1.0f, value) : 0.0f;
    updatePitchBend();
}

void ElectryPerformer::setMasterTuneCents(float value) noexcept
{
    masterTuneCents = finite(value) ? bounded(-100.0f, 100.0f, value) : 0.0f;
    updatePitchBend();
}

void ElectryPerformer::updatePitchBend() noexcept
{
    engine.setPitchBendSemitones(2.0f * pitchBend + 0.01f * masterTuneCents);
    doubleEngine->setPitchBendSemitones(2.0f * pitchBend + 0.01f * masterTuneCents);
}

void ElectryPerformer::noteOnChord(const ElectryEngine::NoteOnEvent* notes,
                                  std::size_t count) noexcept
{
    if (notes == nullptr)
        return;
    std::array<ElectryEngine::NoteOnEvent, ElectryEngine::maximumChordEvents> mapped {};
    std::size_t size = 0;
    const auto flush = [&]()
    {
        if (size == 0) return;
        engine.noteOnChord(mapped.data(), size);
        if (doubleModeActive) doubleEngine->noteOnChord(mapped.data(), size);
        size = 0;
    };
    // Bound host-supplied work to one full MIDI note range per call.
    count = std::min(count, mapped.size());
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto& note = notes[index];
        if (!finite(note.velocity)) continue;
        if (!ElectryEngine::isMidiPlayableNote(note.midiNote) || note.velocity <= 0.0f)
        {
            flush();
            noteOn(note.midiNote, note.velocity);
            continue;
        }
        mapped[size++] = {note.midiNote - ElectryEngine::octaveShiftSemitones,
                          bounded(0.0f, 1.0f, note.velocity), note.expressionId};
    }
    flush();
}

void ElectryPerformer::noteOff(int note, ElectryEngine::ExpressionId expressionId) noexcept
{
    if (!ElectryEngine::isMidiPlayableNote(note))
    {
        noteOff(note);
        return;
    }
    const int engineNote = note - ElectryEngine::octaveShiftSemitones;
    engine.noteOff(engineNote, expressionId);
    doubleEngine->noteOff(engineNote, expressionId);
}

void ElectryPerformer::render(float* left, float* right, int samples) noexcept
{
    if (left == nullptr || right == nullptr || samples <= 0)
        return;
    // Keep the acoustic loop causal independently of a host's block size.
    // Event boundaries may shorten a chunk; the engine FIFO retains the same
    // physical round-trip delay in either host.
    const int feedbackChunkSize = engine.getAcousticReturnDelaySamples();
    while (samples > 0)
    {
        const int count = std::min(feedbackChunkSize, samples);
        engine.process(left, right, count);
        if (doubleModeActive) doubleEngine->process(right, right, count);
        processEffects(left, right, count);
        if (doubleModeActive)
        {
            engine.pushAcousticReturn(left, nullptr, count);
            doubleEngine->pushAcousticReturn(right, nullptr, count);
        }
        else engine.pushAcousticReturn(left, right, count);
        left += count;
        right += count;
        samples -= count;
    }
}

void ElectryPerformer::processEffects(float* left, float* right, int samples) noexcept
{
    while (samples > 0)
    {
        if (fxRampRemaining == 0)
        {
            if (fxWet == 1.0f) effects.process(left, right, samples);
            return;
        }
        const int count = std::min(samples,
                                  static_cast<int>(fxDryScratch[0].size()));
        std::copy_n(left, count, fxDryScratch[0].begin());
        std::copy_n(right, count, fxDryScratch[1].begin());
        effects.process(left, right, count);
        for (int i = 0; i < count; ++i)
        {
            if (fxRampRemaining != 0)
            {
                fxWet += fxWetStep;
                if (--fxRampRemaining == 0) fxWet = fxWetTarget;
            }
            const auto index = static_cast<std::size_t>(i);
            if (fxWet == 0.0f)
            {
                left[i] = fxDryScratch[0][index];
                right[i] = fxDryScratch[1][index];
            }
            else if (fxWet < 1.0f)
            {
                left[i] = fxDryScratch[0][index] + fxWet * (left[i] - fxDryScratch[0][index]);
                right[i] = fxDryScratch[1][index] + fxWet * (right[i] - fxDryScratch[1][index]);
            }
        }
        left += count;
        right += count;
        samples -= count;
        if (fxRampRemaining == 0 && fxWet == 0.0f) effects.reset();
    }
}

void ElectryPerformer::releasePerformanceKeys() noexcept
{
    clearHeldPlayStyles();
    if (appliedPlayStyleKeysHold)
    {
        clearHeldSoloStrings();
        applySoloStringMask (0);
    }
    clearVibratoGesture();
    clearTremoloGesture();
    applyPlayStyle (appliedBasePlayStyleIndex);
}

void ElectryPerformer::releasePlayableNotes() noexcept
{
    engine.allNotesOff();
    doubleEngine->allNotesOff();
}

void ElectryPerformer::noteOn (
    int note, float velocity, bool selectsBaseArticulation) noexcept
{
    if (note < 0 || note > 127 || ! finite(velocity))
        return;
    if (velocity <= 0.0f)
    {
        noteOff(note);
        return;
    }
    velocity = bounded(0.0f, 1.0f, velocity);
    if (ElectryEngine::isVibratoGestureNote (note))
    {
        if (vibratoGestureOwners < std::numeric_limits<std::uint16_t>::max())
            ++vibratoGestureOwners;
        applyVibratoGesture (velocity);
        return;
    }

    if (ElectryEngine::isTremoloGestureNote (note))
    {
        if (tremoloGestureOwners < std::numeric_limits<std::uint16_t>::max())
            ++tremoloGestureOwners;
        applyTremoloGesture (velocity);
        return;
    }

    if (ElectryEngine::isMidiSoloClearNote (note))
    {
        clearHeldSoloStrings();
        applySoloStringMask (0);
        return;
    }

    if (ElectryEngine::isMidiSoloStringNote (note))
    {
        const int stringIndex = note - ElectryEngine::firstMidiSoloStringNote;
        if (appliedPlayStyleKeysHold)
        {
            auto& count = heldSoloStringCounts[static_cast<std::size_t> (stringIndex)];
            if (count < std::numeric_limits<std::uint16_t>::max())
                ++count;
            applySoloStringMask (computeHeldSoloStringMask());
        }
        else
        {
            const auto toggleBit = static_cast<std::uint8_t> (1u << stringIndex);
            const std::uint8_t nextMask = latchedSoloStringMask ^ toggleBit;
            latchedSoloStringMask = nextMask;
            applySoloStringMask (nextMask);
        }
        return;
    }

    const int articulation = note - ElectryEngine::firstKeyswitchNote;
    if (articulation < 0
        || articulation >= ElectryEngine::keyswitchCount)
    {
        if (ElectryEngine::isMidiPlayableNote (note)
            || ElectryEngine::isMidiRepickNote (note))
        {
            const int engineNote = note - ElectryEngine::octaveShiftSemitones;
            engine.noteOn (engineNote, velocity);
            if (doubleModeActive)
                doubleEngine->noteOn (engineNote, velocity);
        }
        return;
    }

    if (articulation < ElectryEngine::pickStyleKeyswitchCount)
    {
        pickStyleIndex = articulation;
        engine.noteOn (note, velocity);
        doubleEngine->noteOn (note, velocity);
        return;
    }

    const int style = articulation
                    - ElectryEngine::pickStyleKeyswitchCount;
    if (selectsBaseArticulation)
    {
        appliedBasePlayStyleIndex = style;
        if (! appliedPlayStyleKeysHold || latestHeldPlayStyle() < 0)
            applyPlayStyle (style);
        return;
    }

    if (! appliedPlayStyleKeysHold)
    {
        appliedBasePlayStyleIndex = style;
        applyPlayStyle (style);
        return;
    }

    auto& count = heldPlayStyleCounts[static_cast<std::size_t> (style)];
    if (count < std::numeric_limits<std::uint16_t>::max())
        ++count;
    heldPlayStyleOrder[static_cast<std::size_t> (style)] = ++heldPlayStyleSequence;
    applyPlayStyle (style);
}

void ElectryPerformer::noteOff (int note) noexcept
{
    if (note < 0 || note > 127)
        return;
    if (ElectryEngine::isVibratoGestureNote (note))
    {
        if (vibratoGestureOwners == 0)
            return;
        if (--vibratoGestureOwners == 0)
            applyVibratoGesture (0.0f);
        return;
    }

    if (ElectryEngine::isTremoloGestureNote (note))
    {
        if (tremoloGestureOwners == 0)
            return;
        if (--tremoloGestureOwners == 0)
            applyTremoloGesture (0.0f);
        return;
    }

    if (ElectryEngine::isMidiSoloClearNote (note))
        return;

    if (ElectryEngine::isMidiSoloStringNote (note))
    {
        if (appliedPlayStyleKeysHold)
        {
            const int stringIndex = note - ElectryEngine::firstMidiSoloStringNote;
            auto& count = heldSoloStringCounts[static_cast<std::size_t> (stringIndex)];
            if (count > 0)
            {
                --count;
                applySoloStringMask (computeHeldSoloStringMask());
            }
        }
        return;
    }

    const int style = note - ElectryEngine::firstPlayStyleKeyswitchNote;
    if (! appliedPlayStyleKeysHold || style < 0
        || style >= ElectryEngine::playStyleKeyswitchCount)
    {
        const int engineNote = ElectryEngine::isMidiPlayableNote (note)
            ? note - ElectryEngine::octaveShiftSemitones
            : note;
        engine.noteOff (engineNote);
        doubleEngine->noteOff (engineNote);
        return;
    }

    auto& count = heldPlayStyleCounts[static_cast<std::size_t> (style)];
    if (count == 0)
        return;
    --count;
    if (count == 0)
        heldPlayStyleOrder[static_cast<std::size_t> (style)] = 0;

    const int held = latestHeldPlayStyle();
    applyPlayStyle (held >= 0 ? held : appliedBasePlayStyleIndex);
}

void ElectryPerformer::applyPlayStyle (int styleIndex) noexcept
{
    styleIndex = bounded (
        0, ElectryEngine::playStyleKeyswitchCount - 1, styleIndex);
    engine.noteOn (ElectryEngine::firstPlayStyleKeyswitchNote
                       + styleIndex, 1.0f);
    doubleEngine->noteOn (ElectryEngine::firstPlayStyleKeyswitchNote
                              + styleIndex, 1.0f);
    effectivePlayStyleIndex = styleIndex;
}

int ElectryPerformer::latestHeldPlayStyle() const noexcept
{
    int latest = -1;
    std::uint64_t latestOrder = 0;
    for (int style = 0;
         style < ElectryEngine::playStyleKeyswitchCount; ++style)
    {
        const auto index = static_cast<std::size_t> (style);
        if (heldPlayStyleCounts[index] != 0
            && heldPlayStyleOrder[index] > latestOrder)
        {
            latest = style;
            latestOrder = heldPlayStyleOrder[index];
        }
    }
    return latest;
}

void ElectryPerformer::clearHeldPlayStyles() noexcept
{
    heldPlayStyleCounts.fill (0);
    heldPlayStyleOrder.fill (0);
    heldPlayStyleSequence = 0;
}

void ElectryPerformer::applySoloStringMask (std::uint8_t mask) noexcept
{
    engine.setSoloStringMask (mask);
    if (doubleModeActive)
        doubleEngine->setSoloStringMask (mask);
    soloStringMask = mask;
}

std::uint8_t ElectryPerformer::computeHeldSoloStringMask() const noexcept
{
    std::uint8_t mask = 0;
    for (int s = 0; s < ElectryEngine::stringCount; ++s)
    {
        if (heldSoloStringCounts[static_cast<std::size_t> (s)] > 0)
            mask |= static_cast<std::uint8_t> (1u << s);
    }
    return mask;
}

void ElectryPerformer::clearHeldSoloStrings() noexcept
{
    heldSoloStringCounts.fill (0);
    latchedSoloStringMask = 0;
}

void ElectryPerformer::applyVibratoGesture (float amount) noexcept
{
    amount = bounded (0.0f, 1.0f, finite (amount) ? amount : 0.0f);
    engine.setVibrato (amount);
    doubleEngine->setVibrato (amount);
    vibratoGesture = roundToInt (127.0f * amount);
}

void ElectryPerformer::clearVibratoGesture() noexcept
{
    vibratoGestureOwners = 0;
    applyVibratoGesture (0.0f);
}

void ElectryPerformer::applyTremoloGesture (float velocity) noexcept
{
    velocity = bounded (0.0f, 1.0f,
                             finite (velocity) ? velocity : 0.0f);
    if (velocity > 0.0f)
    {
        engine.beginTremoloPicking (velocity);
        doubleEngine->beginTremoloPicking (velocity);
    }
    else
    {
        engine.endTremoloPicking();
        doubleEngine->endTremoloPicking();
    }
    tremoloGesture = roundToInt (127.0f * velocity);
}

void ElectryPerformer::clearTremoloGesture() noexcept
{
    tremoloGestureOwners = 0;
    applyTremoloGesture (0.0f);
}

void ElectryPerformer::resetEngineWithArticulations (int pickStyle,
                                                          int playStyle) noexcept
{
    engine.reset();
    doubleEngine->reset();
    engine.noteOn (ElectryEngine::firstKeyswitchNote
                       + pickStyle, 1.0f);
    doubleEngine->noteOn (ElectryEngine::firstKeyswitchNote
                              + pickStyle, 1.0f);
    engine.noteOn (ElectryEngine::firstPlayStyleKeyswitchNote
                       + playStyle, 1.0f);
    doubleEngine->noteOn (ElectryEngine::firstPlayStyleKeyswitchNote
                              + playStyle, 1.0f);
    effectivePlayStyleIndex = playStyle;
    const auto mask = soloStringMask;
    engine.setSoloStringMask (mask);
    doubleEngine->setSoloStringMask (mask);
}

} // namespace electry
