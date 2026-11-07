#include "StratBridge.h"

#include <cmath>

namespace electry
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr double inch = 0.0254;
constexpr int count = StratBridge::stringCount;

// Steel's Young's modulus, as for the strings' stiffness.
constexpr double youngsModulus = 207e9;

// The steel that carries the tension: the whole plain string, or the core
// of a wound one. The low E's 0.0185 in core is the one the '62 reference's
// stiffness implies (StratString.cpp); the A and D cores take its 0.402
// core-to-gauge ratio, an estimate.
constexpr double loadDiameter[count] = {0.0185 * inch, 0.01447 * inch, 0.01046 * inch,
                                        0.017 * inch,  0.013 * inch,   0.010 * inch};

// The whole stretched run, tuner post to ball end: the 25.5 in speaking
// length, about 55 mm from the saddle down through the block, and the
// headstock run from the nut to a six-in-line post, about 50 mm for the low E
// and 28 mm further for each string after it. Estimates from the layout.
double runLength(int s) noexcept
{
    return StratString::scaleMetres + 0.055 + 0.050 + 0.028 * s;
}

// Plate geometry. The speaking strings cross the saddles about 11 mm above
// the pivot edge. Fender's floating setting lifts the back of the plate
// 1/8 in off the body; taking that edge 56 mm behind the pivot matches the
// pull-up Sound On Sound reports for a 3/16 in float, a tone and a half on
// the G ("Guitar Technology", soundonsound.com), at 3.3 semitones here.
// Neither is measured on the '62; both are open to a better source.
constexpr double saddleHeight = 0.011;
constexpr double floatGap = inch / 8.0;
constexpr double plateRear = 0.056;

double clamp(double value, double low, double high) noexcept
{
    return value < low ? low : (value > high ? high : value);
}

} // namespace

double StratBridge::axialStiffness(int s) noexcept
{
    const double d = loadDiameter[s];
    return youngsModulus * pi * d * d / 4.0 / runLength(s);
}

double StratBridge::restTension(int s) noexcept
{
    const auto& spec = StratString::construction(s);
    const double c = 2.0 * StratString::scaleMetres * spec.openHz;
    return spec.kilogramsPerMetre * c * c;
}

double StratBridge::saddleHeightMetres() noexcept { return saddleHeight; }

double StratBridge::pullUpLimit() noexcept { return floatGap / plateRear; }

double StratBridge::diveLimit() noexcept
{
    // The dive stops where the low E, the string the plate slackens most
    // for its tension, is down to a quarter of it: two octaves of travel
    // the string still holds, and as far as the strings' loop can follow.
    return 0.75 * restTension(0) / (axialStiffness(0) * saddleHeight);
}

StratBridge::StratBridge() noexcept
{
    bend.fill(1.0);
    solve();
}

void StratBridge::setArm(double position) noexcept
{
    arm = clamp(position, -1.0, 1.0);
    solve();
}

void StratBridge::setBend(int stringIndex, double pitchRatio) noexcept
{
    if (stringIndex < 0 || stringIndex >= count)
        return;
    bend[static_cast<size_t>(stringIndex)] = pitchRatio > 1.0 ? pitchRatio : 1.0;
    solve();
}

void StratBridge::setPress(int stringIndex, double metres) noexcept
{
    if (stringIndex < 0 || stringIndex >= count)
        return;
    press[static_cast<size_t>(stringIndex)] = metres > 0.0 ? metres : 0.0;
    solve();
}

void StratBridge::solve() noexcept
{
    // The plate sits at the arm's angle whatever the strings pull (the
    // header): each string's tension changes by k (press - h theta), and a
    // bent string is held at its target over that.
    plateAngle = arm >= 0.0 ? -arm * pullUpLimit() : -arm * diveLimit();
    for (int s = 0; s < count; ++s)
    {
        const size_t k = static_cast<size_t>(s);
        const double unbent = restTension(s) + axialStiffness(s) * (press[k] - saddleHeight * plateAngle);
        ratio[k] = bend[k] * bend[k] * unbent / restTension(s);
    }
}

} // namespace electry
