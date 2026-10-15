#include "DSP/DspCompatibility.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
std::uint64_t next(std::uint64_t& state)
{
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

template <typename T>
bool same(T a, T b)
{
    using UInt = typename electry::detail::FloatBits<T>::UInt;
    return (std::isnan(a) && std::isnan(b))
        || std::bit_cast<UInt>(a) == std::bit_cast<UInt>(b);
}

template <typename T>
bool checkValue(T value, T sign)
{
    return same(electry::scalarAbs(value), std::abs(value))
        && same(electry::scalarFloor(value), std::floor(value))
        && same(electry::copySign(value, sign), std::copysign(value, sign))
        && electry::isFinite(value) == std::isfinite(value);
}

template <typename T>
bool checkType()
{
    using UInt = typename electry::detail::FloatBits<T>::UInt;
    constexpr auto infinity = std::numeric_limits<T>::infinity();
    const std::array<T, 15> edges {{
        T(0), T(-0.0), T(1), T(-1), T(0.5), T(-0.5),
        std::numeric_limits<T>::min(), -std::numeric_limits<T>::min(),
        std::numeric_limits<T>::denorm_min(), -std::numeric_limits<T>::denorm_min(),
        std::numeric_limits<T>::max(), -std::numeric_limits<T>::max(),
        infinity, -infinity, std::numeric_limits<T>::quiet_NaN()
    }};
    for (const auto value : edges)
        for (const auto sign : edges)
            if (!checkValue(value, sign)) return false;

    std::uint64_t state = 0x3214af895b7921ddull;
    for (int i = 0; i < 200000; ++i)
    {
        const auto bits = static_cast<UInt>(next(state));
        const T value = std::bit_cast<T>(bits);
        const T sign = std::bit_cast<T>(static_cast<UInt>(next(state)));
        if (electry::bitCast<UInt>(electry::bitCast<T>(bits)) != bits
            || !checkValue(value, sign)) return false;
        if (!std::isfinite(value) || !std::isfinite(sign)) continue;
        for (const T fraction : { T(-1), T(0), T(0.1), T(0.5), T(1), T(2) })
            if (!same(electry::monotonicLerp(value, sign, fraction),
                      std::lerp(value, sign, fraction))) return false;
    }
    return true;
}
}

int main()
{
    if (!checkType<float>() || !checkType<double>())
    {
        std::cerr << "C++17 DSP scalar compatibility differs from standard math\n";
        return EXIT_FAILURE;
    }
    std::cout << "C++17 scalar helpers match standard math over boundaries and "
                 "400000 deterministic IEEE-754 patterns.\n";
    return EXIT_SUCCESS;
}
