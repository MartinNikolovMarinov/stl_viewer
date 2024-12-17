#pragma once

#if defined(OS_MAC) && OS_MAC == 1
    #define VK_USE_PLATFORM_METAL_EXT
#elif defined(OS_WIN) && OS_WIN == 1
    #define VK_USE_PLATFORM_WIN32_KHR
#elif defined(OS_LINUX) && OS_LINUX == 1
// TODO: This should define either wayland or x11 and detecting that is always fun...
#endif

#define VK_USE_PLATFORM_METAL_EXT
#include <vulkan/vulkan.h>
