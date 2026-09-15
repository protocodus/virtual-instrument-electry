// Offline showroom renderer: shipping engine(s), shipping High-quality FX,
// sample-accurate musical score, native 96 kHz floating-point intermediate.
// Double follows PluginProcessor::renderEngines and its per-channel acoustic
// return exactly. No external instrument, reverb, cabinet, EQ or widening.
// Usage: ElectryRenderShowcase performance.score intermediate.wav
// Tools/Showcase/build.py supplies the original scores, MIDI, 24-bit delivery,
// constant-gain loudness alignment and technical audio QA.
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
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int rate = 96000;
constexpr int block = 256;
using electry::ElectryEngine;
using electry::EngineParameters;
using electry::FxParameters;
struct Event {
    std::int64_t frame;
    std::string op;
    std::vector<double> values;
};
struct Score {
    EngineParameters engine;
    FxParameters fx;
    int mode = 1;
    std::int64_t frames = 0;
    std::vector<Event> events;
};

Score readScore(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot read score: " + path.string());
    Score score;
    const std::map<std::string, float EngineParameters::*> engineFields {
        {"pickupType", &EngineParameters::pickupType},
        {"toneKnob", &EngineParameters::toneKnob},
        {"bodyResonance", &EngineParameters::bodyResonance},
        {"stringAge", &EngineParameters::stringAge},
        {"pickPosition", &EngineParameters::pickPosition},
        {"pickHardness", &EngineParameters::pickHardness},
        {"pickNoise", &EngineParameters::pickNoise},
        {"fingerNoise", &EngineParameters::fingerNoise},
        {"releaseNoise", &EngineParameters::releaseNoise},
        {"muteDamping", &EngineParameters::muteDamping},
        {"bendTimeSeconds", &EngineParameters::bendTimeSeconds},
        {"velocityAmount", &EngineParameters::velocityAmount},
        {"artifactAmount", &EngineParameters::artifactAmount},
        {"sympatheticAmount", &EngineParameters::sympatheticAmount},
        {"palmMute", &EngineParameters::palmMute},
        {"strumSpreadSeconds", &EngineParameters::strumSpreadSeconds},
        {"tremoloRateHz", &EngineParameters::tremoloRateHz},
        {"resonanceDepth", &EngineParameters::resonanceDepth},
    };
    const std::map<std::string, float FxParameters::*> fxFields {
        {"distortion", &FxParameters::distortion}, {"amp", &FxParameters::amp},
        {"compressor", &FxParameters::compressor}, {"delay", &FxParameters::delay},
        {"room", &FxParameters::room},
    };
    std::string line;
    int lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream row(line);
        std::string first;
        row >> first;
        if (first == "E" || first == "F") {
            std::string key;
            double number;
            if (!(row >> key >> number) || !std::isfinite(number))
                throw std::runtime_error("Invalid parameter on line " + std::to_string(lineNumber));
            const auto value = static_cast<float>(number);
            if (first == "E") {
                if (key == "guitarBuild") electry::applyGuitarBuild(score.engine, value);
                else if (key == "pickupSelector" && number >= 0 && number <= 2)
                    score.engine.pickupSelector = static_cast<electry::PickupSelector>(static_cast<int>(number));
                else if (key == "mode" && number >= 0 && number <= 2)
                    score.mode = static_cast<int>(number);
                else if (key == "outputDb" && number >= -24 && number <= 6)
                    score.engine.outputGain = std::pow(10.0f, value / 20.0f);
                else if (auto field = engineFields.find(key); field != engineFields.end())
                    score.engine.*(field->second) = value;
                else throw std::runtime_error("Unknown/out-of-range engine control: " + key);
            } else {
                if (key == "ampModel" && number >= 0 && number <= 2)
                    score.fx.ampModel = static_cast<electry::AmpModel>(static_cast<int>(number));
                else if (auto field = fxFields.find(key); field != fxFields.end() && number >= 0 && number <= 1)
                    score.fx.*(field->second) = value;
                else throw std::runtime_error("Unknown/out-of-range FX control: " + key);
            }
        } else if (first == "END") {
            double seconds;
            if (!(row >> seconds) || !std::isfinite(seconds) || seconds < .1 || seconds > 600)
                throw std::runtime_error("Invalid score duration");
            score.frames = std::llround(seconds * rate);
        } else {
            std::size_t consumed = 0;
            const auto seconds = std::stod(first, &consumed);
            if (consumed != first.size() || !std::isfinite(seconds) || seconds < 0 || seconds > 600)
                throw std::runtime_error("Invalid event time");
            Event event { std::llround(seconds * rate), {}, {} };
            row >> event.op;
            double value;
            while (row >> value) {
                if (!std::isfinite(value)) throw std::runtime_error("Non-finite event value");
                event.values.push_back(value);
            }
            if (!row.eof()) throw std::runtime_error("Malformed event on line " + std::to_string(lineNumber));
            score.events.push_back(std::move(event));
        }
    }
    if (score.frames == 0 || score.events.empty()) throw std::runtime_error("Empty score");
    std::int64_t previous = -1;
    for (const auto& event : score.events) {
        if (event.frame < previous || event.frame >= score.frames)
            throw std::runtime_error("Events out of order or beyond score duration");
        previous = event.frame;
    }
    score.engine.outputMode = score.mode == 1 ? electry::OutputMode::Stereo : electry::OutputMode::Mono;
    score.fx.oversampling = electry::FxOversampling::High;
    return score;
}

