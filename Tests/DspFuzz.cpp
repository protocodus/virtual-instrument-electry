// Stateful coverage-guided fuzzing of the public JUCE-free audio boundary.
// Every input starts from fresh objects; a saved libFuzzer crash is therefore
// replayable without earlier corpus entries. Work and allocations are bounded
// independently of input length. Normalized controls and ordinary notes are
// mixed with raw IEEE floats, invalid enums, expression IDs and lifecycle calls.
// Run with -max_len=4096 -timeout=30 -rss_limit_mb=2048 and a corpus directory.
#include "DSP/ElectryEngine.h"
#include "DSP/ElectryFx.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <span>

namespace
{
struct Input
{
    std::span<const std::uint8_t> bytes;
    std::uint8_t byte()
    {
        if (bytes.empty())
            return 0;
        const auto result = bytes.front();
        bytes = bytes.subspan(1);
        return result;
    }
    std::uint32_t word()
    {
        std::uint32_t result = 0;
        for (int shift = 0; shift != 32; shift += 8)
            result |= static_cast<std::uint32_t>(byte()) << shift;
        return result;
    }
    float scalar()
    {
        switch (byte() % 8)
        {
            case 0: return std::bit_cast<float>(word());
            case 1: return std::numeric_limits<float>::max();
            case 2: return -std::numeric_limits<float>::max();
            case 3: return std::numeric_limits<float>::quiet_NaN();
            case 4: return std::numeric_limits<float>::infinity();
            case 5: return -std::numeric_limits<float>::infinity();
            default: return static_cast<float>(byte()) / 255.0f;
        }
    }
    int note()
    {
        // Exercise the full int boundary as well as enough valid notes to
        // reach the active string path regularly.
        return byte() % 4 == 0 ? std::bit_cast<std::int32_t>(word())
                              : static_cast<int>(byte()) - 32;
    }
};

constexpr std::array engineControls {
    &electry::EngineParameters::bodyWood, &electry::EngineParameters::bodySize,
    &electry::EngineParameters::bodyShape, &electry::EngineParameters::construction,
    &electry::EngineParameters::scaleLength, &electry::EngineParameters::pickupType,
    &electry::EngineParameters::toneKnob, &electry::EngineParameters::bodyResonance,
    &electry::EngineParameters::stringGauge, &electry::EngineParameters::stringAge,
    &electry::EngineParameters::pickPosition, &electry::EngineParameters::pickHardness,
    &electry::EngineParameters::pickNoise, &electry::EngineParameters::fingerNoise,
    &electry::EngineParameters::releaseNoise, &electry::EngineParameters::muteDamping,
    &electry::EngineParameters::bendTimeSeconds, &electry::EngineParameters::velocityAmount,
    &electry::EngineParameters::outputGain, &electry::EngineParameters::artifactAmount,
    &electry::EngineParameters::sympatheticAmount, &electry::EngineParameters::palmMute,
    &electry::EngineParameters::strumSpreadSeconds, &electry::EngineParameters::tremoloRateHz,
    &electry::EngineParameters::resonanceDepth, &electry::EngineParameters::vibratoDepth
};
constexpr std::array fxControls {
    &electry::FxParameters::distortion, &electry::FxParameters::amp,
    &electry::FxParameters::compressor, &electry::FxParameters::delay,
    &electry::FxParameters::room
};
constexpr std::array rates {
    8000.0, 44100.0, 48000.0, 96000.0, 96001.0, 192000.0, 384000.0,
    0.0, -1.0, 1.0e300, std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::infinity()
};

void require(bool value)
{
    if (!value)
        std::abort();
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size == 0)
        return 0;
    Input input { { data, size } };
    auto engine = std::make_unique<electry::ElectryEngine>();
    auto fx = std::make_unique<electry::ElectryFx>();
    electry::EngineParameters parameters;
    electry::FxParameters effects;
    const auto initialMode = input.byte();
    const auto rate = rates[initialMode % rates.size()];
    bool fxPrepared = false;
    if ((initialMode & 0x80u) == 0)
    {
        engine->prepare(rate, 1 + input.byte());
        fx->prepare(rate);
        fxPrepared = true;
    }

