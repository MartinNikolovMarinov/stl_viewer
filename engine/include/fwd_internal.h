#pragma once

#include <init_core.h>
#include <stlv_utils.h>

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

