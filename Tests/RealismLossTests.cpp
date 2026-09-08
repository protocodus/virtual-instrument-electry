#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace electry
{
struct ElectryEngineTestAccess
{
    struct Loss { double low, high, middle, onePoleMiddle, maximumGain; float depth; };
    static bool sympatheticHandoff(ElectryEngine& engine)
    {
        auto& voice = engine.voices_[0];
        engine.configureVoiceDamping(voice, PlayStyle::Sustain, 46.2493f, 2.0f);
        for (auto* loop : { &voice.vertical, &voice.horizontal })
        {
            loop->materialLossDip.z1 = 0.125;
            loop->materialLossDip.z2 = -0.0625;
        }
        engine.configureSympatheticString(voice);
        for (const auto* loop : { &voice.vertical, &voice.horizontal })
            if (loop->materialLossDepth != 0 || loop->materialLossShape.dipOmega != 0
                || loop->materialLossDip.z1 != 0 || loop->materialLossDip.z2 != 0)
                return false;
        // Reconfiguring the played string must not revive the old section's
        // memory when a subsequent stroke enables it again.
        engine.configureVoiceDamping(voice, PlayStyle::Sustain, 46.2493f, 2.0f);
        return voice.vertical.materialLossDepth > 0
            && voice.horizontal.materialLossDepth > 0
            && voice.vertical.materialLossDip.z1 == 0
            && voice.vertical.materialLossDip.z2 == 0
            && voice.horizontal.materialLossDip.z1 == 0
            && voice.horizontal.materialLossDip.z2 == 0;
    }
    static std::array<Loss, 2> inspect(ElectryEngine& engine, int stringIndex,
                                      float fret, float frequency, PlayStyle style)
    {
        auto& voice = engine.voices_[static_cast<std::size_t>(stringIndex)];
        engine.configureVoiceDamping(voice, style, frequency, fret);
        std::array<Loss, 2> result {};
        int polarisation = 0;
        for (const auto* loop : { &voice.vertical, &voice.horizontal })
        {
            const auto pole = [&] (double a, double hz)
            {
                const double sine = std::sin(3.14159265358979323846 * hz / engine.sampleRate_);
                return (1.0 - a) / std::sqrt((1.0 - a) * (1.0 - a) + 4.0 * a * sine * sine);
            };
            const auto magnitude = [&] (double hz)
            {
                const auto z = std::polar(1.0, -6.2831853071795864769 * hz / engine.sampleRate_);
                const auto section = [&] (const auto& b)
                {
                    return std::abs((b.b0 + b.b1 * z + b.b2 * z * z)
                                  / (1.0 + b.a1 * z + b.a2 * z * z));
                };
                double m = loop->loopGain * pole(loop->loopDampingCoefficient, hz);
                if (loop->handDipActive) m *= section(loop->handDip);
                if (loop->materialLossDepth > 0) m *= section(loop->materialLossDip);
                return m;
            };
            const double highFrequency = style == PlayStyle::Dead ? frequency * 8.0 : 3600.0;
            const double middleFrequency = std::sqrt(frequency * 3600.0);
            const double low = magnitude(frequency), high = magnitude(highFrequency);
            const auto t60 = [&] (double m) { return -3.0 / (frequency * std::log10(m)); };
            // Reconstruct an equivalent two-anchor one-pole independently in
            // double precision. The candidate must add loss between the same
            // endpoints, rather than merely shorten both endpoints together.
            double lo = 0.0, hi = 0.999999;
            for (int i = 0; i < 64; ++i)
            {
                const double a = (lo + hi) * 0.5;
                if (pole(a, highFrequency) / pole(a, frequency) > high / low) lo = a;
                else hi = a;
            }
            const double a = (lo + hi) * 0.5;
            const double equivalentGain = low / pole(a, frequency);
            double maximum = 0;
            for (int i = 0; i <= 1024; ++i)
                maximum = std::max(maximum, magnitude(engine.sampleRate_ * 0.5 * i / 1024.0));
            result[static_cast<std::size_t>(polarisation++)] = {
                t60(low), t60(high), t60(magnitude(middleFrequency)),
                t60(equivalentGain * pole(a, middleFrequency)), maximum,
                loop->materialLossDepth };
        }
        return result;
    }
};
}

