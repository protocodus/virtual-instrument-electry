#include "DSP/ElectryFx.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using electry::AmpModel;
using electry::ElectryFx;
using electry::FxParameters;
constexpr double pi = 3.14159265358979323846;
constexpr std::array<int, 3> sampleRates { 44100, 48000, 96000 };

struct Scenario
{
    const char* name;
    FxParameters parameters;
    bool automated { false };
};

constexpr std::array<Scenario, 12> scenarios {{
    { "bypass", {} },
    { "pedal", { 0.95f } },
    { "american", { 0.0f, 0.95f, AmpModel::AmericanClean } },
    { "british", { 0.0f, 0.95f, AmpModel::BritishCrunch } },
    { "modern", { 0.0f, 0.95f, AmpModel::ModernHighGain } },
    { "compressor", { 0.0f, 0.0f, AmpModel::ModernHighGain, 0.8f } },
    { "delay", { 0.0f, 0.0f, AmpModel::ModernHighGain, 0.0f, 0.75f } },
    { "room", { 0.0f, 0.0f, AmpModel::ModernHighGain, 0.0f, 0.0f, 0.75f } },
    { "all-american", { 0.8f, 0.95f, AmpModel::AmericanClean, 0.65f, 0.45f, 0.55f } },
    { "all-british", { 0.8f, 0.95f, AmpModel::BritishCrunch, 0.65f, 0.45f, 0.55f } },
    { "all-modern", { 0.8f, 0.95f, AmpModel::ModernHighGain, 0.65f, 0.45f, 0.55f } },
    { "automation", {}, true }
}};

struct Options
{
    enum class Mode { benchmark, capture, compare } mode { Mode::benchmark };
    std::filesystem::path directory;
    double seconds { 2.0 };
    int repeats { 5 };
    int blockSize { 128 };
    int sampleRate { 0 };
    std::string scenario;
    double maximumError { 5.0e-5 };
    double maximumRmsError { 1.0e-6 };
};

struct Audio
{
    std::vector<float> left;
    std::vector<float> right;
};

struct Metrics
{
    double peakError { 0.0 };
    double rmsError { 0.0 };
    double rmsReference { 0.0 };
    std::size_t changedSamples { 0 };
};

// Stateless deterministic excitation: different L/R fundamentals and phases,
// intermodulation tones, noise, pluck-like bursts, and near-full-scale peaks.
// Capture/compare adds low-level intervals and a final quarter of silence to
// exercise nonlinear dynamics, smoothed controls, bypass clearing, and tails.
Audio makeInput(int rate, std::size_t frames, bool includeTail)
{
    Audio audio { std::vector<float>(frames), std::vector<float>(frames) };
    std::uint32_t random = 0x454c4658u;
    for (std::size_t frame = 0; frame < frames; ++frame)
    {
        const double time = static_cast<double>(frame) / rate;
        const double progress = static_cast<double>(frame) / frames;
        const double decay = std::exp(-14.0 * std::fmod(time, 0.173));
        const double level = includeTail && progress < 0.125 ? 0.003
                           : includeTail && progress >= 0.75 ? 0.0 : 0.8;
        random = random * 1664525u + 1013904223u;
        const double noise = static_cast<double>(random >> 8) / 8388608.0 - 1.0;
        audio.left[frame] = static_cast<float>(level * (
            0.57 * std::sin(2.0 * pi * 82.406889 * time)
            + 0.22 * std::sin(2.0 * pi * 1207.0 * time)
            + 0.12 * std::sin(2.0 * pi * 7039.0 * time)
            + decay * (0.31 * std::sin(2.0 * pi * 329.627556 * time)
                       + 0.07 * noise)));
        audio.right[frame] = static_cast<float>(level * (
            0.51 * std::sin(2.0 * pi * 110.0 * time + 0.7)
            + 0.27 * std::sin(2.0 * pi * 1733.0 * time + 0.2)
            + 0.14 * std::sin(2.0 * pi * 6197.0 * time)
            + decay * (0.32 * std::sin(2.0 * pi * 440.0 * time)
                       - 0.05 * noise)));
    }
    return audio;
}

