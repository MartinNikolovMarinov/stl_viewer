#include <renderer/backend/renderer_vulkan.h>

namespace stlv {

void vulkanFrameBufferCreate(RendererBackend& backend,
                             VulkanRenderPass& renderPass,
                             u32 width, u32 height,
                             const VkImageView* attachments, u32 attachmentCount,
                             VulkanFrameBuffer& outFrameBuffer) {

    outFrameBuffer.renderPass = &renderPass;
    outFrameBuffer.attachmentCount = attachmentCount;
    outFrameBuffer.attachments = reinterpret_cast<VkImageView*>(
        RendererBackendAllocator::alloc(sizeof(VkImageView) * attachmentCount));
    core::memcopy(outFrameBuffer.attachments, attachments, sizeof(VkImageView) * attachmentCount);

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass.handle;
    framebufferInfo.attachmentCount = attachmentCount;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;
    framebufferInfo.flags = 0;
    framebufferInfo.pNext = nullptr;

    VK_EXPECT(
        vkCreateFramebuffer(backend.device.logicalDevice, &framebufferInfo, nullptr, &outFrameBuffer.handle),
        "Failed to create framebuffer"
    );
}

void vulkanFrameBufferDestroy(RendererBackend& backend, VulkanFrameBuffer& frameBuffer) {
    vkDestroyFramebuffer(backend.device.logicalDevice, frameBuffer.handle, nullptr);
    if (frameBuffer.attachments) {
        RendererBackendAllocator::free(frameBuffer.attachments);
        frameBuffer.attachments = nullptr;
    }
    frameBuffer.renderPass = nullptr;
    frameBuffer.attachmentCount = 0;
    frameBuffer.renderPass = nullptr;
}

} // namespace stlv
