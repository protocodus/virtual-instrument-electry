// Reuse the production regression oracles for a bounded ASan/UBSan pass.
// The full Release suite separately covers audio references and CPU limits.
#define main electryFullRegressionMain
#include "../../Tests/ElectryEngineTests.cpp"
#undef main

int main()
{
    testSharedHandRetunesActiveStringDamping();
    testFinitePalmContactTransitions();
#if ELECTRY_ENERGY_ATTACK_PITCH
    testEnergyPitchMuteContinuity();
#endif
    testHeldStringRepickKeys();
    testLiveDampingRefitsPreservePitch();
    testPalmMuteHandContactDynamics();
    testHandDipNeverExpands();
    if (failures != 0)
        return EXIT_FAILURE;
    std::cout << "All focused transition sanitizer checks passed.\n";
    return EXIT_SUCCESS;
}
