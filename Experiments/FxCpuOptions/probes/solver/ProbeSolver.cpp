#include "DSP/ElectryFx.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
namespace electry { struct ElectryFxTestAccess {
    static std::array<double, 10> phaseInverterCoupledStep(
        AmpModel model, double drive, double sampleRate,
        std::array<double, 5>& state) noexcept
    {
        ElectryFx::AmpChannel channel {};
        channel.phaseCurrentDelta = state[0];
        channel.couplingHistoryOne = state[1];
        channel.couplingHistoryTwo = state[2];
        channel.powerGridOffsetOne = state[3];
        channel.powerGridOffsetTwo = state[4];
        const auto result = ElectryFx::phaseInverterCoupledStep(
            channel, model, drive, sampleRate);
        state = { channel.phaseCurrentDelta,
                  channel.couplingHistoryOne,
                  channel.couplingHistoryTwo,
                  channel.powerGridOffsetOne,
                  channel.powerGridOffsetTwo };
        return { result.gridOne, result.gridTwo,
                 result.plateOne, result.plateTwo,
                 result.capacitorCurrentOne,
                 result.capacitorCurrentTwo,
                 result.gridCurrentOne, result.gridCurrentTwo,
                 result.totalCurrent, result.maximumResidual };
    }

}; }
int main() {
    using electry::AmpModel;
    using electry::ElectryFxTestAccess;
    std::cout << std::setprecision(12) << "model,rate,drive,max_residual,failed_commits,max_grid_current,common_after_1ms,common_after_20ms\n";
    for (auto model : {AmpModel::AmericanClean, AmpModel::BritishCrunch})
    for (double rate : {176400.0, 192000.0, 352800.0, 384000.0, 705600.0})
    for (double amplitude : {0.000001, 0.001, 0.1, 4.0}) {
        std::array<double, 5> state {};
        double maxResidual = 0, maxGrid = 0; int failedCommits = 0;
        std::array<double, 10> point{};
        const double bias = model == AmpModel::AmericanClean ? -37.0 : -36.0;
        for (int frame = 0; frame < static_cast<int>(rate * 0.02); ++frame) {
            const double drive = amplitude * std::sin(2.0 * 3.14159265358979323846 * 1000.0 * frame/rate);
            point = ElectryFxTestAccess::phaseInverterCoupledStep(model, drive, rate, state);
            maxResidual = std::max(maxResidual, point[9]);
            failedCommits += point[9] > 1.0e-7;
            maxGrid = std::max({maxGrid, point[6], point[7]});
        }
        double common1 = 0, common20 = 0;
        for (int frame = 0; frame < static_cast<int>(rate*0.02); ++frame) {
            point = ElectryFxTestAccess::phaseInverterCoupledStep(model, 0, rate, state);
            maxResidual = std::max(maxResidual, point[9]);
            failedCommits += point[9] > 1.0e-7;
            if (frame == static_cast<int>(rate*0.001)-1) common1=0.5*(point[0]+point[1])-bias;
            common20=0.5*(point[0]+point[1])-bias;
        }
        std::cout << (model==AmpModel::AmericanClean ? "american" : "british") << ',' << rate << ',' << amplitude << ',' << maxResidual << ',' << failedCommits << ',' << maxGrid << ',' << common1 << ',' << common20 << '\n';
    }
}
