// One demo per factory rig (G3), rendered from Docs/audio/midi through the
// plug-in processor itself, so a demo plays exactly what loading the rig in a
// host plays: the rig's parameters, MIDI handling, Performer and FX chain.
//
//   RenderElectryPresetDemos [--jobs N] Docs/audio/midi <out-dir>
//   RenderElectryPresetDemos [--jobs N] --check Docs/audio/midi
#include "PluginProcessor.h"
#include "PresetDemoRenderer.h"

#include <juce_events/juce_events.h>

namespace
{
// Drop-E Metal and Blues Rock Lead are amplifier rigs; their phrases are
// written for the amp. A host keeps FX off until the player turns it on, and
// a rig does not change it, so the demo turns it on the way a player would.
bool demoUsesFx(std::size_t preset) noexcept { return preset == 1 || preset == 3; }

class ElectryVoice final : public protocodus::demos::Voice
{
public:
    ElectryVoice(std::size_t preset, double rate, int maxBlockFrames)
    {
        processor.setCurrentProgram(static_cast<int>(preset));
        for (auto* parameter : processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
                ranged != nullptr && ranged->paramID == electry::parameters::fxEnabled)
                ranged->setValueNotifyingHost(demoUsesFx(preset) ? 1.0f : 0.0f);
        processor.setRateAndBufferSizeDetails(rate, maxBlockFrames);
        processor.prepareToPlay(rate, maxBlockFrames);
        buffer.setSize(std::max({ 2, processor.getTotalNumInputChannels(),
                                  processor.getTotalNumOutputChannels() }),
                       maxBlockFrames);
    }

    ~ElectryVoice() override { processor.releaseResources(); }

    void Midi(const std::uint8_t* bytes, int size) override { midi.addEvent(bytes, size, 0); }

    void Render(float* left, float* right, int frames) override
    {
        buffer.setSize(buffer.getNumChannels(), frames, false, false, true);
        buffer.clear();
        processor.processBlock(buffer, midi);
        midi.clear();
        std::copy_n(buffer.getReadPointer(0), frames, left);
        std::copy_n(buffer.getReadPointer(1), frames, right);
    }

private:
    ElectryAudioProcessor processor;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midi;
};
} // namespace

int main(int argc, char** argv)
{
    const juce::ScopedJuceInitialiser_GUI juce;
    protocodus::demos::Instrument instrument;
    instrument.name = "Electry";
    {
        ElectryAudioProcessor processor;
        for (int program = 0; program < processor.getNumPrograms(); ++program)
            instrument.presets.push_back(processor.getProgramName(program).toStdString());
    }
    instrument.open = [](std::size_t preset, double rate, int maxBlockFrames, std::uint32_t) {
        return std::make_unique<ElectryVoice>(preset, rate, maxBlockFrames);
    };
    return protocodus::demos::Main(argc, argv, instrument);
}
