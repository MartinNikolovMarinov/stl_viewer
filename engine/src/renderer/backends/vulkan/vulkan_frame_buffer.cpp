#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

void vulkanFrameBufferCreate(RendererBackend& backend,
                             VulkanRenderPass& renderPass,
                             u32 width, u32 height,
                             u32 attachmentCount,
                             const VkImageView* attachments,
                             VulkanFrameBuffer& outFramebuffer) {

    // Take a copy of the attachments, renderPass and attachment count
    outFramebuffer.attachments = reinterpret_cast<VkImageView*>(
        RendererBackendAllocator::alloc(sizeof(VkImageView) * attachmentCount));
    for (u32 i = 0; i < attachmentCount; ++i) {
        outFramebuffer.attachments[i] = attachments[i];
    }
    outFramebuffer.renderPass = &renderPass;
    outFramebuffer.attachmentCount = attachmentCount;

    // Creation info
    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = renderPass.handle;
    framebufferCreateInfo.attachmentCount = attachmentCount;
    framebufferCreateInfo.pAttachments = outFramebuffer.attachments;
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;

    VK_EXPECT(
        vkCreateFramebuffer(backend.device.logicalDevice,
                            &framebufferCreateInfo,
                            backend.allocator,
                            &outFramebuffer.handle),
        "Failed to create framebuffer"
    );
}

void vulkanFrameBufferDestroy(RendererBackend& backend, VulkanFrameBuffer& framebuffer) {
    vkDestroyFramebuffer(backend.device.logicalDevice, framebuffer.handle, backend.allocator);
    if (framebuffer.attachments) {
        RendererBackendAllocator::free(framebuffer.attachments);
        framebuffer.attachments = nullptr;
    }
    framebuffer.handle = VK_NULL_HANDLE;
    framebuffer.attachmentCount = 0;
    framebuffer.renderPass = nullptr;
}

} // namespace stlv
