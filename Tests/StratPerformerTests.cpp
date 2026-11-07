// The performer as a guitarist: chord shapes on the strings a player would
// use, strums that cross the strings in order and alternate, releases that
// stop a note unless the pedal is down and quiet the free strings with it,
// legato and slides on the ringing
// string, harmonics at their pitch, pinch harmonics, the palm, the arm and
// finger bends, the stroke and damping keyswitches, and the same sound
// whatever the host's block size.
#include "DSP/StratPerformer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdio>
#include <initializer_list>
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

// What a fret plays over the open string on the intonated guitar
// (StratEngine::intonation): the open speaking length over the length from
// the fret to the set-back saddle. The bridge's tension ratio comes on top.
double fretRatio(int s, double fret)
{
    const auto [open, setback] = StratEngine::intonation(s);
    return fret > 0.0 ? open / (StratString::scaleMetres * std::pow(2.0, -fret / 12.0) + setback)
                      : 1.0;
}

// The open string where the player tuned it (StratEngine::restPitch).
double tunedHz(int s)
{
    return StratString::construction(s).openHz * StratEngine::restPitch(s);
}

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

// First sample at which each string is struck. (Sounding will not do: a
// stroke sets the others moving through the neck at once.)
std::array<int, 6> onsets(StratPerformer& p, int samples)
{
    std::array<int, 6> first {};
    first.fill(-1);
    float y = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        p.render(&y, 1);
        for (int s = 0; s < 6; ++s)
            if (first[static_cast<size_t>(s)] < 0 && p.engine().isDriven(s))
                first[static_cast<size_t>(s)] = i;
    }
    return first;
}

// A stroke after the first from rest: the strings in `order` sound one after
// another, each gap within the hand's scatter of the pace (StratPerformer.cpp:
// 2.5 deviations of 0.29 either way).
bool paced(const std::array<int, 6>& onset, std::initializer_list<int> order, int pace)
{
    int previous = -1;
    for (int s : order)
    {
        const int t = onset[static_cast<size_t>(s)];
        if (previous >= 0)
        {
            const double gap = t - previous;
            if (gap < 0.48 * pace || gap > 2.08 * pace)
                return false;
        }
        else if (t != 0)
            return false;
        previous = t;
    }
    return true;
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
    const auto up = onsets(*q, 12 * pace);
    check(paced(up, {5, 4, 3, 2, 1}, pace), "a stroke that follows quickly goes up, high to low");
    bool even = true;
    for (int s = 1; s < 5; ++s)
        even = even && up[static_cast<size_t>(s)] - up[static_cast<size_t>(s + 1)] == pace;
    check(!even, "the hand crosses the strings of a later stroke unevenly");

    // After a pause the hand starts down again.
    auto r = makePerformer();
    r->setControls(controls);
    r->noteOn(0, 64, 100);
    render(*r, 0.5, 1);
    for (int n : {40, 47, 52, 56, 59})
        r->noteOn(0, n, 100);
    const auto again = onsets(*r, 12 * pace);
    check(paced(again, {0, 1, 2, 3, 4}, pace), "after a pause the next stroke goes down");
}

// The stroke keyswitches: G1 latches downstrokes, G#1 upstrokes and A1 the
// alternating hand of testStrums.
void testStrokeKeys()
{
    electry::StratControls controls;
    controls.strumSeconds = 0.010;
    const int pace = static_cast<int>(controls.strumSeconds * rate);

    auto p = makePerformer();
    p->setControls(controls);
    p->noteOn(0, StratPerformer::firstStrokeKey + 1, 1);
    check(p->stroke() == StratPerformer::Stroke::up, "G#1 latches upstrokes");
    for (int n : {40, 47, 52, 56, 59, 64})
        p->noteOn(0, n, 100);
    const auto up = onsets(*p, 6 * pace);
    bool ordered = true;
    for (int s = 0; s < 6; ++s)
        ordered = ordered && up[static_cast<size_t>(s)] == (5 - s) * pace;
    check(ordered, "an upstroke from rest crosses the strings high to low");

    auto q = makePerformer();
    q->setControls(controls);
    q->noteOn(0, StratPerformer::firstStrokeKey, 1);
    q->noteOn(0, 40, 100);
    render(*q, 0.1, 1);
    for (int n : {45, 52, 57, 61, 64})
        q->noteOn(0, n, 100);
    const auto down = onsets(*q, 12 * pace);
    check(paced(down, {1, 2, 3, 4, 5}, pace), "with downstrokes latched a quick second stroke goes down too");

    // Back to alternating after upstrokes: from rest down, then a quick one up.
    auto r = makePerformer();
    r->setControls(controls);
    r->noteOn(0, StratPerformer::firstStrokeKey + 1, 1);
    r->noteOn(0, StratPerformer::firstStrokeKey + 2, 1);
    check(r->stroke() == StratPerformer::Stroke::alternate, "A1 latches alternate strokes");
    r->noteOn(0, 40, 100);
    render(*r, 0.1, 1);
    for (int n : {45, 52, 57, 61, 64})
        r->noteOn(0, n, 100);
    const auto back = onsets(*r, 12 * pace);
    check(paced(back, {5, 4, 3, 2, 1}, pace), "alternating again, a quick stroke after one from rest goes up");
}

