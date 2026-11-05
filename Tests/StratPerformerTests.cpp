// The performer as a guitarist: chord shapes on the strings a player would
// use, strums that cross the strings in order and alternate, releases that
// stop a note unless the pedal is down, legato and slides on the ringing
// string, harmonics at their pitch, the palm, the arm and finger bends,
// and the same sound whatever the host's block size.
#include "DSP/StratPerformer.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <memory>
#include <vector>

namespace
{
using electry::StratEngine;
using electry::StratPerformer;
using electry::StratString;

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition)
    {
        ++failures;
        std::printf("FAIL: %s\n", what);
    }
}

constexpr double pi = 3.14159265358979323846;
constexpr double rate = 48000.0;
constexpr int openNote[6] = {40, 45, 50, 55, 59, 64};

std::unique_ptr<StratPerformer> makePerformer()
{
    auto p = std::make_unique<StratPerformer>();
    p->prepare(rate);
    return p;
}

std::vector<float> render(StratPerformer& p, double seconds, int block = 480)
{
    std::vector<float> out(static_cast<size_t>(seconds * rate));
    for (size_t done = 0; done < out.size();)
    {
        const int n = static_cast<int>(std::min<size_t>(static_cast<size_t>(block),
                                                        out.size() - done));
        p.render(out.data() + done, n);
        done += static_cast<size_t>(n);
    }
    return out;
}

double level(const std::vector<float>& x, double start, double length, double f)
{
    const int a = static_cast<int>(start * rate), n = static_cast<int>(length * rate);
    std::complex<double> sum = 0.0;
    for (int i = 0; i < n; ++i)
        sum += static_cast<double>(x[static_cast<size_t>(a + i)])
            * (0.5 - 0.5 * std::cos(2.0 * pi * (i + 0.5) / n))
            * std::polar(1.0, -2.0 * pi * f * (a + i) / rate);
    return 20.0 * std::log10(std::abs(sum) + 1e-30);
}

double energy(const std::vector<float>& x, double start, double length)
{
    double sum = 0.0;
    for (int i = static_cast<int>(start * rate); i < static_cast<int>((start + length) * rate); ++i)
        sum += static_cast<double>(x[static_cast<size_t>(i)]) * x[static_cast<size_t>(i)];
    return sum;
}

void testChordShapes()
{
    struct Shape
    {
        const char* name;
        int notes[6];
        int frets[6]; // -1 not played
    };
    const Shape shapes[] = {
        {"E", {40, 47, 52, 56, 59, 64}, {0, 2, 2, 1, 0, 0}},
        {"A", {45, 52, 57, 61, 64, 0}, {-1, 0, 2, 2, 2, 0}},
        {"G", {43, 47, 50, 55, 59, 67}, {3, 2, 0, 0, 0, 3}},
        {"D", {50, 57, 62, 66, 0, 0}, {-1, -1, 0, 2, 3, 2}},
        {"C", {48, 52, 55, 60, 64, 0}, {-1, 3, 2, 0, 1, 0}},
        {"F", {41, 48, 53, 57, 60, 65}, {1, 3, 3, 2, 1, 1}},
    };
    auto p = makePerformer();
    for (const Shape& shape : shapes)
    {
        for (int n : shape.notes)
            if (n > 0)
                p->noteOn(0, n, 100);
        render(*p, 0.1);
        bool same = true;
        for (int s = 0; s < 6; ++s)
            same = same
                && (shape.frets[s] < 0 ? p->stringNote(s) < 0
                                       : p->stringFret(s) == shape.frets[s]
                                             && p->stringNote(s) == openNote[s] + shape.frets[s]);
        if (!same)
            std::printf("  shape %s differs\n", shape.name);
        check(same, "open chords take the shapes a guitarist plays");
        for (int n : shape.notes)
            if (n > 0)
                p->noteOff(0, n);
        render(*p, 0.05);
    }
}