void dispatch(ElectryEngine& engine, const Event& event) {
    const auto& v = event.values;
    if (event.op == "N") {
        if (v.size() < 4 || v.size() > 11 || v[0] < 0 || v[0] > 6
            || v[1] < 0 || v[1] > 2 || v[2] <= 0 || v[2] > 1)
            throw std::runtime_error("Invalid note event");
        // Re-sending Alternate resets its next stroke to Down. Only change
        // the wrist when the musical score asks for a different pick style.
        if (static_cast<int>(engine.getCurrentPickStyle()) != static_cast<int>(v[1]))
            engine.noteOn(ElectryEngine::firstKeyswitchNote + static_cast<int>(v[1]), 1.0f);
        if (static_cast<int>(engine.getCurrentPlayStyle()) != static_cast<int>(v[0]))
            engine.noteOn(ElectryEngine::firstPlayStyleKeyswitchNote + static_cast<int>(v[0]), 1.0f);
        std::vector<ElectryEngine::NoteOnEvent> notes;
        for (std::size_t i = 3; i < v.size(); ++i) {
            if (v[i] < 28 || v[i] > 86 || std::floor(v[i]) != v[i])
                throw std::runtime_error("Invalid sounding MIDI pitch");
            notes.push_back({static_cast<int>(v[i]), static_cast<float>(v[2])});
        }
        engine.noteOnChord(notes);
        return;
    }
    if (v.size() != 1) throw std::runtime_error("Invalid controller event");
    const auto value = static_cast<float>(v[0]);
    if (event.op == "O") engine.noteOff(static_cast<int>(value));
    else if (event.op == "bend" && value >= -1 && value <= 1) engine.setPitchBend(value);
    else if (event.op == "vibrato" && value >= 0 && value <= 1) engine.setVibrato(value);
    else if (event.op == "palm" && value >= 0 && value <= 1) engine.setPalmMutePressure(value);
    else if (event.op == "resonance" && value >= 0 && value <= 1) engine.setResonance(value);
    else if (event.op == "sustain") engine.setSustainPedal(value >= .5f);
    else if (event.op == "tremolo" && value >= 0 && value <= 1) {
        if (value > 0) engine.beginTremoloPicking(value);
        else engine.endTremoloPicking();
    } else if (event.op == "solo" && value >= 0 && value <= 255)
        engine.setSoloStringMask(static_cast<std::uint8_t>(value));
    else throw std::runtime_error("Unknown/out-of-range event: " + event.op);
}

