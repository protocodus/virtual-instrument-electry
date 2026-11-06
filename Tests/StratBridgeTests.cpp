// The floating tremolo in equilibrium: the plate's torques balance in every
// state, a bend reaches its target pitch while the plate tilts under it,
// every other string sags flat by its own stiffness, a fretting finger's
// stretch raises its string less what the plate gives back, and the arm stops
// at the body and at the dive stop.
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

double totalStiffness()
{
    const double h = StratBridge::saddleHeightMetres();
    double sum = StratBridge::springStiffness();
    for (int s = 0; s < count; ++s)
        sum += h * h * StratBridge::axialStiffness(s);
    return sum;
}

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
        check(worst < 1e-12, "the arm moves every saddle by the same distance");
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

            // Torques: springs against the arm and every string's change.
            const double h = StratBridge::saddleHeightMetres();
            double strings = 0.0;
            for (int s = 0; s < count; ++s)
                strings += h * (bridge.tensionRatio(s) - 1.0) * StratBridge::restTension(s);
            const double arms = armAngle(arm) * totalStiffness();
            const double springs = StratBridge::springStiffness() * bridge.angle();
            check(std::abs(springs - arms - strings) < 1e-9 * std::abs(arms + 1.0),
                  "the plate's torques balance under a bend");

            // Everything else sags flat, each by its own stiffness.
            bool flatter = bridge.angle() > reference.angle();
            for (int s = 0; s < count; ++s)
                if (s != bent)
                {
                    const double drop = reference.tensionRatio(s) - bridge.tensionRatio(s);
                    const double want = StratBridge::axialStiffness(s) * h
                        * (bridge.angle() - reference.angle()) / StratBridge::restTension(s);
                    flatter = flatter && drop > 0.0 && std::abs(drop - want) < 1e-12;
                }
            check(flatter, "a bend pulls the other strings flat");
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
    check(std::abs(bridge.angle()) < 1e-15, "lifted bends return the plate to rest");
}

void testPress()
{
    const double h = StratBridge::saddleHeightMetres();
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

            // The pressed string rises by its stiffness times the stretch, less
            // the plate's give; the plate tilts forward under it.
            const double k = StratBridge::axialStiffness(pressed);
            const double want = 1.0
                + k * (press - h * bridge.angle()) / StratBridge::restTension(pressed);
            check(std::abs(bridge.tensionRatio(pressed) / want - 1.0) < 1e-12
                      && bridge.tensionRatio(pressed) > reference.tensionRatio(pressed)
                      && bridge.angle() > reference.angle(),
                  "a pressed string rises by its stretch, less the plate's give");

            double strings = 0.0;
            for (int s = 0; s < count; ++s)
                strings += h * (bridge.tensionRatio(s) - 1.0) * StratBridge::restTension(s);
            const double arms = armAngle(arm) * totalStiffness();
            const double springs = StratBridge::springStiffness() * bridge.angle();
            check(std::abs(springs - arms - strings) < 1e-9 * std::abs(arms + 1.0),
                  "the plate's torques balance under a press");

            bool flatter = true;
            for (int s = 0; s < count; ++s)
                if (s != pressed)
                    flatter = flatter && bridge.tensionRatio(s) < reference.tensionRatio(s);
            check(flatter, "a press pulls the other strings flat");

            // A bend on a pressed string goes to its target over the pressed
            // pitch, which is what the player hears before bending.
            bridge.setBend(pressed, whole);
            check(std::abs(bridge.tensionRatio(pressed) / (whole * whole * want) - 1.0) < 1e-12,
                  "a pressed string bends to its target over its pressed pitch");
            bridge.setBend(pressed, 1.0);
            bridge.setPress(pressed, 0.0);
            check(std::abs(bridge.angle() - reference.angle()) < 1e-15,
                  "lifting the finger returns the plate");
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
