// Reuse the exact previous score, then append a normal low-string rhythm
// with spaces that expose ringing in the unplayed strings.
#define main originalTransitionMain
#include "../Transitions20260911/RenderTransitions.cpp"
#undef main
#include <iterator>

int main(int argc, char** argv)
{
    const int originalResult = originalTransitionMain(argc, argv);
    if (originalResult != 0)
        return originalResult;
    try
    {
        const std::filesystem::path directory(argv[1]);
        const auto path = directory / "manifest.json";
        std::ifstream input(path);
        const std::string previous((std::istreambuf_iterator<char>(input)), {});
        const auto end = previous.rfind("\n]}");
        if (end == std::string::npos)
            throw std::runtime_error("Previous score manifest has an unexpected ending");
        input.close();
        std::ofstream manifest(path);
        manifest << std::setprecision(9) << previous.substr(0, end);
        bool first = false;
        for (bool control : {false, true})
        {
            Take t(control ? "low_rhythm_gaps_no_coupling" : "low_rhythm_gaps",
                   control ? "Low-string rhythm - coupling disabled control"
                           : "Low-string rhythm - open and muted gaps",
                   "One held E1 and three open / muted / open cycles. The spaces expose idle-string ring; the control disables sympathetic coupling with the same picks and hand commands.",
                   control ? 0.0f : 0.20f);
            t.on(0, 28, 0.90f);
            for (int cycle = 0; cycle < 3; ++cycle)
            {
                const double begin = 0.25 + 2.2 * cycle;
                if (cycle > 0)
                {
                    t.time(begin);
                    t.repick(0, 0.90f);
                }
                t.time(begin + 0.25); t.repick(0, 0.84f);
                t.time(begin + 0.55); t.mark("Palm down"); t.style(PlayStyle::PalmMute);
                t.time(begin + 0.60); t.mark("Muted repick"); t.repick(0, 0.82f);
                t.time(begin + 0.85); t.repick(0, 0.75f);
                t.time(begin + 1.10); t.repick(0, 0.82f);
                t.time(begin + 1.55); t.mark("Palm lifts"); t.style(PlayStyle::Sustain);
                t.time(begin + 1.60); t.mark("Open repick"); t.repick(0, 0.90f);
                t.time(begin + 1.85); t.repick(0, 0.84f);
            }
            t.wait(0.55); t.finish(0.90);
            save(manifest, directory, t, first);
        }
        manifest << "\n]}\n";
        manifest.close();
        if (!manifest)
            throw std::runtime_error("Could not write extended manifest");
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
