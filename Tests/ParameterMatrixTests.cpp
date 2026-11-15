// U4 parameter-extremes matrix over the plug-in's automatable parameters and
// factory rigs, with the shared command line (U9). CTest passes --rotation,
// each case at one rate and block size in turn; the main build's long fuzz
// job runs the default, the standard's full matrix (every case at all nine).
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>

#include "ParameterMatrix.h"

#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <string>
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

    // U11. FX is off by default and no rig turns it on, so the effects are
    // tested with it on.
    protocodus::matrix::ReachRule ReachFor(std::size_t index, float value) const override
    {
        using protocodus::matrix::Reach;
        // The matrix indexes the editor-only parameters (FX oversampling)
        // after the automatable ones.
        std::vector<protocodus::matrix::Parameter> parameters = Parameters();
        for (const auto& p : EditorOnlyParameters())
            parameters.push_back(p);
        const auto at = [&](const char* name) {
            for (std::size_t i = 0; i < parameters.size(); ++i)
                if (parameters[i].name == name)
                    return i;
            throw std::runtime_error(std::string("no parameter named ") + name);
        };
        const std::string& name = parameters[index].name;
        const auto is = [&](std::initializer_list<const char*> names) {
            for (const char* n : names)
                if (name == n)
                    return true;
            return false;
        };
        // Read where a pluck starts: each string keeps the pluck it was given.
        if (is({ "Pick position", "Pick hardness", "Velocity response", "Strum spread", "Player Feel" }))
            return { Reach::NewNotes, "the pick's contact at each new pluck" };
        if (name == "Bend time")
            return { Reach::NewNotes, "the travel of the next bend; the held chord is not bent" };
        // Double (Output mode at 1) puts a second, independent take on the
        // right channel. ElectryPerformer::setEngineParameters resets that
        // take's engine when Double comes on, so its strings start silent and
        // its per-stroke contact draws (drawStrokeVariation, keyed to its own
        // note count since the reset) restart at its seed, while T's take has
        // played the whole long phrase and draws that many notes on. On probe 1
        // the two takes are picked with different contacts: the run's right
        // channel is as far from T's (1.21 dB) as from R's (1.53 dB; R to T
        // 1.30 dB), and the left channel, the first take, is bit-identical in
        // all three. Until 2.4.9 the second take also lagged each picked
        // stroke by 3 +/- 1 ms, which the run shared with T: that shared lag
        // is what brought probe 1 towards T (the 2.4.8 engine passes, by the
        // averaged spectrum, 0.47 from 0.68 dB). With the lag removed nothing
        // history-free is left to compare. Probe 1 must still move from R, the
        // held chord must follow, and probes 2 and 3 are checked as usual.
        if (name == "Output mode" && value > 0.75f)
            return { Reach::Held, "", {},
                     "Double's second take starts afresh when Double comes on (silent strings, contact draws "
                     "from its seed), while T's has played the long phrase" };
        if (name == "FX enabled")
            return { Reach::Held, "", { { at("Distortion"), 0.5f } } };
        if (is({ "Distortion", "Amp simulation", "Compressor", "Delay", "Room" }))
            return { Reach::Held, "", { { fxEnabled, 1.0f } } };
        if (name == "Amp voice")
            return { Reach::Held, "", { { fxEnabled, 1.0f }, { at("Amp simulation"), 1.0f } } };
        if (name == "FX oversampling")
            return { Reach::Held, "", { { fxEnabled, 1.0f }, { at("Distortion"), 1.0f } } };
        // Only the Palm Mute style's heel reads it (handT60 in
        // ElectryEngine.cpp); Mute pressure damps through its own law.
        if (name == "Mute tightness")
            return { Reach::Exempt, "the Palm Mute style's tightness, which a keyswitch starts; the phrase plays none" };
        if (name == "Resonance depth")
            return { Reach::Exempt, "how far the mod wheel pushes the resonance; the phrase does not move it" };
        if (name == "Tremolo picking rate")
            return { Reach::Exempt, "the repick rate of tremolo picking, which a keyswitch starts; the phrase plays none" };
        // Each sits 30-60 dB under the strings on any chord.
        if (name == "Body resonance")
            return { Reach::Exempt, "the body's EMF, far under the strings' on the probe chord" };
        if (name == "Artifacts")
            return { Reach::Exempt, "hardware ring and saddle buzz, far under the strings on the probe chord" };
        if (is({ "Pick noise", "Finger noise", "Release noise" }))
            return { Reach::Exempt, "contact noise far under the strings on the probe chord" };
        return {};
    }

private:
    std::size_t sympathetic = 0, fxEnabled = 0, delay = 0, room = 0, output = 0;
};
} // namespace

int main(int argc, char** argv)
{
    const juce::ScopedJuceInitialiser_GUI juce;
    protocodus::matrix::Options options;
    options.jobs = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    return protocodus::matrix::Main(argc, argv, [] { return std::make_unique<ElectryMatrixDevice>(); }, options);
}
