#ifndef ELECTRY_ATTACK_TEST_SOURCE
#define ELECTRY_ATTACK_TEST_SOURCE "../../../Tests/ElectryEngineTests.cpp"
#endif
#define main fullRegressionMain
#include ELECTRY_ATTACK_TEST_SOURCE
#undef main
int main() {
    testReopenedPickRetainsHandRelaxation();
    testFinitePalmContactTransitions();
    testSharedHandRetunesActiveStringDamping();
    return failures ? 1 : 0;
}
