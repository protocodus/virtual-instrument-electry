// Compile against the regression source from the same source tree as the DSP.
#ifndef ELECTRY_HAND_TEST_SOURCE
#define ELECTRY_HAND_TEST_SOURCE "../../../Tests/ElectryEngineTests.cpp"
#endif
#define main fullRegressionMain
#include ELECTRY_HAND_TEST_SOURCE
#undef main
int main() {
#ifdef ELECTRY_HAND_FINITE_TEST
    testFinitePalmContactTransitions();
#endif
    testSharedHandRetunesActiveStringDamping();
    testLiveDampingRefitsPreservePitch();
    testDeadNote();
    return failures ? 1 : 0;
}
