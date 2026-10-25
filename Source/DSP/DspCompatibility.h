#pragma once

#include <cmath>
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

// std::sqrt as the IEEE-754 square root instruction. sqrt is correctly
// rounded, so the instruction and libm give the same bits. Under -fno-builtin
// the std:: spelling is an out-of-line call (in the Rack SDK's libc a
// bit-serial software routine), and several run per string per sample. Clang
// still lowers __builtin_sqrt to the errno-setting libm call on a hosted
// target with -fno-builtin; its elementwise form is the plain instruction.
#if defined(__has_builtin)
#if __has_builtin(__builtin_elementwise_sqrt)
#define ELECTRY_ELEMENTWISE_SQRT 1
#endif
#endif

[[nodiscard]] inline float scalarSqrt(float value) noexcept
{
#if defined(ELECTRY_ELEMENTWISE_SQRT)
    return __builtin_elementwise_sqrt(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_sqrtf(value);
#else
    return std::sqrt(value);
#endif
}

[[nodiscard]] inline double scalarSqrt(double value) noexcept
{
#if defined(ELECTRY_ELEMENTWISE_SQRT)
    return __builtin_elementwise_sqrt(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_sqrt(value);
#else
    return std::sqrt(value);
#endif
}

// std::ceil for the delay-line split points, as the integer the ring index
// needs. Callers have already clamped the delay to a positive range far below
// 2^31; truncation is floor for a positive float and every such float
// converts to int exactly, so static_cast<float> of the result is std::ceil
// bit for bit there. Under -fno-builtin std::ceil would otherwise be an
// out-of-line libm call in every fractional delay read of the sample loop.
// Not for zero, negative or non-finite input.
[[nodiscard]] inline int positiveCeilToInt(float value) noexcept
{
    const int truncated = static_cast<int>(value);
    return static_cast<float>(truncated) < value ? truncated + 1 : truncated;
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
