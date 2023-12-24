#pragma once

#include <fwd_internal.h>
#include <application/logger.h>

#include <vulkan/vulkan.h>

namespace stlv {

bool vulkanResultIsSuccess(VkResult result);
const char* vulkanResultToCptr(VkResult result, bool extended = true);

#define VK_EXPECT_OR_RETURN(expr, msg)                                 \
{                                                                      \
    if (VkResult res = (expr); !vulkanResultIsSuccess(res)) {          \
        logErrTagged(LogTag::T_RENDERER,                               \
                    "Vulkan expect failed: %s, result error code: %d", \
                    (msg), vulkanResultToCptr(res, true));             \
        return false;                                                  \
    }                                                                  \
}

#define VK_EXPECT(expr, msg)                                            \
{                                                                       \
    if (VkResult res = (expr); !vulkanResultIsSuccess(res)) {           \
        logFatalTagged(LogTag::T_RENDERER,                              \
                      "Vulkan check failed: %s, result error code: %d", \
                      (msg), vulkanResultToCptr(res, true));            \
        Panic(res == VK_SUCCESS, (msg));                                \
    }                                                                   \
}

struct RendererBackend {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

    u32 frameBufferWidth;
    u32 frameBufferHeight;

#if STLV_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

};

} // namespace stlv