namespace
{
using Engine = electry::ElectryEngine;
using Access = electry::ElectryEngineTestAccess;
using Style = electry::PlayStyle;
constexpr std::array<int, 8> openNotes { 28, 35, 40, 45, 50, 55, 59, 64 };
constexpr std::array<int, 5> frets { 0, 2, 8, 14, 22 };
int failures = 0;
void expect(bool value, const std::string& message)
{
    if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
std::unique_ptr<Engine> makeEngine(double rate)
{
    auto engine = std::make_unique<Engine>();
    electry::EngineParameters p;
    p.sympatheticAmount = p.artifactAmount = p.bodyResonance = 0;
    p.pickNoise = p.fingerNoise = p.releaseNoise = 0;
    p.stringAge = 0.25f;
    p.stringGauge = 0.65f;
    engine->setParameters(p);
    engine->prepare(rate, 512);
    engine->setVariationSeed(0);
    return engine;
}
void lossGrid(double rate)
{
    // Frozen vertical anchors from commit 809aecc at 48 kHz, before the extra
    // section. These are compatibility targets, not new fitted recordings.
    constexpr double fundamental[2][5] {
        {24.6473770101, 21.9571815635, 15.0745332289, 10.9785744695, 6.9163888985},
        {23.4147405937, 20.8598968448, 14.3911039121, 10.4269936668, 6.57067943763}
    };
    constexpr double upper[2][5] {
        {.32985171361, .329836652879, .320242516202, .329841459087, .329852775573},
        {.313357819685, .313353916364, .305726902915, .313263595451, .313359731231}
    };
    auto engine = makeEngine(rate);
    expect(Access::sympatheticHandoff(*engine),
           "sympathetic handoff and resumed picking cannot revive stale loss state");
    for (int stringIndex = 0; stringIndex < 8; ++stringIndex)
        for (std::size_t f = 0; f < frets.size(); ++f)
            for (const auto style : { Style::Sustain, Style::PalmMute, Style::Dead })
            {
                const float hz = static_cast<float>(440.0 * std::exp2(
                    (openNotes[static_cast<std::size_t>(stringIndex)] + frets[f] - 69.0) / 12.0));
                const auto result = Access::inspect(*engine, stringIndex,
                                                    static_cast<float>(frets[f]), hz, style);
                for (const auto& loss : result)
                {
                    expect(std::isfinite(loss.low) && loss.low > 0 && loss.high > 0
                               && loss.maximumGain < 1.0,
                           "the complete loss cascade stays passive from DC through Nyquist");
                    if (stringIndex >= 2 || style == Style::Dead)
                        expect(loss.depth == 0.0f, "six-string surface and Dead contact bypass material correction");
                    if (stringIndex < 2 && style == Style::Sustain)
                        expect(loss.depth > 0 && loss.middle < loss.onePoleMiddle * 0.85,
                               "bass-string partials lose energy faster between preserved anchors");
                }
                if (stringIndex < 2 && style == Style::Sustain)
                {
                    expect(std::abs(result[0].low / fundamental[stringIndex][f] - 1.0) < .005,
                           "bass material loss preserves the previous fundamental anchor across rates");
                    expect(std::abs(result[0].high / upper[stringIndex][f] - 1.0) < .005,
                           "bass material loss preserves the previous fixed-Hz upper anchor across rates");
                    expect(std::abs(result[1].low / result[0].low - 1.7) < .015,
                           "both polarisations retain their independent sustain scale");
                }
            }
}
std::vector<float> phrase(double rate, int callback)
{
    auto engine = makeEngine(rate);
    engine->setSoloStringMask(1);
    std::vector<float> output;
    const auto render = [&] (double seconds)
    {
        int remaining = static_cast<int>(std::round(seconds * rate));
        std::array<float, 512> l {}, r {};
        while (remaining > 0)
        {
            const int n = std::min(remaining, callback);
            engine->process(l.data(), r.data(), n);
            output.insert(output.end(), l.begin(), l.begin() + n);
            remaining -= n;
        }
    };
    engine->noteOn(30, .85f); render(.12);
    engine->setPitchBend(.8f); render(.08);
    engine->noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(Style::Slide), 1);
    engine->noteOn(42, .8f); render(.18);
    engine->setPalmMutePressure(1); render(.08);
    engine->setPalmMutePressure(0); render(.10);
    engine->noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(Style::Dead), 1);
    engine->noteOn(34, .6f); render(.10);
    engine->noteOff(34); render(.12);
    expect(std::all_of(output.begin(), output.end(), [] (float x)
           { return std::isfinite(x) && std::abs(x) < 2; }),
           "moving filters, hand-pressure sweeps and bypasses remain bounded");
    engine->reset();
    std::array<float, 512> l {}, r {};
    engine->process(l.data(), r.data(), 512);
    expect(std::all_of(l.begin(), l.end(), [] (float x) { return x == 0; })
               && std::all_of(r.begin(), r.end(), [] (float x) { return x == 0; }),
           "reset clears every material-loss state");
    return output;
}
// Independent analysis filters measure rendered bands, not the production
// loss-section coefficients. Apply them continuously before selecting the two
// equal 160 ms windows, so resetting a filter cannot create an onset artifact.
struct MeasureFilter
{
    double b0, b1, b2, a1, a2, z1 = 0, z2 = 0;
    double process(double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    static MeasureFilter make(double rate, double cutoff, bool high)
    {
        const double w = 6.283185307179586 * cutoff / rate;
        const double c = std::cos(w), alpha = std::sin(w) / std::sqrt(2.0);
        const double a0 = 1 + alpha;
        const double b0 = (high ? 1 + c : 1 - c) / (2 * a0);
        return { b0, (high ? -2 : 2) * b0, b0, -2 * c / a0, (1 - alpha) / a0 };
    }
};
void renderedCooling(double rate)
{
    // Audible voicing regression, not calibration to a new recording: the
    // earlier quarter-strength material curve retained too much midrange
    // ringing. With this fixture it cooled the upper/low balance by only
    // 1.75-1.88 dB on string 8 and 2.88 dB on string 7. The revised plucked
    // output cools by 3.62-3.89 / 5.65-5.67 dB across all four sample rates.
    // Observe that change in the actual waveform, independently of the
    // existing preserved-anchor, passivity and bypass checks above.
    for (const auto fixture : { std::array<int, 2>{0, 30}, {0, 36}, {0, 42}, {1, 37} })
    {
        auto engine = makeEngine(rate);
        engine->setSoloStringMask(1 << fixture[0]);
        engine->noteOn(Engine::firstKeyswitchNote + static_cast<int>(electry::PickStyle::Down), 1);
        engine->noteOn(fixture[1], .85f);
        std::vector<float> audio(static_cast<std::size_t>(std::lround(1.04 * rate)));
        std::array<float, 512> right {};
        for (int at = 0; at < static_cast<int>(audio.size()); at += 512)
            engine->process(audio.data() + at, right.data(),
                            std::min(512, static_cast<int>(audio.size()) - at));
        auto lowHP = MeasureFilter::make(rate, 25, true);
        auto lowLP = MeasureFilter::make(rate, 160, false);
        auto upperHP = MeasureFilter::make(rate, 250, true);
        auto upperLP = MeasureFilter::make(rate, 1200, false);
        std::array<double, 2> low {}, upper {}, full {};
        for (std::size_t i = 0; i < audio.size(); ++i)
        {
            const double lo = lowLP.process(lowHP.process(audio[i]));
            const double hi = upperLP.process(upperHP.process(audio[i]));
            const double seconds = i / rate;
            const int window = seconds >= .05 && seconds < .21 ? 0
                             : seconds >= .85 && seconds < 1.01 ? 1 : -1;
            if (window >= 0)
            {
                low[window] += lo * lo; upper[window] += hi * hi;
                full[window] += double(audio[i]) * audio[i];
            }
        }
        expect(std::all_of(audio.begin(), audio.end(), [](float x) {
            return std::isfinite(x);
        }), "bass sustain remains finite through the complete ringing tail");
        const double cooling = 10 * std::log10((upper[1] / low[1]) / (upper[0] / low[0]));
        const double lowDecay = 10 * std::log10(low[1] / low[0]);
        const double fullDecay = 10 * std::log10(full[1] / full[0]);
        const std::string label = std::to_string(rate) + " Hz string "
            + std::to_string(8 - fixture[0]) + " note " + std::to_string(fixture[1]);
        expect(std::isfinite(cooling) && cooling < (fixture[0] == 0 ? -3.0 : -4.2),
               "bass upper partials audibly recede relative to the low body: " + label);
        expect(std::isfinite(lowDecay) && lowDecay > -8.0
                   && std::isfinite(fullDecay) && fullDecay > -10.0,
               "spectral cooling preserves an audible low-string sustain: " + label);
        std::cout << "COOLING " << rate << " string " << 8 - fixture[0] << " note " << fixture[1]
                  << " upper/low " << cooling << " low " << lowDecay << " full " << fullDecay << '\n';
    }
}

}
int main()
{
    for (double rate : {44100.0, 48000.0, 96000.0, 192000.0})
    {
        lossGrid(rate);
        renderedCooling(rate);
        expect(phrase(rate, 1) == phrase(rate, 257),
               "bass loss is sample-identical across callback sizes");
    }
    if (!failures) std::cout << "Realism loss tests passed at all four sample rates.\n";
    return failures ? 1 : 0;
}
