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

enum struct CommandBufferState {
    NOT_ALLOCATED,
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED
};

struct VulkanCommandBuffer {
    VkCommandBuffer handle;
    CommandBufferState state;
};

enum struct VulkanRenderPassState {
    NOT_ALLOCATED,
    READY,
    RECORDING,
    IN_RENDER_PASS,
    SUBMITTED
};

struct VulkanRenderPass {
    VkRenderPass handle;
    f32 x, y, w, h;
    core::vec4f clearColor;

    f32 depth;
    u32 stencil;

    VulkanRenderPassState state;
};

bool vulkanCreateRenderPass(RendererBackend& backend,
    VulkanRenderPass& renderPass,
    f32 x, f32 y, f32 w, f32 h,
    f32 depth, u32 stencil,
    core::vec4f clearColor);
bool vulkanDestroyRenderPass(RendererBackend& backend, VulkanRenderPass& renderPass);
bool vulkanRenderPassBegin(VulkanCommandBuffer cmdBuffer, VulkanRenderPass& renderPass, VkFramebuffer frameBuffer);
bool vulkanRenderPassEnd(VulkanCommandBuffer cmdBuffer, VulkanRenderPass& renderPass);

struct VulkanImage {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
};

bool vulkanImageCreate(
    RendererBackend& backend,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties,
    bool createView,
    VkImageAspectFlags aspectFlags,
    VulkanImage& outImage);
bool vulkanImageViewCreate(
    RendererBackend& backend,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VulkanImage& outImage);
void vulkanDestroyImage(RendererBackend& backend, VulkanImage& image);

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

    VulkanImage depthImage;
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
    u32& outImgIdx);
bool vulkanSwapchainPreset(
    RendererBackend& backend,
    VulkanSwapchain& swapchain,
    VkQueue graphicsQueue,
    VkQueue presentQueue,
    VkSemaphore renderCompleteSemaphore,
    u32 presentImageIdx);

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
    VulkanRenderPass mainRenderPass;

    i32 findMemoryIndex(u32 typeFilter, u32 propertyFlags);
};

} // namespace stlv
