#pragma once

#include <core_API.h>
#include <core_system_checks.h>
#include <core_types.h>
#include <core_traits.h>
#include <core_utils.h>

#if COMPILER_MSVC == 1
#include <intrin.h>
#endif

namespace core {

using namespace coretypes;

namespace detail {

template<typename TInt>
constexpr u32 leadingZeroCountCompiletimeImpl(TInt n) {
    u32 leadingZeroes = sizeof(n) * core::BYTE_SIZE;
    for (i32 i = 0; i < i32(sizeof(n) * core::BYTE_SIZE); i++) {
        leadingZeroes--;
        n = n >> 1;
        if (n == 0) break;
    }
    return leadingZeroes;
}

template<typename TInt>
constexpr u32 intrin_countLeadingZeros(TInt n) {
    if (n == 0) return 0; // The gnu gcc __builtin_clz and __builtin_clzll documentation states that n = 0 is undefined behavior!

    IS_CONST_EVALUATED { return leadingZeroCountCompiletimeImpl(n); }

#if COMPILER_CLANG == 1 || COMPILER_GCC == 1
    if constexpr (sizeof(TInt) == 4) {
        return u32(__builtin_clz(u32(n)));
    }
    else {
        return u32(__builtin_clzll(u64(n)));
    }
#elif COMPILER_MSVC == 1
    if constexpr (sizeof(TInt) == 4) {
        return u32(__lzcnt(u32(n)));
    }
    else {
        return u32(__lzcnt64(u64(n)));
    }
#else
    return leadingZeroCountCompiletimeImpl(n);
#endif
}

} // namespace detail

constexpr u32 intrin_countLeadingZeros(u32 n) { return detail::intrin_countLeadingZeros(n); }
constexpr u32 intrin_countLeadingZeros(u64 n) { return detail::intrin_countLeadingZeros(n); }
constexpr u32 intrin_countLeadingZeros(i32 n) { return detail::intrin_countLeadingZeros(n); }
constexpr u32 intrin_countLeadingZeros(i64 n) { return detail::intrin_countLeadingZeros(n); }

constexpr f32 intrin_hugeValf()  { return __builtin_huge_valf(); }
constexpr f32 intrin_nanf()      { return __builtin_nanf(""); }
constexpr f32 intrin_nansf()     { return __builtin_nansf(""); }

constexpr f64 intrin_hugeVal()  { return __builtin_huge_val(); }
constexpr f64 intrin_nan()      { return __builtin_nan(""); }
constexpr f64 intrin_nans()     { return __builtin_nans(""); }

namespace detail {

template<typename TUint>
constexpr u32 numberOfSetBitsCompiletimeImpl(TUint n) {
    u32 setBits = 0;
    for (i32 i = 0; i < i32(sizeof(n) * core::BYTE_SIZE); i++) {
        if (n & 1) setBits++;
        n = n >> 1;
    }
    return setBits;
}

template<typename TUint>
constexpr u32 intrin_numberOfSetBits(TUint n) {
    IS_CONST_EVALUATED { return detail::numberOfSetBitsCompiletimeImpl(n); }

#if COMPILER_CLANG == 1 || COMPILER_GCC == 1
    #if defined(CORE_NO_STD) && CORE_NO_STD == 1
        // The builtin popcount seems to be unavailable for clang and gcc when compiling with -nostdlib.
        return detail::numberOfSetBitsCompiletimeImpl(n);
    #else
        if constexpr (sizeof(TUint) == 4) {
            return u32(__builtin_popcount(TUint(n)));
        }
        else {
            return u32(__builtin_popcountll(TUint(n)));
        }
    #endif
#elif COMPILER_MSVC == 1
    if constexpr (sizeof(TUint) == 4) {
        return u32(__popcnt(u32(n)));
    }
    else {
        return u32(__popcnt64(u64(n)));
    }
#else
    return details::numberOfSetBitsCompiletimeImpl(n);
#endif
}

} // namespace detail

constexpr u32 intrin_numberOfSetBits(u32 n) { return detail::intrin_numberOfSetBits(n); }
constexpr u32 intrin_numberOfSetBits(u64 n) { return detail::intrin_numberOfSetBits(n); }

CORE_API_EXPORT u64 intrin_getCpuTicks();

namespace detail {

template<typename TUint>
constexpr inline TUint intrin_rotl(TUint x, i32 s) {
    constexpr i32 N = sizeof(TUint) * core::BYTE_SIZE;
    auto r = s % N;
    if (r == 0)     return x;
    else if (r > 0) return (x << r) | (x >> (N - r));
    else            return (x >> (-r)) | (x << (N + r));
}

}

// Left circular shift.
constexpr inline u32 intrin_rotl(u32 x, i32 s) { return detail::intrin_rotl(x, s); }
constexpr inline u64 intrin_rotl(u64 x, i32 s) { return detail::intrin_rotl(x, s); }

namespace detail {

template<typename TUint>
constexpr inline TUint intrin_rotr(TUint x, i32 s) {
    constexpr i32 N = sizeof(TUint) * core::BYTE_SIZE;
    auto r = s % N;
    if (r == 0)     return x;
    else if (r > 0) return (x >> r) | (x << (N - r));
    else            return (x << (-r)) | (x >> (N + r));
}

}

// Right circular shift.
constexpr inline u32 intrin_rotr(u32 x, i32 s) { return detail::intrin_rotr(x, s); }
constexpr inline u64 intrin_rotr(u64 x, i32 s) { return detail::intrin_rotr(x, s); }

} // namespace core
