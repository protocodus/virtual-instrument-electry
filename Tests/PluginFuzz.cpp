// Coverage-guided fuzzing of the real host boundary. Each input creates a
// fresh processor, so a saved reproducer never depends on earlier inputs.
// Run under ASan/UBSan, with a display server on Linux (xvfb-run -a), and
// -max_len=4096 -timeout=30 -rss_limit_mb=2048. UI calls run on the message
// thread; lifecycle/state changes are deliberately serialized with audio.
#include "PluginProcessor.h"

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
        bytes = bytes.subspan (1);
        return result;
    }

    std::uint32_t word()
    {
        std::uint32_t result = 0;
        for (int shift = 0; shift != 32; shift += 8)
            result |= static_cast<std::uint32_t> (byte()) << shift;
        return result;
    }

    float scalar()
    {
        switch (byte() % 8)
        {
            case 0: return std::bit_cast<float> (word());
            case 1: return std::numeric_limits<float>::max();
            case 2: return -std::numeric_limits<float>::max();
            case 3: return std::numeric_limits<float>::quiet_NaN();
            case 4: return std::numeric_limits<float>::infinity();
            case 5: return -std::numeric_limits<float>::infinity();
            default: return static_cast<float> (byte()) / 255.0f;
        }
    }
};

void require (bool condition)
{
    if (! condition)
        std::abort();
}

constexpr std::array rates {
    44100.0, 48000.0, 96000.0, 192000.0, 384000.0, 8000.0,
    0.0, -1.0, 1.0e300, std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::infinity()
};
constexpr std::array sizes { 0, 1, 2, 31, 64, 127, 256, 257, 1024 };
constexpr std::array offsets {
    std::numeric_limits<int>::min(), -1, 0, 1, 31, 64, 256, 1024,
    std::numeric_limits<int>::max()
};
} // namespace

extern "C" int LLVMFuzzerInitialize (int*, char***);
extern "C" int LLVMFuzzerTestOneInput (const std::uint8_t*, std::size_t);

