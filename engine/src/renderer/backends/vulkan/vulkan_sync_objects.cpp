#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

bool vulkanFenceCreate(RendererBackend& backend, bool isSignaled, VulkanFence& outFence) {
    VkFenceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = isSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VK_EXPECT_OR_RETURN(
        vkCreateFence(backend.device.logicalDevice, &createInfo, backend.allocator, &outFence.handle),
        "Failed to create Vulkan fence"
    );

    return true;
}

void vulkanFenceDestroy(RendererBackend& backend, VulkanFence& fence) {
    if (fence.handle) {
        vkDestroyFence(backend.device.logicalDevice, fence.handle, backend.allocator);
        fence.handle = VK_NULL_HANDLE;
    }
}

bool vulkanFenceWait(RendererBackend& backend, VulkanFence& fence, u64 timeoutNs) {
    if (!fence.isSignaled) {
        VkResult result = vkWaitForFences(backend.device.logicalDevice, 1, &fence.handle, VK_TRUE, timeoutNs);

        if (result == VK_SUCCESS) {
            fence.isSignaled = true;
            return true;
        }

        if (result == VK_TIMEOUT) {
            logWarnTagged(LogTag::T_RENDERER, "Vulkan fence wait timed out");
            return false;
        }

        // Rest of the result codes are treated as errors.
        VK_EXPECT_OR_RETURN(result, "Failed to wait for Vulkan fence");
        return false;
    }

    return true;
}

bool vulkanFenceReset(RendererBackend& backend, VulkanFence& fence) {
    VK_EXPECT_OR_RETURN(
        vkResetFences(backend.device.logicalDevice, 1, &fence.handle),
        "Failed to reset Vulkan fence"
    );

    return true;
}

} // namespace stlv