    // Sentinels also catch writes outside the caller's frame count when they
    // remain inside the allocation and would otherwise evade AddressSanitizer.
    std::array<float, 258> left {}, right {};
    constexpr float sentinel = 12345.0f;
    const auto render = [&](int count, bool externalInput)
    {
        left.fill(sentinel);
        right.fill(sentinel);
        if (externalInput)
        {
            for (int i = 1; i <= count; ++i)
            {
                left[static_cast<std::size_t>(i)] = input.scalar();
                right[static_cast<std::size_t>(i)] = input.scalar();
            }
        }
        else
        {
            engine->process(left.data() + 1, right.data() + 1, count);
            for (int i = 1; i <= count; ++i)
                require(std::isfinite(left[static_cast<std::size_t>(i)])
                        && std::isfinite(right[static_cast<std::size_t>(i)]));
        }
        const auto beforeLeft = left;
        const auto beforeRight = right;
        fx->process(left.data() + 1, right.data() + 1, count);
        if (! fxPrepared)
            for (int i = 1; i <= count; ++i)
            {
                const auto index = static_cast<std::size_t>(i);
                require(std::bit_cast<std::uint32_t>(left[index])
                            == std::bit_cast<std::uint32_t>(beforeLeft[index])
                        && std::bit_cast<std::uint32_t>(right[index])
                            == std::bit_cast<std::uint32_t>(beforeRight[index]));
            }
        engine->pushAcousticReturn(left.data() + 1, right.data() + 1, count);
        require(left.front() == sentinel && right.front() == sentinel);
        for (std::size_t i = static_cast<std::size_t>(count + 1); i < left.size(); ++i)
            require(left[i] == sentinel && right[i] == sentinel);
        if (fxPrepared || ! externalInput)
            for (int i = 1; i <= count; ++i)
                require(std::isfinite(left[static_cast<std::size_t>(i)])
                        && std::isfinite(right[static_cast<std::size_t>(i)]));
        require(engine->getActiveVoiceCount() >= 0
                && engine->getActiveVoiceCount() <= electry::ElectryEngine::stringCount);
        std::array<electry::StringVisualState, electry::ElectryEngine::stringCount>
            visuals {};
        engine->getStringVisualState(visuals);
        for (const auto& visual : visuals)
            require(std::isfinite(visual.level)
                    && visual.level >= 0.0f && visual.level <= 1.0f);
    };

