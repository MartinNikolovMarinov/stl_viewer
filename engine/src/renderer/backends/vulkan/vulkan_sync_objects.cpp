#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

void vulkanFenceCreate(RendererBackend& backend, bool isSignaled, VulkanFence& outFence) {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = outFence.isSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VK_EXPECT(
        vkCreateFence(backend.device.logicalDevice, &fenceCreateInfo, backend.allocator, &outFence.handle),
        "Failed to create fence"
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
    if (!fence.isSignaled) {
        VkResult result = vkWaitForFences(backend.device.logicalDevice, 1, &fence.handle, VK_TRUE, timeoutNs);
        if (result == VK_SUCCESS) {
            return true;
        }

        if (result == VK_TIMEOUT) {
            logWarnTagged(LogTag::T_RENDERER, "Fence wait timed out.");
        }
        else if (result == VK_ERROR_DEVICE_LOST) {
            logErrTagged(LogTag::T_RENDERER, "Fence wait failed due to device lost.");
        }
        else if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
            logErrTagged(LogTag::T_RENDERER, "Fence wait failed due to out of host memory.");
        }
        else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            logErrTagged(LogTag::T_RENDERER, "Fence wait failed due to out of device memory.");
        }
        else {
            logErrTagged(LogTag::T_RENDERER, "Fence wait failed due to unknown error.");
        }

        return false;
    }

    return true;
}

void vulkanFenceReset(RendererBackend& backend, VulkanFence& fence) {
    if (fence.isSignaled) {
        VK_EXPECT(
            vkResetFences(backend.device.logicalDevice, 1, &fence.handle),
            "Failed to reset fence"
        );
        fence.isSignaled = false;
    }
}

} // namespace stlv
