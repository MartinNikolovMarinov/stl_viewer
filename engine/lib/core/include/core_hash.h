#pragma once

#include "core_API.h"
#include "core_types.h"

namespace core {

using namespace coretypes;

/**
 * \brief A very simple and fast hashing function to provide as a default hasher. Will have collisions and should not be
 *        used in serious applications that require unique hashes. It can however be used as a decent hash by appending
 *        some additional state.
 *
 * \param input The input data to hash.
 * \param len The length of the input data.
 *
 * \return The hash.
*/
CORE_API_EXPORT u32 simpleHash_32(const void* input, addr_size len, u32 seed = 0);

CORE_API_EXPORT u32 djb2_32(const void* input, addr_size len, u32 seed = 0);

} // namespace core
