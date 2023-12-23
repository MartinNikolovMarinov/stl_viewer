#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

void vulkanCommandBufferAllocate(RendererBackend& backend,
                                 VkCommandPool pool,
                                 bool isPrimary,
                                 VulkanCommandBuffer& outCommandBuffer) {
    outCommandBuffer = {};

    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = pool;
    allocateInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocateInfo.commandBufferCount = 1;
    allocateInfo.pNext = 0;

    outCommandBuffer.state = VulkanCommandBufferState::NOT_ALLOCATED;
    VK_EXPECT(
        vkAllocateCommandBuffers(backend.device.logicalDevice, &allocateInfo, &outCommandBuffer.handle),
        "Failed to allocate command buffer"
    );
    outCommandBuffer.state = VulkanCommandBufferState::READY;
}

void vulkanCommandBufferFree(RendererBackend& backend,
                             VkCommandPool pool,
                             VulkanCommandBuffer& commandBuffer) {
    vkFreeCommandBuffers(backend.device.logicalDevice, pool, 1, &commandBuffer.handle);
    commandBuffer = {};
    commandBuffer.handle = VK_NULL_HANDLE;
    commandBuffer.state = VulkanCommandBufferState::NOT_ALLOCATED;
}

void vulkanCommandBufferBegin(VulkanCommandBuffer& commandBuffer,
                              bool isSingleUse,
                              bool isRenderpassContinue,
                              bool isSimultaneousUse) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
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
        vkBeginCommandBuffer(commandBuffer.handle, &beginInfo),
        "Failed to begin command buffer"
    );
    commandBuffer.state = VulkanCommandBufferState::RECORDING;
}

void vulkanCommandBufferEnd(VulkanCommandBuffer& commandBuffer) {
    VK_EXPECT(vkEndCommandBuffer(commandBuffer.handle), "Failed to end command buffer");
    commandBuffer.state = VulkanCommandBufferState::RECORDING_ENDED;
}

void vulkanCommandBufferUpdateSubmitted(VulkanCommandBuffer& commandBuffer) {
    commandBuffer.state = VulkanCommandBufferState::SUBMITTED;
}

void vulkanCommandBufferReset(VulkanCommandBuffer& commandBuffer) {
    // VK_EXPECT(vkResetCommandBuffer(commandBuffer.handle, 0), "Failed to reset command buffer");
    commandBuffer.state = VulkanCommandBufferState::READY;
}

void vulkanCommandBufferAllocateAndBeginSingleUse(RendererBackend& backend,
                                                  VkCommandPool pool,
                                                  VulkanCommandBuffer& outCommandBuffer) {
    vulkanCommandBufferAllocate(backend, pool, true, outCommandBuffer);
    vulkanCommandBufferBegin(outCommandBuffer, true, false, false);
}

void vulkanCommandBufferEndSingleUse(RendererBackend& backend,
                                     VkCommandPool pool,
                                     VulkanCommandBuffer& commandBuffer,
                                     VkQueue queue) {
    // End the command buffer
    vulkanCommandBufferEnd(commandBuffer);

    // Submit it to the queue
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer.handle;
    VK_EXPECT(
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE),
        "Failed to submit command buffer"
    );

    // Wait for the queue to finish
    VK_EXPECT(
        vkQueueWaitIdle(queue),
        "Failed to wait for queue to finish"
    );

    // Free the command buffer
    vulkanCommandBufferFree(backend, pool, commandBuffer);
}

} // namespace stlv
