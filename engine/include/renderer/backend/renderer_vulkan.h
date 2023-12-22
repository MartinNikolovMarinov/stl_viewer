#pragma once

#include <fwd_internal.h>
#include <application/logger.h>

#include <vulkan/vulkan.h>

namespace stlv {

#define VK_EXPECT(expr, msg)                                           \
    if (VkResult res = (expr); res != VK_SUCCESS) {                    \
        logErrTagged(LogTag::T_RENDERER,                               \
                    "Vulkan expect failed: %s, result error code: %d", \
                    (msg), i32(res));                                  \
        return false;                                                  \
    }

#define VK_CHECK(expr, msg)                                             \
    if (VkResult res = (expr); res != VK_SUCCESS) {                     \
        logFatalTagged(LogTag::T_RENDERER,                              \
                      "Vulkan check failed: %s, result error code: %d", \
                      (msg), i32(res));                                 \
        Panic(res == VK_SUCCESS, (msg));                                \
    }

using VkSurfaceFormatKHRList = core::Arr<VkSurfaceFormatKHR, RendererBackendAllocator>;
using VkPresentModeKHRList = core::Arr<VkPresentModeKHR, RendererBackendAllocator>;

struct VulkanSwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHRList formats;
    VkPresentModeKHRList presentModes;
};

struct VulkanDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    VulkanSwapchainSupportInfo swapchainSupportInfo;
    VkFormat depthFormat;

    u32 graphicsQueueFamilyIdx;
    u32 computeQueueFamilyIdx;
    u32 transferQueueFamilyIdx;
    u32 presentQueueFamilyIdx;

    VkQueue graphicsQueue;
    VkQueue transferQueue;
    VkQueue presentQueue;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;
};

struct RendererBackend;
bool vulkanDeviceCreate(RendererBackend& backend);
void vulcanDeviceDestroy(RendererBackend& backend);
bool vulkanDeviceQuerySwapchainSupport(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo& info);
bool vulkanDeviceDetectDepthFormat(VulkanDevice& device);

struct VulkanImage {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
};

void vulkanImageCreate(RendererBackend& backend, u32 width, u32 height, VkFormat format,
                       VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags,
                       bool createView, VkImageAspectFlags viewAspectFlags,
                       VulkanImage& outImage);
void vulkanImageViewCreate(RendererBackend& backend, VkFormat format,
                           VulkanImage& image, VkImageAspectFlags aspectFlags);
void vulkanImageDestroy(RendererBackend& backend, VulkanImage& image);

struct VulkanSwapchain {
    VkSurfaceFormatKHR imageFormat;
    u32 maxFramesInFlight;
    VkSwapchainKHR handle;
    u32 imageCount;
    VkImage* images;
    VkImageView* imageViews;

    VulkanImage depthAttachment;
};

void vulkanSwapchainCreate(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& swapchain);
void vulkanSwapchainRecreate(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& swapchain);
void vulkanSwapchainDestroy(RendererBackend& backend, VulkanSwapchain& swapchain);
bool vulkanSwapchainAcquireNextImageIdx(RendererBackend& backend, VulkanSwapchain& swapchain, u64 timeoutNs,
                                        VkSemaphore imageAvailableSemaphore, VkFence fence, u32& imageIdx);
void vulkanSwapchainPresent(RendererBackend& backend, VulkanSwapchain& swapchain,
                            VkQueue graphicsQueue, VkQueue presentQueue,
                            VkSemaphore renderCompleteSemaphore, u32 imageIdx);

enum struct VulkanCommandBufferState {
    NOT_ALLOCATED = 0,

    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,

    SENTINEL
};

struct VulkanCommandBuffer {
    VkCommandBuffer handle;
    VulkanCommandBufferState state;
};

enum struct VulkanRenderPassState {
    NOT_ALLOCATED = 0,

    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,

    SENTINEL
};

struct VulkanRenderPass {
    VkRenderPass handle;
    f32 x, y, w, h;
    core::vec4f clearColor;
    f32 depth;
    u32 stencil;

    VulkanRenderPassState state;
};

void vulkanRenderpassCreate(RendererBackend& backend,
                            VulkanRenderPass& renderPass,
                            f32 x, f32 y, f32 w, f32 h,
                            const core::vec4f& clearColor,
                            f32 depth, u32 stencil);
void vulkanRenderpassDestroy(RendererBackend& backend, VulkanRenderPass& renderPass);
void vulkanRenderpassBegin(VulkanRenderPass& renderPass, VulkanCommandBuffer& cmdBuffer, VkFramebuffer framebuffer);
void vulkanRenderpassEnd(VulkanRenderPass& renderPass, VulkanCommandBuffer& cmdBuffer);

struct RendererBackend {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

    u32 framebufferWidth;
    u32 framebufferHeight;

#if STLV_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

    VulkanDevice device;

    VulkanSwapchain swapchain;
    u32 imageIndx;
    u32 currentFrame;
    bool recreatingSwapchain;
    VulkanRenderPass mainRenderPass;

    i32 findMemoryTypeIndex(u32 memoryTypeBits, VkMemoryPropertyFlags memoryFlags);
};

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
