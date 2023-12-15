#include <core_bits.h>
#include <core_mem.h>

namespace core {

namespace {

template <typename TFloat>
void floatToBinImpl(u8 bytes[sizeof(TFloat)], TFloat v) {
    union { TFloat a; u8 bytes[sizeof(TFloat)]; } floatUnion;
    floatUnion.a = v;
    core::memcopy(bytes, floatUnion.bytes, sizeof(TFloat));
}

} // namespace detail

void floatToBin(u8 bytes[sizeof(f32)], f32 v) { return floatToBinImpl(bytes, v); }
void floatToBin(u8 bytes[sizeof(f64)], f64 v) { return floatToBinImpl(bytes, v); }

} // namespace core
