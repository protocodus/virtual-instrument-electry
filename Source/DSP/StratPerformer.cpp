#include "StratPerformer.h"

#include <cmath>

namespace electry
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr int openNote[StratEngine::stringCount] = {40, 45, 50, 55, 59, 64};
constexpr double scale = StratString::scaleMetres;

// The hand's force on the pick. Velocity reads linearly as force, so the
// string's deflection and the pickup's output follow it linearly, from the
// lightest stroke that still slips off the pick at 1/20 of full. 4 N holds
// the low E 5 mm aside at a fifth of its length. Voiced until phase 2 fits
// the reference's velocity layers.
constexpr double fullForce = 4.0;
constexpr double lightestShare = 0.05;

// The string leaves the pick in two stages: it slides across the tip's grip
// depth d at its kink velocity F / Z, taking Z d / F, and the tip then springs
// clear in a quarter of its own period, (pi/2) sqrt(m / k). A Strat recorded
// direct (IDMT-SMT-Guitar dataset 1, neck pickup, 78 picked notes) fixes both
// from its 2 to 5 kHz in the first 50 ms, within 2.5 dB on the wound strings
// and 4 dB on the plain ones: d = 0.41 mm, and 123 us for the tip, a 0.73 mm
// celluloid medium of about 6 kN/m moving 37 mg. That assumes the
// recording's ordinary stroke is 2.5 N, velocity 76 here. Release in the
// string's time alone (Z / k for the tip) left the wound strings 5 to 8 dB
// and the plain 2 to 7 dB too bright above 3 kHz. A tip's stiffness grows with
// the cube of its thickness and its mass in proportion, so a thicker pick
// springs clear sooner.
constexpr double gripDepth = 0.00041;
constexpr double mediumStiffness = 6000.0;
constexpr double mediumTipKilograms = 37e-6;
constexpr double mediumThickness = 0.73e-3;
constexpr double thinThickness = 0.46e-3;
constexpr double heavyThickness = 1.0e-3;

// Before it slips, the pick drives the string at the hand's speed through
// the grip (StratString::contactStiffness). At one force on all six strings,
// IDMT's push wants that speed in proportion to each string's own release
// speed F / 2Z, at 1.5 times it (1.4 to 1.7 score within 0.015); a single
// hand speed for all six, 2.5 to 6 m/s, correlates 0.74 to 0.80 against
// 0.835. With the push the release law above still holds 2 to 5 kHz within
// 2.5 dB on every string, and the partials under 500 Hz come within 1.6 dB of
// the reference, from 3.
constexpr double pushRatio = 1.5;

// The palm, fitted to the palm mutes of IDMT-SMT-Guitar's Stratocaster
// (dataset 2: five increasingly heavy mutes at fret 5 on every string, each
// against the same string's unmuted fret-5 note, per-partial decay). A point
// dashpot 20 mm in front of the saddle, working in parallel with the short
// stretch behind it, fits all thirty mutes within 1.8 dB per partial (15 to
// 25 mm within 0.05 dB of that): light, it damps partials as n^2; pressed
// harder than that stretch is stiff, every partial loses the same share each
// period. The lightest rest adds the same decay on every string, R = 12.7/s
// times the string's mass per metre (all six within 30%); the heaviest press
// is the same flesh on every string, 1.7 to 2.2 kg/s on the four whose
// partials last long enough to measure. Pressure blends the two
// geometrically; the player's three middle mutes sat between 0.1 and 0.9 of
// the way, and the blend at 0, 0.15, 0.45, 0.75 and 1 reproduces the thirty
// in this waveguide within 1.6 dB median per partial.
constexpr double palmMetres = 0.020;
constexpr double palmRestPerSecond = 12.7;
constexpr double palmPressedResistance = 2.0;

// A finger lying on the string without pressing it, fitted to the 72 dead
// notes of the same Stratocaster (frets 3, 8 and 14 on every string; decay of
// each octave band 250 Hz to 4 kHz over the first 80 ms). A point cannot
// stop the string fast enough: at its best resistance the 2 and 4 kHz bands
// still ring 4 to 6 times too long (median of the 18 cases), an error of
// 5.9 dB in decay rate (mean of each case's RMS over the bands). Spread along
// 25 mm of string, 2 kg/s in all, it fits within 2.0 dB and the early tilt of
// the spectra within a few dB, with the same 2 kg/s on every string; 20 to
// 40 mm and 1.5 to 3 kg/s score within 0.4 dB of that. It is the palm's
// heaviest press: flesh pressed on a string.
constexpr double fingerResistance = 2.0;
constexpr double fingerWidth = 0.025;

