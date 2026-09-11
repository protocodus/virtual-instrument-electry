// Isolated diagnostic; see README.md for the frozen sources and replay command.
#define main fullEngineTestMain
#include "Tests/ElectryEngineTests.cpp"
#undef main
#include <fstream>
#include <filesystem>
int main(int argc, char** argv)
{
    const std::filesystem::path directory = argc > 1 ? argv[1] : ".";
    std::filesystem::create_directories(directory);
    for (const auto scenario : {0, 1, 2, 3})
    for (const int block : {17, 256})
    {
        constexpr double rate = 48000;
        EngineParameters p;
        p.sympatheticAmount = p.artifactAmount = p.pickNoise = p.fingerNoise = p.releaseNoise = 0;
        p.strumSpreadSeconds = 0;
        ElectryEngine engine; engine.prepare(rate, 512); engine.setParameters(p); engine.reset();
        std::vector<float> audio;
        const auto render = [&] (double seconds) {
            StereoBuffer buffer(static_cast<int>(rate * seconds)); renderInto(engine, buffer, block);
            for (std::size_t i = 0; i < buffer.left.size(); ++i)
                audio.push_back((buffer.left[i] + buffer.right[i]) * 0.5f);
        };
        if (scenario == 1) engine.noteOn(styleKeyswitch(PlayStyle::PalmMute), 1);
        if (scenario == 3) {
            std::array<ElectryEngine::NoteOnEvent, 3> chord {{{28, .95f}, {35, .85f}, {40, .80f}}};
            engine.noteOnChord(chord);
        } else engine.noteOn(28, .95);
        if (scenario >= 2) {
            render(.05);
            engine.noteOn(styleKeyswitch(PlayStyle::PalmMute), 1); engine.noteOn(28, .75);
            render(.15);
            engine.noteOn(styleKeyswitch(PlayStyle::Sustain), 1); engine.noteOn(28, .85);
            render(.3);
            engine.noteOn(styleKeyswitch(PlayStyle::PalmMute), 1); engine.noteOn(28, .65);
            render(.15);
            engine.noteOn(styleKeyswitch(PlayStyle::Sustain), 1); engine.noteOn(28, .95);
            render(.55);
        } else render(1.2);
        std::ofstream output(directory / (std::to_string(scenario) + "-" + std::to_string(block) + ".f32"), std::ios::binary);
        output.write(reinterpret_cast<const char*>(audio.data()), audio.size() * sizeof(float));
    }
}
