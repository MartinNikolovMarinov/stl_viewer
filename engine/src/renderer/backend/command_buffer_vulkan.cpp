#include <application/logger.h>
#include <renderer/backend/renderer_vulkan.h>

namespace stlv {

void vulkanCommandBufferAllocate(RendererBackend& backend,
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
    allocInfo.pNext = nullptr;

    VK_EXPECT(
        vkAllocateCommandBuffers(backend.device.logicalDevice, &allocInfo, &outCmdBuffer.handle),
        "Failed to allocate command buffer"
    );
    outCmdBuffer.state = VulkanCommandBufferState::READY;
}

void vulkanCommandBufferFree(RendererBackend& backend, VkCommandPool pool, VulkanCommandBuffer& cmdBuffer) {
    vkFreeCommandBuffers(backend.device.logicalDevice, pool, 1, &cmdBuffer.handle);
    cmdBuffer.handle = VK_NULL_HANDLE;
    cmdBuffer.state = VulkanCommandBufferState::NOT_ALLOCATED;
}

void vulkanCommandBufferBegin(VulkanCommandBuffer& cmdBuffer,
                              bool isSingleUse,
                              bool isRenderpassContinue,
                              bool isSimultaneousUse) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.pInheritanceInfo = nullptr;

    // Set begin info flags:
    beginInfo.flags = 0;
    if (isSingleUse) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (isRenderpassContinue) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if (isSimultaneousUse) {
        beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VK_EXPECT(
        vkBeginCommandBuffer(cmdBuffer.handle, &beginInfo),
        "Failed to begin command buffer"
    );
    cmdBuffer.state = VulkanCommandBufferState::RECORDING;
}

void vulkanCommandBufferEnd(VulkanCommandBuffer& cmdBuffer) {
    VK_EXPECT(
        vkEndCommandBuffer(cmdBuffer.handle),
        "Failed to end command buffer"
    );
    cmdBuffer.state = VulkanCommandBufferState::RECORDING_ENDED;
}

void vulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer& cmdBuffer) {
    cmdBuffer.state = VulkanCommandBufferState::SUBMITTED;
}

void vulkanCommandBufferReset(VulkanCommandBuffer& cmdBuffer) {
    cmdBuffer.state = VulkanCommandBufferState::READY;
}

void vulkanCommandBufferAllocateAndBeginSingleUse(RendererBackend& backend,
                                                  VkCommandPool pool,
                                                  VulkanCommandBuffer& cmdBuffer) {
    vulkanCommandBufferAllocate(backend, pool, true, cmdBuffer);
    vulkanCommandBufferBegin(cmdBuffer, true, false, false);
}

void vulkanCommandBufferEndSingleUse(RendererBackend& backend,
                                     VkCommandPool pool,
                                     VulkanCommandBuffer& cmdBuffer,
                                     VkQueue queue) {
    vulkanCommandBufferEnd(cmdBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer.handle;

    VK_EXPECT(
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE),
        "Failed to submit single use command buffer"
    );

    VK_EXPECT(
        vkQueueWaitIdle(queue),
        "Failed to wait for queue to idle"
    );

    vulkanCommandBufferFree(backend, pool, cmdBuffer);
}

} // namespace stlv