// A harmonic touch has to absorb, not reflect: a rigid point at the node
// splits the string into two halves whose motion turns back into the
// fundamental when the finger lifts (20 dB of it left, measured), while a
// touch matched to the string's own impedance takes the waves passing it
// and leaves the modes with a node there, 30 to 40 dB clear after 40 ms.
constexpr double touchSeconds = 0.040;

// A hammer-on or pull-off: the finger strikes or flicks the string at its
// fret with a share of a pick's force, and releases it slowly, being soft.
constexpr double legatoShare = 0.5;
constexpr double fingerReleaseSeconds = 0.002;

// A slide's travel time, and how soon a stroke must follow the last one to
// go the other way, as a strumming hand keeps moving. Voiced.
constexpr double slideBaseSeconds = 0.020;
constexpr double slideSecondsPerFret = 0.030;
constexpr double alternateWithinSeconds = 0.35;

// Fretting-hand vibrato: a bend rocking up from the note, at a rate inside
// the 5 to 7 Hz guitarists use, up to a quarter tone above it at full wheel.
constexpr double vibratoHz = 5.5;
constexpr double vibratoSemitones = 0.5;

// MPE's default member-channel bend range; a finger bends a string up only,
// and four semitones is as far as a string goes under a finger.
constexpr double memberBendSemitones = 48.0;
constexpr double maximumFingerBend = 4.0;

double fretMetres(double fret) noexcept { return scale * std::pow(2.0, -fret / 12.0); }

// The string's wave impedance at standard tension, kg/s.
double impedance(int s) noexcept
{
    const auto& spec = StratString::construction(s);
    return spec.kilogramsPerMetre * 2.0 * scale * spec.openHz;
}

// A fingertip presses just behind its fret, a third of the way to the next
// one toward the nut; an open string is stopped by the hand near fret 3.
double fingerMetres(double fret) noexcept
{
    const double f = fret < 1.0 ? 3.0 : std::floor(fret + 0.5);
    return fretMetres(f) + (fretMetres(f - 1.0) - fretMetres(f)) / 3.0;
}

double clamp(double value, double low, double high) noexcept
{
    return value < low ? low : (value > high ? high : value);
}

} // namespace

StratPerformer::StratPerformer() : guitar(std::make_unique<StratEngine>()) {}

void StratPerformer::prepare(double rate)
{
    sampleRate = rate;
    guitar->prepare(rate);
    reset();
    setControls(current);
}

void StratPerformer::reset() noexcept
{
    guitar->reset();
    guitar->setArm(0.0);
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        guitar->setBend(s, 1.0);
        guitar->setFret(s, 0.0);
        guitar->setDamper(s, 1, 0.0, 0.0);
        strings[static_cast<size_t>(s)] = StringState {};
    }
    channelBend.fill(0.0);
    vibratoDepth = vibratoPhase = 0.0;
    pedal = false;
    hand = 2.0;
    nextStrokeUp = false;
    lastStroke = -1000000;
    pendingCount = eventCount = 0;
    now = 0;
    applyPalm();
}

void StratPerformer::setControls(const StratControls& controls) noexcept
{
    current = controls;
    current.pickMetres = clamp(controls.pickMetres, 0.025, 0.250);
    current.pick = clamp(controls.pick, 0.0, 1.0);
    current.palmMute = clamp(controls.palmMute, 0.0, 1.0);
    current.strumSeconds = clamp(controls.strumSeconds, 0.0, 0.040);
    current.outputDecibels = clamp(controls.outputDecibels, -24.0, 12.0);
    guitar->setControls(current.circuit);
    outputGain = std::pow(10.0, current.outputDecibels / 20.0);
    applyPalm();
}

int StratPerformer::stringNote(int s) const noexcept
{
    return s >= 0 && s < StratEngine::stringCount ? strings[static_cast<size_t>(s)].note : -1;
}

int StratPerformer::stringFret(int s) const noexcept
{
    return s >= 0 && s < StratEngine::stringCount
        ? static_cast<int>(std::floor(strings[static_cast<size_t>(s)].fret + 0.5))
        : 0;
}