// First sample at which each string sounds.
std::array<int, 6> onsets(StratPerformer& p, int samples)
{
    std::array<int, 6> first {};
    first.fill(-1);
    float y = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        p.render(&y, 1);
        for (int s = 0; s < 6; ++s)
            if (first[static_cast<size_t>(s)] < 0 && p.engine().isSounding(s))
                first[static_cast<size_t>(s)] = i;
    }
    return first;
}

void testStrums()
{
    electry::StratControls controls;
    controls.strumSeconds = 0.010;
    const int pace = static_cast<int>(controls.strumSeconds * rate);

    // A stroke from rest goes down, low string first, at the strum pace.
    auto p = makePerformer();
    p->setControls(controls);
    for (int n : {40, 47, 52, 56, 59, 64})
        p->noteOn(0, n, 100);
    const auto down = onsets(*p, 6 * pace);
    bool ordered = true;
    for (int s = 0; s < 6; ++s)
        ordered = ordered && down[static_cast<size_t>(s)] == s * pace;
    check(ordered, "a down-stroke crosses the strings low to high at the strum pace");

    // One that follows quickly comes back up, high string first.
    auto q = makePerformer();
    q->setControls(controls);
    q->noteOn(0, 40, 100);
    render(*q, 0.1, 1);
    for (int n : {45, 52, 57, 61, 64})
        q->noteOn(0, n, 100);
    const auto up = onsets(*q, 6 * pace);
    ordered = true;
    for (int s = 1; s < 6; ++s)
        ordered = ordered && up[static_cast<size_t>(s)] == (5 - s) * pace;
    check(ordered, "a stroke that follows quickly goes up, high to low");

    // After a pause the hand starts down again.
    auto r = makePerformer();
    r->setControls(controls);
    r->noteOn(0, 64, 100);
    render(*r, 0.5, 1);
    for (int n : {40, 47, 52, 56, 59})
        r->noteOn(0, n, 100);
    const auto again = onsets(*r, 6 * pace);
    ordered = true;
    for (int s = 0; s < 5; ++s)
        ordered = ordered && again[static_cast<size_t>(s)] == s * pace;
    check(ordered, "after a pause the next stroke goes down");
}

void testReleaseAndPedal()
{
    const auto ring = [](bool pedal) {
        auto p = makePerformer();
        if (pedal)
            p->sustain(true);
        p->noteOn(0, 57, 100); // the A at the 12th fret, or the D at the 7th
        render(*p, 0.2);
        p->noteOff(0, 57);
        const auto y = render(*p, 0.6);
        return energy(y, 0.4, 0.2) / energy(y, 0.0, 0.05);
    };
    const double released = ring(false), held = ring(true);
    std::printf("after release %.1f dB, held by the pedal %.1f dB\n",
                10.0 * std::log10(released), 10.0 * std::log10(held));
    check(released < 1e-3 * held, "a released note stops unless the pedal holds it");

    auto p = makePerformer();
    p->sustain(true);
    p->noteOn(0, 57, 100);
    render(*p, 0.1);
    p->noteOff(0, 57);
    render(*p, 0.1);
    const int s = [&] {
        for (int k = 0; k < 6; ++k)
            if (p->stringNote(k) == 57)
                return k;
        return -1;
    }();
    check(s >= 0, "the pedal keeps a released note on its string");
    p->sustain(false);
    check(s >= 0 && p->stringNote(s) < 0, "lifting the pedal releases it");
}

