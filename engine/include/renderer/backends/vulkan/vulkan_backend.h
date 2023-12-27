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
struct VulkanRenderPass;
struct VulkanFrameBuffer;
struct VulkanFence;
using VkSurfaceFormatKHRList = core::Arr<VkSurfaceFormatKHR, RendererBackendAllocator>;
using VkPresentModeKHRList = core::Arr<VkPresentModeKHR, RendererBackendAllocator>;
using VkImageList = core::Arr<VkImage, RendererBackendAllocator>;
using VkImageViewList = core::Arr<VkImageView, RendererBackendAllocator>;
using VulkanCommandBufferList = core::Arr<VulkanCommandBuffer, RendererBackendAllocator>;
using VulkanFrameBufferList = core::Arr<VulkanFrameBuffer, RendererBackendAllocator>;
using VkSemaphoreList = core::Arr<VkSemaphore, RendererBackendAllocator>;
using VulkanFenceList = core::Arr<VulkanFence, RendererBackendAllocator>;
using ImagesInFlight = core::Arr<VulkanFence*, RendererBackendAllocator>;

struct VulkanFence {
    VkFence handle;
    bool isSignaled;
};

bool vulkanFenceCreate(RendererBackend& backend, bool isSignaled, VulkanFence& outFence);
void vulkanFenceDestroy(RendererBackend& backend, VulkanFence& fence);
bool vulkanFenceWait(RendererBackend& backend, VulkanFence& fence, u64 timeoutNs);
bool vulkanFenceReset(RendererBackend& backend, VulkanFence& fence);

struct VulkanFrameBuffer {
    VkFramebuffer handle;
    u32 attachmentCount;
    VkImageViewList attachments;
    VulkanRenderPass* renderPass;
};

bool vulkanFrameBufferCreate(
    RendererBackend& backend,
    VulkanRenderPass& renderPass,
    u32 width, u32 height,
    u32 attachmentCount,
    const VkImageView* attachments,
    VulkanFrameBuffer& outFrameBuffer
);
void vulkanFrameBufferDestroy(RendererBackend& backend, VulkanFrameBuffer& frameBuffer);

enum struct VulkanCommandBufferState {
    NOT_ALLOCATED,
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED
};

struct VulkanCommandBuffer {
    VkCommandBuffer handle;
    VulkanCommandBufferState state;
};

bool vulkanCommandBufferAllocate(
    RendererBackend& backend,
    VkCommandPool pool,
    bool isPrimary,
    VulkanCommandBuffer& outCmdBuffer);
void vulkanCommandBufferFree(RendererBackend& backend, VkCommandPool pool, VulkanCommandBuffer& cmdBuffer);
bool vulkanCommandBufferBegin(
    VulkanCommandBuffer& cmdBuffer,
    bool isSingleUse,
    bool isRenderPassContinue,
    bool isSimultaneousUse
);
bool vulkanCommandBufferEnd(VulkanCommandBuffer& cmdBuffer);
bool vulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer& cmdBuffer);
bool vulkanCommandBufferReset(VulkanCommandBuffer& cmdBuffer);
bool vulkanCommandBufferAllocateAndBeginSingleUse(
    RendererBackend& backend,
    VkCommandPool pool,
    VulkanCommandBuffer& outCmdBuffer);
bool vulkanCommandBufferEndSingleUse(
    RendererBackend& backend,
    VkCommandPool pool,
    VulkanCommandBuffer& cmdBuffer,
    VkQueue queue);

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

    VulkanFrameBufferList frameBuffers;

    VulkanImage depthImage;
};

bool vulkanSwapchainCreate(RendererBackend& backend, VulkanSwapchain& swapchain, const VulkanSwapchainCreationInfo& createInfo);
void vulkanSwapchainDestroy(RendererBackend& backend, VulkanSwapchain& swapchain);
bool vulkanSwapchainRecreate(RendererBackend& backend, VulkanSwapchain& swapchain, const VulkanSwapchainCreationInfo& createInfo);
bool vulkanSwapchainAcquireNextImageIdx(
    RendererBackend& backend,
    VulkanSwapchain& swapchain,
    u64 timeoutNs,
    VkSemaphore imageAvailableSemaphore,
    VkFence fence,
    u32& outImgIdx);
bool vulkanSwapchainPresent(
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
bool vulkanDeviceDetectDepthFormat(VkPhysicalDevice pdevice, VkFormat& outFormat);

struct RendererBackend {
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;
    VulkanDevice device;

    u32 frameBufferWidth;
    u32 frameBufferHeight;
    u64 frameBufferSizeGen;
    u64 frameBufferSizeLastGen;
    bool recreatingSwapchain;

#if STLV_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
#endif

    VulkanSwapchain swapchain;
    VulkanRenderPass mainRenderPass;
    VulkanCommandBufferList graphicsCommandBuffers;

    u32 currentFrame;
    u32 imageIdx;

    VkSemaphoreList imageAvailableSemaphores;
    VkSemaphoreList queueCompleteSemaphores;
    u32 inFlightFenceCount;
    VulkanFenceList inFlightFences;
    ImagesInFlight imagesInFlight;

    i32 findMemoryIndex(u32 typeFilter, u32 propertyFlags);
};

} // namespace stlv
