#include <renderer/backend/renderer_vulkan.h>

namespace stlv {

void vulkanFenceCreate(RendererBackend& backend, bool isSignaled, VulkanFence& outFence) {
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = isSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VK_EXPECT(
        vkCreateFence(backend.device.logicalDevice, &fenceInfo, backend.allocator, &outFence.handle),
        "Failed to create Vulkan fence."
    );
    outFence.isSignaled = isSignaled;
}

void vulkanFenceDestroy(RendererBackend& backend, VulkanFence& fence) {
    if (fence.handle) {
        vkDestroyFence(backend.device.logicalDevice, fence.handle, backend.allocator);
        fence.handle = VK_NULL_HANDLE;
    }
    fence.isSignaled = false;
}

bool vulkanFenceWait(RendererBackend& backend, VulkanFence& fence, u64 timeoutNs) {
    if (fence.isSignaled) {
        return true;
    }

    VkResult result = vkWaitForFences(backend.device.logicalDevice, 1, &fence.handle, VK_TRUE, timeoutNs);

    if (result == VK_SUCCESS) {
        fence.isSignaled = true;
        return true;
    }

    if (result == VK_TIMEOUT) {
        logWarnTagged(LogTag::T_RENDERER, "Fence wait timed out.");
    }
    else if (result == VK_ERROR_DEVICE_LOST) {
        logErrTagged(LogTag::T_RENDERER, "Device Lost!");
    }
    else if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
        logErrTagged(LogTag::T_RENDERER, "Out of Host Memory!");
    }
    else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        logErrTagged(LogTag::T_RENDERER, "Out of Device Memory!");
    }
    else {
        logErrTagged(LogTag::T_RENDERER, "Unknown error!");
    }

    return false;
}

void vulkanFenceReset(RendererBackend& backend, VulkanFence& fence) {
    if (fence.isSignaled) {
        VK_EXPECT(
            vkResetFences(backend.device.logicalDevice, 1, &fence.handle),
            "Failed to reset Vulkan fence."
        );
        fence.isSignaled = false;
    }
}

} // namespace stlv