// B1 latches the fingers: plucked further from the bridge and let go more
// slowly, a note keeps its fundamental and loses its top against the pick's.
void testFingerKey()
{
    const auto tilt = [](bool fingers) {
        auto p = makePerformer();
        if (fingers)
            p->noteOn(0, StratPerformer::fingerKey, 1);
        p->noteOn(0, 45, 100);
        const auto x = render(*p, 0.3);
        return level(x, 0.02, 0.2, 110.0 * 12.0) - level(x, 0.02, 0.2, 110.0);
    };
    check(tilt(true) < tilt(false) - 3.0, "a finger-plucked note is darker than a picked one");

    auto p = makePerformer();
    p->noteOn(0, StratPerformer::fingerKey, 1);
    check(p->isFingerPlucking(), "B1 latches finger plucking");
    p->noteOn(0, StratPerformer::firstStrokeKey, 1);
    check(!p->isFingerPlucking(), "G1 takes the pick back");
}

// A#1 held lays the fretting hand across the strings: a chord the pedal holds
// stops, a stroke under it is muted, and the strings ring again once it lifts.
void testDampAll()
{
    const auto chord = [](StratPerformer& p) {
        for (int n : {40, 47, 52, 56, 59, 64})
            p.noteOn(0, n, 100);
    };
    auto free = makePerformer();
    free->sustain(true);
    chord(*free);
    render(*free, 0.3);
    const auto ringing = render(*free, 0.5);

    auto p = makePerformer();
    p->sustain(true);
    chord(*p);
    render(*p, 0.3);
    p->noteOn(0, StratPerformer::dampKey, 1);
    check(p->isDamping(), "A#1 lays the hand on the strings");
    const auto choked = render(*p, 0.5);
    const double stopped = energy(choked, 0.3, 0.2) / energy(ringing, 0.3, 0.2);
    std::printf("damp all: the held chord %.1f dB under its free ring\n",
                10.0 * std::log10(stopped));
    check(stopped < 1e-4, "damping stops a chord the pedal holds");
    bool cleared = true;
    for (int s = 0; s < 6; ++s)
        cleared = cleared && p->stringNote(s) < 0;
    check(cleared, "damping lets go of every string");

    const auto decay = [](const std::vector<float>& y) {
        return energy(y, 0.15, 0.1) / energy(y, 0.0, 0.05);
    };
    p->sustain(false);
    p->noteOn(0, 64, 100);
    const auto muted = render(*p, 0.3);
    p->noteOff(0, 64);
    p->noteOff(0, StratPerformer::dampKey);
    check(!p->isDamping(), "releasing A#1 lifts the hand");
    render(*p, 0.5);
    p->noteOn(0, 64, 100);
    const auto open = render(*p, 0.3);
    std::printf("damp all: a stroke under the hand keeps %.1f dB, after it lifts %.1f dB\n",
                10.0 * std::log10(decay(muted)), 10.0 * std::log10(decay(open)));
    check(decay(muted) < 1e-3 * decay(open), "a stroke under the damping hand is muted");
    check(decay(open) > 0.1, "the strings ring once the hand lifts");
}