void testLegatoAndSlide()
{
    // Hammer-on: the next note sounds on the same string without a pick.
    auto p = makePerformer();
    p->noteOn(0, 27, 1);
    p->noteOn(0, 69, 100);
    render(*p, 0.2);
    int s = -1;
    for (int k = 0; k < 6; ++k)
        if (p->stringNote(k) == 69)
            s = k;
    p->noteOn(0, 71, 100);
    render(*p, 0.01);
    check(s >= 0 && p->stringNote(s) == 71
              && p->stringFret(s) == 71 - openNote[s],
          "a hammer-on moves the ringing string's fret");

    // Slide: the fret travels, then rests on the target in tune.
    auto q = makePerformer();
    q->noteOn(0, 28, 1);
    q->noteOn(0, 62, 100);
    render(*q, 0.2);
    for (int k = 0; k < 6; ++k)
        if (q->stringNote(k) == 62)
            s = k;
    const int from = q->stringFret(s);
    q->noteOn(0, 67, 100);
    render(*q, 0.05);
    const int middle = q->stringFret(s);
    render(*q, 0.3);
    const double want = StratString::construction(s).openHz * std::pow(2.0, (67 - openNote[s]) / 12.0);
    check(middle > from && middle < 67 - openNote[s], "a slide passes the frets between");
    check(q->stringNote(s) == 67
              && std::abs(q->engine().fundamentalHz(s) / want - 1.0) < 1e-9,
          "a slide lands in tune");
}

void testHarmonics()
{
    // Natural harmonics on the low E (12th and 7th fret), the high E (12th)
    // and the G (7th), and an octave harmonic over a fretted note.
    for (int note : {52, 59, 76, 74, 85})
    {
        auto p = makePerformer();
        p->noteOn(0, 29, 1);
        p->noteOn(0, note, 110);
        const auto y = render(*p, 0.8);
        int s = -1;
        for (int k = 0; k < 6; ++k)
            if (p->stringNote(k) == note)
                s = k;
        if (s < 0)
        {
            check(false, "a harmonic finds a string");
            continue;
        }
        const double base = p->engine().fundamentalHz(s);
        const double tempered = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        const double k = std::round(tempered / base);
        const double b = StratString::construction(s).stiffness
            * std::pow(2.0, p->stringFret(s) / 6.0);
        const double partial = k * base * std::sqrt((1.0 + b * k * k) / (1.0 + b));
        // The touch lasts 40 ms; listen after it lifts.
        const double harmonic = level(y, 0.2, 0.5, partial);
        const double fundamental = level(y, 0.2, 0.5, base);
        check(std::abs(1200.0 * std::log2(partial / tempered)) < 20.0,
              "a harmonic is the note asked for");
        std::printf("harmonic %d on string %d: %.1f dB over the fundamental\n", note, s,
                    harmonic - fundamental);
        check(harmonic > fundamental + 25.0, "a harmonic leaves out the string's fundamental");
    }
}

void testPalm()
{
    // What the palm adds to a partial, dB/s between two windows, at a pressure.
    // Played softly: a hard note starts sharp and settles (StratString's
    // tension modulation), and its level at the partial's resting frequency
    // would read low early; at velocity 100 the open G's third partial then
    // seems to decay at 0.4 dB/s, against 3.4 at velocity 20.
    const auto addedDecay = [](int note, int partial, double pressure, double from, double to,
                               double window) {
        double decay[2] {};
        for (int palm = 0; palm < 2; ++palm)
        {
            auto p = makePerformer();
            electry::StratControls controls;
            controls.palmMute = pressure;
            p->setControls(controls);
            if (palm == 1)
                p->noteOn(0, 25, 1);
            p->noteOn(0, note, 20);
            const auto y = render(*p, to + window);
            int s = 0;
            for (int k = 0; k < 6; ++k)
                s = p->stringNote(k) == note ? k : s;
            const double b = StratString::construction(s).stiffness
                * std::pow(2.0, p->stringFret(s) / 6.0);
            const double f = partial * p->engine().fundamentalHz(s)
                * std::sqrt((1.0 + b * partial * partial) / (1.0 + b));
            decay[palm] = (level(y, from, window, f) - level(y, to, window, f)) / (to - from);
        }
        return decay[1] - decay[0];
    };
    // The recorded Strat's lightest rest damps every string alike.
    double lightest = 1e9, heaviest = 0.0;
    for (int s = 0; s < 6; ++s)
    {
        const double added = addedDecay(openNote[s], 3, 0.0, 0.2, 1.1, 0.1);
        std::printf("lightest palm on open string %d adds %.1f dB/s to partial 3\n", s, added);
        lightest = added < lightest ? added : lightest;
        heaviest = added > heaviest ? added : heaviest;
    }
    check(lightest > 0.0 && heaviest < 1.1 * lightest, "a light palm damps every string alike");
    const double light = addedDecay(40, 1, 0.0, 0.2, 1.1, 0.1),
                 middle = addedDecay(40, 1, 0.5, 0.2, 1.1, 0.1),
                 heavy = addedDecay(40, 1, 1.0, 0.2, 1.1, 0.1);
    std::printf("palm on the low E's fundamental: %.1f, %.1f, %.1f dB/s at 0, 0.5, 1\n", light,
                middle, heavy);
    check(light < middle && middle < heavy, "a harder palm damps more");
    // Its heaviest mutes took the high E's fundamental at fret 5 down at 405
    // and 490 dB/s more than an open-handed note. (This replaces a check that
    // the default palm took 10 dB off the low E by 0.4 s, set for the voiced
    // palm the fit found 3 to 12 times too heavy.)
    const double pressed = addedDecay(69, 1, 1.0, 0.03, 0.08, 0.02);
    std::printf("full palm on the high E's fret 5: %.0f dB/s\n", pressed);
    check(pressed > 365.0 && pressed < 540.0, "a full palm damps as the recorded heaviest mutes");
}