FxParameters parametersAt(const Scenario& scenario, std::size_t offset,
                          std::size_t frames)
{
    if (! scenario.automated)
        return scenario.parameters;

    // Start dry, engage each voice, move every control continuously, switch
    // voices while the circuit is live, then disengage and re-engage tails.
    const double progress = static_cast<double>(offset) / frames;
    const auto phase = std::min(7, static_cast<int>(progress * 8.0));
    if (phase == 0 || phase == 6)
        return {};
    const auto voice = static_cast<AmpModel>(phase % 3);
    const float motion = static_cast<float>(0.5 + 0.5 * std::sin(progress * 57.0));
    return { phase == 2 ? 0.0f : motion,
             phase == 4 ? 0.0f : 0.15f + 0.85f * motion,
             voice, 0.1f + 0.8f * motion,
             phase == 7 ? 0.6f : 0.7f * motion,
             phase == 7 ? 0.7f : 0.8f * (1.0f - motion) };
}

void process(ElectryFx& fx, Audio& audio, const Scenario& scenario, int blockSize)
{
    for (std::size_t offset = 0; offset < audio.left.size();
         offset += static_cast<std::size_t>(blockSize))
    {
        const auto count = std::min(static_cast<std::size_t>(blockSize),
                                    audio.left.size() - offset);
        // Include the once-per-host-block control update in measured cost.
        fx.setParameters(parametersAt(scenario, offset, audio.left.size()));
        fx.process(audio.left.data() + offset, audio.right.data() + offset,
                   static_cast<int>(count));
    }
}

void checkFinite(const Audio& audio)
{
    for (const auto* channel : { &audio.left, &audio.right })
        for (const float sample : *channel)
            if (! std::isfinite(sample))
                throw std::runtime_error("non-finite output");
}

Metrics compare(const Audio& actual, const Audio& reference)
{
    if (actual.left.size() != reference.left.size()
        || actual.right.size() != reference.right.size()
        || actual.left.size() != actual.right.size() || actual.left.empty())
        throw std::runtime_error("incompatible audio sizes");
    checkFinite(actual);
    checkFinite(reference);
    Metrics result;
    double squareError = 0.0;
    double squareReference = 0.0;
    for (std::size_t i = 0; i < actual.left.size(); ++i)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            const float actualSample = channel == 0 ? actual.left[i] : actual.right[i];
            const float referenceSample = channel == 0 ? reference.left[i] : reference.right[i];
            const double a = actualSample;
            const double b = referenceSample;
            const double error = a - b;
            result.peakError = std::max(result.peakError, std::abs(error));
            squareError += error * error;
            squareReference += b * b;
            result.changedSamples += std::bit_cast<std::uint32_t>(actualSample)
                                  != std::bit_cast<std::uint32_t>(referenceSample) ? 1u : 0u;
        }
    }
    const double count = static_cast<double>(actual.left.size()) * 2.0;
    result.rmsError = std::sqrt(squareError / count);
    result.rmsReference = std::sqrt(squareReference / count);
    return result;
}

double db(double amplitude)
{
    return amplitude > 0.0 ? 20.0 * std::log10(amplitude)
                           : -std::numeric_limits<double>::infinity();
}

// Deliberately a local, native-endian float reference format. Comparisons
// require the same architecture/compiler flags; it is not a media interchange
// format. The header guards scenario, stimulus version, rate, frame count,
// callback size, and the optional measured-cabinet build configuration.
std::array<std::uint32_t, 8> referenceHeader(int rate, std::size_t frames,
                                          int blockSize, std::size_t scenario)
{
    return { 0x58464c45u, 1u, static_cast<std::uint32_t>(rate),
             static_cast<std::uint32_t>(frames),
             static_cast<std::uint32_t>(blockSize),
             static_cast<std::uint32_t>(scenario),
             ELECTRY_MEASURED_MODERN_CABINET, 2u };
}

void writeReference(const std::filesystem::path& path, const Audio& audio,
                    const std::array<std::uint32_t, 8>& header)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(header.data()), sizeof(header));
    for (const auto* channel : { &audio.left, &audio.right })
        file.write(reinterpret_cast<const char*>(channel->data()),
                   static_cast<std::streamsize>(channel->size() * sizeof(float)));
    file.close();
    if (! file)
        throw std::runtime_error("could not write " + path.string());
}