void u32(std::ostream& out, std::uint32_t value, int bytes=4) {
    for (int i=0; i<bytes; ++i) out.put(static_cast<char>((value >> (8*i)) & 255));
}
void writeFloatWav(const std::filesystem::path& path, const std::vector<float>& left,
                   const std::vector<float>& right) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write WAV: " + path.string());
    const auto bytes = static_cast<std::uint32_t>(left.size()*8);
    out.write("RIFF",4); u32(out,48+bytes); out.write("WAVEfmt ",8);
    u32(out,16); u32(out,3,2); u32(out,2,2); u32(out,rate);
    u32(out,rate*8); u32(out,8,2); u32(out,32,2);
    out.write("fact",4); u32(out,4); u32(out,static_cast<std::uint32_t>(left.size()));
    out.write("data",4); u32(out,bytes);
    for (std::size_t i=0; i<left.size(); ++i) {
        u32(out,std::bit_cast<std::uint32_t>(left[i]));
        u32(out,std::bit_cast<std::uint32_t>(right[i]));
    }
    if (!out) throw std::runtime_error("Failed while writing WAV");
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: ElectryRenderShowcase performance.score intermediate.wav\n";
            return 2;
        }
        const auto score = readScore(argv[1]);
        auto engine = std::make_unique<ElectryEngine>();
        auto second = score.mode == 2 ? std::make_unique<ElectryEngine>() : nullptr;
        auto fx = std::make_unique<electry::ElectryFx>();
        for (auto* instance : {engine.get(), second.get()}) {
            if (!instance) continue;
            instance->setVariationSeed(instance == second.get() ? 0x9e3779b9u : 0u);
            instance->prepare(rate, block);
            instance->setParameters(score.engine);
            instance->reset();
            instance->setAcousticReturnLevel(std::min(1.0f, score.fx.amp+.6f*score.fx.distortion));
        }
        fx->prepare(rate);
        fx->setParameters(score.fx);
        fx->reset();
        auto process = [&](float* left, float* right, int count) {
            engine->process(left, right, count);
            if (second) second->process(right, right, count);
            fx->process(left, right, count);
            if (second) {
                engine->pushAcousticReturn(left,nullptr,count);
                second->pushAcousticReturn(right,nullptr,count);
            } else engine->pushAcousticReturn(left,right,count);
        };
        std::array<float,block> warmL{},warmR{};
        for (int i=0; i<rate/4;) {
            const auto n=std::min({block,engine->getAcousticReturnDelaySamples(),rate/4-i});
            process(warmL.data(),warmR.data(),n);
            i+=n;
        }
        std::vector<float> left(static_cast<std::size_t>(score.frames));
        std::vector<float> right(left.size());
        std::int64_t frame=0;
        std::size_t eventIndex=0;
        while (frame < score.frames) {
            while (eventIndex < score.events.size() && score.events[eventIndex].frame == frame) {
                dispatch(*engine,score.events[eventIndex]);
                if (second) dispatch(*second,score.events[eventIndex]);
                ++eventIndex;
            }
            const auto boundary=eventIndex < score.events.size() ? score.events[eventIndex].frame : score.frames;
            const auto count=static_cast<int>(std::min<std::int64_t>(
                std::min(block,engine->getAcousticReturnDelaySamples()),boundary-frame));
            if (count <= 0) throw std::runtime_error("Score scheduler stalled");
            process(left.data()+frame,right.data()+frame,count);
            frame+=count;
        }
        double peak=0;
        for (std::size_t i=0; i<left.size(); ++i) {
            if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
                throw std::runtime_error("Non-finite rendered audio");
            peak=std::max({peak,std::abs(static_cast<double>(left[i])),std::abs(static_cast<double>(right[i]))});
        }
        if (peak < 1e-7) throw std::runtime_error("Silent performance");
        int held=0;
        for (const auto* instance : {engine.get(), second.get()}) {
            if (!instance) continue;
            std::array<electry::StringVisualState, ElectryEngine::stringCount> states;
            instance->getStringVisualState(states);
            for (const auto& state : states) if(state.sounding && !state.releasing) ++held;
        }
        if (held) throw std::runtime_error("Performance leaves held strings at end: " + std::to_string(held));
        writeFloatWav(argv[2],left,right);
        std::cout << std::setprecision(9) << "{\"sample_rate\":" << rate
                  << ",\"channels\":2,\"intermediate_bits\":32,\"seconds\":" << left.size()/double(rate)
                  << ",\"fx_quality\":\"High\",\"oversampling_factor\":" << fx->oversamplingFactor()
                  << ",\"output_mode\":\"" << (score.mode == 2 ? "Double" : score.mode == 1 ? "Stereo" : "Mono")
                  << "\",\"raw_peak_dbfs\":" << 20*std::log10(peak)
                  << ",\"held_strings_at_end\":" << held << "}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Showcase render failed: " << error.what() << '\n';
        return 1;
    }
}
