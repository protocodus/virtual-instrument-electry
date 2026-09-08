#include "DSP/ElectryEngine.h"
#include "DSP/ElectryFx.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    // Remove only the released-string edge. The loaded modal displacement,
    // force, timing, pick image, filtering and output chain all remain intact.
    static void removeEdge(ElectryEngine& engine)
    {
        for (auto& voice : engine.voices_)
            voice.excitationTransientAmplitude = 0.0f;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Style = electry::PlayStyle;
constexpr double pi = 3.14159265358979323846;
int failures = 0;
void expect(bool ok, const std::string& message)
{
    if (! ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}

struct Render { std::vector<float> dry, modern; };

Render render(double rate, int pitch, float hardness, Style style,
              bool removeEdge, int block = 256)
{
    auto engine = std::make_unique<Engine>();
    electry::EngineParameters p;
    electry::applyGuitarBuild(p, electry::defaultGuitarBuild);
    p.pickPosition = .18f; p.pickHardness = hardness; p.stringAge = .1f;
    p.velocityAmount = .7f; p.outputGain = 1;
    p.pickNoise = p.fingerNoise = p.releaseNoise = p.artifactAmount = 0;
    p.sympatheticAmount = p.bodyResonance = 0;
    engine->prepare(rate, block); engine->setParameters(p);
    engine->setVariationSeed(0); engine->reset();
    engine->setSoloStringMask(pitch == 64 ? 128 : 1);
    engine->noteOn(Engine::firstKeyswitchNote
                  + static_cast<int>(electry::PickStyle::Down), 1);
    engine->noteOn(Engine::firstPlayStyleKeyswitchNote
                  + static_cast<int>(style), 1);
    engine->noteOn(pitch, .98f);
    if (removeEdge) electry::ElectryEngineTestAccess::removeEdge(*engine);

    electry::ElectryFx fx;
    electry::FxParameters settings;
    settings.distortion = .45f; settings.amp = .95f; settings.compressor = .60f;
    settings.ampModel = electry::AmpModel::ModernHighGain;
    fx.prepare(rate); fx.setParameters(settings); fx.reset();
    Render result;
    result.dry.resize(static_cast<std::size_t>(std::lround(rate * 1.05)));
    result.modern.resize(result.dry.size());
    std::array<float, 511> left {}, right {};
    for (int at = 0; at < static_cast<int>(result.dry.size()); at += block)
    {
        const int count = std::min(block, static_cast<int>(result.dry.size()) - at);
        engine->process(left.data(), right.data(), count);
        std::copy_n(left.data(), count, result.dry.data() + at);
        fx.process(left.data(), right.data(), count);
        std::copy_n(left.data(), count, result.modern.data() + at);
    }
    for (const auto* tap : { &result.dry, &result.modern })
        expect(std::all_of(tap->begin(), tap->end(), [](float x) {
            return std::isfinite(x) && std::abs(x) < 1.0f;
        }), "ordinary pick attack remains finite with dry and Modern headroom");
    return result;
}

double rms(const std::vector<float>& x, double rate, double start, double end)
{
    double sum = 0;
    const int first = static_cast<int>(std::lround(rate * start));
    const int last = static_cast<int>(std::lround(rate * end));
    for (int i = first; i < last; ++i) sum += double(x[i]) * x[i];
    return std::sqrt(sum / (last - first));
}

double relativeDb(double a, double b)
{
    return 20 * std::log10(std::max(a, 1e-20) / std::max(b, 1e-20));
}

double highAttack(const std::vector<float>& x, double rate)
{
    // Independent Butterworth high pass observes the audible plectrum edge.
    const double w = 2 * pi * 1800 / rate, c = std::cos(w);
    const double alpha = std::sin(w) / std::sqrt(2.0), a0 = 1 + alpha;
    const double b0 = (1 + c) / (2 * a0), b1 = -(1 + c) / a0;
    const double a1 = -2 * c / a0, a2 = (1 - alpha) / a0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0, sum = 0;
    const int count = static_cast<int>(std::lround(rate * .020));
    for (int i = 0; i < count; ++i)
    {
        const double y = b0 * x[i] + b1 * x1 + b0 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x[i]; y2 = y1; y1 = y; sum += y * y;
    }
    return std::sqrt(sum / count);
}

void edgeAndBody(double rate)
{
    for (int pitch : { 28, 30, 40, 64 })
    {
        const auto hard = render(rate, pitch, .85f, Style::Sustain, false);
        const auto modalOnly = render(rate, pitch, .85f, Style::Sustain, true);
        const auto soft = render(rate, pitch, 0, Style::Sustain, false);
        const std::string fixture = std::to_string(rate) + " Hz note " + std::to_string(pitch);
        const double edgeDifference = relativeDb(highAttack(hard.dry, rate),
                                                 highAttack(modalOnly.dry, rate));
        const double lateDifference = relativeDb(rms(hard.dry, rate, .5, 1),
                                                 rms(modalOnly.dry, rate, .5, 1));
        const double hardContour = relativeDb(highAttack(hard.dry, rate),
                                              rms(hard.dry, rate, .1, .3));
        const double softContour = relativeDb(highAttack(soft.dry, rate),
                                              rms(soft.dry, rate, .1, .3));
        const double modernContour = relativeDb(rms(hard.modern, rate, 0, .02),
                                                rms(hard.modern, rate, .1, .3));
        const double modalModernContour = relativeDb(rms(modalOnly.modern, rate, 0, .02),
                                                     rms(modalOnly.modern, rate, .1, .3));
        std::cout << "PROBE " << fixture << " high-edge ablation " << edgeDifference
                  << " dB, late-body " << lateDifference << " dB, hard-soft edge/body "
                  << hardContour - softContour << " dB, Modern edge/body "
                  << modernContour - modalModernContour << " dB\n";
        if (pitch != 64)
            expect(edgeDifference > 4, "wound string has a distinct short pick edge: " + fixture);
        else
            expect(std::abs(edgeDifference) < 2,
                   "plain string retains its modal-dominated pick attack: " + fixture);
        expect(std::abs(lateDifference) < 1.0,
               "plectrum edge does not replace the persistent string body: " + fixture);
        expect(hardContour - softContour > (pitch == 64 ? 3 : 6),
               "Pick Hardness separates attack texture from ringing level: " + fixture);
    }
}

void fingerAndCallbacks(double rate)
{
    const auto hammer = render(rate, 30, .85f, Style::Hammer, false);
    const auto noEdgeHammer = render(rate, 30, .85f, Style::Hammer, true);
    expect(hammer.dry == noEdgeHammer.dry && hammer.modern == noEdgeHammer.modern,
           "finger-only Hammer has no synthetic plectrum edge");
    for (auto style : { Style::Sustain, Style::PalmMute })
    {
        const auto reference = render(rate, 30, .85f, style, false, 1);
        for (int block : { 17, 128, 511 })
        {
            const auto partitioned = render(rate, 30, .85f, style, false, block);
            expect(reference.dry == partitioned.dry && reference.modern == partitioned.modern,
                   "pick attack is sample-identical across dry and Modern callback partitions");
        }
    }
}
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    { edgeAndBody(rate); fingerAndCallbacks(rate); }
    if (failures) std::cerr << failures << " plectrum attack checks failed\n";
    else std::cout << "Plectrum attack is audible, localized, controllable and finger-specific\n";
    return failures ? 1 : 0;
}