Audio readReference(const std::filesystem::path& path,
                    const std::array<std::uint32_t, 8>& expected)
{
    std::ifstream file(path, std::ios::binary);
    std::array<std::uint32_t, 8> actual {};
    file.read(reinterpret_cast<char*>(actual.data()), sizeof(actual));
    if (! file || actual != expected)
        throw std::runtime_error("missing/incompatible reference: " + path.string());
    const auto frames = static_cast<std::size_t>(expected[3]);
    Audio audio { std::vector<float>(frames), std::vector<float>(frames) };
    for (auto* channel : { &audio.left, &audio.right })
        file.read(reinterpret_cast<char*>(channel->data()),
                  static_cast<std::streamsize>(frames * sizeof(float)));
    if (! file || file.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("truncated/oversized reference: " + path.string());
    checkFinite(audio);
    return audio;
}

void help()
{
    std::cout
        << "Usage: ElectryBenchmarkFx [--benchmark | --capture DIR | --compare DIR]\n"
           "       [--seconds 2] [--repeats 5] [--block-size 128]\n"
           "       [--rate 44100|48000|96000] [--scenario NAME]\n"
           "       [--max-error 0.00005] [--max-rms-error 0.000001]\n\n"
           "Default: CSV median/min elapsed processing time at all three rates.\n"
           "Input generation, copies, allocation, reset and initial table warmup\n"
           "are excluded. Per-block setParameters is included. Run Release builds\n"
           "sequentially on an otherwise idle machine using matching flags.\n\n"
           "Capture/compare: deterministic stereo high-drive excitation, low-level\n"
           "passages, automation and silence tails. Compare exits nonzero for a\n"
           "missing/incompatible reference, non-finite sample, or exceeded limit.\n"
           "References require the same seconds/block-size/build configuration.\n"
           "Defaults bound peak error to 5e-5 FS and RMS error to -120 dBFS.\n"
           "A numerical null comparison supports, but cannot prove, inaudibility.\n\n"
           "Scenarios:";
    for (const auto& scenario : scenarios)
        std::cout << ' ' << scenario.name;
    std::cout << '\n';
}

Options parseOptions(int argc, char** argv)
{
    Options options;
    bool modeSet = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string option = argv[i];
        const auto value = [&]() -> std::string {
            if (++i >= argc)
                throw std::runtime_error("missing value for " + option);
            return argv[i];
        };
        if (option == "--benchmark" || option == "--capture" || option == "--compare")
        {
            if (modeSet)
                throw std::runtime_error("choose exactly one mode");
            modeSet = true;
            options.mode = option == "--capture" ? Options::Mode::capture
                         : option == "--compare" ? Options::Mode::compare
                                                 : Options::Mode::benchmark;
            if (options.mode != Options::Mode::benchmark)
                options.directory = value();
        }
        else if (option == "--seconds") options.seconds = std::stod(value());
        else if (option == "--repeats") options.repeats = std::stoi(value());
        else if (option == "--block-size") options.blockSize = std::stoi(value());
        else if (option == "--rate") options.sampleRate = std::stoi(value());
        else if (option == "--scenario") options.scenario = value();
        else if (option == "--max-error") options.maximumError = std::stod(value());
        else if (option == "--max-rms-error") options.maximumRmsError = std::stod(value());
        else throw std::runtime_error("unknown option: " + option);
    }
    if (! std::isfinite(options.seconds) || options.seconds < 0.05 || options.seconds > 60.0
        || options.repeats < 1 || options.repeats > 100
        || options.blockSize < 1 || options.blockSize > 65536
        || ! std::isfinite(options.maximumError) || options.maximumError < 0.0
        || ! std::isfinite(options.maximumRmsError) || options.maximumRmsError < 0.0)
        throw std::runtime_error("invalid duration/repeats/block size/error limit");
    if (options.sampleRate != 0
        && std::find(sampleRates.begin(), sampleRates.end(), options.sampleRate) == sampleRates.end())
        throw std::runtime_error("rate must be 44100, 48000 or 96000");
    if (! options.scenario.empty()
        && std::none_of(scenarios.begin(), scenarios.end(), [&](const auto& scenario) {
            return options.scenario == scenario.name;
        }))
        throw std::runtime_error("unknown scenario: " + options.scenario);
    return options;
}
} // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--help")
    {
        help();
        return 0;
    }
    try
    {
        const auto options = parseOptions(argc, argv);
        const bool benchmark = options.mode == Options::Mode::benchmark;
        if (options.mode == Options::Mode::capture)
            std::filesystem::create_directories(options.directory);

        std::cout << std::setprecision(10);
        if (benchmark)
            std::cout << "scenario,rate,block_size,frames,repeats,median_ns_per_frame,min_ns_per_frame,median_realtime_percent,checksum\n";
        else if (options.mode == Options::Mode::compare)
            std::cout << "scenario,rate,block_size,frames,peak_error,rms_error,rms_error_dbfs,relative_rms_error_db,reference_rms_dbfs,changed_samples,pass\n";
        else
            std::cout << "scenario,rate,block_size,frames,reference\n";

        bool passed = true;
        for (const int rate : sampleRates)
        {
            if (options.sampleRate != 0 && options.sampleRate != rate)
                continue;
            const auto frames = static_cast<std::size_t>(std::llround(options.seconds * rate));
            const auto input = makeInput(rate, frames, ! benchmark);
            for (std::size_t index = 0; index < scenarios.size(); ++index)
            {
                const auto& scenario = scenarios[index];
                if (! options.scenario.empty() && options.scenario != scenario.name)
                    continue;
                ElectryFx fx;
                fx.prepare(rate);
                Audio audio = input;
                if (benchmark)
                {
                    process(fx, audio, scenario, options.blockSize); // table/cache warmup
                    std::vector<double> timings;
                    double checksum = 0.0;
                    for (int repeat = 0; repeat < options.repeats; ++repeat)
                    {
                        fx.reset();
                        audio = input;
                        const auto start = std::chrono::steady_clock::now();
                        process(fx, audio, scenario, options.blockSize);
                        const auto end = std::chrono::steady_clock::now();
                        timings.push_back(std::chrono::duration<double, std::nano>(end - start).count()
                                          / static_cast<double>(frames));
                        checkFinite(audio);
                        checksum += std::accumulate(audio.left.begin(), audio.left.end(), 0.0)
                                  + std::accumulate(audio.right.begin(), audio.right.end(), 0.0);
                    }
                    std::sort(timings.begin(), timings.end());
                    const auto n = timings.size();
                    const double median = (timings[(n - 1) / 2] + timings[n / 2]) * 0.5;
                    std::cout << scenario.name << ',' << rate << ',' << options.blockSize
                              << ',' << frames << ',' << options.repeats << ',' << median
                              << ',' << timings.front() << ',' << median * rate / 1.0e7
                              << ',' << checksum << '\n';
                }
                else
                {
                    process(fx, audio, scenario, options.blockSize);
                    checkFinite(audio);
                    const auto header = referenceHeader(rate, frames, options.blockSize, index);
                    const auto path = options.directory
                        / (std::string(scenario.name) + "-" + std::to_string(rate) + ".efxref");
                    if (options.mode == Options::Mode::capture)
                    {
                        writeReference(path, audio, header);
                        if (compare(audio, readReference(path, header)).changedSamples != 0)
                            throw std::runtime_error("reference round trip changed samples");
                        std::cout << scenario.name << ',' << rate << ',' << options.blockSize
                                  << ',' << frames << ',' << path.string() << '\n';
                    }
                    else
                    {
                        const auto metrics = compare(audio, readReference(path, header));
                        const bool pass = metrics.peakError <= options.maximumError
                                       && metrics.rmsError <= options.maximumRmsError;
                        passed = passed && pass;
                        const auto relative = metrics.rmsReference > 0.0
                            ? db(metrics.rmsError / metrics.rmsReference)
                            : metrics.rmsError == 0.0
                                ? -std::numeric_limits<double>::infinity()
                                : std::numeric_limits<double>::infinity();
                        std::cout << scenario.name << ',' << rate << ',' << options.blockSize
                                  << ',' << frames << ',' << metrics.peakError << ',' << metrics.rmsError
                                  << ',' << db(metrics.rmsError) << ',' << relative
                                  << ',' << db(metrics.rmsReference) << ',' << metrics.changedSamples
                                  << ',' << (pass ? "true" : "false") << '\n';
                    }
                }
                std::cout.flush();
            }
        }
        return passed ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ElectryBenchmarkFx: " << error.what() << '\n';
        return 1;
    }
}
