#pragma once

#include <cstdint>

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
 #include <xmmintrin.h>
#elif defined(_MSC_VER) && defined(_M_ARM64)
 #include <float.h>
#endif

namespace electry
{

// Raw DSP clients (offline renderers, tests and embedders) need the same
// denormal policy as the plug-in's JUCE callback. Recursive tails otherwise
// settle at subnormal fixed points, which can be expensive on x86. Only the
// current thread is affected, and a caller's existing policy is restored.
// Nested use in an already-guarded audio callback performs no register writes.
class ScopedDspNoDenormals
{
public:
    ScopedDspNoDenormals() noexcept
        : previous_(readControl()),
          changed_((previous_ & denormalMask) != denormalMode)
    {
        if (changed_)
            writeControl((previous_ & ~denormalMask) | denormalMode);
    }

    ~ScopedDspNoDenormals() noexcept
    {
        if (changed_)
            writeControl(previous_);
    }

    ScopedDspNoDenormals(const ScopedDspNoDenormals&) = delete;
    ScopedDspNoDenormals& operator=(const ScopedDspNoDenormals&) = delete;

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) \
    || defined(__aarch64__) || (defined(_MSC_VER) && defined(_M_ARM64))
    static constexpr bool supported = true;
#else
    static constexpr bool supported = false;
#endif

private:
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    static constexpr std::uint64_t denormalMask = (1u << 15u) | (1u << 6u);
    static constexpr std::uint64_t denormalMode = denormalMask; // FTZ + DAZ
    static std::uint64_t readControl() noexcept { return _mm_getcsr(); }
    static void writeControl(std::uint64_t value) noexcept
    {
        // Retain the write with older AppleClang versions that can incorrectly
        // optimise the intrinsic's non-volatile argument away.
        volatile auto control = static_cast<unsigned int>(value);
        _mm_setcsr(control);
    }
#elif defined(_MSC_VER) && defined(_M_ARM64)
    static constexpr std::uint64_t denormalMask = _MCW_DN;
    static constexpr std::uint64_t denormalMode = _DN_FLUSH;
    static std::uint64_t readControl() noexcept { return _control87(0, 0) & _MCW_DN; }
    static void writeControl(std::uint64_t value) noexcept
    {
        _control87(static_cast<unsigned int>(value), _MCW_DN);
    }
#elif defined(__aarch64__)
    static constexpr std::uint64_t denormalMask = 1u << 24u;
    static constexpr std::uint64_t denormalMode = denormalMask; // FZ
    static std::uint64_t readControl() noexcept
    {
        std::uint64_t value;
        __asm__ volatile("mrs %0, fpcr" : "=r"(value) : : "memory");
        return value;
    }
    static void writeControl(std::uint64_t value) noexcept
    {
        __asm__ volatile("msr fpcr, %0" : : "r"(value) : "memory");
    }
#else
    static constexpr std::uint64_t denormalMask = 0;
    static constexpr std::uint64_t denormalMode = 0;
    static std::uint64_t readControl() noexcept { return 0; }
    static void writeControl(std::uint64_t) noexcept {}
#endif

    const std::uint64_t previous_;
    const bool changed_;
};

} // namespace electry
