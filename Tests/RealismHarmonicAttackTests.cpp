#include "DSP/ElectryEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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

std::vector<float> note(double rate, int pitch, float velocity, Style style,
                        int block = 256)
{
    auto e = std::make_unique<Engine>();
    electry::EngineParameters p;
    electry::applyGuitarBuild(p, electry::defaultGuitarBuild);
    p.pickPosition = 0.18f; p.pickHardness = 0.85f; p.stringAge = 0.1f;
    p.velocityAmount = 0.7f; p.outputGain = 1;
    p.pickNoise = p.fingerNoise = p.releaseNoise = p.artifactAmount = 0;
    p.sympatheticAmount = p.bodyResonance = 0;
    e->prepare(rate, block); e->setParameters(p); e->setVariationSeed(0);
    e->reset(); e->setSoloStringMask(pitch == 64 ? 128 : 1);
    e->noteOn(Engine::firstKeyswitchNote + static_cast<int>(electry::PickStyle::Down), 1);
    e->noteOn(Engine::firstPlayStyleKeyswitchNote + static_cast<int>(style), 1);
    e->noteOn(pitch, velocity);
    std::vector<float> result(static_cast<std::size_t>(std::lround(rate * 0.8)));
    std::array<float, 511> right {};
    for (int at = 0; at < static_cast<int>(result.size()); at += block)
        e->process(result.data() + at, right.data(),
                   std::min(block, static_cast<int>(result.size()) - at));
    expect(std::all_of(result.begin(), result.end(), [](float x) {
        return std::isfinite(x);
    }), "finite harmonic attack and ringing tail");
    return result;
}

double rms(const std::vector<float>& x, int start, int end)
{
    double sum = 0;
    for (int i = start; i < end; ++i) sum += double(x[i]) * x[i];
    return std::sqrt(sum / std::max(1, end - start));
}

double peak(const std::vector<float>& x)
{
    double result = 0;
    for (float value : x) result = std::max(result, std::abs(double(value)));
    return result;
}

// Independent two-pole high pass for the sharp edge. Divide its first 25 ms
// energy by the ringing body so simply turning the old pulse down cannot pass.
double highAttack(const std::vector<float>& x, double rate)
{
    const double w = 2 * pi * 2000 / rate, c = std::cos(w);
    const double alpha = std::sin(w) / std::sqrt(2.0), a0 = 1 + alpha;
    const double b0 = (1 + c) / (2 * a0), b1 = -(1 + c) / a0;
    const double a1 = -2 * c / a0, a2 = (1 - alpha) / a0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0, energy = 0;
    const int n = static_cast<int>(rate * 0.025);
    for (int i = 0; i < n; ++i)
    {
        const double y = b0 * x[i] + b1 * x1 + b0 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x[i]; y2 = y1; y1 = y;
        energy += y * y;
    }
    return std::sqrt(energy / n);
}

void pluckedAttack(double rate)
{
    double worstEdgeToBody = 0, worstPeak = 0;
    for (int pitch : { 28, 30, 40, 45, 64 })
    {
        std::array<double, 2> harmonicDynamics {}, sustainDynamics {};
        for (int effort = 0; effort < 2; ++effort)
        {
            const float velocity = effort ? 1.0f : 0.35f;
            const auto natural = note(rate, pitch, velocity, Style::Harmonics);
            const auto sustain = note(rate, pitch, velocity, Style::Sustain);
            const int start = static_cast<int>(rate * .10), end = static_cast<int>(rate * .60);
            const double body = rms(natural, start, end);
            const double pickedBody = rms(sustain, start, end);
            const double edgeToBody = highAttack(natural, rate) / std::max(body, 1e-20);
            harmonicDynamics[effort] = body; sustainDynamics[effort] = pickedBody;
            worstEdgeToBody = std::max(worstEdgeToBody, edgeToBody);
            worstPeak = std::max(worstPeak, peak(natural));
            const std::string fixture = std::to_string(rate) + " Hz note "
                + std::to_string(pitch) + " velocity " + std::to_string(velocity);
            expect(body > pickedBody * 0.20 && body < pickedBody * 0.90,
                   "lightly touched harmonic has audible body below the ordinary pluck: " + fixture);
            expect(edgeToBody < 2.5,
                   "harmonic attack is not dominated by an artificial high-frequency pulse: " + fixture);
            expect(peak(natural) < 0.5 && peak(natural) < peak(sustain) * 2.0,
                   "natural harmonic does not introduce a raw-output spike: " + fixture);
        }
        const double harmonicRange = harmonicDynamics[1] / harmonicDynamics[0];
        const double sustainRange = sustainDynamics[1] / sustainDynamics[0];
        // The production velocityAmount=.7 force mapping compresses these
        // velocities to roughly 3-5 dB. Retain an audible range and keep its
        // response consistent with the same player's ordinary pick stroke.
        expect(harmonicRange > 1.26 && sustainRange > 1.26
                   && harmonicRange / sustainRange > 0.80
                   && harmonicRange / sustainRange < 1.25,
               "soft and hard playing retain the ordinary pick's ringing-level dynamics");
    }
    std::cout << "PROBE " << rate << " Hz harmonic peak "
              << 20 * std::log10(worstPeak) << " dBFS; worst high-edge/body "
              << 20 * std::log10(worstEdgeToBody) << " dB\n";
}

// Scan the actual output lines so stiffness does not get mistaken for loss.
double partial(const std::vector<float>& x, double rate, double frequency)
{
    const int start = static_cast<int>(rate * .10), count = static_cast<int>(rate * .35);
    double peakMagnitude = 0;
    for (int step = 0; step <= 20; ++step)
    {
        const double w = 2 * pi * frequency * (0.99 + .0015 * step) / rate;
        const std::complex<double> advance(std::cos(w), -std::sin(w));
        std::complex<double> oscillator(1, 0), total(0, 0);
        for (int i = 0; i < count; ++i)
        {
            const double window = .5 - .5 * std::cos(2 * pi * i / (count - 1));
            total += oscillator * (double(x[start + i]) * window);
            oscillator *= advance;
        }
        peakMagnitude = std::max(peakMagnitude, std::abs(total));
    }
    return peakMagnitude;
}

void octaveAndCallbacks(double rate)
{
    const auto natural = note(rate, 40, .85f, Style::Harmonics, 1);
    const double f0 = 440 * std::exp2((40.0 - 69) / 12);
    const double octave = partial(natural, rate, 2 * f0);
    for (int odd : { 1, 3, 5 })
        expect(partial(natural, rate, odd * f0) < octave * .10,
               "ordinary pluck retains the finger-selected octave and suppresses odd modes");
    expect(partial(natural, rate, 4 * f0) > octave * .10,
           "natural harmonic retains ringing upper partials rather than becoming a sine");
    for (int block : { 17, 128, 511 })
        expect(note(rate, 40, .85f, Style::Harmonics, block) == natural,
               "harmonic waveform is sample-identical across callback partitions");
}
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    { pluckedAttack(rate); octaveAndCallbacks(rate); }
    if (failures) std::cerr << failures << " natural-harmonic attack checks failed\n";
    else std::cout << "Natural harmonics retain a plucked attack, body, dynamics and node selection\n";
    return failures ? 1 : 0;
}
