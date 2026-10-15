#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace electry
{

// The embedded host uses C++17 and compiles with -fno-builtin. Keep these
// scalar operations explicit so they neither need C++20 library headers nor
// turn into out-of-line libm calls in the sample loop.
template <typename To, typename From>
[[nodiscard]] inline To bitCast(const From& from) noexcept
{
    static_assert(sizeof(To) == sizeof(From), "bitCast sizes must match");
    static_assert(std::is_trivially_copyable<To>::value
                  && std::is_trivially_copyable<From>::value,
                  "bitCast requires trivially copyable types");
    To to;
#if defined(__GNUC__) || defined(__clang__)
    // Explicit builtin remains inline under the embedded SDK's -fno-builtin.
    __builtin_memcpy(&to, &from, sizeof(to));
#else
    std::memcpy(&to, &from, sizeof(to));
#endif
    return to;
}

namespace detail
{
template <typename T> struct FloatBits;
template <> struct FloatBits<float>
{
    using UInt = std::uint32_t;
    static constexpr int fractionBits = 23;
    static constexpr int exponentBias = 127;
    static constexpr UInt sign = 0x80000000u;
    static constexpr UInt exponent = 0x7f800000u;
};
template <> struct FloatBits<double>
{
    using UInt = std::uint64_t;
    static constexpr int fractionBits = 52;
    static constexpr int exponentBias = 1023;
    static constexpr UInt sign = 0x8000000000000000ull;
    static constexpr UInt exponent = 0x7ff0000000000000ull;
};
} // namespace detail

template <typename T>
[[nodiscard]] inline T scalarAbs(T value) noexcept
{
    if constexpr (std::is_integral<T>::value)
        return value < 0 ? -value : value;
    else
    {
        using Bits = detail::FloatBits<T>;
        static_assert(std::numeric_limits<T>::is_iec559,
                      "DSP scalar helpers require IEEE-754 floating point");
        return bitCast<T>(bitCast<typename Bits::UInt>(value) & ~Bits::sign);
    }
}

template <typename T>
[[nodiscard]] inline bool isFinite(T value) noexcept
{
    using Bits = detail::FloatBits<T>;
    return (bitCast<typename Bits::UInt>(value) & Bits::exponent)
        != Bits::exponent;
}

template <typename T>
[[nodiscard]] inline T copySign(T value, T sign) noexcept
{
    using Bits = detail::FloatBits<T>;
    return bitCast<T>((bitCast<typename Bits::UInt>(value) & ~Bits::sign)
        | (bitCast<typename Bits::UInt>(sign) & Bits::sign));
}

template <typename T>
[[nodiscard]] inline T scalarFloor(T value) noexcept
{
    using Bits = detail::FloatBits<T>;
    using UInt = typename Bits::UInt;
    const UInt bits = bitCast<UInt>(value);
    const int exponent = static_cast<int>((bits & Bits::exponent)
        >> Bits::fractionBits) - Bits::exponentBias;
    if (exponent < 0)
        return value == T(0) ? value : ((bits & Bits::sign) ? T(-1) : T(0));
    // Large finite values are already integers. Inf/NaN pass through too.
    if (exponent >= Bits::fractionBits)
        return value;
    const UInt fraction = (UInt(1) << (Bits::fractionBits - exponent)) - 1;
    if ((bits & fraction) == 0)
        return value;
    const T truncated = bitCast<T>(bits & ~fraction);
    return (bits & Bits::sign) ? truncated - T(1) : truncated;
}

// C++20 lerp's endpoint and monotonicity guarantees, including extrapolation.
// Keep the older float-only electry::lerp arithmetic unchanged in DspMath.h.
template <typename T>
[[nodiscard]] constexpr T monotonicLerp(T a, T b, T t) noexcept
{
    if ((a <= T(0) && b >= T(0)) || (a >= T(0) && b <= T(0)))
        return t * b + (T(1) - t) * a;
    if (t == T(1))
        return b;
    const T value = a + t * (b - a);
    return ((t > T(1)) == (b > a))
        ? (b < value ? value : b) : (value < b ? value : b);
}

} // namespace electry
