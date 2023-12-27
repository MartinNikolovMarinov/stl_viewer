#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

bool vulkanFrameBufferCreate(
    RendererBackend& backend,
    VulkanRenderPass& renderPass,
    u32 width, u32 height,
    u32 attachmentCount,
    const VkImageView* attachments,
    VulkanFrameBuffer& outFrameBuffer
) {
    outFrameBuffer.renderPass = &renderPass;
    outFrameBuffer.attachmentCount = attachmentCount;
    outFrameBuffer.attachments.fill(VK_NULL_HANDLE, 0, attachmentCount); // Copy the attachments array.
    for (u32 i = 0; i < attachmentCount; ++i) {
        outFrameBuffer.attachments[i] = attachments[i];
    }

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = renderPass.handle;
    createInfo.attachmentCount = attachmentCount;
    createInfo.pAttachments = outFrameBuffer.attachments.data();
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    VK_EXPECT_OR_RETURN(
        vkCreateFramebuffer(backend.device.logicalDevice, &createInfo, backend.allocator, &outFrameBuffer.handle),
        "Failed to create Vulkan framebuffer"
    );

    return true;
}

void vulkanFrameBufferDestroy(RendererBackend& backend, VulkanFrameBuffer& frameBuffer) {
    if (frameBuffer.handle) {
        vkDestroyFramebuffer(backend.device.logicalDevice, frameBuffer.handle, backend.allocator);
        frameBuffer.handle = VK_NULL_HANDLE;
    }
    frameBuffer.attachmentCount = 0;
    frameBuffer.attachments.clear();
    frameBuffer.renderPass = nullptr;
}

} // namespace stlv
