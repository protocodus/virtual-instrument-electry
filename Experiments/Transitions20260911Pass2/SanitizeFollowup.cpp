#define main electryFullRegressionMain
#include "../../Tests/ElectryEngineTests.cpp"
#undef main

int main()
{
    testReopenedPickRetainsHandRelaxation();
    testCoupledStringHandLossHasPhysicalTimeUnits();
    testCoupledStringKeepsItsFundamentalDecayTarget();
    testFinitePalmContactTransitions();
    testSharedHandRetunesActiveStringDamping();
    testSympatheticBridgeCoupling();
    if (failures != 0)
        return EXIT_FAILURE;
    std::cout << "All follow-up transition sanitizer checks passed.\n";
    return EXIT_SUCCESS;
}
