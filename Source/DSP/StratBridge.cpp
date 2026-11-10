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
// the G ("Guitar Technology", soundonsound.com), at 3.3 semitones here. The
// three vintage springs are taken at 2.5 N/mm each, hooked 40 mm below the
// pivot on the block. None of these is measured on the '62; all are open
// to a better source.
constexpr double saddleHeight = 0.011;
constexpr double floatGap = inch / 8.0;
constexpr double plateRear = 0.056;
constexpr double springRate = 2500.0;
constexpr int springCount = 3;
constexpr double springLever = 0.040;

double sumStiffness() noexcept
{
    double sum = 0.0;
    for (int s = 0; s < count; ++s)
        sum += StratBridge::axialStiffness(s);
    return sum;
}

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

// One string pressed alone, nothing bent, the arm at rest: its tension rises
// by k p less the plate's give, k h theta with theta = h k p / K_total
// (solve()).
double StratBridge::ownPressRatio(int s, double metres) noexcept
{
    if (!(metres > 0.0))
        return 1.0;
    const double k = axialStiffness(s);
    const double total = springStiffness() + saddleHeight * saddleHeight * sumStiffness();
    return 1.0 + k * metres * (1.0 - saddleHeight * saddleHeight * k / total) / restTension(s);
}

double StratBridge::springStiffness() noexcept
{
    return springCount * springRate * springLever * springLever;
}

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
    // Plate equilibrium: K theta = tau + h sum(dT_j). The arm's position is
    // the tilt it gives with nothing pressed or bent, so tau = K_total
    // theta_arm, K_total = K + h^2 sum(k_j). An unbent string's tension
    // changes by k_j (press_j - h theta). A bent one is held at its target:
    // the pitch ratio squared over the tension it would have with no string
    // bent, at theta_0 = theta_arm + h P / K_total, P = sum(k_j press_j). With
    // P_unbent the unbent strings' share of P and B the bent strings' changes
    // this closes:
    //   theta = (K_total theta_arm + h (P_unbent + B)) / (K_total - h^2 K_bent).
    const double h = saddleHeight;
    const double total = springStiffness() + h * h * sumStiffness();
    const double armAngle = arm >= 0.0 ? -arm * pullUpLimit() : -arm * diveLimit();

    double pressedAll = 0.0;
    for (int s = 0; s < count; ++s)
        pressedAll += axialStiffness(s) * press[static_cast<size_t>(s)];
    const double unbentAngle = armAngle + h * pressedAll / total;

    double bentStiffness = 0.0, targets = 0.0, pressed = 0.0;
    std::array<double, count> target {};
    for (int s = 0; s < count; ++s)
    {
        const size_t k = static_cast<size_t>(s);
        if (bend[k] <= 1.0)
        {
            pressed += axialStiffness(s) * press[k];
            continue;
        }
        target[k] = bend[k] * bend[k]
            * (restTension(s) + axialStiffness(s) * (press[k] - h * unbentAngle));
        targets += target[k] - restTension(s);
        bentStiffness += axialStiffness(s);
    }
    // The body stops the plate's back edge; the arm's stop limits the dive.
    plateAngle = clamp((total * armAngle + h * (pressed + targets)) / (total - h * h * bentStiffness),
                       -pullUpLimit(), diveLimit());

    for (int s = 0; s < count; ++s)
    {
        const size_t k = static_cast<size_t>(s);
        const double tension = bend[k] > 1.0
            ? target[k]
            : restTension(s) + axialStiffness(s) * (press[k] - h * plateAngle);
        ratio[k] = tension / restTension(s);
    }
}

} // namespace electry
