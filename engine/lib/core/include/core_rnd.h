#pragma once

#include <core_API.h>
#include <core_types.h>
#include <core_utils.h>

namespace core {

using namespace coretypes;

/**
 * Initializes the global state needed for pseudo-random number generation.
 * By default the initial seed is based on cpu ticks since last restart.
 *
 * The algorithm/s chosen will prioritize speed above security, therefore:
 * - These functions should not be used for applications that need a
 *   cryptographically-secure random number generation.
 * - The random number generation is not thread safe.
 * - None of the provided parameters, in any of the functions, will
 *   be checked for validity. If, for example, a min value is bigger
 *   that max value the behaviour is undefined.
*/
CORE_API_EXPORT void  rndInit();
CORE_API_EXPORT void  rndInit(u64 seed);
CORE_API_EXPORT u32   rndU32();
CORE_API_EXPORT u32   rndU32(u32 min, u32 max);
CORE_API_EXPORT u64   rndU64();
CORE_API_EXPORT u64   rndU64(u64 min, u64 max);
CORE_API_EXPORT i32   rndI32();
CORE_API_EXPORT i32   rndI32(i32 min, i32 max);
CORE_API_EXPORT i64   rndI64();
CORE_API_EXPORT i64   rndI64(i64 min, i64 max);
CORE_API_EXPORT f32   rndF32();
CORE_API_EXPORT f32   rndF32(f32 min, f32 max);
CORE_API_EXPORT f64   rndF64();
CORE_API_EXPORT f64   rndF64(f64 min, f64 max);
CORE_API_EXPORT char* rndCptr(char* out, addr_size len);

} // namespace core
