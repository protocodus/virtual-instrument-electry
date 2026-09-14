// A reproducible, bounded state-chunk mutation campaign through the public host
// interface. Run under ASan/UBSan as well as Release: raw binary mutations and
// structurally valid hostile XML exercise different parser/restore paths.
#include "PluginProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    auto processor = std::make_unique<ElectryAudioProcessor>();
    processor->prepareToPlay (48000.0, 256);
    juce::MemoryBlock original;
    processor->getStateInformation (original);
    const auto* originalBytes = static_cast<const std::uint8_t*> (original.getData());
    const auto originalXml = juce::AudioProcessor::getXmlFromBinary (
        original.getData(), static_cast<int> (original.getSize()));
    if (originalXml == nullptr)
        return 1;
    std::vector<float> originalValues;
    for (auto* parameter : processor->getParameters())
        originalValues.push_back (parameter->getValue());

    constexpr std::uint32_t seed = 0x5a7ef022u;
    std::uint32_t random = seed;
    const auto draw = [&]
    {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        return random;
    };
    constexpr std::array values { "NaN", "Inf", "-Inf", "1e309", "-1e309",
        "1e-300", "-2147483649", "2147483648", "0", "1", "", "garbage",
        "0.5", "10000000000000000000000000000000000000000" };
    constexpr std::array sizes { 0, 1, 17, 64, 257 };
    constexpr int iterations = 2048;
    juce::AudioBuffer<float> audio;
    juce::MidiBuffer midi;
    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        std::vector<std::uint8_t> chunk (originalBytes,
                                        originalBytes + original.getSize());
        switch (iteration % 4)
        {
            case 0:
                for (std::uint32_t edits = 1 + draw() % 16; edits != 0; --edits)
                    chunk[draw() % chunk.size()] ^= static_cast<std::uint8_t> (1 + draw() % 255);
                break;
            case 1:
                chunk.resize (draw() % chunk.size());
                break;
            case 2:
                chunk.resize (draw() % 4096);
                for (auto& byte : chunk)
                    byte = static_cast<std::uint8_t> (draw());
                break;
            case 3:
            {
                auto xml = std::make_unique<juce::XmlElement> (*originalXml);
                for (auto* child : xml->getChildIterator())
                    child->setAttribute ("value", values[draw() % values.size()]);
                for (const auto* property : { "pickStyle", "playStyle", "factoryProgram",
                                               "playStyleKeysHold" })
                    xml->setAttribute (property, values[draw() % values.size()]);
                if ((draw() & 1u) != 0 && xml->getFirstChildElement() != nullptr)
                    xml->addChildElement (new juce::XmlElement (*xml->getFirstChildElement()));
                juce::MemoryBlock structured;
                juce::AudioProcessor::copyXmlToBinary (*xml, structured);
                const auto* bytes = static_cast<const std::uint8_t*> (structured.getData());
                chunk.assign (bytes, bytes + structured.getSize());
                break;
            }
        }
        processor->setStateInformation (chunk.data(), static_cast<int> (chunk.size()));
        for (auto* parameter : processor->getParameters())
        {
            const float value = parameter->getValue();
            if (! std::isfinite (value) || value < 0.0f || value > 1.0f)
            {
                std::cerr << "Invalid parameter after state mutation seed=" << seed
                          << " iteration=" << iteration << '\n';
                return 1;
            }
        }
        audio.setSize (2, sizes[draw() % sizes.size()]);
        audio.clear();
        midi.clear();
        midi.addEvent (juce::MidiMessage::noteOn (1, 40 + static_cast<int> (draw() % 48),
                                                static_cast<juce::uint8> (100)), 0);
        processor->processBlock (audio, midi);
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int frame = 0; frame < audio.getNumSamples(); ++frame)
                if (! std::isfinite (audio.getSample (channel, frame)))
                {
                    std::cerr << "Non-finite audio seed=" << seed
                              << " iteration=" << iteration << '\n';
                    return 1;
                }
        if (iteration % 64 == 63)
        {
            // A valid session must recover after every class of bad chunk,
            // including when the host deactivates and reactivates the instance.
            processor->releaseResources();
            processor->setStateInformation (original.getData(),
                                            static_cast<int> (original.getSize()));
            juce::MemoryBlock restored;
            processor->getStateInformation (restored);
            const auto restoredXml = juce::AudioProcessor::getXmlFromBinary (
                restored.getData(), static_cast<int> (restored.getSize()));
            bool recovered = restoredXml != nullptr;
            if (restoredXml != nullptr)
            {
                for (const auto* property : { "pickStyle", "playStyle", "factoryProgram",
                                               "playStyleKeysHold" })
                    recovered = recovered && restoredXml->getStringAttribute (property)
                        == originalXml->getStringAttribute (property);
            }
            // APVTS canonicalizes child order and normalized float rounding
            // during restore; compare the published values, not XML bytes.
            for (int index = 0; index < processor->getParameters().size(); ++index)
                recovered = recovered && std::abs (
                    processor->getParameters()[index]->getValue()
                    - originalValues[static_cast<std::size_t> (index)]) < 1.0e-6f;
            if (! recovered)
            {
                std::cerr << "State did not recover at iteration=" << iteration << '\n';
                return 1;
            }
            processor->prepareToPlay (iteration % 128 == 63 ? 44100.0 : 96000.0, 64);
        }
    }
    processor->releaseResources();
    std::cout << "Passed " << iterations << " state mutations and 32 verified restores; seed="
              << seed << '\n';
}
