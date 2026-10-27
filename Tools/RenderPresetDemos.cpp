// One demo per factory rig (G3), rendered from Docs/audio/midi through the
// plug-in processor itself, so a demo plays exactly what loading the rig in a
// host plays: the rig's parameters, MIDI handling, Performer and FX chain.
//
//   RenderElectryPresetDemos [--more] [--jobs N] Docs/audio/midi[/more] <out-dir>
//   RenderElectryPresetDemos [--more] [--jobs N] --check Docs/audio/midi[/more]
//
// --more renders the README's More demos (G3d) instead: techniques the rig
// demos do not feature, each from Docs/audio/midi/more/<NN>-<slug>.mid through
// the rig named in the table below.
#include "PluginProcessor.h"
#include "PresetDemoRenderer.h"

#include <juce_events/juce_events.h>

#include <cstring>
#include <vector>

namespace
{
// Drop-E Metal and Blues Rock Lead are amplifier rigs; their phrases are
// written for the amp. A host keeps FX off until the player turns it on, and
// a rig does not change it, so the demo turns it on the way a player would.
bool demoUsesFx(std::size_t preset) noexcept { return preset == 1 || preset == 3; }

struct Showcase
{
    const char* name;
    std::size_t preset;
};

// The order is the file numbering; renaming one renames its files.
constexpr Showcase showcases[] {
    { "Full Range", 0 },
    { "Picking-Hand Triggers", 3 }, // the lead rig's delay and room carry the pattern
    { "Tremolo Picking", 1 },
    { "Whammy and Feedback", 1 },   // feedback needs the amplified return
};

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
    const bool more = argc > 1 && std::strcmp(argv[1], "--more") == 0;
    if (more)
    {
        argv[1] = argv[0];
        ++argv;
        --argc;
    }

    const juce::ScopedJuceInitialiser_GUI juce;
    protocodus::demos::Instrument instrument;
    instrument.name = "Electry";
    std::vector<std::size_t> presetFor;
    if (more)
        for (const auto& showcase : showcases)
        {
            instrument.presets.emplace_back(showcase.name);
            presetFor.push_back(showcase.preset);
        }
    else
    {
        ElectryAudioProcessor processor;
        for (int program = 0; program < processor.getNumPrograms(); ++program)
        {
            instrument.presets.push_back(processor.getProgramName(program).toStdString());
            presetFor.push_back(static_cast<std::size_t>(program));
        }
    }
    // U7 perf budget: 0 reports without failing until the CI runner's median
    // is measured.
    instrument.cpuBudget = 0.0;
    instrument.open = [presetFor](std::size_t demo, double rate, int maxBlockFrames, std::uint32_t) {
        return std::make_unique<ElectryVoice>(presetFor[demo], rate, maxBlockFrames);
    };
    return protocodus::demos::Main(argc, argv, instrument);
}