double StratPerformer::velocityForce(int velocity) const noexcept
{
    const double v = clamp(velocity / 127.0, 0.0, 1.0);
    return fullForce * (lightestShare + (1.0 - lightestShare) * v);
}

double StratPerformer::releaseSeconds(int s, double force) const noexcept
{
    const double thickness = thinThickness + (heavyThickness - thinThickness) * current.pick;
    const double ratio = thickness / mediumThickness;
    const double stiffness = mediumStiffness * ratio * ratio * ratio;
    const double tip = 0.5 * pi * std::sqrt(mediumTipKilograms * ratio / stiffness);
    return impedance(s) * gripDepth / force + tip;
}

void StratPerformer::applyPalm() noexcept
{
    const bool on = latched == Articulation::palmMute;
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        const double rest = palmRestPerSecond * StratString::construction(s).kilogramsPerMetre;
        const double resistance
            = rest * std::pow(palmPressedResistance / rest, current.palmMute);
        guitar->setDamper(s, 0, palmMetres, on ? resistance : 0.0);
    }
}

void StratPerformer::noteOn(int channel, int note, int velocity) noexcept
{
    if (velocity <= 0)
    {
        noteOff(channel, note);
        return;
    }
    if (note >= firstArticulationKey && note < firstArticulationKey + articulationCount)
    {
        latched = static_cast<Articulation>(note - firstArticulationKey);
        applyPalm();
        return;
    }
    const int top = latched == Articulation::harmonic ? highestNote + 10 : highestNote;
    if (note < lowestNote || note > top || pendingCount >= maxPending)
        return;
    pending[static_cast<size_t>(pendingCount++)] = {note, channel & 15, velocity};
}

void StratPerformer::noteOff(int channel, int note) noexcept
{
    channel &= 15;
    for (int i = 0; i < pendingCount; ++i)
        if (pending[static_cast<size_t>(i)].note == note
            && pending[static_cast<size_t>(i)].channel == channel)
        {
            pending[static_cast<size_t>(i)] = pending[static_cast<size_t>(--pendingCount)];
            return;
        }
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        StringState& st = strings[static_cast<size_t>(s)];
        if (st.note != note || st.channel != channel || !st.keyDown)
            continue;
        st.keyDown = false;
        bool struck = true;
        for (int e = 0; e < eventCount; ++e)
            struck = struck
                && !(events[static_cast<size_t>(e)].kind == EventKind::strike
                     && events[static_cast<size_t>(e)].string == s);
        if (pedal)
            st.sustained = true;
        else if (struck)
            release(s);
        return;
    }
}

void StratPerformer::release(int s) noexcept
{
    StringState& st = strings[static_cast<size_t>(s)];
    // The fretting finger lets go of the fret but stays on the string.
    guitar->setDamper(s, 1, fingerMetres(st.fret), fingerResistance, fingerWidth);
    if (st.fret > 0.0)
        guitar->setFret(s, 0.0);
    guitar->setBend(s, 1.0);
    const std::uint64_t order = st.order;
    st = StringState {};
    st.order = order;
}

void StratPerformer::pitchBend(int channel, double position) noexcept
{
    channel &= 15;
    position = clamp(position, -1.0, 1.0);
    if (channel == 0)
        guitar->setArm(position);
    else
        channelBend[static_cast<size_t>(channel)]
            = clamp(position * memberBendSemitones, 0.0, maximumFingerBend);
}

void StratPerformer::modWheel(double position) noexcept
{
    vibratoDepth = clamp(position, 0.0, 1.0) * vibratoSemitones;
}

void StratPerformer::sustain(bool down) noexcept
{
    pedal = down;
    if (down)
        return;
    for (int s = 0; s < StratEngine::stringCount; ++s)
        if (strings[static_cast<size_t>(s)].sustained)
            release(s);
}

void StratPerformer::allNotesOff() noexcept
{
    pendingCount = 0;
    eventCount = 0;
    pedal = false;
    for (int s = 0; s < StratEngine::stringCount; ++s)
        if (strings[static_cast<size_t>(s)].note >= 0)
            release(s);
}

void StratPerformer::schedule(const Event& e) noexcept
{
    if (eventCount >= maxEvents)
        return;
    int i = eventCount++;
    while (i > 0 && events[static_cast<size_t>(i - 1)].time > e.time)
    {
        events[static_cast<size_t>(i)] = events[static_cast<size_t>(i - 1)];
        --i;
    }
    events[static_cast<size_t>(i)] = e;
}

