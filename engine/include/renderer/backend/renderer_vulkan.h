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

using VkSurfaceFormatKHRList = core::Arr<VkSurfaceFormatKHR, RendererBackendAllocator>;
using VkPresentModeKHRList = core::Arr<VkPresentModeKHR, RendererBackendAllocator>;

struct VulkanSwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHRList formats;
    VkPresentModeKHRList presentModes;
};

bool querySwapchainSupport(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo& info);

struct VulkanDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    VulkanSwapchainSupportInfo swapchainSupportInfo;

    u32 graphicsQueueFamilyIdx;
    u32 computeQueueFamilyIdx;
    u32 transferQueueFamilyIdx;
    u32 presetQueueFamilyIdx;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;
};

struct RendererBackend {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
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
