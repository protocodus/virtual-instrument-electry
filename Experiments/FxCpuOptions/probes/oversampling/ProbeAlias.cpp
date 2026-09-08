#define main electryOriginalTestMain
#include "../../Tests/ElectryFxTests.cpp"
#undef main
int main() {
    struct Probe { float distortion, amp; double amplitude; AmpModel model; const char* name; };
    constexpr std::array<Probe, 6> probes {{
        {1.0f,0.0f,0.30,AmpModel::ModernHighGain,"pedal"},
        {0.0f,1.0f,0.30,AmpModel::ModernHighGain,"modern"},
        {0.0f,1.0f,0.30,AmpModel::AmericanClean,"american"},
        {0.0f,1.0f,0.30,AmpModel::BritishCrunch,"british"},
        {0.7f,1.0f,0.30,AmpModel::ModernHighGain,"pedal-modern"},
        {0.0f,1.0f,0.08,AmpModel::ModernHighGain,"modern-quiet"}
    }};
    std::cout << std::setprecision(12) << "scenario,rate,frequency_hz,alias_db,pass_existing_rail,latency_samples,internal_rate\n";
    for (double rate : {44100.0,48000.0,96000.0}) {
        ElectryFx fx;
        fx.prepare(rate);
        FxParameters parameters; parameters.amp=1;
        fx.setParameters(parameters);
        std::vector<float> left(4096,0.0f), right=left;
        fx.process(left.data(),right.data(),4096);
        const double fixedFrequency = rate == 96000.0 ? 1262.7 : 0.0;
        const int cycles = fixedFrequency>0 ? static_cast<int>(std::lround(fixedFrequency*aliasProbeLength/rate)) : aliasProbeCycles;
        for(const auto& probe:probes) {
            const double floorDb=aliasFloorDb(probe.distortion,probe.amp,probe.amplitude,probe.model,rate,fixedFrequency);
            std::cout << probe.name << ',' << rate << ',' << cycles*rate/aliasProbeLength << ',' << floorDb << ',' << (floorDb < -70 ? "true" : "false") << ',' << fx.gainStageLatencySamples() << ',' << FxAccess::internalRate(fx) << '\n';
            std::cout.flush();
        }
    }
}