bool StratPerformer::playOnSoundingString(const Pending& p) noexcept
{
    // The line continues on the string it is ringing on.
    int s = -1;
    std::uint64_t latest = 0;
    for (int k = 0; k < StratEngine::stringCount; ++k)
    {
        const StringState& st = strings[static_cast<size_t>(k)];
        if (st.order > latest && guitar->isSounding(k))
        {
            latest = st.order;
            s = k;
        }
    }
    if (s < 0)
        return false;
    const int fret = p.note - openNote[s];
    if (fret < 0 || fret > frets)
        return false;

    StringState& st = strings[static_cast<size_t>(s)];
    const double from = st.fret;
    guitar->setDamper(s, 1, 0.0, 0.0);
    if (latched == Articulation::legato)
    {
        guitar->setFret(s, fret);
        // A hammer-on strikes at the new fret, a pull-off flicks at the old.
        const double at = fingerMetres(fret > from ? fret : from);
        const double force = legatoShare * velocityForce(p.velocity);
        guitar->pluck(s, nextStrokeUp ? -force : force, at, fingerReleaseSeconds);
        st.slide = false;
    }
    else
    {
        st.slide = true;
        st.slideFrom = from;
        st.slideTo = fret;
        st.slideStart = now;
        st.slideLength = static_cast<std::int64_t>(
            (slideBaseSeconds + slideSecondsPerFret * std::abs(fret - from)) * sampleRate);
    }
    st.note = p.note;
    st.channel = p.channel;
    st.keyDown = true;
    st.sustained = false;
    st.fret = latched == Articulation::legato ? fret : from;
    st.fretted = true;
    st.order = ++orderCounter;
    if (fret > 0)
        hand = fret;
    return true;
}

bool StratPerformer::playHarmonic(const Pending& p, double force) noexcept
{
    // A natural harmonic, the lowest partial that gives the note, else the
    // octave harmonic over a fretted note, touched at half its length.
    int string = -1, partial = 0;
    double fret = 0.0;
    for (int k = 2; k <= 6 && string < 0; ++k)
        for (int s = 0; s < StratEngine::stringCount && string < 0; ++s)
            if (!strings[static_cast<size_t>(s)].keyDown
                && std::abs(openNote[s] + 12.0 * std::log2(k) - p.note) < 0.3)
            {
                string = s;
                partial = k;
            }
    if (string < 0)
        for (int s = StratEngine::stringCount - 1; s >= 0 && string < 0; --s)
        {
            const int f = p.note - 12 - openNote[s];
            if (f >= 1 && f <= frets && !strings[static_cast<size_t>(s)].keyDown)
            {
                string = s;
                partial = 2;
                fret = f;
            }
        }
    if (string < 0)
        return false;

    Event e;
    e.time = now;
    e.kind = EventKind::strike;
    e.string = string;
    e.fret = fret;
    e.force = force;
    // The node nearest the nut, where a player touches.
    e.touchMetres = fretMetres(fret) * (partial - 1) / partial;
    e.touchResistance = impedance(string);
    schedule(e);
    Event lift;
    lift.time = now + static_cast<std::int64_t>(touchSeconds * sampleRate);
    lift.kind = EventKind::liftTouch;
    lift.string = string;
    schedule(lift);

    StringState& st = strings[static_cast<size_t>(string)];
    st = StringState {};
    st.note = p.note;
    st.channel = p.channel;
    st.keyDown = true;
    st.fret = fret;
    st.fretted = fret > 0.0;
    st.order = ++orderCounter;
    return true;
}