// Decay of the octave band around `hz`, dB/s, over the first 80 ms after
// the band's loudest 10 ms frame (Hann frames 5 ms apart, the band's DFT bins
// summed), until it first falls 30 dB below that.
double bandDecay(const std::vector<float>& x, double hz)
{
    const double length = 0.010, hop = 0.005;
    std::vector<double> frames;
    for (double start = 0.0; start + length <= x.size() / rate; start += hop)
    {
        double sum = 0.0;
        for (double f = std::ceil(hz / std::sqrt(2.0) * length) / length; f < hz * std::sqrt(2.0);
             f += 1.0 / length)
            sum += std::pow(10.0, level(x, start, length, f) / 10.0);
        frames.push_back(10.0 * std::log10(sum + 1e-30));
    }
    size_t peak = 0;
    for (size_t i = 0; i < frames.size(); ++i)
        peak = frames[i] > frames[peak] ? i : peak;
    double st = 0.0, sy = 0.0, stt = 0.0, sty = 0.0;
    int n = 0;
    for (size_t i = peak; i < frames.size() && i <= peak + 16 && frames[i] > frames[peak] - 30.0;
         ++i, ++n)
    {
        const double t = hop * static_cast<double>(i - peak);
        st += t;
        sy += frames[i];
        stt += t * t;
        sty += t * frames[i];
    }
    return n < 3 ? 0.0 : -(n * sty - st * sy) / (n * stt - st * st);
}

void testDeadNotes()
{
    // The recorded Strat's dead notes at frets 3, 8 and 14 lost their 2 kHz
    // octave at 830 to 2360 dB/s and their 4 kHz octave at 980 to 2540.
    // The performer picks the string, so a note is played once.
    double slowest2 = 1e9, slowest4 = 1e9;
    for (int note = openNote[0] + 3; note <= openNote[5] + 14; note += 2)
    {
        auto p = makePerformer();
        p->noteOn(0, 26, 1);
        p->noteOn(0, note, 100);
        const auto y = render(*p, 0.15);
        int s = 0;
        for (int k = 0; k < 6; ++k)
            s = p->stringNote(k) == note ? k : s;
        const double two = bandDecay(y, 2000.0), four = bandDecay(y, 4000.0);
        std::printf("dead note %d, string %d fret %d: 2 kHz %.0f dB/s, 4 kHz %.0f dB/s\n", note,
                    s, note - openNote[s], two, four);
        slowest2 = two < slowest2 ? two : slowest2;
        slowest4 = four < slowest4 ? four : slowest4;
    }
    check(slowest2 > 0.8 * 830.0 && slowest4 > 0.8 * 980.0,
          "a dead note's top dies as fast as the recorded dead notes'");
}

