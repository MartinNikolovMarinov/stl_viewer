#pragma once

#include <core_API.h>
#include <core_types.h>
#include <core_intrinsics.h>
#include <core_utils.h>

namespace core {

using namespace coretypes;

namespace detail {

// Lookup tables for DigitCount:
static constexpr u64 powers[] = {
    0,
    u64(1e1),  u64(1e2),  u64(1e3),  u64(1e4),  u64(1e5),
    u64(1e6),  u64(1e7),  u64(1e8),  u64(1e9),  u64(1e10),
    u64(1e11), u64(1e12), u64(1e13), u64(1e14), u64(1e15),
    u64(1e16), u64(1e17), u64(1e18), u64(1e19)
};
static constexpr u32 maxdigits[] = {
    1, 1, 1, 1,
    2, 2, 2,
    3, 3, 3,
    4, 4, 4, 4,
    5, 5, 5,
    6, 6, 6,
    7, 7, 7, 7,
    8, 8, 8,
    9, 9, 9,
    10, 10, 10,
    11, 11, 11, 11,
    12, 12, 12,
    13, 13, 13,
    14, 14, 14, 14,
    15, 15, 15,
    16, 16, 16,
    17, 17, 17, 17,
    18, 18, 18,
    19, 19, 19,
    20, 20, 20, 20,
};

template <typename TInt>
constexpr u32 digitCount(TInt n) {
    if (n == 0) return 1;
    if constexpr (core::is_signed_v<TInt>) {
        if (n < 0) n = -n;
    }
    u32 leadingZeroes = intrin_countLeadingZeros(n);
    u32 usedBits = (sizeof(n) * 8) - u32(leadingZeroes);
    u32 digits = detail::maxdigits[usedBits];
    if (n < static_cast<TInt>(detail::powers[digits - 1])) digits--;
    return digits;
}

} // namespace detail

constexpr u32 digitCount(u32 n) { return detail::digitCount(n); }
constexpr u32 digitCount(u64 n) { return detail::digitCount(n); }
constexpr u32 digitCount(i32 n) { return detail::digitCount(n); }
constexpr u32 digitCount(i64 n) { return detail::digitCount(n); }

} // namespace core