void StratPerformer::assign(const Pending* notes, int count,
                            std::array<int, 6>& stringFor) const noexcept
{
    // Every way of putting the notes on distinct strings, lowest note first,
    // scored as a hand would find it: frets inside a four-fret span around
    // the hand, open strings in the low positions, no held string taken, low
    // notes on low strings.
    std::array<int, 6> trial {};
    // At most one note per string.
    const int placed = count < StratEngine::stringCount ? count : StratEngine::stringCount;
    double best = 1e300;
    bool used[StratEngine::stringCount] {};
    const auto noteCost = [&](int s, int fret) {
        double c = strings[static_cast<size_t>(s)].keyDown ? 20.0 : 0.0;
        if (fret == 0)
            return c + (hand <= 5.0 ? 0.0 : 3.0);
        const double low = hand - 1.0, high = hand + 3.0;
        c += (fret < low ? low - fret : 0.0) + (fret > high ? fret - high : 0.0);
        return c + 0.05 * fret;
    };
    const auto search = [&](const auto& self, int i, double cost) -> void {
        if (cost >= best)
            return;
        // The constant bound is what lets GCC see trial is never overrun
        // through the inlined recursion (-Warray-bounds).
        if (i >= placed || i >= StratEngine::stringCount)
        {
            int lowest = frets + 1, highest = 0;
            double extra = 0.0;
            for (int a = 0; a < placed; ++a)
            {
                if (trial[static_cast<size_t>(a)] < 0)
                    continue;
                const int f = notes[a].note - openNote[trial[static_cast<size_t>(a)]];
                if (f > 0)
                {
                    lowest = f < lowest ? f : lowest;
                    highest = f > highest ? f : highest;
                }
                for (int b = a + 1; b < placed; ++b)
                    if (trial[static_cast<size_t>(b)] >= 0
                        && trial[static_cast<size_t>(b)] < trial[static_cast<size_t>(a)])
                        extra += 2.0;
            }
            if (highest - lowest > 4)
                extra += 3.0 * (highest - lowest - 4);
            if (cost + extra < best)
            {
                best = cost + extra;
                stringFor = trial;
            }
            return;
        }
        // Left out, if no string can take it.
        trial[static_cast<size_t>(i)] = -1;
        self(self, i + 1, cost + 100.0);
        for (int s = 0; s < StratEngine::stringCount; ++s)
        {
            const int fret = notes[i].note - openNote[s];
            if (used[s] || fret < 0 || fret > frets)
                continue;
            used[s] = true;
            trial[static_cast<size_t>(i)] = s;
            self(self, i + 1, cost + noteCost(s, fret));
            used[s] = false;
        }
    };
    stringFor.fill(-1);
    search(search, 0, 0.0);
}

void StratPerformer::startStroke() noexcept
{
    // Sort what arrived together, lowest first.
    for (int i = 1; i < pendingCount; ++i)
        for (int j = i; j > 0 && pending[static_cast<size_t>(j - 1)].note
                                     > pending[static_cast<size_t>(j)].note;
             --j)
        {
            const Pending t = pending[static_cast<size_t>(j)];
            pending[static_cast<size_t>(j)] = pending[static_cast<size_t>(j - 1)];
            pending[static_cast<size_t>(j - 1)] = t;
        }

    const bool alternate = now - lastStroke
        < static_cast<std::int64_t>(alternateWithinSeconds * sampleRate);
    const bool strokeUp = alternate ? nextStrokeUp : false;
    nextStrokeUp = !strokeUp;
    lastStroke = now;
    const double sign = strokeUp ? -1.0 : 1.0;

    Pending notes[6];
    int count = 0;
    for (int i = 0; i < pendingCount; ++i)
    {
        const Pending& p = pending[static_cast<size_t>(i)];
        if ((latched == Articulation::legato || latched == Articulation::slide)
            && playOnSoundingString(p))
            continue;
        if (latched == Articulation::harmonic && playHarmonic(p, sign * velocityForce(p.velocity)))
            continue;
        if (count < 6 && p.note <= highestNote)
            notes[count++] = p;
    }
    pendingCount = 0;
    if (count == 0)
        return;

    std::array<int, 6> stringFor {};
    assign(notes, count, stringFor);
    int first = StratEngine::stringCount, last = -1;
    for (int i = 0; i < count; ++i)
        if (stringFor[static_cast<size_t>(i)] >= 0)
        {
            first = stringFor[static_cast<size_t>(i)] < first ? stringFor[static_cast<size_t>(i)] : first;
            last = stringFor[static_cast<size_t>(i)] > last ? stringFor[static_cast<size_t>(i)] : last;
        }

    double lowestFret = frets + 1.0;
    for (int i = 0; i < count; ++i)
    {
        const int s = stringFor[static_cast<size_t>(i)];
        if (s < 0)
            continue;
        const int fret = notes[i].note - openNote[s];
        const bool dead = latched == Articulation::deadNote;
        // The pick crosses the strings at an even pace, muted ones included.
        const int crossed = strokeUp ? last - s : s - first;
        Event e;
        e.time = now + static_cast<std::int64_t>(crossed * current.strumSeconds * sampleRate);
        e.kind = EventKind::strike;
        e.string = s;
        e.fret = dead ? 0.0 : fret;
        e.force = sign * velocityForce(notes[i].velocity);
        e.touchMetres = dead ? fingerMetres(fret) : 0.0;
        e.touchResistance = dead ? fingerResistance : 0.0;
        e.touchWidth = dead ? fingerWidth : 0.0;
        schedule(e);

        StringState& st = strings[static_cast<size_t>(s)];
        st = StringState {};
        st.note = notes[i].note;
        st.channel = notes[i].channel;
        st.keyDown = true;
        st.fret = e.fret;
        st.fretted = !dead && fret > 0;
        st.order = ++orderCounter;
        if (fret > 0 && fret < lowestFret)
            lowestFret = fret;
    }
    // The hand settles where the shape's lowest fretted note sits.
    if (lowestFret <= frets && (lowestFret < hand - 1.0 || lowestFret > hand + 3.0))
        hand = lowestFret;
}

