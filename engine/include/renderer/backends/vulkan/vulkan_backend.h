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

struct RendererBackend;
using VkSurfaceFormatKHRList = core::Arr<VkSurfaceFormatKHR, RendererBackendAllocator>;
using VkPresentModeKHRList = core::Arr<VkPresentModeKHR, RendererBackendAllocator>;
using VkImageList = core::Arr<VkImage, RendererBackendAllocator>;
using VkImageViewList = core::Arr<VkImageView, RendererBackendAllocator>;

struct VulkanSwapchainCreationInfo {
    u32 width;
    u32 height;
    u32 maxFramesInFlight;
};

struct VulkanSwapchain {
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR imageFormat;
    u32 maxFramesInFlight;

    u32 imageCount; // capabilities minImageCount + 1
    VkImageList images;
    VkImageViewList imageViews;
};

bool vulkanSwapchainCreate(RendererBackend& backend, VulkanSwapchain& swapchain, const VulkanSwapchainCreationInfo& createInfo);
void vulkanSwapchainDestroy(RendererBackend& backend, VulkanSwapchain& swapchain);
bool vulkanSwapchainRecreate(RendererBackend& backend, VulkanSwapchain& swapchain, const VulkanSwapchainCreationInfo& createInfo);
bool vulkanSwapchainAqureNextImage(
    RendererBackend& backend,
    VulkanSwapchain& swapchain,
    u64 timeoutNs,
    VkSemaphore imageAvailableSemaphore,
    VkFence fence,
    u32& outImgIdx
);
bool vulkanSwapchainPreset(
    RendererBackend& backend,
    VulkanSwapchain& swapchain,
    VkQueue graphicsQueue,
    VkQueue presentQueue,
    VkSemaphore renderCompleteSemaphore,
    u32 presentImageIdx
);

struct VulkanSwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHRList formats;
    VkPresentModeKHRList presentModes;
};

struct VulkanQueue {
    VkQueue handle;
    u32 familyIndex;
};

struct VulkanDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkFormat depthFormat;

    VulkanQueue graphicsQueue;
    VulkanQueue presentQueue;
    VulkanQueue transferQueue;
    VulkanQueue computeQueue;

    VulkanSwapchainSupportInfo swapchainSupportInfo;

    VkCommandPool graphicsCommandPool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkPhysicalDeviceFeatures features;
};

bool vulkanDeviceCreate(RendererBackend& device);
void vulkanDeviceDestroy(RendererBackend& device);
void vulkanDeviceQuerySwapchainSupport(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo& info);

struct RendererBackend {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    VulkanDevice device;

    u32 frameBufferWidth;
    u32 frameBufferHeight;

#if STLV_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

    VulkanSwapchain swapchain;
    VkRenderPass mainRenderPass;
};

} // namespace stlv
