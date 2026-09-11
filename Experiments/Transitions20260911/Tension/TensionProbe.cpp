// Isolated diagnostic; see README.md for the frozen sources and replay command.
#define main fullEngineTestMain
#include "Tests/ElectryEngineTests.cpp"
#undef main
#include <fstream>
#include <iomanip>
int main()
{
    std::cout << "rate,note,style,hold_ms,q_before,q_after,coordinate_jump_c,target_jump_c,current_jump_c\n";
    for (double rate : {44100.0, 48000.0, 96000.0, 192000.0})
    for (int note : {28, 40, 55})
    for (auto style : {PlayStyle::PalmMute, PlayStyle::Dead})
    for (double hold : {0.05, 0.15})
    {
        ElectryEngine engine;
        EngineParameters p;
        p.sympatheticAmount = p.artifactAmount = p.pickNoise = p.fingerNoise = p.releaseNoise = 0;
        p.strumSpreadSeconds = 0;
        engine.prepare(rate, 512); engine.setParameters(p); engine.reset();
        engine.noteOn(note, 1);
        StereoBuffer initial(static_cast<int>(hold * rate)); renderInto(engine, initial);
        int si = TestAccess::stringForNote(engine, note);
        engine.noteOn(styleKeyswitch(style), 1);
        engine.noteOn(note, 0.75);
        while (TestAccess::snapshot(engine, si).excitationInContact)
            TestAccess::renderOneInternalSample(engine);
        const auto before = TestAccess::attackPitchState(engine, si);
        const auto targetBefore = TestAccess::effectiveLoopFrequency(engine, si, false, true);
        const auto currentBefore = TestAccess::effectiveLoopFrequency(engine, si);
        TestAccess::renderOneInternalSample(engine);
        const auto after = TestAccess::attackPitchState(engine, si);
        const auto targetAfter = TestAccess::effectiveLoopFrequency(engine, si, false, true);
        const auto currentAfter = TestAccess::effectiveLoopFrequency(engine, si);
        std::cout << std::setprecision(10) << rate << ',' << note << ',' << static_cast<int>(style) << ',' << hold * 1000
            << ',' << before.tensionRatio << ',' << after.tensionRatio
            << ',' << 1200 * std::log2(after.frequencyFactor / before.frequencyFactor)
            << ',' << 1200 * std::log2(targetAfter / targetBefore)
            << ',' << 1200 * std::log2(currentAfter / currentBefore) << '\n';
    }
}