void StratPerformer::runEvent(const Event& e) noexcept
{
    const int s = e.string;
    if (e.kind == EventKind::liftTouch)
    {
        guitar->setDamper(s, 1, 0.0, 0.0);
        return;
    }
    guitar->setDamper(s, 1, e.touchMetres, e.touchResistance, e.touchWidth);
    guitar->setFret(s, e.fret);
    guitar->setBend(s, strings[static_cast<size_t>(s)].bend);
    const double force = std::abs(e.force);
    guitar->pluck(s, e.force, current.pickMetres, releaseSeconds(s, force),
                  pushRatio * force / (2.0 * impedance(s)));
    const StringState& st = strings[static_cast<size_t>(s)];
    if (!st.keyDown && !st.sustained && st.note >= 0)
        release(s);
}

void StratPerformer::controlUpdate() noexcept
{
    vibratoPhase += 2.0 * pi * vibratoHz * StratEngine::controlPeriod / sampleRate;
    if (vibratoPhase > 2.0 * pi)
        vibratoPhase -= 2.0 * pi;
    const double rock = 0.5 - 0.5 * std::cos(vibratoPhase);
    for (int s = 0; s < StratEngine::stringCount; ++s)
    {
        StringState& st = strings[static_cast<size_t>(s)];
        if (st.slide)
        {
            const double u = st.slideLength > 0
                ? static_cast<double>(now - st.slideStart) / static_cast<double>(st.slideLength)
                : 1.0;
            if (u >= 1.0)
            {
                st.fret = st.slideTo;
                st.slide = false;
            }
            else
                st.fret = st.slideFrom + (st.slideTo - st.slideFrom) * u * u * (3.0 - 2.0 * u);
            guitar->setFret(s, st.fret, true);
        }
        if (st.note < 0 || !st.fretted)
            continue;
        const double semitones = (st.channel != 0 ? channelBend[static_cast<size_t>(st.channel)] : 0.0)
            + vibratoDepth * rock;
        const double ratio = std::pow(2.0, semitones / 12.0);
        if (ratio != st.bend)
        {
            guitar->setBend(s, ratio);
            st.bend = ratio;
        }
    }
}

void StratPerformer::render(float* out, int samples) noexcept
{
    if (pendingCount > 0)
        startStroke();
    int done = 0;
    while (done < samples)
    {
        if (now % StratEngine::controlPeriod == 0)
            controlUpdate();
        while (eventCount > 0 && events[0].time <= now)
        {
            const Event e = events[0];
            for (int i = 1; i < eventCount; ++i)
                events[static_cast<size_t>(i - 1)] = events[static_cast<size_t>(i)];
            --eventCount;
            runEvent(e);
        }
        std::int64_t chunk = samples - done;
        const std::int64_t toControl = StratEngine::controlPeriod - now % StratEngine::controlPeriod;
        chunk = chunk < toControl ? chunk : toControl;
        if (eventCount > 0 && events[0].time - now < chunk)
            chunk = events[0].time - now;
        guitar->process(out + done, static_cast<int>(chunk));
        for (int i = 0; i < chunk; ++i)
            out[done + i] = static_cast<float>(out[done + i] * outputGain);
        done += static_cast<int>(chunk);
        now += chunk;
    }
}

} // namespace electry
