// A circulating-line state proxy, not a full time-varying energy proof.
#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace electry {
struct ElectryEngineTestAccess {
    static double lineEnergy(const ElectryEngine& engine, int string) {
        const auto& voice = engine.voices_[static_cast<std::size_t>(string)];
        double result = 0;
        for (const auto* line : {&voice.vertical.line, &voice.horizontal.line})
            for (const float sample : *line)
                result += static_cast<double>(sample) * sample;
        return result;
    }
    static int stringForNote(const ElectryEngine& engine, int note) {
        for (int i=0; i<ElectryEngine::stringCount; ++i)
            if (engine.voices_[static_cast<std::size_t>(i)].midiNote == note) return i;
        throw std::runtime_error("Energy fixture lost its physical string");
    }
    static int hostControlFrames(const ElectryEngine& engine, double hostRate) {
        return ElectryEngine::controlPeriod
            / static_cast<int>(std::lround(engine.sampleRate_ / hostRate));
    }
};
}
int main() {
    using electry::ElectryEngine;
    using Access = electry::ElectryEngineTestAccess;
    for (const double rate : {44100.,48000.,96000.}) {
        auto contact = std::make_unique<ElectryEngine>();
        auto open = std::make_unique<ElectryEngine>();
        electry::EngineParameters p;
        p.sympatheticAmount=0; p.bodyResonance=0; p.pickNoise=0;
        p.fingerNoise=0; p.releaseNoise=0; p.artifactAmount=0;
        std::array<float,256> left{},right{};
        for (auto* engine : {contact.get(),open.get()}) {
            engine->prepare(rate,256); engine->setParameters(p); engine->reset();
            engine->noteOn(40,.9f);
            for (int n=0;n<static_cast<int>(.2*rate);n+=256)
                engine->process(left.data(),right.data(),std::min(256,static_cast<int>(.2*rate)-n));
        }
        const int string = Access::stringForNote(*contact,40);
        const int tick = Access::hostControlFrames(*contact,rate);
        double worst=0, silencePeak=0;
        for (const float pressure : {.85f,0.f,.85f,0.f,.85f,0.f}) {
            contact->setPalmMutePressure(pressure);
            for (int n=0;n<static_cast<int>(.12*rate);n+=tick) {
                contact->process(left.data(),right.data(),tick);
                open->process(left.data(),right.data(),tick);
                worst=std::max(worst,Access::lineEnergy(*contact,string)
                    /std::max(Access::lineEnergy(*open,string),1.e-30));
            }
        }
        contact->reset();
        for (const float pressure : {.85f,0.f,.85f,0.f}) {
            contact->setPalmMutePressure(pressure);
            for (int n=0;n<100;++n) {
                contact->process(left.data(),right.data(),tick);
                for (int i=0;i<tick;++i) {
                    if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
                        throw std::runtime_error("Silent hand motion produced invalid audio");
                    silencePeak=std::max({silencePeak,static_cast<double>(std::abs(left[i])),
                                          static_cast<double>(std::abs(right[i]))});
                }
            }
        }
        std::cout<<rate<<" Hz: maximum circulating-line energy / never-muted twin "
                 <<worst<<"; silent hand motion peak "<<silencePeak<<'\n';
        if (worst>1.01 || silencePeak!=0)
            throw std::runtime_error("Hand motion exceeded the line-energy proxy bound or created audio from silence");
    }
}
