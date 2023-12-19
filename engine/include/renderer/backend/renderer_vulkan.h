#pragma once

#include <fwd_internal.h>
#include <application/logger.h>

#include <vulkan/vulkan.h>

namespace stlv {

#define VK_EXPECT(expr, msg)                     \
    if ((expr) != VK_SUCCESS) {                  \
        logErrTagged(LogTag::T_RENDERER, (msg)); \
        return false;                            \
    }

using RequiredValidationLayers = core::Arr<const char*, RendererBackendAllocator>;

struct VulkanDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
};

struct RendererBackend {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    ExtensionNames requiredExtensions;
    RequiredValidationLayers requiredValidationLayers;
    VkSurfaceKHR surface;

#if STLV_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

    VulkanDevice device;
};

bool createVulkanDevice(RendererBackend& backend);
void destroyVulkanDevice(RendererBackend& backend);

#if STLV_DEBUG
VkResult call_vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pMessenger);
void call_vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                         VkDebugUtilsMessengerEXT messenger,
                                         const VkAllocationCallbacks* pAllocator);
#endif

} // namespace stlv
