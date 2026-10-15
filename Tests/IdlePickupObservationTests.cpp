#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

namespace electry
{
struct ElectryEngineTestAccess
{
    // A coincident pair of identical magnetic sensors must produce identical
    // output for the same unowned string. This reciprocity-style oracle does
    // not prescribe an output level or duplicate the summing equation.
    static bool coincidentSensors(ElectryEngine& e, int stringIndex)
    {
        auto& v = e.voices_[static_cast<std::size_t>(stringIndex)];
        e.configureSympatheticString(v);
        v.sympatheticReady = true;
        v.pickupTapNeck = v.pickupTapBridge;
        v.apertureNeck = v.apertureBridge;
        v.coilPairNeck = v.coilPairBridge;
        e.neckPathActive_ = e.bridgePathActive_ = true;
        e.sympatheticInjection_ = 0.0045f;
        e.feedbackDrive_ = 0;
        double energy = 0;
        for (int i = 0; i < 10000; ++i)
        {
            ElectryEngine::RenderSums sums;
            const float drive = i < 160 ? 0.01f * std::sin(0.13f * i) : 0;
            e.renderSympatheticString(v, sums, drive);
            for (int ch = 0; ch < (e.channelsLinked_ ? 1 : 2); ++ch)
            {
                if (!std::isfinite(sums.neck[ch])
                    || sums.neck[ch] != sums.bridge[ch]) return false;
                energy += sums.neck[ch] * sums.neck[ch];
            }
        }
        return energy > 1e-12;
    }
};
}

int main()
{
    int failures = 0;
    for (double rate : {44100., 48000., 96000., 192000.})
        for (auto mode : {electry::OutputMode::Mono, electry::OutputMode::Stereo})
            for (int stringIndex : {0, 2, 7})
            {
                auto e = std::make_unique<electry::ElectryEngine>();
                electry::EngineParameters p;
                p.outputMode = mode;
                p.pickupSelector = electry::PickupSelector::Both;
                e->setParameters(p); e->prepare(rate, 64);
                if (!electry::ElectryEngineTestAccess::coincidentSensors(*e, stringIndex))
                {
                    ++failures;
                    std::cerr << "Coincident idle-string sensors disagree at " << rate
                              << " Hz, string " << stringIndex << '\n';
                }
            }
    return failures == 0 ? 0 : 1;
}