void testArmAndBends()
{
    auto p = makePerformer();
    p->noteOn(0, 64, 100);
    p->noteOn(1, 69, 100); // B string, 10th fret, on a member channel
    render(*p, 0.05);
    p->pitchBend(0, -0.5);
    p->pitchBend(1, 2.0 / 48.0);
    render(*p, 0.01);
    bool tuned = true;
    for (int s = 0; s < 6; ++s)
    {
        const double fret = p->stringNote(s) >= 0 ? p->stringFret(s) : 0.0;
        const double want = StratString::construction(s).openHz * std::pow(2.0, fret / 12.0)
            * std::sqrt(p->engine().bridge().tensionRatio(s));
        tuned = tuned && std::abs(p->engine().fundamentalHz(s) / want - 1.0) < 1e-9;
    }
    check(tuned, "every string follows the bridge");
    int bent = -1;
    for (int s = 0; s < 6; ++s)
        if (p->stringNote(s) == 69)
            bent = s;
    electry::StratBridge reference;
    reference.setArm(-0.5);
    check(bent >= 0
              && std::abs(p->engine().bridge().tensionRatio(bent)
                          / (std::pow(2.0, 4.0 / 12.0) * reference.tensionRatio(bent)) - 1.0)
                  < 1e-9,
          "a member channel's bend takes its string a whole tone up");
}

std::vector<float> performance(int block)
{
    auto p = makePerformer();
    std::vector<float> out(static_cast<size_t>(1.2 * rate));
    struct Step
    {
        int at;
        void (*act)(StratPerformer&);
    };
    const Step steps[] = {
        {0, [](StratPerformer& q) { for (int n : {40, 47, 52, 56, 59, 64}) q.noteOn(0, n, 100); }},
        {9000, [](StratPerformer& q) { q.modWheel(0.8); q.pitchBend(0, -0.3); }},
        {14001, [](StratPerformer& q) { for (int n : {40, 47, 52, 56, 59, 64}) q.noteOff(0, n); }},
        {14001, [](StratPerformer& q) { q.noteOn(0, 28, 1); q.noteOn(0, 62, 90); }},
        {20000, [](StratPerformer& q) { q.noteOn(0, 66, 90); }},
        {33333, [](StratPerformer& q) { q.noteOn(0, 29, 1); q.noteOn(0, 64, 120); }},
        {45000, [](StratPerformer& q) { q.allNotesOff(); }},
    };
    size_t done = 0;
    for (const Step& step : steps)
    {
        while (done < static_cast<size_t>(step.at))
        {
            const int n = static_cast<int>(std::min<size_t>(static_cast<size_t>(block),
                                                            static_cast<size_t>(step.at) - done));
            p->render(out.data() + done, n);
            done += static_cast<size_t>(n);
        }
        step.act(*p);
    }
    while (done < out.size())
    {
        const int n = static_cast<int>(std::min<size_t>(static_cast<size_t>(block), out.size() - done));
        p->render(out.data() + done, n);
        done += static_cast<size_t>(n);
    }
    return out;
}

void testBlockPartition()
{
    const auto reference = performance(1);
    bool same = true;
    for (int block : {5, 64, 480, 4096})
        same = same && performance(block) == reference;
    check(same, "any block size gives the same bits");
    check(performance(480) == performance(480), "the same performance gives the same bits");
}

} // namespace

int main()
{
    testChordShapes();
    testStrums();
    testReleaseAndPedal();
    testLegatoAndSlide();
    testHarmonics();
    testPalm();
    testDeadNotes();
    testArmAndBends();
    testBlockPartition();
    if (failures == 0)
        std::printf("StratPerformer: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
