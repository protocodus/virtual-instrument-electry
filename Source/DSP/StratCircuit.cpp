#include "StratCircuit.h"

#include "DspCompatibility.h"

#include <cmath>

namespace electry
{
namespace
{

// The augmented system [x; u; du] that integrates a linearly interpolated
// input exactly: its exponential over one sample carries phi and both input
// matrices.
constexpr int augmentedSize
    = StratCircuit::stateCount + 2 * StratCircuit::inputCount;
using Matrix = std::array<double, augmentedSize * augmentedSize>;

constexpr int n = augmentedSize;

void multiply(const Matrix& a, const Matrix& b, Matrix& out) noexcept
{
    for (int row = 0; row < n; ++row)
    {
        double* const outRow = &out[row * n];
        for (int column = 0; column < n; ++column)
            outRow[column] = 0.0;
        for (int k = 0; k < n; ++k)
        {
            const double aik = a[row * n + k];
            if (aik == 0.0)
                continue;
            const double* const bRow = &b[k * n];
            for (int column = 0; column < n; ++column)
                outRow[column] += aik * bRow[column];
        }
    }
}

double oneNorm(const Matrix& a) noexcept
{
    double norm = 0.0;
    for (int column = 0; column < n; ++column)
    {
        double sum = 0.0;
        for (int row = 0; row < n; ++row)
            sum += scalarAbs(a[row * n + column]);
        norm = sum > norm ? sum : norm;
    }
    return norm;
}

// Solves lhs * out = rhs by Gaussian elimination with partial pivoting.
// Both inputs are consumed.
void solve(Matrix& lhs, Matrix& rhs, Matrix& out) noexcept
{
    for (int pivotColumn = 0; pivotColumn < n; ++pivotColumn)
    {
        int pivotRow = pivotColumn;
        for (int row = pivotColumn + 1; row < n; ++row)
            if (scalarAbs(lhs[row * n + pivotColumn])
                > scalarAbs(lhs[pivotRow * n + pivotColumn]))
                pivotRow = row;
        if (pivotRow != pivotColumn)
            for (int column = 0; column < n; ++column)
            {
                const double a = lhs[pivotRow * n + column];
                lhs[pivotRow * n + column] = lhs[pivotColumn * n + column];
                lhs[pivotColumn * n + column] = a;
                const double b = rhs[pivotRow * n + column];
                rhs[pivotRow * n + column] = rhs[pivotColumn * n + column];
                rhs[pivotColumn * n + column] = b;
            }
        const double pivot = lhs[pivotColumn * n + pivotColumn];
        for (int row = pivotColumn + 1; row < n; ++row)
        {
            const double factor = lhs[row * n + pivotColumn] / pivot;
            if (factor == 0.0)
                continue;
            for (int column = pivotColumn; column < n; ++column)
                lhs[row * n + column] -= factor * lhs[pivotColumn * n + column];
            for (int column = 0; column < n; ++column)
                rhs[row * n + column] -= factor * rhs[pivotColumn * n + column];
        }
    }
    for (int row = n - 1; row >= 0; --row)
        for (int column = 0; column < n; ++column)
        {
            double sum = rhs[row * n + column];
            for (int k = row + 1; k < n; ++k)
                sum -= lhs[row * n + k] * out[k * n + column];
            out[row * n + column] = sum / lhs[row * n + row];
        }
}

// Matrix exponential by scaling and squaring with the degree-13 Pade
// approximant (Higham, "The scaling and squaring method for the matrix
// exponential revisited", SIAM J. Matrix Anal. Appl. 26, 2005). The contact
// resistances make the network stiff, so the norm is large and the squaring
// count does the work; diagonal Pade alone would not damp the fast modes.
void exponential(const Matrix& a, Matrix& out) noexcept
{
    constexpr double theta13 = 5.371920351148152;
    constexpr double b[] = {64764752532480000.0, 32382376266240000.0,
                            7771770303897600.0,  1187353796428800.0,
                            129060195264000.0,   10559470521600.0,
                            670442572800.0,      33522128640.0,
                            1323241920.0,        40840800.0,
                            960960.0,            16380.0,
                            182.0,               1.0};

    int squarings = 0;
    const double norm = oneNorm(a);
    if (norm > theta13)
        squarings = static_cast<int>(std::ceil(std::log2(norm / theta13)));
    const double scale = std::ldexp(1.0, -squarings);

    Matrix x {}, x2 {}, x4 {}, x6 {};
    for (int i = 0; i < n * n; ++i)
        x[i] = a[i] * scale;
    multiply(x, x, x2);
    multiply(x2, x2, x4);
    multiply(x4, x2, x6);

    Matrix inner {}, u {}, v {};
    for (int i = 0; i < n * n; ++i)
        inner[i] = b[13] * x6[i] + b[11] * x4[i] + b[9] * x2[i];
    multiply(x6, inner, u);
    for (int i = 0; i < n * n; ++i)
        inner[i] = u[i] + b[7] * x6[i] + b[5] * x4[i] + b[3] * x2[i];
    for (int i = 0; i < n; ++i)
        inner[i * n + i] += b[1];
    multiply(x, inner, u);

    for (int i = 0; i < n * n; ++i)
        inner[i] = b[12] * x6[i] + b[10] * x4[i] + b[8] * x2[i];
    multiply(x6, inner, v);
    for (int i = 0; i < n * n; ++i)
        v[i] += b[6] * x6[i] + b[4] * x4[i] + b[2] * x2[i];
    for (int i = 0; i < n; ++i)
        v[i * n + i] += b[0];

    Matrix denominator {}, numerator {};
    for (int i = 0; i < n * n; ++i)
    {
        denominator[i] = v[i] - u[i];
        numerator[i] = v[i] + u[i];
    }
    solve(denominator, numerator, out);

    for (int i = 0; i < squarings; ++i)
    {
        multiply(out, out, x);
        out = x;
    }
}

struct Wiring
{
    bool coil[3];   // neck, middle, bridge
    bool tone[2];   // Tone 1 (neck; the Tele's master), Tone 2 (middle)
};

Wiring wiringFor(StratCircuit::Guitar guitar, StratCircuit::Position position) noexcept
{
    using Position = StratCircuit::Position;
    if (guitar == StratCircuit::Guitar::tele)
    {
        switch (position)
        {
            case Position::bridge:
            case Position::bridgeMiddle:
                return {{false, false, true}, {true, false}};
            case Position::middle:
            case Position::bridgeNeck:
                return {{true, false, true}, {true, false}};
            case Position::middleNeck:
            case Position::neck:
            default:
                return {{true, false, false}, {true, false}};
        }
    }
    switch (position)
    {
        case Position::bridge:
            return {{false, false, true}, {false, false}};
        case Position::bridgeMiddle:
            return {{false, true, true}, {false, true}};
        case Position::middle:
        case Position::bridgeNeck:
            return {{false, true, false}, {false, true}};
        case Position::middleNeck:
            return {{true, true, false}, {true, true}};
        case Position::neck:
        default:
            return {{true, false, false}, {true, false}};
    }
}

struct Coil
{
    double ohms, henries, farads, eddyOhms;
};

// Neck, middle and bridge coils of the guitar played; the Tele's middle slot
// is never wired, so its coil only keeps the network's shape.
std::array<Coil, 3> coilsFor(StratCircuit::Guitar guitar) noexcept
{
    using C = StratCircuit;
    const Coil strat {C::coilOhms, C::coilHenries, C::coilFarads, C::eddyOhms};
    if (guitar == C::Guitar::tele)
        return {Coil {C::teleNeckOhms, C::teleNeckHenries, C::teleNeckFarads, C::teleNeckEddyOhms},
                strat,
                Coil {C::teleBridgeOhms, C::teleBridgeHenries, C::teleBridgeFarads,
                      C::teleBridgeEddyOhms}};
    return {strat, strat, strat};
}

} // namespace

double StratCircuit::audioTaper(double rotation) noexcept
{
    rotation = rotation < 0.0 ? 0.0 : (rotation > 1.0 ? 1.0 : rotation);
    return (std::pow(81.0, rotation) - 1.0) / 80.0;
}

void StratCircuit::prepare(double sampleRate) noexcept
{
    samplePeriod = 1.0 / sampleRate;
    updateCoefficients();
    reset();
}

void StratCircuit::setControls(const Controls& controls) noexcept
{
    if (controls == current)
        return;
    current = controls;
    updateCoefficients();
}

void StratCircuit::reset() noexcept
{
    state.fill(0.0);
    lastInput.fill(0.0);
    idle = true;
}

void StratCircuit::updateCoefficients() noexcept
{
    // States: coil currents (neck, middle, bridge) 0-2, coil terminal
    // voltages 3-5, tone capacitor voltages 6-7, jack voltage 8. Node X has
    // no capacitance of its own; its voltage is the conductance-weighted mean
    // of the nodes joined to it. The currents are carried times the coil's
    // characteristic impedance, sqrt(L / C), so they are volts like the other
    // states: unscaled, the matrix spans ten decades and the exponential's
    // squarings lose six digits (1e-6 against 4e-11 relative, checked
    // against SciPy's expm).
    const Wiring wiring = wiringFor(current.guitar, current.position);
    const std::array<Coil, 3> coils = coilsFor(current.guitar);
    const double toneFarads = current.guitar == Guitar::tele ? teleToneFarads : StratCircuit::toneFarads;
    activeCount = 0;
    activeCoilCount = 0;
    for (int k = 0; k < 3; ++k)
    {
        connectedCoil[static_cast<size_t>(k)] = wiring.coil[k];
        if (wiring.coil[k])
        {
            activeCoils[static_cast<size_t>(activeCoilCount++)] = k;
            activeRows[static_cast<size_t>(activeCount++)] = k;
            activeRows[static_cast<size_t>(activeCount++)] = 3 + k;
        }
    }
    for (int t = 0; t < 2; ++t)
    {
        connectedTone[static_cast<size_t>(t)] = wiring.tone[t];
        if (wiring.tone[t])
            activeRows[static_cast<size_t>(activeCount++)] = 6 + t;
    }
    activeRows[static_cast<size_t>(activeCount++)] = 8;

    const double volume = audioTaper(current.volume);

    double coilConductance[3];
    for (int k = 0; k < 3; ++k)
        coilConductance[k] = wiring.coil[k] ? 1.0 / contactOhms : 0.0;
    const double toneRotation[2] = {current.toneNeck, current.toneMiddle};
    double toneConductance[2];
    for (int t = 0; t < 2; ++t)
        toneConductance[t] = wiring.tone[t]
            ? 1.0 / (contactOhms + audioTaper(toneRotation[t]) * potOhms)
            : 0.0;
    const double topConductance
        = 1.0 / ((1.0 - volume) * potOhms + potEndOhms);
    const double groundConductance
        = 1.0 / (volume * potOhms + potEndOhms) + 1.0 / ampInputOhms;

    double total = topConductance;
    for (double g : coilConductance)
        total += g;
    for (double g : toneConductance)
        total += g;
    // Node X voltage as weights over the state vector.
    double nodeX[stateCount] {};
    for (int k = 0; k < 3; ++k)
        nodeX[3 + k] = coilConductance[k] / total;
    for (int t = 0; t < 2; ++t)
        nodeX[6 + t] = toneConductance[t] / total;
    nodeX[8] = topConductance / total;

    double a[stateCount][stateCount] {};
    // A branch of conductance g from node X into a capacitor C.
    const auto feedFromX = [&](int row, double g, double c) {
        for (int j = 0; j < stateCount; ++j)
            a[row][j] += g * nodeX[j] / c;
        a[row][row] -= g / c;
    };
    double coilImpedance[3];
    for (int k = 0; k < 3; ++k)
    {
        const Coil& coil = coils[static_cast<size_t>(k)];
        coilImpedance[k] = std::sqrt(coil.henries / coil.farads);
        a[k][k] = -coil.ohms / coil.henries;
        a[k][3 + k] = -coilImpedance[k] / coil.henries;
        a[3 + k][k] = 1.0 / (coilImpedance[k] * coil.farads);
        a[3 + k][3 + k] -= 1.0 / (coil.eddyOhms * coil.farads);
        feedFromX(3 + k, coilConductance[k], coil.farads);
    }
    for (int t = 0; t < 2; ++t)
        feedFromX(6 + t, toneConductance[t], toneFarads);
    feedFromX(8, topConductance, cableFarads);
    a[8][8] -= groundConductance / cableFarads;

    Matrix augmented {};
    const double h = samplePeriod;
    for (int row = 0; row < stateCount; ++row)
        for (int column = 0; column < stateCount; ++column)
            augmented[row * n + column] = a[row][column] * h;
    for (int k = 0; k < inputCount; ++k)
    {
        augmented[k * n + stateCount + k] = h * coilImpedance[k] / coils[static_cast<size_t>(k)].henries;
        augmented[(stateCount + k) * n + stateCount + inputCount + k] = 1.0;
    }

    Matrix e {};
    exponential(augmented, e);
    for (int row = 0; row < stateCount; ++row)
    {
        for (int column = 0; column < stateCount; ++column)
            phi[row * stateCount + column] = e[row * n + column];
        for (int k = 0; k < inputCount; ++k)
        {
            const double level = e[row * n + stateCount + k];
            const double ramp = e[row * n + stateCount + inputCount + k];
            previousInput[row * inputCount + k] = level - ramp;
            currentInput[row * inputCount + k] = ramp;
        }
    }
    for (int j = 0; j < activeCount; ++j)
    {
        const int column = activeRows[static_cast<size_t>(j)];
        for (int i = 0; i < activeCount; ++i)
        {
            const int row = activeRows[static_cast<size_t>(i)];
            activePhiCol[static_cast<size_t>(j)][static_cast<size_t>(i)]
                = phi[static_cast<size_t>(row * stateCount + column)];
        }
    }
    for (int c = 0; c < activeCoilCount; ++c)
    {
        const int k = activeCoils[static_cast<size_t>(c)];
        for (int i = 0; i < activeCount; ++i)
        {
            const int row = activeRows[static_cast<size_t>(i)];
            activePrevCol[static_cast<size_t>(c)][static_cast<size_t>(i)]
                = previousInput[static_cast<size_t>(row * inputCount + k)];
            activeCurrCol[static_cast<size_t>(c)][static_cast<size_t>(i)]
                = currentInput[static_cast<size_t>(row * inputCount + k)];
        }
    }
}

double StratCircuit::process(double neck, double middle,
                             double bridge) noexcept
{
    if (idle && neck == 0.0 && middle == 0.0 && bridge == 0.0)
        return 0.0;

    const double input[inputCount] = {neck, middle, bridge};
    std::array<double, stateCount> next {};
    double activeState[stateCount] {};
    for (int j = 0; j < activeCount; ++j)
        activeState[j] = state[static_cast<size_t>(activeRows[static_cast<size_t>(j)])];
    double prevIn[inputCount] {};
    double currIn[inputCount] {};
    for (int c = 0; c < activeCoilCount; ++c)
    {
        const int k = activeCoils[static_cast<size_t>(c)];
        prevIn[c] = lastInput[static_cast<size_t>(k)];
        currIn[c] = input[k];
    }

    // Connected subspace: vertical 2-row Double2 keeps each row's exact left-to-right
    // accumulation order over active columns and active coil inputs.
    int i = 0;
    for (; i + 1 < activeCount; i += 2)
    {
        Double2 sum = Double2::splat(0.0);
        for (int j = 0; j < activeCount; ++j)
        {
            const Double2 col = Double2::load(activePhiCol[static_cast<size_t>(j)][static_cast<size_t>(i)],
                                              activePhiCol[static_cast<size_t>(j)][static_cast<size_t>(i + 1)]);
            sum += col * Double2::splat(activeState[j]);
        }
        for (int c = 0; c < activeCoilCount; ++c)
        {
            const Double2 prevW = Double2::load(activePrevCol[static_cast<size_t>(c)][static_cast<size_t>(i)],
                                                activePrevCol[static_cast<size_t>(c)][static_cast<size_t>(i + 1)]);
            const Double2 currW = Double2::load(activeCurrCol[static_cast<size_t>(c)][static_cast<size_t>(i)],
                                                activeCurrCol[static_cast<size_t>(c)][static_cast<size_t>(i + 1)]);
            sum += prevW * Double2::splat(prevIn[c]) + currW * Double2::splat(currIn[c]);
        }
        const int r0 = activeRows[static_cast<size_t>(i)];
        const int r1 = activeRows[static_cast<size_t>(i + 1)];
        const double v0 = sum[0];
        const double v1 = sum[1];
        next[static_cast<size_t>(r0)] = scalarAbs(v0) < 1e-30 ? 0.0 : v0;
        next[static_cast<size_t>(r1)] = scalarAbs(v1) < 1e-30 ? 0.0 : v1;
    }
    if (i < activeCount)
    {
        double sum = 0.0;
        for (int j = 0; j < activeCount; ++j)
            sum += activePhiCol[static_cast<size_t>(j)][static_cast<size_t>(i)] * activeState[j];
        for (int c = 0; c < activeCoilCount; ++c)
            sum += activePrevCol[static_cast<size_t>(c)][static_cast<size_t>(i)] * prevIn[c]
                + activeCurrCol[static_cast<size_t>(c)][static_cast<size_t>(i)] * currIn[c];
        const int row = activeRows[static_cast<size_t>(i)];
        next[static_cast<size_t>(row)] = scalarAbs(sum) < 1e-30 ? 0.0 : sum;
    }

    // Disconnected coils still track their own 2-state tank so switching to
    // them mid-note needs no state mapping.
    for (int k = 0; k < inputCount; ++k)
    {
        if (connectedCoil[static_cast<size_t>(k)])
            continue;
        const double s0 = state[static_cast<size_t>(k)];
        const double s1 = state[static_cast<size_t>(3 + k)];
        const double u0 = lastInput[static_cast<size_t>(k)];
        const double u1 = input[k];
        if (s0 != 0.0 || s1 != 0.0 || u0 != 0.0 || u1 != 0.0)
        {
            const int r0 = k;
            const int r1 = 3 + k;
            const double sum0 = phi[static_cast<size_t>(r0 * stateCount + r0)] * s0
                + phi[static_cast<size_t>(r0 * stateCount + r1)] * s1
                + previousInput[static_cast<size_t>(r0 * inputCount + k)] * u0
                + currentInput[static_cast<size_t>(r0 * inputCount + k)] * u1;
            const double sum1 = phi[static_cast<size_t>(r1 * stateCount + r0)] * s0
                + phi[static_cast<size_t>(r1 * stateCount + r1)] * s1
                + previousInput[static_cast<size_t>(r1 * inputCount + k)] * u0
                + currentInput[static_cast<size_t>(r1 * inputCount + k)] * u1;
            next[static_cast<size_t>(r0)] = scalarAbs(sum0) < 1e-30 ? 0.0 : sum0;
            next[static_cast<size_t>(r1)] = scalarAbs(sum1) < 1e-30 ? 0.0 : sum1;
        }
    }

    // Disconnected tone capacitors have zero conductance to node X (phi = 1).
    for (int t = 0; t < 2; ++t)
        if (!connectedTone[static_cast<size_t>(t)])
            next[static_cast<size_t>(6 + t)] = state[static_cast<size_t>(6 + t)];

    state = next;
    bool zeroAll = (neck == 0.0 && middle == 0.0 && bridge == 0.0);
    for (int k = 0; k < inputCount; ++k)
        lastInput[k] = input[k];
    if (zeroAll)
        for (double x : state)
            zeroAll = zeroAll && (x == 0.0);
    idle = zeroAll;
    return state[8];
}

} // namespace electry
