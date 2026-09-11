// Standalone before/after probes for the September 2026 fourth realism pass.
// Build against either archived or current Source with the same production
// definitions. Every score produces simultaneous unnormalised pre/post-FX taps.
// For example, from the repository root:
// clang++ -std=c++20 -O2 -DNDEBUG -DELECTRY_DECOUPLED_PICK_RELEASE=1
//   -DELECTRY_MEASURED_BODY_RESPONSE=1 -ISource
//   Experiments/Realism20260908Pass4/RenderRealism.cpp Source/DSP/ElectryEngine.cpp
//   Source/DSP/ElectryFx.cpp Source/DSP/ElectryVisuals.cpp -o build/render-realism
// build/render-realism build/realism
#include "DSP/ElectryEngine.h"
#include "DSP/ElectryFx.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr int rate = 44100;
constexpr int block = 256;
constexpr double lead = 0.25;
using electry::ElectryEngine;
using electry::PickStyle;
using electry::PlayStyle;

struct Event
{
    int frame;
    std::string kind;
    int note;
    float value;
};

electry::EngineParameters guitar()
{
    electry::EngineParameters p;
    electry::applyGuitarBuild(p, electry::defaultGuitarBuild);
    p.pickupSelector = electry::PickupSelector::Bridge;
    p.outputMode = electry::OutputMode::Mono;
    p.toneKnob = 1.0f;
    p.stringAge = 0.10f;
    p.pickHardness = 0.85f;
    p.pickPosition = 0.18f;
    p.velocityAmount = 0.70f;
    p.sympatheticAmount = 0.0f;
    p.muteDamping = 0.85f;
    p.outputGain = 1.0f;
    p.fingerNoise = 0.55f;
    p.artifactAmount = 0.15f;
    return p;
}

electry::FxParameters amplifier()
{
    electry::FxParameters p;
    p.distortion = 0.45f;
    p.amp = 0.95f;
    p.ampModel = electry::AmpModel::ModernHighGain;
    p.compressor = 0.60f;
    return p;
}

void little(std::ostream& out, std::uint32_t value, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        out.put(static_cast<char>((value >> (i * 8)) & 255));
}

void wav(const std::filesystem::path& path, const std::vector<float>& samples)
{
    std::ofstream out(path, std::ios::binary);
    const auto size = static_cast<std::uint32_t>(samples.size() * sizeof(float));
    out.write("RIFF", 4); little(out, size + 36, 4); out.write("WAVEfmt ", 8);
    little(out, 16, 4); little(out, 3, 2); little(out, 1, 2);
    little(out, rate, 4); little(out, rate * 4, 4);
    little(out, 4, 2); little(out, 32, 2); out.write("data", 4);
    little(out, size, 4);
    for (const auto value : samples)
        little(out, std::bit_cast<std::uint32_t>(value), 4);
    out.close();
    if (! out)
        throw std::runtime_error("Could not write " + path.string());
}


} // namespace
int main(int argc, char** argv) {
 if(argc != 2) return 2;
 std::filesystem::path out(argv[1]); std::filesystem::create_directories(out);
 for(int pickup=0;pickup<3;++pickup) for(int note:{28,40}) {
  ElectryEngine engine; engine.prepare(rate,block);
  electry::EngineParameters p; electry::applyGuitarBuild(p,electry::defaultGuitarBuild);
  p.pickupSelector=static_cast<electry::PickupSelector>(pickup); p.outputMode=electry::OutputMode::Mono;
  engine.setParameters(p); engine.setVariationSeed(0); engine.reset();
  std::vector<float> samples; std::array<float,block> l{},r{};
  auto render=[&](int n) { while(n>0) { int size=std::min(n,block); engine.process(l.data(),r.data(),size); samples.insert(samples.end(),l.begin(),l.begin()+size); n-=size; } };
  render(rate/4); engine.noteOn(note,0.9f); render(rate*2); engine.noteOff(note); render(rate/2);
  wav(out/("e"+std::to_string(note==28?1:2)+"-pickup"+std::to_string(pickup)+".wav"),samples);
 }
}
