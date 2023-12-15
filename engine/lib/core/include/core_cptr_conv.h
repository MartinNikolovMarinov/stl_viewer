#pragma once

#include <core_cptr.h>
#include <core_ints.h>
#include <core_traits.h>
#include <core_types.h>
#include <core_utils.h>
#include <math/core_math.h>

// TODO2: [PERFORMACE] Everything in this file can be much faster.

namespace core {

using namespace coretypes;

template <typename Ta>
constexpr inline Ta charToInt(char s) {
    static_assert(core::is_arithmetic_v<Ta>, "TInt must be an arithmetic type.");
    return static_cast<Ta>(s - '0');
}

template<typename TInt>
constexpr char digitToChar(TInt digit) {
    static_assert(core::is_integral_v<TInt>, "TInt must be an integral type.");
    return static_cast<char>((digit % 10) + '0');
}

namespace detail {

template<typename TInt>
constexpr void intToCptr(TInt n, char* out, u32 digits) {
    static_assert(core::is_integral_v<TInt>, "TInt must be an integral type.");
    Assert(out != nullptr);
    if constexpr (core::is_signed_v<TInt>) {
        if (n < 0) {
            *out++ = '-';
            n = -n;
        }
    }
    i32 dc = (digits == 0) ? i32(digitCount(n)) : i32(digits);
    for (i32 i = dc - 1; i >= 0; i--) {
        // There is a lot of believe in all this static casting, but it 'should not' be dangerous.
        TInt curr = static_cast<TInt>((n / static_cast<TInt>(pow10(u32(i))))) % 10;
        *out++ = digitToChar(curr);
        dc--;
    }
}

} // detail namespace

/**
 * \brief Converts an integer to a string.
 *        If the buffer is too small, or the digit count is incorrect, the result is undefined.
 *
 * \param n The integer to convert.
 * \param out The output buffer.
 * \param digitCount The number of digits to convert, if 0 then the number of digits is calculated.
*/
constexpr void intToCptr(u32 n, char* out, u32 digits = 0) { detail::intToCptr(n, out, digits); }
constexpr void intToCptr(u64 n, char* out, u32 digits = 0) { detail::intToCptr(n, out, digits); }
constexpr void intToCptr(i32 n, char* out, u32 digits = 0) { detail::intToCptr(n, out, digits); }
constexpr void intToCptr(i64 n, char* out, u32 digits = 0) { detail::intToCptr(n, out, digits); }

// This function does not handle TInt overflows!
template <typename TInt>
constexpr TInt cptrToInt(const char* s) {
    static_assert(core::is_integral_v<TInt>, "TInt must be an integral type.");
    s = cptrSkipSpace(s);
    if (s == nullptr || (s[0] != '-' && !isDigit(s[0]))) return 0;

    TInt res = 0;
    bool neg = s[0] == '-';
    if constexpr (core::is_signed_v<TInt>) {
        if (neg) s++;
    }

    while (isDigit(*s)) {
        TInt next = static_cast<TInt>(res * 10) + charToInt<TInt>(*s);
        if (next < res) {
            // Overflow occurred. Does NOT handle all overflow cases, that is not the point of this check!
            return neg ? core::limitMin<TInt>() : core::limitMax<TInt>();
        }
        res = next;
        s++;
    }

    if constexpr (core::is_signed_v<TInt>) {
        if (neg) res = -res;
    }

    return res;
}

// This function does not handle TFloat overflows!
template <typename TFloat>
constexpr TFloat cptrToFloat(const char* s) {
    static_assert(core::is_float_v<TFloat>, "TFloat must be a floating point type.");
    s = cptrSkipSpace(s);
    if (s == nullptr || ((s[0] != '-' && s[0] != '.') && !isDigit(s[0]))) return 0;
    TFloat res = 0;
    bool neg = s[0] == '-';
    i32 fractionalPart = 0;
    if (neg) s++;

    while(true) {
        if (*s == '.') {
            if (fractionalPart > 0) break; // Number has more than one decimal point.
            fractionalPart = 1;
            s++;
        }
        if (!isDigit(*s)) {
            break;
        }

        if (fractionalPart > 0) {
            res += charToInt<TFloat>(*s) / TFloat(pow10(u32(fractionalPart++)));
        }
        else {
            res = res * 10 + charToInt<TFloat>(*s);
        }

        s++;
    };

    return neg ? -res : res;
}

namespace detail {

static constexpr const char* hexDigits = "0123456789ABCDEF";

// The out argument must have enough space to hold the result!
template <typename TInt>
constexpr void intToHex(TInt v, char* out, u64 hexLen) {
    for (size_t i = 0, j = (hexLen - 1) * 4; i < hexLen; i++, j-=4) {
        out[i] = detail::hexDigits[(v >> j) & 0x0f];
    }
}

} // namespace detail

constexpr void intToHex(u8 v, char* out, u64 hexLen = (sizeof(u8) << 1))   { detail::intToHex(v, out, hexLen); }
constexpr void intToHex(u16 v, char* out, u64 hexLen = (sizeof(u16) << 1)) { detail::intToHex(v, out, hexLen); }
constexpr void intToHex(u32 v, char* out, u64 hexLen = (sizeof(u32) << 1)) { detail::intToHex(v, out, hexLen); }
constexpr void intToHex(u64 v, char* out, u64 hexLen = (sizeof(u64) << 1)) { detail::intToHex(v, out, hexLen); }
constexpr void intToHex(i8 v, char* out, u64 hexLen = (sizeof(i8) << 1))   { detail::intToHex(v, out, hexLen); }
constexpr void intToHex(i16 v, char* out, u64 hexLen = (sizeof(i16) << 1)) { detail::intToHex(v, out, hexLen); }
constexpr void intToHex(i32 v, char* out, u64 hexLen = (sizeof(i32) << 1)) { detail::intToHex(v, out, hexLen); }
constexpr void intToHex(i64 v, char* out, u64 hexLen = (sizeof(i64) << 1)) { detail::intToHex(v, out, hexLen); }

} // namespace core
