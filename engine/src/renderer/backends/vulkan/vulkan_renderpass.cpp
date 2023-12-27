#include <renderer/backends/vulkan/vulkan_backend.h>

namespace stlv {

bool vulkanCreateRenderPass(RendererBackend& backend,
    VulkanRenderPass& renderPass,
    f32 x, f32 y, f32 w, f32 h,
    f32 depth, u32 stencil,
    core::vec4f clearColor
) {
    renderPass.x = x;
    renderPass.y = y;
    renderPass.w = w;
    renderPass.h = h;
    renderPass.depth = depth;
    renderPass.stencil = stencil;
    renderPass.clearColor = clearColor;
    renderPass.state = VulkanRenderPassState::NOT_ALLOCATED;

    /**
     * NOTE: On load and store operations.
     *
     * VkAttachmentLoadOp - Specifies how contents of an attachment are initialized at the beginning of a subpass.
     * VkAttachmentStoreOp - Specifies how contents of an attachment are treated at the end of a subpass.
     *
     * For load operations, there are 3 options (excluding extensions):
     *  VK_ATTACHMENT_LOAD_OP_LOAD - specifies that the previous contents of the image within the render area will be
     *      preserved as the initial values.
     *  VK_ATTACHMENT_LOAD_OP_CLEAR - specifies that the contents of the image will be cleared to a uniform value.
     *  VK_ATTACHMENT_LOAD_OP_DONT_CARE - specifies that the previous contents within the area need not be preserved.
     *
     * For store operations, there are 2 options (excluding extensions):
     *  VK_ATTACHMENT_STORE_OP_STORE - specifies that the contents generated during the render pass and within the
     *      render area are written to memory.
     *  VK_ATTACHMENT_STORE_OP_DONT_CARE - specifies that the contents within the render area are not needed after
     *      rendering, and may be discarded.
    */

    /**
     * NOTE: On layout transitions.
     *
     * initialLayout - Layout to automatically transition from at the start of the render pass.
     * finalLayout - Layout to automatically transition to at the end of the render pass.
     *
     * There are a lot of image layouts to choose from, but the most importnat ones are:
     *  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL - used as a color or resolve attachment in a VkFramebuffer.
     *  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR - used for presenting a swapchain image for display.
     *  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL - used as a depth/stencil attachment in a VkFramebuffer.
    */

    // Attachments

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = backend.swapchain.imageFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = backend.device.depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
    constexpr addr_size attachmentsCount = sizeof(attachments) / sizeof(VkAttachmentDescription);

    // Subpasses

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr; // TODO: add input attachments.
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr; // TODO: add preserve attachments.
    subpass.pResolveAttachments = nullptr; // TODO: add resolve attachments.

    // Subpass Dependencies

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // The src of the first subpass is an implicit subpass that refers to all commands executed before the render pass.
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    // Render Pass

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = u32(attachmentsCount);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VK_EXPECT_OR_RETURN(
        vkCreateRenderPass(backend.device.logicalDevice, &renderPassInfo, backend.allocator, &backend.mainRenderPass.handle),
        "Failed to create Vulkan render pass."
    );

    return true;
}

bool vulkanDestroyRenderPass(RendererBackend& backend, VulkanRenderPass& renderPass) {
    if (renderPass.handle) {
        vkDestroyRenderPass(backend.device.logicalDevice, renderPass.handle, backend.allocator);
        renderPass.handle = VK_NULL_HANDLE;
    }

    renderPass.state = VulkanRenderPassState::NOT_ALLOCATED;
    return true;
}

bool vulkanRenderPassBegin(VulkanCommandBuffer cmdBuffer, VulkanRenderPass& renderPass, VkFramebuffer frameBuffer) {
    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = renderPass.handle;
    beginInfo.framebuffer = frameBuffer;
    beginInfo.renderArea.offset = { i32(renderPass.x), i32(renderPass.y) };
    beginInfo.renderArea.extent = { u32(renderPass.w), u32(renderPass.h) };

    VkClearValue clearValues[2] = {};
    clearValues[0].color = {
        renderPass.clearColor.r(),
        renderPass.clearColor.g(),
        renderPass.clearColor.b(),
        renderPass.clearColor.a()
    };
    clearValues[1].depthStencil.depth = renderPass.depth;
    clearValues[1].depthStencil.stencil = renderPass.stencil;

    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmdBuffer.handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer.state = VulkanCommandBufferState::IN_RENDER_PASS;

    return true;
}

bool vulkanRenderPassEnd(VulkanCommandBuffer cmdBuffer, VulkanRenderPass&) {
    vkCmdEndRenderPass(cmdBuffer.handle);
    cmdBuffer.state = VulkanCommandBufferState::RECORDING;
    return true;
}

} // namespace stlv
