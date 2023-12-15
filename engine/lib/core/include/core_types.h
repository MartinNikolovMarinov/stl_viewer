#pragma once

/*
    IMPORTANT: It's best to avoid adding any includes to this file to avoid cyclic dependencies, because it is
    literarily included in every other file. This code will likely be used in a 'using namspace coretypes' statement,
    which is the reason it is in a namespace separate from the rest of the code.
*/

// Include system defaults.
#include <stdint.h>
#include <stddef.h>

namespace coretypes {

using i8        = int8_t;
using i16       = int16_t;
using i32       = int32_t;
using i64       = int64_t;
using u8        = uint8_t;
using u16       = uint16_t;
using u32       = uint32_t;
using u64       = uint64_t;
using f32       = float;
using f64       = double;
using uchar     = unsigned char;
using schar     = signed char;
using addr_size = u64;
using addr_off  = i64;
using rune      = u32; // Runes represent a single UTF-32 encoded character.

static constexpr i32 BYTE_SIZE = 8;

// Max unsigned integer constants
static constexpr u64 MAX_U64 = static_cast<u64>(0xFFFFFFFFFFFFFFFF);           // 18446744073709551615
static constexpr u32 MAX_U32 = static_cast<u32>(MAX_U64 >> (sizeof(u32) * 8)); // 4294967295
static constexpr u16 MAX_U16 = static_cast<u16>(MAX_U32 >> (sizeof(u16) * 8)); // 65535
static constexpr u8  MAX_U8  = static_cast<u8>(MAX_U16 >> (sizeof(u8) * 8));   // 255

// Max signed integer constants
static constexpr i64 MAX_I64 = static_cast<i64>(MAX_U64 >> 1);                 // 9223372036854775807
static constexpr i32 MAX_I32 = static_cast<i32>(MAX_I64 >> (sizeof(i32) * 8)); // 2147483647
static constexpr i16 MAX_I16 = static_cast<i16>(MAX_I32 >> (sizeof(i16) * 8)); // 32767
static constexpr i8  MAX_I8  = static_cast<i8>(MAX_I16 >> (sizeof(i8) * 8));   // 127

// Min signed integer constants
static constexpr i64 MIN_I64 = static_cast<i64>((MAX_U64 >> 1) ^ MAX_U64); // -9223372036854775808
static constexpr i32 MIN_I32 = static_cast<i32>((MAX_U32 >> 1) ^ MAX_U32); // -2147483648
static constexpr i16 MIN_I16 = static_cast<i16>((MAX_U16 >> 1) ^ MAX_U16); // -32768
static constexpr i8  MIN_I8  = static_cast<i8>((MAX_U8 >> 1) ^ MAX_U8);    // -128

// Max floating point constants
static constexpr f64 MAX_F64 = static_cast<f64>(1.79769313486231570814527423731704357e+308L);
static constexpr f32 MAX_F32 = static_cast<f32>(3.40282346638528859811704183484516925e+38f);

// Min floating point constants
static constexpr f64 MIN_F64        = static_cast<f64>(MAX_F64);
static constexpr f64 MIN_NORMAL_F64 = static_cast<f64>(2.2250738585072014e-308);
static constexpr f32 MIN_F32        = static_cast<f32>(-MAX_F32);
static constexpr f32 MIN_NORMAL_F32 = static_cast<f32>(1.175494351e-38f);

// Special constants
static constexpr char term_char = '\0';

// Standard I/O
static constexpr i32 STDIN  = 0;
static constexpr i32 STDOUT = 1;
static constexpr i32 STDERR = 2;

// Storage Sizes
static constexpr u64 BYTE     = static_cast<u64>(1);
static constexpr u64 KILOBYTE = static_cast<u64>(1024 * BYTE);
static constexpr u64 MEGABYTE = static_cast<u64>(1024 * KILOBYTE);
static constexpr u64 GIGABYTE = static_cast<u64>(1024 * MEGABYTE);
static constexpr u64 TERABYTE = static_cast<u64>(1024 * GIGABYTE);

// Duration constants in ns
static constexpr u64 NANOSECOND  = static_cast<u64>(1);                  //                 1ns
static constexpr u64 MICROSECOND = static_cast<u64>(1000 * NANOSECOND);  //             1_000ns
static constexpr u64 MILLISECOND = static_cast<u64>(1000 * MICROSECOND); //         1_000_000ns
static constexpr u64 SECOND      = static_cast<u64>(1000 * MILLISECOND); //     1_000_000_000ns
static constexpr u64 MINUTE      = static_cast<u64>(60 * SECOND);        //    60_000_000_000ns
static constexpr u64 HOUR        = static_cast<u64>(60 * MINUTE);        // 3_600_000_000_000ns

static constexpr f32 PI = 3.14159265358979323846f;

} // namespace coretypes

// Sanity static asserts
static_assert(sizeof(coretypes::i8) == 1, "i8 is not 1 byte");
static_assert(sizeof(coretypes::i16) == 2, "i16 is not 2 bytes");
static_assert(sizeof(coretypes::i32) == 4, "i32 is not 4 bytes");
static_assert(sizeof(coretypes::i64) == 8, "i64 is not 8 bytes");

static_assert(sizeof(coretypes::u8) == 1, "u8 is not 1 byte");
static_assert(sizeof(coretypes::u16) == 2, "u16 is not 2 bytes");
static_assert(sizeof(coretypes::u32) == 4, "u32 is not 4 bytes");
static_assert(sizeof(coretypes::u64) == 8, "u64 is not 8 bytes");

static_assert(sizeof(coretypes::f32) == 4, "f32 is not 4 bytes");
static_assert(sizeof(coretypes::f64) == 8, "f64 is not 8 bytes");

static_assert(sizeof(coretypes::addr_size) == 8, "addr_size is not 8 bytes");
static_assert(sizeof(coretypes::addr_off) == 8, "addr_off is not 8 bytes");

static_assert(sizeof(coretypes::rune) == 4, "rune is not 4 bytes");
static_assert(sizeof(coretypes::uchar) == 1, "uchar is not 1 byte");
static_assert(sizeof(coretypes::schar) == 1, "schar is not 1 byte");
