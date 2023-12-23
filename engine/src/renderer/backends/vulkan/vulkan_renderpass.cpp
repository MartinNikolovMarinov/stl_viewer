#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

void vulkanRenderPassCreate(RendererBackend& backend,
                            VulkanRenderPass& outRenderPass,
                            f32 x, f32 y, f32 w, f32 h,
                            core::vec4f clearColor,
                            f32 depth,
                            u32 stencil) {
    logInfoTagged(LogTag::T_RENDERER, "Vulkan creating render pass...");

    outRenderPass.x = x;
    outRenderPass.y = y;
    outRenderPass.w = w;
    outRenderPass.h = h;
    outRenderPass.clearColor = clearColor;
    outRenderPass.depth = depth;
    outRenderPass.stencil = stencil;

    // Main subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Attachments
    u32 attacmentDescriptionCount = 2; // TODO: make configurable
    VkAttachmentDescription attachmentDescriptions[attacmentDescriptionCount] = {};

    // Color attachment
    {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = backend.swapchain.imageFormat.format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorAttachment.flags = 0;

        attachmentDescriptions[0] = colorAttachment;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
    }

    // Depth attachment
    {
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = backend.device.depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.flags = 0;

        attachmentDescriptions[1] = depthAttachment;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }

    // TODO: Add other attachments types like input, resolve, preserve, etc.
    // Input from a shader
    {
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
    }

    // Resolve, used for multisampling color attachments.
    {
        subpass.pResolveAttachments = nullptr;
    }

    // Preserve, used to preserve attachments from the previous subpass.
    {
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;
    }

    // Renderpass dependencies:
    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = attacmentDescriptionCount;
    renderPassCreateInfo.pAttachments = attachmentDescriptions;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;

    VK_EXPECT(
        vkCreateRenderPass(backend.device.logicalDevice, &renderPassCreateInfo, backend.allocator, &outRenderPass.handle),
        "Failed to create render pass"
    );
}

void vulkanRenderPassDestroy(RendererBackend& backend, VulkanRenderPass& renderPass) {
    logInfoTagged(LogTag::T_RENDERER, "Vulkan destroying render pass.");

    if (renderPass.handle) {
        vkDestroyRenderPass(backend.device.logicalDevice, renderPass.handle, backend.allocator);
        renderPass.handle = VK_NULL_HANDLE;
    }
}

void vulkanRenderPassBegin(VulkanRenderPass& renderPass, VulkanCommandBuffer& cmdBuffer, VkFramebuffer framebuffer) {
    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.framebuffer = framebuffer;
    beginInfo.renderArea.offset.x = i32(renderPass.x);
    beginInfo.renderArea.offset.y = i32(renderPass.y);
    beginInfo.renderArea.extent.width = u32(renderPass.w);
    beginInfo.renderArea.extent.height = u32(renderPass.h);

    VkClearValue clearValues[2] = {};

    clearValues[0].color.float32[0] = renderPass.clearColor.r();
    clearValues[0].color.float32[1] = renderPass.clearColor.g();
    clearValues[0].color.float32[2] = renderPass.clearColor.b();
    clearValues[0].color.float32[3] = renderPass.clearColor.a();

    clearValues[1].depthStencil.depth = renderPass.depth;
    clearValues[1].depthStencil.stencil = renderPass.stencil;

    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmdBuffer.handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer.state = VulkanCommandBufferState::IN_RENDER_PASS;
}

void vulkanRenderPassEnd(VulkanRenderPass&, VulkanCommandBuffer& cmdBuffer) {
    vkCmdEndRenderPass(cmdBuffer.handle);
    cmdBuffer.state = VulkanCommandBufferState::RECORDING;
}

} // namespace stlv