    for (int command = 0; command < 64 && !input.bytes.empty(); ++command)
    {
        switch (input.byte() % 23)
        {
            case 0:
            {
                const auto field = input.byte() % engineControls.size();
                parameters.*engineControls[field] = input.scalar();
                engine->setParameters(parameters);
                break;
            }
            case 1:
            {
                const auto field = input.byte() % fxControls.size();
                effects.*fxControls[field] = input.scalar();
                fx->setParameters(effects);
                break;
            }
            case 2:
            {
                const int note = input.note();
                const float velocity = input.scalar();
                const auto id = input.byte();
                engine->noteOn(note, velocity, id, input.byte());
                break;
            }
            case 3:
            {
                const int note = input.note();
                const auto id = input.byte();
                engine->noteOff(note, id, input.byte());
                break;
            }
            case 4: engine->setPitchBend(input.scalar()); break;
            case 5:
            {
                const auto id = input.byte();
                const float bend = input.scalar();
                engine->setExpressionPitchBend(id, bend);
                engine->setExpressionMasterPitchBend(id, input.scalar());
                engine->snapExpressionPitchBendToTarget(id);
                break;
            }
            case 6: engine->setResonance(input.scalar()); break;
            case 7: engine->setPalmMutePressure(input.scalar()); break;
            case 8: engine->setVibrato(input.scalar()); break;
            case 9: engine->setSustainPedal(input.byte() % 2 != 0); break;
            case 10:
                engine->beginTremoloPicking(input.scalar());
                if (input.byte() % 2 != 0)
                    engine->endTremoloPicking();
                break;
            case 11:
                parameters.pickupSelector = static_cast<electry::PickupSelector>(input.byte());
                parameters.outputMode = static_cast<electry::OutputMode>(input.byte());
                effects.ampModel = static_cast<electry::AmpModel>(input.byte());
                effects.oversampling = static_cast<electry::FxOversampling>(input.byte());
                engine->setParameters(parameters);
                fx->setParameters(effects);
                break;
            case 12:
            {
                // Include groups exceeding the engine's fixed 128-event
                // capacity, and exact boundary sizes, without allocating.
                std::array<electry::ElectryEngine::NoteOnEvent, 256> notes {};
                const std::size_t count = input.byte();
                for (std::size_t i = 0; i < count; ++i)
                {
                    notes[i].midiNote = input.note();
                    notes[i].velocity = input.scalar();
                    notes[i].expressionId = input.byte();
                    notes[i].ownershipSource = input.byte();
                }
                engine->noteOnChord({ notes.data(), count });
                break;
            }
            case 13:
                engine->setAcousticReturnLevel(input.scalar());
                left.fill(input.scalar());
                right.fill(input.scalar());
                engine->pushAcousticReturn(left.data(), right.data(), 256);
                break;
            case 14:
                engine->allNotesOff();
                if (input.byte() % 2 != 0)
                {
                    engine->reset();
                    fx->reset();
                }
                break;
            case 15:
                engine->setSoloStringMask(input.byte());
                electry::applyGuitarBuild(parameters, input.scalar());
                engine->setParameters(parameters);
                break;
            case 16:
                engine->process(nullptr, right.data(), 256);
                fx->process(left.data(), nullptr, 256);
                engine->pushAcousticReturn(nullptr, nullptr, 256);
                break;
            case 18:
            {
                const auto nextRate = rates[input.byte() % rates.size()];
                engine->prepare(nextRate, std::bit_cast<std::int32_t>(input.word()));
                fx->prepare(nextRate);
                fxPrepared = true;
                break;
            }
            case 19:
            {
                // Non-positive frame counts must neither advance state nor
                // touch caller storage, including the extreme signed count.
                const int count = input.byte() % 2 == 0
                    ? 0 : std::numeric_limits<int>::min();
                left.fill(sentinel);
                right.fill(sentinel);
                engine->process(left.data(), right.data(), count);
                fx->process(left.data(), right.data(), count);
                engine->pushAcousticReturn(left.data(), right.data(), count);
                for (std::size_t i = 0; i < left.size(); ++i)
                    require(left[i] == sentinel && right[i] == sentinel);
                break;
            }
            case 20:
            {
                left.fill(sentinel);
                const int count = 1 + input.byte();
                engine->process(left.data() + 1, left.data() + 1, count);
                fx->process(left.data() + 1, left.data() + 1, count);
                engine->pushAcousticReturn(left.data() + 1, nullptr, count);
                require(left.front() == sentinel);
                for (int i = 1; i <= count; ++i)
                    require(std::isfinite(left[static_cast<std::size_t>(i)]));
                for (std::size_t i = static_cast<std::size_t>(count + 1);
                     i < left.size(); ++i)
                    require(left[i] == sentinel);
                break;
            }
            case 21:
                engine->setVariationSeed(input.word());
                engine->reset();
                break;
            case 22:
                // Reach multiple control periods and delayed contacts even
                // when a corpus entry contains only a short event prefix.
                for (int block = 0; block < 32; ++block)
                    render(32, false);
                break;
            default: render(1 + input.byte(), true); break;
        }
        // Time advances after every event, including control-only prefixes.
        // Keep causal feedback chunks below the smallest supported delay.
        render(1 + input.byte() % 32, false);
    }
    engine->reset();
    fx->reset();
    render(128, false);
    require(engine->getActiveVoiceCount() == 0);
    return 0;
}
