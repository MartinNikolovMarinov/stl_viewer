#include <core_hash.h>

namespace core {

u32 simpleHash_32(const void* input, addr_size len, u32 seed) {
    const u32 prime = 397;
    u32 h32 = 2166136261U ^ seed;
    const u8* p = reinterpret_cast<const u8*>(input);

    for (addr_size i = 0; i < len; i++) {
        u8 c = p[i];
        h32 = (h32 ^ c) * prime;
    }

    return h32;
}

u32 djb2_32(const void* input, addr_size len, u32 seed) {
    u32 h32 = 5381 ^ seed;
    const u8* p = reinterpret_cast<const u8*>(input);

    for (addr_size i = 0; i < len; i++) {
        u8 c = p[i];
        h32 = h32 * 33 + c;
        // hash = ((hash << 5) + hash) + c;
    }

    return h32;
}

} // namespace core