extern "C" int LLVMFuzzerInitialize (int*, char***)
{
    struct Runtime
    {
        juce::ScopedJuceInitialiser_GUI initialiser;

        Runtime()
        {
            // APVTS creates JUCE's timer/shutdown singletons lazily. Construct
            // them before this static Runtime finishes initialization, so its
            // GUI shutdown runs BEFORE their C++ static destructors at exit.
            // A bare static ScopedJuceInitialiser_GUI reverses that order.
            auto warmup = std::make_unique<ElectryAudioProcessor>();
        }
    };
    static Runtime runtime;
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput (const std::uint8_t* data, std::size_t size)
{
    if (size == 0)
        return 0;

    Input input { { data, size } };
    auto processor = std::make_unique<ElectryAudioProcessor>();
    juce::MemoryBlock initialState;
    processor->getStateInformation (initialState);
    require (initialState.getSize() > 8);
    juce::MidiBuffer midi;
    midi.ensureSize (4096);
    constexpr float sentinel = 12345.0f;
    std::array<float, 1026> left {}, right {};
    float* channels[] { left.data() + 1, right.data() + 1 };

    const auto render = [&] (int count, int channelCount)
    {
        left.fill (sentinel);
        right.fill (sentinel);
        juce::AudioBuffer<float> audio (channels, channelCount, count);
        processor->processBlock (audio, midi);
        for (int channel = 0; channel < 2; ++channel)
        {
            const auto& samples = channel == 0 ? left : right;
            require (std::bit_cast<std::uint32_t> (samples.front())
                     == std::bit_cast<std::uint32_t> (sentinel));
            for (std::size_t i = 1; i < samples.size(); ++i)
            {
                if (channel < channelCount && i <= static_cast<std::size_t> (count))
                    require (std::isfinite (samples[i]));
                else
                    require (std::bit_cast<std::uint32_t> (samples[i])
                             == std::bit_cast<std::uint32_t> (sentinel));
            }
        }
        require (processor->getActiveVoiceCount() >= 0
                 && processor->getActiveVoiceCount() <= 2 * electry::ElectryEngine::stringCount);
        midi.clear();
    };

    // Exercise pre-prepare calls as well as normal and repeated preparation.
    if ((input.byte() & 1u) != 0u)
        processor->prepareToPlay (rates[input.byte() % rates.size()], 64);

    for (int command = 0; command < 48 && ! input.bytes.empty(); ++command)
    {
        switch (input.byte() % 15)
        {
            case 0:
                render (sizes[input.byte() % sizes.size()], 2);
                break;
            case 1:
            {
                const int offset = offsets[input.byte() % offsets.size()];
                const auto count = 1 + input.byte() % 64;
                for (int event = 0; event < count; ++event)
                {
                    // Back every short message with three bytes, including
                    // deliberately truncated payloads and invalid data bytes.
                    const std::array<juce::uint8, 3> bytes {
                        static_cast<juce::uint8> (0x80u + input.byte() % 0x70u),
                        input.byte(), input.byte()
                    };
                    midi.addEvent (bytes.data(), 1 + input.byte() % 3, offset);
                }
                break;
            }
            case 2:
            {
                const auto& parameters = processor->getParameters();
                auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (
                    parameters[static_cast<int> (input.byte()) % parameters.size()]);
                require (parameter != nullptr);
                // Include corrupt automation without invoking undefined
                // conversions in a host's normalization code first.
                processor->parameters.getRawParameterValue (parameter->paramID)
                    ->store (input.scalar(), std::memory_order_relaxed);
                break;
            }
            case 3:
            {
                const auto rate = rates[input.byte() % rates.size()];
                const int hint = static_cast<int> (input.byte()) - 64;
                processor->prepareToPlay (rate, hint);
                break;
            }
            case 4:
                processor->releaseResources();
                break;
            case 5:
            {
                // Mutate a structurally valid JUCE state, including XML length
                // and magic, to reach past the parser's normal early rejects.
                auto state = initialState;
                auto* bytes = static_cast<std::uint8_t*> (state.getData());
                const auto count = 1 + input.byte() % 16;
                for (int edit = 0; edit < count; ++edit)
                {
                    const auto position = input.word() % state.getSize();
                    bytes[position] = input.byte();
                }
                processor->setStateInformation (state.getData(), static_cast<int> (state.getSize()));
                break;
            }
            case 6:
            {
                const auto count = std::min<std::size_t> (input.byte(), input.bytes.size());
                processor->setStateInformation (input.bytes.data(), static_cast<int> (count));
                input.bytes = input.bytes.subspan (count);
                break;
            }
            case 7:
            {
                // Entity, nesting and numeric state values can evolve freely
                // inside a valid binary envelope. Copy the verified magic.
                const auto count = std::min<std::size_t> (input.byte(), input.bytes.size());
                juce::MemoryBlock state;
                juce::MemoryOutputStream stream (state, false);
                stream.write (initialState.getData(), 4);
                stream.writeInt (static_cast<int> (count));
                stream.write (input.bytes.data(), count);
                stream.writeByte (0);
                stream.flush();
                processor->setStateInformation (state.getData(), static_cast<int> (state.getSize()));
                input.bytes = input.bytes.subspan (count);
                break;
            }
            case 8:
                processor->setCurrentProgram (static_cast<int> (input.byte()) - 8);
                break;
            case 9:
                processor->triggerArticulation (static_cast<int> (input.byte()) - 32);
                processor->setPlayStyleKeysHold ((input.byte() & 1u) != 0u);
                break;
            case 10:
            {
                const int string = static_cast<int> (input.byte()) - 8;
                processor->triggerStringRepick (string, input.scalar());
                break;
            }
            case 11:
                processor->requestPanic();
                break;
            case 12:
            {
                const int channel = 1 + input.byte() % 16;
                const int note = input.byte() % 128;
                if ((input.byte() & 1u) != 0u)
                    processor->keyboardState.noteOn (channel, note,
                        static_cast<float> (input.byte()) / 255.0f);
                else
                    processor->keyboardState.noteOff (channel, note, 0.0f);
                break;
            }
            case 13:
            {
                const int count = sizes[input.byte() % sizes.size()];
                const int channelCount = input.byte() % 2;
                render (count, channelCount);
                break;
            }
            case 14:
            {
                // Hosts may save after corrupt automation as well as load
                // externally damaged chunks. Exercise both sides of state I/O.
                juce::MemoryBlock state;
                processor->getStateInformation (state);
                require (state.getSize() > 8 && state.getSize() <= 1024 * 1024);
                processor->setStateInformation (state.getData(), static_cast<int> (state.getSize()));
                break;
            }
        }
    }
    render (257, 2);

    // Prove that hostile input has not permanently poisoned the processor.
    midi.clear();
    processor->setStateInformation (initialState.getData(), static_cast<int> (initialState.getSize()));
    processor->prepareToPlay (48000.0, 64);
    midi.addEvent (juce::MidiMessage::noteOn (1, 52, (juce::uint8) 100), 0);
    render (1024, 2);
    bool audible = false;
    for (int i = 1; i <= 1024; ++i)
        audible = audible || std::abs (left[static_cast<std::size_t> (i)]) > 1.0e-7f;
    require (audible);
    processor->releaseResources();
    return 0;
}
