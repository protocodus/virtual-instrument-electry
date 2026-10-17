// U4 parameter-extremes matrix over the plug-in's automatable parameters and
// factory rigs. CTest runs each case at one rate and block size in rotation;
// `--full` runs the standard's full matrix (every case at all nine), which the
// main build's long fuzz job does.
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>

#include "ParameterMatrix.h"

#include <cmath>
#include <cstring>
#include <thread>

namespace
{
using protocodus::matrix::Case;
using protocodus::matrix::Limits;

class ElectryMatrixDevice final : public protocodus::matrix::JuceDevice
{
public:
    ElectryMatrixDevice() : JuceDevice(std::make_unique<ElectryAudioProcessor>())
    {
        const auto parameters = Parameters();
        const auto find = [&](const char* name) {
            for (std::size_t i = 0; i < parameters.size(); ++i)
                if (parameters[i].name == name)
                    return i;
            throw std::runtime_error(std::string("no parameter named ") + name);
        };
        sympathetic = find("Sympathetic ring");
        fxEnabled = find("FX enabled");
        delay = find("Delay");
        room = find("Room");
        output = find("Output level");
    }

    Limits LimitsFor(const Case& c) const override
    {
        // Automation ramps between the case's values and the base patch, so
        // each limit takes whichever end is the more demanding.
        const auto most = [&](std::size_t i) { return std::max(c.values[i], c.base[i]); };
        Limits limits;
        // Sympathetic strings keep ringing after the played notes stop, and
        // the delay and room tails outlast the release window.
        if (most(sympathetic) > 0.0f
            || (most(fxEnabled) >= 0.5f && (most(delay) > 0.0f || most(room) > 0.0f)))
            limits.releaseSeconds = -1.0;
        // Output level is a -24..+6 dB trim; the default is -6 dB, where the
        // declared ceiling and floors apply unscaled.
        const float decibels = -24.0f + 30.0f * most(output);
        const float gain = std::pow(10.0f, (decibels + 6.0f) / 20.0f);
        limits.ceiling *= gain;
        limits.silenceFloor *= gain;
        limits.dcLimit *= gain;
        return limits;
    }

private:
    std::size_t sympathetic = 0, fxEnabled = 0, delay = 0, room = 0, output = 0;
};
} // namespace

int main(int argc, char** argv)
{
    const juce::ScopedJuceInitialiser_GUI juce;
    protocodus::matrix::Options options;
    options.everyConfiguration = argc > 1 && std::strcmp(argv[1], "--full") == 0;
    options.jobs = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    return protocodus::matrix::Run([] { return std::make_unique<ElectryMatrixDevice>(); }, options) == 0
        ? 0 : 1;
}
