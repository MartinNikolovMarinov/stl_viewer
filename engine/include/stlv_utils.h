#pragma once

#include <fwd_internal.h>

namespace stlv {

#define STLV_DEFINE_FLAG_TYPE(T)                                                         \
    constexpr inline bool operator==(T lhs, u32 rhs) { return u32(lhs) == rhs; }         \
    constexpr inline bool operator==(u32 lhs, T rhs) { return lhs == u32(rhs); }         \
    constexpr inline bool operator!=(T lhs, u32 rhs) { return u32(lhs) != rhs; }         \
    constexpr inline bool operator!=(u32 lhs, T rhs) { return lhs != u32(rhs); }         \
    constexpr inline T    operator|(T lhs, T rhs)    { return T(u32(lhs) | u32(rhs)); }  \
    constexpr inline T    operator|(T lhs, u32 rhs)  { return T(u32(lhs) | rhs); }       \
    constexpr inline T    operator|(u32 lhs, T rhs)  { return T(lhs | u32(rhs)); }       \
    constexpr inline T    operator&(T lhs, T rhs)    { return T(u32(lhs) & u32(rhs)); }  \
    constexpr inline T    operator&(T lhs, u32 rhs)  { return T(u32(lhs) & rhs); }       \
    constexpr inline T    operator&(u32 lhs, T rhs)  { return T(lhs & u32(rhs)); }       \
    constexpr inline T    operator^(T lhs, T rhs)    { return T(u32(lhs) ^ u32(rhs)); }  \
    constexpr inline T    operator^(T lhs, u32 rhs)  { return T(u32(lhs) ^ rhs); }       \
    constexpr inline T    operator^(u32 lhs, T rhs)  { return T(lhs ^ u32(rhs)); }       \
    constexpr inline T    operator~(T rhs)           { return T(~u32(rhs)); }            \
    constexpr inline T    operator<<(T lhs, T rhs)   { return T(u32(lhs) << u32(rhs)); } \
    constexpr inline T    operator<<(T lhs, u32 rhs) { return T(u32(lhs) << rhs); }      \
    constexpr inline T    operator<<(u32 lhs, T rhs) { return T(lhs << u32(rhs)); }      \
    constexpr inline T    operator>>(T lhs, T rhs)   { return T(u32(lhs) >> u32(rhs)); } \
    constexpr inline T    operator>>(T lhs, u32 rhs) { return T(u32(lhs) >> rhs); }      \
    constexpr inline T    operator>>(u32 lhs, T rhs) { return T(lhs >> u32(rhs)); }

} // namespace stlv
