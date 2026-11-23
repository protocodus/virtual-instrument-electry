#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

#if defined(__GNUC__) || defined(__clang__)
#define ELECTRY_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define ELECTRY_FORCE_INLINE inline
#endif

namespace electry
{

#if defined(__GNUC__) || defined(__clang__)
// Packed 2-lane SIMD structs keep member alignment at scalar alignment (4 for
// Float2, 8 for Double2) so x86_64 Jukebox Clang 15 never emits aligned SSE
// movaps/movapd instructions against 8-aligned heap members.
struct alignas(4) Float2
{
    typedef float Lanes __attribute__((vector_size(8)));
    Lanes v;

    ELECTRY_FORCE_INLINE static Float2 splat(float x) noexcept { return Float2 { Lanes { x, x } }; }
    ELECTRY_FORCE_INLINE static Float2 load(float a, float b) noexcept { return Float2 { Lanes { a, b } }; }
    ELECTRY_FORCE_INLINE float operator[](int i) const noexcept { return v[i]; }
    ELECTRY_FORCE_INLINE Float2 operator+(Float2 o) const noexcept { return Float2 { v + o.v }; }
    ELECTRY_FORCE_INLINE Float2 operator-(Float2 o) const noexcept { return Float2 { v - o.v }; }
    ELECTRY_FORCE_INLINE Float2 operator*(Float2 o) const noexcept { return Float2 { v * o.v }; }
    ELECTRY_FORCE_INLINE Float2& operator+=(Float2 o) noexcept { v += o.v; return *this; }
    ELECTRY_FORCE_INLINE Float2& operator-=(Float2 o) noexcept { v -= o.v; return *this; }
} __attribute__((packed, aligned(4)));
static_assert(sizeof(Float2) == 8 && alignof(Float2) == 4, "Float2 ABI");

struct alignas(8) Double2
{
    typedef double Lanes __attribute__((vector_size(16)));
    Lanes v;

    ELECTRY_FORCE_INLINE static Double2 splat(double x) noexcept { return Double2 { Lanes { x, x } }; }
    ELECTRY_FORCE_INLINE static Double2 load(double a, double b) noexcept { return Double2 { Lanes { a, b } }; }
    ELECTRY_FORCE_INLINE double operator[](int i) const noexcept { return v[i]; }
    ELECTRY_FORCE_INLINE Double2 operator+(Double2 o) const noexcept { return Double2 { v + o.v }; }
    ELECTRY_FORCE_INLINE Double2 operator-(Double2 o) const noexcept { return Double2 { v - o.v }; }
    ELECTRY_FORCE_INLINE Double2 operator*(Double2 o) const noexcept { return Double2 { v * o.v }; }
    ELECTRY_FORCE_INLINE Double2& operator+=(Double2 o) noexcept { v += o.v; return *this; }
    ELECTRY_FORCE_INLINE Double2& operator-=(Double2 o) noexcept { v -= o.v; return *this; }
} __attribute__((packed, aligned(8)));
static_assert(sizeof(Double2) == 16 && alignof(Double2) == 8, "Double2 ABI");
#else
struct Float2
{
    float v[2];
    static Float2 splat(float x) noexcept { return { { x, x } }; }
    static Float2 load(float a, float b) noexcept { return { { a, b } }; }
    float operator[](int i) const noexcept { return v[i]; }
    Float2 operator+(Float2 o) const noexcept { return { { v[0] + o.v[0], v[1] + o.v[1] } }; }
    Float2 operator-(Float2 o) const noexcept { return { { v[0] - o.v[0], v[1] - o.v[1] } }; }
    Float2 operator*(Float2 o) const noexcept { return { { v[0] * o.v[0], v[1] * o.v[1] } }; }
    Float2& operator+=(Float2 o) noexcept { v[0] += o.v[0]; v[1] += o.v[1]; return *this; }
    Float2& operator-=(Float2 o) noexcept { v[0] -= o.v[0]; v[1] -= o.v[1]; return *this; }
};
struct Double2
{
    double v[2];
    static Double2 splat(double x) noexcept { return { { x, x } }; }
    static Double2 load(double a, double b) noexcept { return { { a, b } }; }
    double operator[](int i) const noexcept { return v[i]; }
    Double2 operator+(Double2 o) const noexcept { return { { v[0] + o.v[0], v[1] + o.v[1] } }; }
    Double2 operator-(Double2 o) const noexcept { return { { v[0] - o.v[0], v[1] - o.v[1] } }; }
    Double2 operator*(Double2 o) const noexcept { return { { v[0] * o.v[0], v[1] * o.v[1] } }; }
    Double2& operator+=(Double2 o) noexcept { v[0] += o.v[0]; v[1] += o.v[1]; return *this; }
    Double2& operator-=(Double2 o) noexcept { v[0] -= o.v[0]; v[1] -= o.v[1]; return *this; }
};
#endif

// The embedded host uses C++17 and compiles with -fno-builtin. Keep these
// scalar operations explicit so they neither need C++20 library headers nor
// turn into out-of-line libm calls in the sample loop.
template <typename To, typename From>
[[nodiscard]] ELECTRY_FORCE_INLINE To bitCast(const From& from) noexcept
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
[[nodiscard]] ELECTRY_FORCE_INLINE T scalarAbs(T value) noexcept
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
[[nodiscard]] ELECTRY_FORCE_INLINE T scalarFloor(T value) noexcept
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
