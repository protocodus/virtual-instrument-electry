#pragma once

#include "ElectryEngine.h"
#include "ElectryFx.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace electry
{
// Complete instrument configuration shared by every host. outputMode is
// 0 = mono, 1 = divided-pickup stereo, 2 = independent double performance.
struct PerformerParameters
{
    EngineParameters engine {};
    FxParameters effects {};
    int outputMode = 0;
    bool fxEnabled = false;
};

// Host-neutral playing logic and complete guitar/rig signal path. All note
// numbers at this boundary are WRITTEN MIDI (including octave-transposed
// playable and repick notes). Construction/prepare are the allocation boundary;
// every event, parameter change and render uses bounded storage afterward.
class ElectryPerformer
{
public:
    ElectryPerformer();
    void prepare(double sampleRate, int maximumBlockSize);
    void setParameters(const PerformerParameters&) noexcept;
    void setEngineParameters(const EngineParameters&, int outputMode) noexcept;
    void setEffectParameters(const FxParameters&, bool enabled) noexcept;
    void render(float* left, float* right, int samples) noexcept;
    void reset() noexcept;
    void reset(int pickStyle, int playStyle) noexcept;
    void allSoundOff() noexcept;
    void allNotesOff() noexcept;
    void releasePerformanceKeys() noexcept;
    void releasePlayableNotes() noexcept;
    void resetControllers() noexcept;
    void resetEffects() noexcept;
    void noteOn(int writtenMidiNote, float velocity,
                bool selectsBaseArticulation = false) noexcept;
    void noteOff(int writtenMidiNote) noexcept;
    void noteOff(int writtenMidiNote, ElectryEngine::ExpressionId) noexcept;
    void noteOnChord(const ElectryEngine::NoteOnEvent* writtenNotes,
                     std::size_t count) noexcept;
    static bool isAttackConditioningNote(int writtenMidiNote) noexcept;
    void selectArticulation(int articulationIndex) noexcept;
    void setPlayStyleKeysHold(bool) noexcept;
    void setSustainPedal(bool) noexcept;
    void setResonance(float) noexcept;
    void setPalmMutePressure(float) noexcept;
    void setPitchBend(float normalizedBipolar) noexcept;
    void setMasterTuneCents(float) noexcept;
    ElectryEngine& getEngine() noexcept { return engine; }
    const ElectryEngine& getEngine() const noexcept { return engine; }
    ElectryEngine& getDoubleEngine() noexcept { return *doubleEngine; }
    int getPickStyle() const noexcept { return pickStyleIndex; }
    int getPlayStyle() const noexcept { return appliedBasePlayStyleIndex; }
    int getEffectivePlayStyle() const noexcept { return effectivePlayStyleIndex; }
    bool getPlayStyleKeysHold() const noexcept { return appliedPlayStyleKeysHold; }
    std::uint8_t getSoloStringMask() const noexcept { return soloStringMask; }
    int getVibratoGesture() const noexcept { return vibratoGesture; }
    int getTremoloGesture() const noexcept { return tremoloGesture; }
    bool isDoubleModeActive() const noexcept { return doubleModeActive; }

private:
    void applyPlayStyle(int) noexcept;
    int latestHeldPlayStyle() const noexcept;
    void clearHeldPlayStyles() noexcept;
    void applySoloStringMask(std::uint8_t) noexcept;
    std::uint8_t computeHeldSoloStringMask() const noexcept;
    void clearHeldSoloStrings() noexcept;
    void applyVibratoGesture(float) noexcept;
    void clearVibratoGesture() noexcept;
    void applyTremoloGesture(float) noexcept;
    void clearTremoloGesture() noexcept;
    void resetEngineWithArticulations(int pickStyle, int playStyle) noexcept;
    void processEffects(float*, float*, int) noexcept;
    void updatePitchBend() noexcept;

    ElectryEngine engine;
    std::unique_ptr<ElectryEngine> doubleEngine;
    ElectryFx effects;
    bool doubleModeActive = false;
    bool effectsEnabled = false;
    float fxWet = 0.0f;
    float fxWetTarget = 0.0f;
    float fxWetStep = 0.0f;
    int fxRampLength = 220;
    int fxRampRemaining = 0;
    std::array<std::array<float, 256>, 2> fxDryScratch {};
    int pickStyleIndex = 0;
    int appliedBasePlayStyleIndex = 0;
    int effectivePlayStyleIndex = 0;
    bool appliedPlayStyleKeysHold = false;
    std::array<std::uint16_t, ElectryEngine::playStyleKeyswitchCount> heldPlayStyleCounts {};
    std::array<std::uint64_t, ElectryEngine::playStyleKeyswitchCount> heldPlayStyleOrder {};
    std::uint64_t heldPlayStyleSequence = 0;
    std::uint8_t soloStringMask = 0;
    std::uint8_t latchedSoloStringMask = 0;
    std::array<std::uint16_t, ElectryEngine::stringCount> heldSoloStringCounts {};
    std::uint16_t vibratoGestureOwners = 0;
    std::uint16_t tremoloGestureOwners = 0;
    int vibratoGesture = 0;
    int tremoloGesture = 0;
    bool sustainPedalDown = false;
    float pitchBend = 0.0f;
    float masterTuneCents = 0.0f;
};
} // namespace electry
