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
struct VulkanCommandBuffer;
using VkSurfaceFormatKHRList = core::Arr<VkSurfaceFormatKHR, RendererBackendAllocator>;
using VkPresentModeKHRList = core::Arr<VkPresentModeKHR, RendererBackendAllocator>;
using VulkanCommandBufferList = core::Arr<VulkanCommandBuffer, RendererBackendAllocator>;

struct VulkanSwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHRList formats;
    VkPresentModeKHRList presentModes;
};

struct VulkanQueue {
    VkQueue handle = VK_NULL_HANDLE;
    u32 familyIdx = core::MAX_U32;
};

struct VulkanDevice {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    VulkanSwapchainSupportInfo swapchainSupportInfo;
    VkFormat depthFormat;

    VulkanQueue graphicsQueue;
    VulkanQueue transferQueue;
    VulkanQueue presentQueue;
    VulkanQueue computeQueue;

    VkCommandPool graphicsCmdPool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;
};

struct RendererBackend;
bool vulkanDeviceCreate(RendererBackend& backend);
void vulkanDeviceDestroy(RendererBackend& backend);
void vulkanDeviceQuerySwapchainSupport(VkPhysicalDevice pdevice, VkSurfaceKHR surface, VulkanSwapchainSupportInfo& info);
bool vulkanDeviceDetectDepthFormat(VulkanDevice& device);

struct VulkanImage {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
};

void vulkanImageCreate(RendererBackend& backend,
                       u32 width, u32 height,
                       VkFormat format,
                       VkImageTiling tiling,
                       VkImageUsageFlags usage,
                       VkMemoryPropertyFlags memoryFlags,
                       bool createView,
                       VkImageAspectFlags viewAspectFlags,
                       VulkanImage& outImage);
void vulkanImageViewCreate(RendererBackend& backend,
                           VkFormat format,
                           VkImageAspectFlags aspectFlags,
                           VulkanImage& outImage);
void vulkanImageDestroy(RendererBackend& backend, VulkanImage& image);

struct VulkanSwapchain {
    VkSurfaceFormatKHR imageFormat;
    u32 maxFramesInFlight;
    VkSwapchainKHR handle;

    u32 imageCount;
    VkImage* images;
    VkImageView* imageViews;

    VulkanImage depthAttachment;

    // VulkanFrameBuffer* frameBuffers; // FIXME: Uncomment this
};

void vulkanSwapchainCreate(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& outSwapchain);
void vulkanSwapchainRecreate(RendererBackend& backend, u32 width, u32 height, VulkanSwapchain& outSwapchain);
void vulkanSwapchainDestroy(RendererBackend& backend, VulkanSwapchain& swapchain);
bool vulkanSwapchainAcquireNextImage(RendererBackend& backend,
                                     VulkanSwapchain& swapchain,
                                     u64 timeoutNs,
                                     VkSemaphore imageAvailableSemaphore,
                                     VkFence fence,
                                     u32& outImageIdx);
void vulkanSwapchainPresent(RendererBackend& backend,
                            VulkanSwapchain& swapchain,
                            VulkanQueue& graphicsQueue,
                            VulkanQueue& presentQueue,
                            VkSemaphore renderCompleteSemaphore,
                            u32 presentImageIdx);

enum struct VulkanCommandBufferState {
    NOT_ALLOCATED,

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

void vulkanCommandBufferAllocate(RendererBackend& backend,
                                 VkCommandPool pool,
                                 bool isPrimary,
                                 VulkanCommandBuffer& outCommandBuffer);
void vulkanCommandBufferFree(RendererBackend& backend,
                             VkCommandPool pool,
                             VulkanCommandBuffer& commandBuffer);
void vulkanCommandBufferBegin(VulkanCommandBuffer& commandBuffer,
                              bool isSingleUse,
                              bool isRenderpassContinue,
                              bool isSimultaneousUse);
void vulkanCommandBufferEnd(VulkanCommandBuffer& commandBuffer);
void vulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer& commandBuffer);
void vulkanCommandBufferReset(VulkanCommandBuffer& commandBuffer);
void vulkanCommandBufferAllocateAndBeginSingleUse(RendererBackend& backend,
                                                  VkCommandPool pool,
                                                  VulkanCommandBuffer& outCommandBuffer);
void vulkanCommandBufferEndSingleUse(RendererBackend& backend,
                                     VkCommandPool pool,
                                     VulkanCommandBuffer& commandBuffer,
                                     VkQueue queue);

enum struct VulkanRenderPassState {
    NONE,

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

void vulkanRenderPassCreate(RendererBackend& backend,
                            VulkanRenderPass& outRenderPass,
                            f32 x, f32 y, f32 width, f32 height,
                            core::vec4f clearColor,
                            f32 depth,
                            u32 stencil);
void vulkanRenderPassDestroy(RendererBackend& backend, VulkanRenderPass& renderPass);
void vulkanRenderPassBegin(VulkanRenderPass& renderPass, VulkanCommandBuffer& cmdBuffer, VkFramebuffer framebuffer);
void vulkanRenderPassEnd(VulkanRenderPass& renderPass, VulkanCommandBuffer& cmdBuffer);

struct RendererBackend {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

    u32 frameBufferWidth;
    u32 frameBufferHeight;
    u64 frameBufferSizeGen;
    u64 frameBufferSizeGenLast;

#if STLV_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

    VulkanDevice device;

    VulkanSwapchain swapchain;
    VulkanRenderPass mainRenderPass;
    u32 imageIdx;
    u64 currentFrame;

    VulkanCommandBufferList graphicsCmdBuffers;

    i32 findMemoryTypeIndex(u32 typeFilter, u32 propertyFlags);
};

} // namespace stlv
