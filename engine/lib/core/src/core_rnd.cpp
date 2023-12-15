#include <core_rnd.h>

#include <core_intrinsics.h>
#include <core_cptr.h>

namespace core {

/**
 * The Algorithm used for pseudo number generation was discovered by Marsaglia in his paper "Xorshift RNGs".
 * The paper is freely available and the algorithm itself is based on shift-register generators.
 * This perticular version does not guarantee the best entropy, it's very simple to implement and incredibly fast.
 *
 * TODO2: If there is a need for generating random bits, in really large arrays, there is a version of xorshift-ing
 *       called "xorwow", which is used by default in Nvidia's CUDA toolkit.
*/

namespace {

u64 seedU64;
const char alphaChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const u32 alphaCharsLen = sizeof(alphaChars) - 1; // alphaChars includes terminating '\0'

u64 xorShift64(u64 state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

}

void rndInit() {
    seedU64 = core::intrin_getCpuTicks();
}
void rndInit(u64 seed) {
    seedU64 = seed;
}

u32 rndU32() {
    seedU64 = xorShift64(seedU64);
    return u32(seedU64);
}
u32 rndU32(u32 min, u32 max) {
    if (min == max) return min;
    u32 ret = (rndU32() % (max - min))  + min;
    return ret;
}

u64 rndU64() {
    seedU64 = xorShift64(seedU64);
    return seedU64;
}
u64 rndU64(u64 min, u64 max) {
    if (min == max) return min;
    u64 ret = (rndU64() % (max - min))  + min;
    return ret;
}

i32 rndI32()                 { return i32(rndU32()); }
i32 rndI32(i32 min, i32 max) { return i32(rndU32(u32(min), u32(max))); }

i64 rndI64()                 { return i64(rndU64()); }
i64 rndI64(i64 min, i64 max) { return i64(rndU64(u64(min), u64(max))); }

f32 rndF32()                 { return f32(rndU32()) / (f32(MAX_U32) + 1.0f); }
f32 rndF32(f32 min, f32 max) { return min + (max - min) * rndF32(); }
f64 rndF64()                 { return f64(rndU64()) / (f64(MAX_U64) + f64(1.0f)); }
f64 rndF64(f64 min, f64 max) { return min + (max - min) * rndF64(); }

char* rndCptr(char* out, addr_size len) {
    for (addr_size i = 0; i < len; i++) {
        i32 randIndex = i32(rndU32() % alphaCharsLen);
        out[i] = alphaChars[randIndex];
    }
    return out + len;
}

} // namespace core