void testReleaseAndPedal()
{
    const auto ring = [](bool pedal) {
        auto p = makePerformer();
        if (pedal)
            p->sustain(true);
        // The B-flat on the G string's 3rd fret.
        p->noteOn(0, 58, 100);
        render(*p, 0.2);
        p->noteOff(0, 58);
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

// The A on the G string's 2nd fret drives the open A at its octave through
// the neck (StratString's neck); a free string that hears the B-flat rings at
// its own pitch. The hand that lets the A go stops both, and lifts at the
// next stroke.
void testRestingHand()
{
    auto p = makePerformer();
    p->noteOn(0, 57, 100);
    render(*p, 0.2);
    p->noteOff(0, 57);
    const auto y = render(*p, 0.6);
    const double left = energy(y, 0.4, 0.2) / energy(y, 0.0, 0.05);
    std::printf("after an A, the open A %.1f dB\n", 10.0 * std::log10(left));
    check(left < 1e-5, "the hand that lets a note go stops the open string it drives");

    const auto sympathy = [](bool afterRelease) {
        auto q = makePerformer();
        if (afterRelease)
        {
            q->noteOn(0, 57, 100);
            render(*q, 0.2);
            q->noteOff(0, 57);
            render(*q, 0.3);
        }
        q->noteOn(0, 58, 100);
        const auto z = render(*q, 1.0);
        return level(z, 0.5, 0.4, 110.0) - level(z, 0.5, 0.4, 233.08);
    };
    const double fresh = sympathy(false), after = sympathy(true);
    std::printf("open A under a B-flat: fresh %.1f dB, after a release %.1f dB\n", fresh, after);
    check(std::abs(after - fresh) < 3.0, "the free strings ring again from the next stroke");
}

void testReleaseLift()
{
    // A released fretted note stops fast, even over a node of the open
    // string the finger then lies on (StratPerformer.cpp, liftSeconds): the
    // high E at the 12th and 7th frets, the low E at the 3rd.
    // The finger lying beside the fret alone let them fall 14, 16 and 11 dB.
    struct Case
    {
        int note, string;
        double least;
    };
    for (const Case c : {Case {76, 5, 20.0}, Case {71, 5, 20.0}, Case {43, 0, 13.0}})
    {
        auto p = makePerformer();
        p->noteOn(0, c.note, 90);
        auto held = render(*p, 0.4);
        check(p->stringNote(c.string) == c.note, "the note is on the string the test means");
        p->noteOff(0, c.note);
        auto after = render(*p, 0.2);
        held.insert(held.end(), after.begin(), after.end());
        // The steepest 20 ms fall of its 4 ms energy around the release.
        double fall = 0.0;
        for (double t = 0.39; t < 0.46; t += 0.001)
            fall = std::max(fall, 10.0 * std::log10(energy(held, t, 0.004) / energy(held, t + 0.02, 0.004)));
        std::printf("release of note %d: falls %.1f dB in 20 ms\n", c.note, fall);
        check(fall > c.least, "a released fretted note stops");
    }

    // The same note again within the lift sounds on its fret and rings.
    auto p = makePerformer();
    p->noteOn(0, 76, 90);
    render(*p, 0.3);
    p->noteOff(0, 76);
    render(*p, 0.005);
    p->noteOn(0, 76, 90);
    const auto again = render(*p, 0.6);
    const double want = tunedHz(5) * fretRatio(5, 12.0)
        * std::sqrt(p->engine().bridge().tensionRatio(5));
    check(std::abs(p->engine().fundamentalHz(5) / want - 1.0) < 1e-9
              && level(again, 0.45, 0.1, 659.26) > level(again, 0.05, 0.1, 659.26) - 12.0,
          "a note struck again as the finger lifts rings on its fret");
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

    // Slide: the finger travels, then rests on the target in tune.
    int middle = -1;
    auto q = makePerformer();
    q->noteOn(0, 28, 1);
    q->noteOn(0, 62, 100);
    render(*q, 0.2);
    for (int k = 0; k < 6; ++k)
        if (q->stringNote(k) == 62)
            s = k;
    const int from = q->stringFret(s);
    q->noteOn(0, 67, 100);
    // Frets stop the string all the way: every control period of the slide
    // it sounds a fret, one after another, and every fret between.
    // fretRatio turned round: the fret a pitch is stopped at.
    const auto fretFrom = [&](double hz) {
        const double open = tunedHz(s) * std::sqrt(q->engine().bridge().tensionRatio(s));
        const auto [length, setback] = StratEngine::intonation(s);
        return -12.0 * std::log2((length * open / hz - setback) / StratString::scaleMetres);
    };
    bool fretted = true, inOrder = true;
    int visited = 0;
    double last = fretFrom(q->engine().fundamentalHz(s));
    for (int i = 0; i < 400; ++i)
    {
        render(*q, 0.0005);
        const double at = fretFrom(q->engine().fundamentalHz(s));
        fretted = fretted && std::abs(at - std::floor(at + 0.5)) < 1e-6;
        if (std::floor(at + 0.5) != std::floor(last + 0.5))
        {
            inOrder = inOrder && std::floor(at + 0.5) == std::floor(last + 0.5) + 1.0;
            ++visited;
        }
        last = at;
        if (i == 99)
            middle = q->stringFret(s);
    }
    check(fretted && inOrder && visited == 67 - openNote[s] - from,
          "a slide steps through every fret between");
    // The fretting finger stretches the string (StratEngine::fretPressMetres),
    // and the bridge's tension ratio carries it.
    const double want = tunedHz(s) * fretRatio(s, 67 - openNote[s])
        * std::sqrt(q->engine().bridge().tensionRatio(s));
    check(middle > from && middle < 67 - openNote[s], "a slide's finger passes the frets between");
    check(q->stringNote(s) == 67
              && std::abs(q->engine().fundamentalHz(s) / want - 1.0) < 1e-9,
          "a slide lands in tune");
}

void testHarmonics()
{
    // Natural harmonics on the low E (12th and 7th fret), the high E (12th)
    // and the G (7th), an octave harmonic over a fretted note, and on the
    // strings a tab names: the A's 5th fret (the 4th partial; untabbed, the
    // D's 7th takes it) and the D's 4th (the 5th partial).
    const int tabbed[][2] = {{52, -1}, {59, -1}, {76, -1}, {74, -1}, {85, -1},
                             {69, 1}, {78, 2}};
    for (const auto& t : tabbed)
    {
        const int note = t[0];
        auto p = makePerformer();
        p->noteOn(0, 29, 1);
        p->noteOn(0, note, 110, t[1]);
        const auto y = render(*p, 1.0);
        int s = -1;
        for (int k = 0; k < 6; ++k)
            if (p->stringNote(k) == note)
                s = k;
        if (s < 0 || (t[1] >= 0 && s != t[1]))
        {
            check(false, "a harmonic finds a string, the one a tab names");
            continue;
        }
        const double base = p->engine().fundamentalHz(s);
        const double tempered = 440.0 * std::pow(2.0, (note - 69) / 12.0);
        const double k = std::round(tempered / base);
        const double b = StratString::construction(s).stiffness
            * std::pow(2.0, p->stringFret(s) / 6.0);
        const double partial = k * base * std::sqrt((1.0 + b * k * k) / (1.0 + b));
        // The touch lasts 0.2 s; listen after it lifts.
        const double harmonic = level(y, 0.3, 0.6, partial);
        const double fundamental = level(y, 0.3, 0.6, base);
        check(std::abs(1200.0 * std::log2(partial / tempered)) < 20.0,
              "a harmonic is the note asked for");
        std::printf("harmonic %d on string %d: %.1f dB over the fundamental\n", note, s,
                    harmonic - fundamental);
        check(harmonic > fundamental + 25.0, "a harmonic leaves out the string's fundamental");
    }
}

// A pinch at the ordinary pick position, 130 mm from the bridge: its node a
// fifth of the string from the bridge is within reach, so the 5th partial
// comes out of the fundamental, open or at the 12th fret (2/5 of half the
// string), where an ordinary stroke from there leaves it far under. The low
// E's own fundamental lasts longest under the thumb.
void testPinch()
{
    // The thumb picks the node: whichever partial it keeps, from the 3rd to
    // the 12th, must come out of the stroke. The pick strikes the low E and A
    // nearest the bridge (StratPerformer's pickLine), where the node within
    // reach is the 7th partial's, which their wraps drain: there it comes out
    // of the stroke but stays under the fundamental.
    for (int note : {40, 45, 50, 55, 59, 64, 76})
    {
        const auto pluck = [note](bool pinch) {
            auto p = makePerformer();
            if (pinch)
                p->noteOn(0, StratPerformer::firstArticulationKey + 6, 1);
            p->noteOn(0, note, 110);
            const auto y = render(*p, 0.8);
            int s = 0;
            for (int k = 0; k < 6; ++k)
                s = p->stringNote(k) == note ? k : s;
            const double base = p->engine().fundamentalHz(s);
            const double b = StratString::construction(s).stiffness
                * std::pow(2.0, p->stringFret(s) / 6.0);
            std::vector<double> over;
            for (int k = 3; k <= 12; ++k)
            {
                const double partial = k * base * std::sqrt((1.0 + k * k * b) / (1.0 + b));
                over.push_back(partial < 0.45 * rate
                                   ? level(y, 0.2, 0.5, partial) - level(y, 0.2, 0.5, base)
                                   : -1e9);
            }
            return over;
        };
        const auto pinched = pluck(true), ordinary = pluck(false);
        size_t kept = 0;
        for (size_t k = 1; k < pinched.size(); ++k)
            if (pinched[k] - ordinary[k] > pinched[kept] - ordinary[kept])
                kept = k;
        std::printf("pinch %d: partial %zu %.1f dB over the fundamental (ordinary %.1f)\n", note,
                    kept + 3, pinched[kept], ordinary[kept]);
        check(note < 50 || pinched[kept] > 0.0, "a pinch's harmonic rings over the fundamental");
        check(pinched[kept] > ordinary[kept] + 20.0,
              "a pinch brings its harmonic out of an ordinary stroke");
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

void testWheelAndPressure()
{
    // The mod wheel lays the palm down: off at the bottom, Palm mute's at the
    // top, a lighter touch between.
    const auto take = [](int how, double wheel) {
        auto p = makePerformer();
        if (how == 1)
            p->noteOn(0, 25, 1);
        if (how == 2)
            p->modWheel(wheel);
        p->noteOn(0, 45, 90);
        return render(*p, 0.6);
    };
    const auto open = take(0, 0.0), muted = take(1, 0.0), top = take(2, 1.0), bottom = take(2, 0.0),
               half = take(2, 0.5);
    check(top == muted, "the mod wheel at the top mutes as Palm mute does");
    check(bottom == open, "the mod wheel at the bottom leaves the strings alone");
    const double late = energy(open, 0.4, 0.2), lateHalf = energy(half, 0.4, 0.2),
                 lateMuted = energy(muted, 0.4, 0.2);
    std::printf("open A, its last 0.2 s: half wheel %.1f dB, full wheel %.1f dB from open\n",
                10.0 * std::log10(lateHalf / late), 10.0 * std::log10(lateMuted / late));
    check(lateMuted < lateHalf && lateHalf < late, "the palm mutes more as the wheel rises");

    // Channel pressure rocks fretted notes; an open string has no finger to rock it.
    const auto rocked = [](int note, double pressure) {
        auto p = makePerformer();
        p->channelPressure(pressure);
        p->noteOn(0, note, 90);
        return render(*p, 0.6);
    };
    check(rocked(47, 1.0) != rocked(47, 0.0), "channel pressure gives a fretted note vibrato");
    check(rocked(45, 1.0) == rocked(45, 0.0), "channel pressure leaves an open string alone");
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
        const double want = tunedHz(s) * fretRatio(s, fret)
            * std::sqrt(p->engine().bridge().tensionRatio(s));
        tuned = tuned && std::abs(p->engine().fundamentalHz(s) / want - 1.0) < 1e-9;
    }
    check(tuned, "every string follows the bridge");
    int bent = -1;
    for (int s = 0; s < 6; ++s)
        if (p->stringNote(s) == 69)
            bent = s;
    // The pitch the string would have unbent: the same arm, and the same
    // fretting fingers stretching their strings.
    electry::StratBridge reference;
    reference.setArm(-0.5);
    for (int s = 0; s < 6; ++s)
        if (p->stringNote(s) >= 0 && p->stringFret(s) > 0)
            reference.setPress(s, electry::StratEngine::fretPressMetres
                                      + electry::StratEngine::actionStretchMetres(p->stringFret(s)));
    check(bent >= 0
              && std::abs(p->engine().bridge().tensionRatio(bent)
                          / (std::pow(2.0, 4.0 / 12.0) * reference.tensionRatio(bent)) - 1.0)
                  < 1e-9,
          "a member channel's bend takes its string a whole tone up");
}

// The frequency the windowed spectrum peaks at within `span` of a guess.
double peakHz(const std::vector<float>& x, double start, double length, double guess, double span)
{
    double low = guess - span, high = guess + span;
    for (int i = 0; i < 40; ++i)
    {
        const double a = low + 0.382 * (high - low), b = low + 0.618 * (high - low);
        if (level(x, start, length, a) < level(x, start, length, b))
            low = a;
        else
            high = b;
    }
    return 0.5 * (low + high);
}

// Each open string, picked at the ordinary stroke by a fresh hand, reads in
// tune on a tuner once the attack has passed (StratEngine::restPitch): its
// fundamental over 0.4 to 0.8 s within 0.3 cents of equal temperament from
// A4 = 440 Hz.
void testTunedAtTheOrdinaryStroke()
{
    double worst = 0.0;
    std::printf("open strings at the ordinary stroke, cents over 0.4-0.8 s:");
    for (int s = 0; s < 6; ++s)
    {
        auto p = makePerformer();
        p->noteOn(0, openNote[s], 77, s);
        const auto y = render(*p, 0.8);
        const double tempered = 440.0 * std::pow(2.0, (openNote[s] - 69) / 12.0);
        const double off = 1200.0 * std::log2(peakHz(y, 0.4, 0.4, tempered, 2.0) / tempered);
        std::printf(" %+.2f", off);
        worst = std::max(worst, std::abs(off));
    }
    std::printf("\n");
    check(worst < 0.3, "every open string reads in tune at the ordinary stroke");
}

std::vector<float> performance(int block, bool lookahead = false)
{
    auto p = makePerformer();
    electry::StratControls controls;
    controls.lookahead = lookahead;
    p->setControls(controls);
    std::vector<float> out(static_cast<size_t>(1.2 * rate));
    struct Step
    {
        int at;
        void (*act)(StratPerformer&);
    };
    const Step steps[] = {
        {0, [](StratPerformer& q) { for (int n : {40, 47, 52, 56, 59, 64}) q.noteOn(0, n, 100); }},
        {9000, [](StratPerformer& q) { q.channelPressure(0.8); q.modWheel(0.3); q.pitchBend(0, -0.3); }},
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

void testTilt()
{
    // Sixteen strokes of the open A: the swing along the top starts with the
    // stroke's or against it, changing from stroke to stroke, and a fresh
    // performer starts with it. The hand's scatter (testScatter) makes every
    // stroke after the first sound different anyway, so the tilt is read from
    // the string; StratString's own tests hear what it does.
    auto p = makePerformer();
    int opposed = 0;
    bool firstWith = true;
    for (int k = 0; k < 16; ++k)
    {
        p->noteOn(0, 45, 90);
        render(*p, 0.05);
        const bool against = p->engine().planesOpposed(1);
        firstWith = k > 0 ? firstWith : !against;
        opposed += against ? 1 : 0;
        p->noteOff(0, 45);
        render(*p, 0.5);
    }
    std::printf("tilt: %d of 16 strokes against the top\n", opposed);
    check(firstWith, "a fresh performer's first stroke sets the planes off together");
    check(opposed >= 5 && opposed <= 11, "the hand's tilt changes from stroke to stroke");
}

// The hand's scatter: the first stroke after a reset lands where the controls
// put it, sample for sample; the strokes after it differ in force and in where
// they land, by about the 0.9 dB and 6 mm the recorded player's notes spread.
// Strokes are compared at one tilt, which moves the level by itself.
void testScatter()
{
    const double f0 = StratString::construction(3).openHz;
    auto q = makePerformer();
    q->noteOn(0, 55, 100);
    const auto fresh = render(*q, 0.25);
    auto p = makePerformer();
    std::vector<double> fundamental, third;
    bool firstSame = true;
    for (int k = 0; k < 24; ++k)
    {
        p->noteOn(0, 55, 100);
        const auto y = render(*p, 0.25);
        const bool against = p->engine().planesOpposed(3);
        p->noteOff(0, 55);
        render(*p, 0.25);
        if (k == 0)
            firstSame = y == fresh;
        if (against)
            continue;
        fundamental.push_back(level(y, 0.02, 0.2, f0));
        third.push_back(level(y, 0.02, 0.2, 3.0 * f0) - fundamental.back());
    }
    check(firstSame, "the first stroke lands where the controls put it");
    const auto sd = [](const std::vector<double>& v) {
        double m = 0.0, s = 0.0;
        for (double x : v)
            m += x / static_cast<double>(v.size());
        for (double x : v)
            s += (x - m) * (x - m) / static_cast<double>(v.size() - 1);
        return std::sqrt(s);
    };
    std::printf("scatter over %zu strokes with the planes together: fundamental SD %.2f dB, "
                "3rd partial's balance SD %.2f dB\n",
                fundamental.size(), sd(fundamental), sd(third));
    check(fundamental.size() >= 6, "enough strokes share a tilt");
    check(sd(fundamental) > 0.3 && sd(fundamental) < 2.0, "strokes differ in force as a player's do");
    check(sd(third) > 0.1, "strokes differ in where they land");
}

void testBlockPartition()
{
    const auto reference = performance(1);
    bool same = true;
    for (int block : {5, 64, 480, 4096})
        same = same && performance(block) == reference;
    check(same, "any block size gives the same bits");
    check(performance(480) == performance(480), "the same performance gives the same bits");
    const auto ahead = performance(1, true);
    bool aheadSame = true;
    for (int block : {5, 480, 4096})
        aheadSame = aheadSame && performance(block, true) == ahead;
    check(aheadSame, "with the lookahead too, any block size gives the same bits");
}

// The lookahead delays everything by its 50 ms and spends them on the
// fretting hand: the finger reaches its fret before the pick, with a tick
// far under the note, and the note lands as late as the latency says.
void testLookahead()
{
    const auto play = [](bool lookahead, int& latency) {
        auto p = makePerformer();
        electry::StratControls controls;
        controls.lookahead = lookahead;
        p->setControls(controls);
        latency = p->latencySamples();
        // The C on the B string's 1st fret.
        p->noteOn(0, 60, 100);
        return render(*p, 0.4);
    };
    int off = 0, on = 0;
    const auto plain = play(false, off), ahead = play(true, on);
    check(off == 0 && on == static_cast<int>(StratPerformer::lookaheadSeconds * rate),
          "the lookahead reports its 50 ms as latency, and none without it");
    const auto onset = [](const std::vector<float>& x) {
        float peak = 0.0f;
        for (float v : x)
            peak = std::max(peak, std::abs(v));
        size_t i = 0;
        while (std::abs(x[i]) < 0.3f * peak)
            ++i;
        return std::make_pair(static_cast<int>(i), peak);
    };
    const auto [plainOnset, plainPeak] = onset(plain);
    const auto [aheadOnset, aheadPeak] = onset(ahead);
    std::printf("lookahead: attack %d samples later, peak %.1f dB against without\n",
                aheadOnset - plainOnset, 20.0 * std::log10(aheadPeak / plainPeak));
    check(std::abs(aheadOnset - plainOnset - on) < static_cast<int>(0.002 * rate),
          "the pick lands the latency later");
    float before = 0.0f;
    for (int i = 0; i < aheadOnset - static_cast<int>(0.003 * rate); ++i)
        before = std::max(before, std::abs(ahead[static_cast<size_t>(i)]));
    const double tick = 20.0 * std::log10(before / aheadPeak + 1e-30);
    std::printf("the finger's tick before the pick: %.1f dB under the note\n", -tick);
    check(tick > -80.0 && tick < -25.0, "the finger reaches its fret audibly but quietly before the pick");
    float plainBefore = 0.0f;
    for (int i = 0; i < plainOnset - static_cast<int>(0.003 * rate); ++i)
        plainBefore = std::max(plainBefore, std::abs(plain[static_cast<size_t>(i)]));
    check(plainBefore < 1e-6f, "without the lookahead nothing sounds before the pick");
}

// The fretting hand's scatter: with the lookahead on, the finger reaches its
// fret before each pick, the first time after a reset as the controls put
// it and every time after at a speed of its own, so the ticks before the
// notes spread about as the recorded player's do (6.2 dB, StratPerformer.cpp).
void testFrettingHandScatter()
{
    electry::StratControls controls;
    controls.lookahead = true;
    // The C on the B string's 1st fret, picked, and let go.
    const auto play = [](StratPerformer& p) {
        p.noteOn(0, 60, 90);
        const auto y = render(p, 0.4);
        p.noteOff(0, 60);
        render(p, 0.3);
        // The steepest step before the pick: the tick's sharp edge.
        const int pick = p.latencySamples();
        double tick = 0.0;
        for (int i = pick - static_cast<int>(0.045 * rate); i < pick - static_cast<int>(0.003 * rate); ++i)
            tick = std::max(tick, std::abs(static_cast<double>(y[static_cast<size_t>(i)])
                                           - y[static_cast<size_t>(i - 1)]));
        return 20.0 * std::log10(tick + 1e-30);
    };
    auto fresh = makePerformer();
    fresh->setControls(controls);
    const double first = play(*fresh);
    auto p = makePerformer();
    p->setControls(controls);
    std::vector<double> ticks;
    for (int k = 0; k < 24; ++k)
        ticks.push_back(play(*p));
    check(ticks[0] == first, "the first landing after a reset is as the controls put it");
    double mean = 0.0, spread = 0.0;
    for (size_t k = 1; k < ticks.size(); ++k)
        mean += ticks[k] / static_cast<double>(ticks.size() - 1);
    for (size_t k = 1; k < ticks.size(); ++k)
        spread += (ticks[k] - mean) * (ticks[k] - mean) / static_cast<double>(ticks.size() - 2);
    spread = std::sqrt(spread);
    std::printf("the fretting finger's ticks over %zu landings: SD %.1f dB\n", ticks.size() - 1, spread);
    check(spread > 3.0 && spread < 9.0, "every landing ticks at its own level");
}

// The coils' hum: off, silence; on, the mains line and its odd harmonics,
// at 50 or 60 Hz.
void testHum()
{
    const auto lines = [](double hum, double mains) {
        auto p = makePerformer();
        electry::StratControls controls;
        controls.hum = hum;
        controls.mainsHz = mains;
        p->setControls(controls);
        const auto y = render(*p, 0.5);
        return std::array<double, 3> {level(y, 0.1, 0.4, mains), level(y, 0.1, 0.4, 2.0 * mains),
                                      level(y, 0.1, 0.4, 3.0 * mains)};
    };
    const auto off = lines(0.0, 50.0), fifty = lines(1.0, 50.0), sixty = lines(1.0, 60.0);
    std::printf("hum: 50 Hz %.1f dB, 100 Hz %.1f, 150 Hz %.1f; at 60 Hz mains %.1f, %.1f, %.1f\n",
                fifty[0], fifty[1], fifty[2], sixty[0], sixty[1], sixty[2]);
    check(off[0] < -250.0, "no hum, no line");
    check(fifty[0] > off[0] + 200.0 && fifty[2] > fifty[0] - 3.0 && fifty[1] < fifty[0] - 40.0,
          "the coils hum at the mains frequency and its odd harmonics");
    check(std::abs(sixty[0] - fifty[0]) < 1.0, "60 Hz mains hums as loud");
}

} // namespace

int main()
{
    testChordShapes();
    testStrums();
    testStrokeKeys();
    testFingerKey();
    testDampAll();
    testReleaseAndPedal();
    testRestingHand();
    testReleaseLift();
    testLegatoAndSlide();
    testHarmonics();
    testPinch();
    testPalm();
    testWheelAndPressure();
    testDeadNotes();
    testArmAndBends();
    testTunedAtTheOrdinaryStroke();
    testTilt();
    testScatter();
    testBlockPartition();
    testLookahead();
    testFrettingHandScatter();
    testHum();
    if (failures == 0)
        std::printf("StratPerformer: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
