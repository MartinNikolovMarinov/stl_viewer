#include <application/logger.h>
#include <renderer/backend/renderer_vulkan.h>

namespace stlv {

void vulkanRenderpassCreate(RendererBackend& backend,
                            VulkanRenderPass& renderPass,
                            f32 x, f32 y, f32 w, f32 h,
                            const core::vec4f& clearColor,
                            f32 depth, u32 stencil) {

    logInfoTagged(LogTag::T_RENDERER, "Vulkan creating render pass...");

    renderPass.x = x;
    renderPass.y = y;
    renderPass.w = w;
    renderPass.h = h;
    renderPass.clearColor = clearColor;
    renderPass.depth = depth;
    renderPass.stencil = stencil;

    // Main subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Attachments
    u32 attachmentDescriptionsCount = 2;
    VkAttachmentDescription attachmentDescriptions[attachmentDescriptionsCount] = {};

    // TODO: Add other attachments types here, like input, resolve, preserve, etc.

    // Color attachment
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = backend.swapchain.imageFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the framebuffer before rendering.
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the framebuffer after rendering.
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Don't care about stencil load.
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Don't care about stencil store.
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Don't care about the initial layout.
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Transitioned to after the render pass.
    colorAttachment.flags = 0;

    // Set the color attachment description in the array
    attachmentDescriptions[0] = colorAttachment;

    // Color attachment reference
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // Index of the attachment in the attachment descriptions array.
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Set the color attachment reference in the subpass
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Depth attachment
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = backend.device.depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the framebuffer before rendering.
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Don't care about the framebuffer after rendering.
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Don't care about stencil load.
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Don't care about stencil store.
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Don't care about the initial layout.
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Transitioned to after the render pass.

    // Set the depth attachment description in the array
    attachmentDescriptions[1] = depthAttachment;

    // Depth attachment reference
    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1; // Index of the attachment in the attachment descriptions array.
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Set the depth attachment reference in the subpass
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Input from a shader
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;

    // Resolve attachments used for multisampling color attachments
    subpass.pResolveAttachments = nullptr;

    // Attachments not used in this subpass, but must be preserved for the next:
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    // Render pass dependencies.
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    // Render pass create info.
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = attachmentDescriptionsCount;
    renderPassCreateInfo.pAttachments = attachmentDescriptions;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;

    VK_EXPECT(
        vkCreateRenderPass(backend.device.logicalDevice, &renderPassCreateInfo, backend.allocator, &renderPass.handle),
        "Failed to create render pass."
    );
}

void vulkanRenderpassDestroy(RendererBackend& backend, VulkanRenderPass& renderPass) {
    logInfoTagged(LogTag::T_RENDERER, "Vulkan destroying render pass.");

    if (renderPass.handle) {
        vkDestroyRenderPass(backend.device.logicalDevice, renderPass.handle, backend.allocator);
        renderPass.handle = VK_NULL_HANDLE;
    }
}

void vulkanRenderpassBegin(VulkanRenderPass& renderPass, VulkanCommandBuffer& cmdBuffer, VkFramebuffer framebuffer) {
    VkClearValue clearValues[2] = {};
    clearValues[0].color = {
        renderPass.clearColor.r(),
        renderPass.clearColor.g(),
        renderPass.clearColor.b(),
        renderPass.clearColor.a()
    };
    clearValues[1].depthStencil = { renderPass.depth, renderPass.stencil };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass.handle;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea.offset = { i32(renderPass.x) , i32(renderPass.y) };
    renderPassBeginInfo.renderArea.extent = { u32(renderPass.w), u32(renderPass.h) };
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmdBuffer.handle, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer.state = VulkanCommandBufferState::IN_RENDER_PASS;
}

void vulkanRenderpassEnd(VulkanRenderPass&, VulkanCommandBuffer& cmdBuffer) {
    vkCmdEndRenderPass(cmdBuffer.handle);
    cmdBuffer.state = VulkanCommandBufferState::RECORDING;
}

} // namespace stlv
