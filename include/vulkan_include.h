#pragma once

#include <core_system_checks.h>

#if defined(OS_MAC) && OS_MAC == 1
    #define VK_USE_PLATFORM_METAL_EXT
    #include <vulkan/vulkan.h>
#elif defined(OS_WIN) && OS_WIN == 1
    #define VK_USE_PLATFORM_WIN32_KHR
    #define WIN32_LEAN_AND_MEAN // vulkan.h might import windows.h
    #include <vulkan/vulkan.h>
#elif defined(OS_LINUX) && OS_LINUX == 1
    // FIXME: This should define either wayland or x11 and detecting that is always fun... Currently only x11 working.
    #define VK_USE_PLATFORM_XLIB_KHR
    #include <vulkan/vulkan.h>
#endif
