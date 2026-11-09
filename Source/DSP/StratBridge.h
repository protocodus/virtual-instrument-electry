#pragma once

#include "StratString.h"

#include <array>
#include <cstddef>

namespace electry
{

// The synchronized tremolo, quasi-static: a plate rocking on its front
// pivot, the six strings pulling its saddles forward and three springs
// pulling its block back. Tilting it by an angle moves every saddle by the
// same distance along its string, and each string's tension changes by its
// own axial stiffness, so every string retunes by its own amount.
//
// The plate stays where the arm puts it. Left floating, it tilts forward
// under every extra pull: a fretted chord's fingers stretched their strings
// and sagged every string 1 to 6.5 cents flat (the low E most, an F barre),
// and a bend sagged the others further. The owner asked for the guitar in
// perfect tune (Docs/decisions.md, 2026-11-07), so it is set up as a player
// who wants that sets a Strat up: a stabilizer in the spring cavity holds the
// plate at rest against anything the fingers or a ringing string pull, and
// the hand on the arm holds it wherever it takes it. A bend or a press
// retunes only its own string.
class StratBridge
{
public:
    static constexpr int stringCount = StratString::stringCount;

    StratBridge() noexcept;

    // -1 dives as far as the arm goes, 0 is the floating rest, +1 pulls up
    // until the back of the plate rests on the body.
    void setArm(double position) noexcept;
    // A finger holds this string bent to `pitchRatio` times the pitch it
    // would have unbent. 1 lifts it.
    void setBend(int stringIndex, double pitchRatio) noexcept;
    double bendRatio(int stringIndex) const noexcept
    {
        return bend[static_cast<std::size_t>(stringIndex)];
    }
    // A fretting finger stretches this string by `metres` as it presses it
    // onto the fret; 0 lifts it. Its tension rises by its axial stiffness
    // times that.
    void setPress(int stringIndex, double metres) noexcept;
    double pressMetres(int stringIndex) const noexcept
    {
        return press[static_cast<std::size_t>(stringIndex)];
    }

    // Each string's tension over its tension at rest, unbent.
    double tensionRatio(int stringIndex) const noexcept
    {
        return ratio[static_cast<std::size_t>(stringIndex)];
    }
    // Forward tilt of the plate, radians.
    double angle() const noexcept { return plateAngle; }

    // Axial stiffness of a string from tuner post to ball end, N/m, and its
    // tension at rest, N.
    static double axialStiffness(int stringIndex) noexcept;
    static double restTension(int stringIndex) noexcept;
    // Saddle height over the pivot.
    static double saddleHeightMetres() noexcept;
    static double pullUpLimit() noexcept;
    static double diveLimit() noexcept;

private:
    void solve() noexcept;

    double arm = 0.0;
    std::array<double, stringCount> bend {};
    std::array<double, stringCount> press {};
    std::array<double, stringCount> ratio {};
    double plateAngle = 0.0;
};

} // namespace electry
