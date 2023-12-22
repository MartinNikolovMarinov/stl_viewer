#pragma once

#include <init_core.h>

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

#ifdef STLV_LIBRARY_BUILD
    #if defined(_MSC_VER)
        #define STLV_EXPORT __declspec(dllexport)
    #elif defined(__GNUC__) || defined(__GNUG__) || defined(__clang__)
        #define STLV_EXPORT __attribute__((visibility("default")))
    #else
        #define STLV_EXPORT
    #endif
#else
    #ifdef _WIN32
        #define STLV_EXPORT __declspec(dllimport)
    #else
        #define STLV_EXPORT
    #endif
#endif

namespace stlv {

constexpr addr_size VULKAN_MAX_INSTANCE_EXTENSIONS = 8;
using ExtensionNames = core::SArr<const char*, VULKAN_MAX_INSTANCE_EXTENSIONS>;

} // namespace stlv

