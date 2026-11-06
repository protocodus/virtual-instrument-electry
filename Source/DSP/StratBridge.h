#pragma once

#include "StratString.h"

#include <array>
#include <cstddef>

namespace electry
{

// The floating synchronized tremolo, quasi-static: a plate rocking on its
// front pivot, the six strings pulling its saddles forward and three
// springs pulling its block back. Tilting it by an angle moves every
// saddle by the same distance along its string, and each string's tension
// changes by its own axial stiffness, so every string retunes by its own
// amount. A finger bend raises one string's tension; the plate tilts
// forward under it and the other strings sag flat.
//
// The plate's own resonance (about 100 Hz) is far above any arm or bend
// gesture, so it is solved in equilibrium; the strings' vibration does not
// move it here.
class StratBridge
{
public:
    static constexpr int stringCount = StratString::stringCount;

    StratBridge() noexcept;

    // -1 dives as far as the arm goes, 0 is the floating rest, +1 pulls up
    // until the back of the plate rests on the body.
    void setArm(double position) noexcept;
    // A finger holds this string bent to `pitchRatio` times the pitch it
    // would have unbent, as a player bends to a target by ear. 1 lifts it.
    void setBend(int stringIndex, double pitchRatio) noexcept;
    // A fretting finger stretches this string by `metres` as it presses it
    // onto the fret; 0 lifts it. Its tension rises by its axial stiffness
    // times that, less what the plate gives back, and the others sag a
    // little, as under a bend.
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
    // Saddle height over the pivot, and the plate's rotational stiffness
    // from its springs alone, N m/rad.
    static double saddleHeightMetres() noexcept;
    static double springStiffness() noexcept;
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
