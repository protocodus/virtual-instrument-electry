// The tremolo held by its stabilizer: the plate sits where the arm puts it,
// the arm retunes every string by its own stiffness, a bend reaches its
// target pitch, a fretting finger's stretch raises its own string by its
// stiffness, neither moves any other string, and the arm stops at the body
// and at the dive stop.
#include "DSP/StratBridge.h"

#include <cmath>
#include <cstdio>

namespace
{
using electry::StratBridge;

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition)
    {
        ++failures;
        std::printf("FAIL: %s\n", what);
    }
}

constexpr int count = StratBridge::stringCount;

// The plate's angle with the arm alone, as the arm's position defines it.
double armAngle(double arm)
{
    return arm >= 0.0 ? -arm * StratBridge::pullUpLimit() : -arm * StratBridge::diveLimit();
}

void testRest()
{
    StratBridge bridge;
    bool unity = true;
    for (int s = 0; s < count; ++s)
        unity = unity && bridge.tensionRatio(s) == 1.0;
    check(unity && bridge.angle() == 0.0, "at rest every string has its own tension");
}

void testArm()
{
    // With nothing bent, the strings' tension falls by k h theta each: an
    // independent sum of the saddle moves.
    for (double arm : {-1.0, -0.3, 0.4, 1.0})
    {
        StratBridge bridge;
        bridge.setArm(arm);
        const double theta = armAngle(arm);
        double worst = 0.0;
        for (int s = 0; s < count; ++s)
        {
            const double want = 1.0
                - StratBridge::axialStiffness(s) * StratBridge::saddleHeightMetres() * theta
                    / StratBridge::restTension(s);
            const double e = std::abs(bridge.tensionRatio(s) / want - 1.0);
            worst = e > worst ? e : worst;
        }
        check(worst < 1e-12 && bridge.angle() == theta, "the arm moves every saddle by the same distance");
    }
    StratBridge bridge;
    bridge.setArm(1.0);
    check(std::abs(bridge.angle() + StratBridge::pullUpLimit()) < 1e-15,
          "a full pull-up rests the plate on the body");
    bridge.setArm(-1.0);
    check(std::abs(bridge.tensionRatio(0) - 0.25) < 1e-12,
          "the dive stops with the low E at a quarter of its tension");
    double lowest = 1.0;
    for (int s = 1; s < count; ++s)
        lowest = bridge.tensionRatio(s) < lowest ? bridge.tensionRatio(s) : lowest;
    check(lowest > 0.25, "the low E is the string the dive slackens most");
}

void testBends()
{
    const double whole = std::pow(2.0, 2.0 / 12.0);
    for (double arm : {0.0, -0.5, 0.6})
        for (int bent : {0, 3, 5})
        {
            StratBridge reference;
            reference.setArm(arm);
            StratBridge bridge;
            bridge.setArm(arm);
            bridge.setBend(bent, whole);

            // The bent string reaches its target over its unbent pitch.
            check(std::abs(bridge.tensionRatio(bent)
                           / (whole * whole * reference.tensionRatio(bent)) - 1.0)
                      < 1e-12,
                  "a bend reaches its target pitch");

            // The stabilizer holds the plate, so no other string moves.
            bool held = bridge.angle() == reference.angle();
            for (int s = 0; s < count; ++s)
                if (s != bent)
                    held = held && bridge.tensionRatio(s) == reference.tensionRatio(s);
            check(held, "a bend leaves the other strings in tune");
        }

    // Two strings bent together both reach their targets.
    StratBridge bridge;
    bridge.setBend(4, whole);
    bridge.setBend(5, whole);
    check(std::abs(bridge.tensionRatio(4) - whole * whole) < 1e-12
              && std::abs(bridge.tensionRatio(5) - whole * whole) < 1e-12,
          "a double-stop bend reaches both targets");
    bridge.setBend(4, 1.0);
    bridge.setBend(5, 1.0);
    check(bridge.tensionRatio(4) == 1.0 && bridge.tensionRatio(5) == 1.0,
          "lifted bends return the strings to rest");
}

void testPress()
{
    const double press = 30.5e-6;
    const double whole = std::pow(2.0, 2.0 / 12.0);
    for (double arm : {0.0, -0.5, 0.6})
        for (int pressed : {0, 3, 5})
        {
            StratBridge reference;
            reference.setArm(arm);
            StratBridge bridge;
            bridge.setArm(arm);
            bridge.setPress(pressed, press);

            // The pressed string rises by its stiffness times the stretch;
            // the plate stays where the arm holds it.
            const double want = reference.tensionRatio(pressed)
                + StratBridge::axialStiffness(pressed) * press / StratBridge::restTension(pressed);
            check(std::abs(bridge.tensionRatio(pressed) / want - 1.0) < 1e-12
                      && bridge.angle() == reference.angle(),
                  "a pressed string rises by its stretch");

            bool held = true;
            for (int s = 0; s < count; ++s)
                if (s != pressed)
                    held = held && bridge.tensionRatio(s) == reference.tensionRatio(s);
            check(held, "a press leaves the other strings in tune");

            // A bend on a pressed string goes to its target over the pressed
            // pitch, which is what the player hears before bending.
            bridge.setBend(pressed, whole);
            check(std::abs(bridge.tensionRatio(pressed) / (whole * whole * want) - 1.0) < 1e-12,
                  "a pressed string bends to its target over its pressed pitch");
            bridge.setBend(pressed, 1.0);
            bridge.setPress(pressed, 0.0);
            check(std::abs(bridge.tensionRatio(pressed) / reference.tensionRatio(pressed) - 1.0) < 1e-15,
                  "lifting the finger returns the string");
        }
}

} // namespace

int main()
{
    testRest();
    testArm();
    testBends();
    testPress();
    if (failures == 0)
        std::printf("StratBridge: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
