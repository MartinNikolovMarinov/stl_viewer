#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

bool vulkanCommandBufferAllocate(
    RendererBackend& backend,
    VkCommandPool pool,
    bool isPrimary,
    VulkanCommandBuffer& outCmdBuffer) {

    outCmdBuffer = {};
    outCmdBuffer.state = VulkanCommandBufferState::NOT_ALLOCATED;

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    VK_EXPECT_OR_RETURN(
        vkAllocateCommandBuffers(backend.device.logicalDevice, &allocInfo, &outCmdBuffer.handle),
        "Failed to allocate command buffers!"
    );

    outCmdBuffer.state = VulkanCommandBufferState::READY;

    return true;
}

void vulkanCommandBufferFree(RendererBackend& backend, VkCommandPool pool, VulkanCommandBuffer& cmdBuffer) {
    if (cmdBuffer.handle) {
        vkFreeCommandBuffers(backend.device.logicalDevice, pool, 1, &cmdBuffer.handle);
        cmdBuffer.handle = VK_NULL_HANDLE;
    }

    cmdBuffer.state = VulkanCommandBufferState::NOT_ALLOCATED;
}

bool vulkanCommandBufferBegin(
    VulkanCommandBuffer& cmdBuffer,
    bool isSingleUse,
    bool isRenderPassContinue,
    bool isSimultaneousUse
) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;

    if (isSingleUse) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (isRenderPassContinue) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if (isSimultaneousUse) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VK_EXPECT_OR_RETURN(
        vkBeginCommandBuffer(cmdBuffer.handle, &beginInfo),
        "Failed to begin recording command buffer!"
    );
    cmdBuffer.state = VulkanCommandBufferState::RECORDING;

    return true;
}

bool vulkanCommandBufferEnd(VulkanCommandBuffer& cmdBuffer) {
    VK_EXPECT_OR_RETURN(
        vkEndCommandBuffer(cmdBuffer.handle),
        "Failed to record command buffer!"
    );
    cmdBuffer.state = VulkanCommandBufferState::RECORDING_ENDED;
    return true;
}

bool vulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer& cmdBuffer) {
    cmdBuffer.state = VulkanCommandBufferState::SUBMITTED;
    return true;
}

bool vulkanCommandBufferReset(VulkanCommandBuffer& cmdBuffer) {
    cmdBuffer.state = VulkanCommandBufferState::READY;
    return true;
}

bool vulkanCommandBufferAllocateAndBeginSingleUse(
    RendererBackend& backend,
    VkCommandPool pool,
    VulkanCommandBuffer& outCmdBuffer) {

    vulkanCommandBufferAllocate(backend, pool, true, outCmdBuffer);
    vulkanCommandBufferBegin(outCmdBuffer, true, false, false);

    return true;
}

bool vulkanCommandBufferEndSingleUse(
    RendererBackend& backend,
    VkCommandPool pool,
    VulkanCommandBuffer& cmdBuffer,
    VkQueue queue) {

    vulkanCommandBufferEnd(cmdBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer.handle;

    VK_EXPECT_OR_RETURN(
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE),
        "Failed to submit single use command buffer!"
    );

    VK_EXPECT_OR_RETURN(
        vkQueueWaitIdle(queue),
        "Failed to wait for queue to become idle!"
    );

    vulkanCommandBufferFree(backend, pool, cmdBuffer);

    return true;
}

} // namespace stlv
