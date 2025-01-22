#include <app_logger.h>
#include <platform.h>
#include <renderer.h>
#include <vulkan_renderer.h>

// namespace {

// struct RecreateSwapchain {
//     u32 width = 0;
//     u32 height = 0;
//     bool recreate = false;
// };

// constexpr i32 MAX_FRAMES_IN_FLIGHT = 2;
// u32 g_currentFrame = 0;

// } // namespace

// void Renderer::drawFrame() {
//     Assert(vkWaitForFences(g_device, 1, &g_inFlightFences[g_currentFrame], VK_TRUE, core::limitMax<u64>()) == VK_SUCCESS);
//     vkResetFences(g_device, 1, &g_inFlightFences[g_currentFrame]);

//     u32 imageIdx;
//     // TODO: Needs different error handling!
//     vkAcquireNextImageKHR(g_device,
//                           g_swapchain.swapchain,
//                           core::limitMax<u64>(),
//                           g_imageAvailableSemaphores[g_currentFrame],
//                           VK_NULL_HANDLE,
//                           &imageIdx);

//     Assert(vkResetCommandBuffer(g_cmdBufs[g_currentFrame], 0) == VK_SUCCESS);
//     recordCommandBuffer(g_cmdBufs[g_currentFrame], imageIdx, g_renderPipeline, g_swapchainFrameBuffers, g_swapchain);

//     // Submit to the command buffer to the Graphics Queue
//     {
//         VkSubmitInfo submitInfo{};
//         submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//         submitInfo.commandBufferCount = 1;
//         submitInfo.pCommandBuffers = &g_cmdBufs[g_currentFrame];

//         VkSemaphore waitSemaphores[] = { g_imageAvailableSemaphores[g_currentFrame] };
//         constexpr addr_size waitSemaphoresLen = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]);
//         VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
//         submitInfo.waitSemaphoreCount = waitSemaphoresLen;
//         submitInfo.pWaitSemaphores = waitSemaphores;
//         submitInfo.pWaitDstStageMask = waitStages;

//         VkSemaphore signalSemaphores[] = { g_renderFinishedSemaphores[g_currentFrame] };
//         constexpr addr_size signalSemaphoresLen = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);
//         submitInfo.signalSemaphoreCount = signalSemaphoresLen;
//         submitInfo.pSignalSemaphores = signalSemaphores;

//         Assert(vkQueueSubmit(g_graphicsQueue.queue, 1, &submitInfo, g_inFlightFences[g_currentFrame]) == VK_SUCCESS);
//     }

//     // Present
//     {
//         VkPresentInfoKHR presentInfo{};
//         presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

//         VkSemaphore signalSemaphores[] = { g_renderFinishedSemaphores[g_currentFrame] };
//         constexpr addr_size signalSemaphoresLen = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);

//         presentInfo.waitSemaphoreCount = signalSemaphoresLen;
//         presentInfo.pWaitSemaphores = signalSemaphores;

//         VkSwapchainKHR swapchains[] = { g_swapchain.swapchain };
//         constexpr addr_size swapchainsLen = sizeof(swapchains) / sizeof(swapchains[0]);
//         presentInfo.swapchainCount = swapchainsLen;
//         presentInfo.pSwapchains = swapchains;
//         presentInfo.pImageIndices = &imageIdx;
//         presentInfo.pResults = nullptr;

//         // TODO: Needs different error handling!
//         vkQueuePresentKHR(g_presentQueue.queue, &presentInfo);
//     }

//     g_currentFrame = (g_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
// }

// void Renderer::resizeTarget(u32 width, u32 height) {
//     g_swapchainRecreate.height = height;
//     g_swapchainRecreate.width = width;
//     g_swapchainRecreate.recreate = true;
// }

// void Renderer::shutdown() {
//     logInfoTagged(RENDERER_TAG, "Destroying Synchronization objects");
//     for (i32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
//         vkDestroySemaphore(g_device, g_imageAvailableSemaphores[i], nullptr);
//         vkDestroySemaphore(g_device, g_renderFinishedSemaphores[i], nullptr);
//         vkDestroyFence(g_device, g_inFlightFences[i], nullptr);
//     }

//     if (g_commandPool != VK_NULL_HANDLE) {
//         logInfoTagged(RENDERER_TAG, "Destroying Command Pool");
//         vkDestroyCommandPool(g_device, g_commandPool, nullptr);
//         g_commandPool = VK_NULL_HANDLE;
//     }

//     if (!g_swapchainFrameBuffers.empty()) {
//         logInfoTagged(RENDERER_TAG, "Destroying FrameBuffers");
//         for (addr_size i = 0; i < g_swapchainFrameBuffers.len(); i++) {
//             vkDestroyFramebuffer(g_device, g_swapchainFrameBuffers[i], nullptr);
//         }
//         g_swapchainFrameBuffers.free();
//     }

//     Swapchain::destroy(g_device, g_swapchain);
//     RenderPipeline::destroy(g_device, g_renderPipeline);
// }

// namespace {

// core::expected<VkCommandPool, AppError> createCommandPool(VkDevice logicalDevice, const PickedGPUDevice& picked) {
//     VkCommandPoolCreateInfo poolCreateInfo{};
//     poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//     poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//     poolCreateInfo.queueFamilyIndex = picked.graphicsQueueIdx;

//     VkCommandPool ret;
//     if (
//         VkResult vres =vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &ret);
//         vres != VK_SUCCESS
//     ) {
//         return FAILED_TO_CREATE_VULKAN_COMMAND_POOL_ERREXPR;
//     }

//     return ret;
// }

// core::expected<core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>, AppError> createCommandBuffers(VkDevice logicalDevice, VkCommandPool pool) {
//     VkCommandBufferAllocateInfo allocCreateInfo{};
//     allocCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//     allocCreateInfo.commandPool = pool;
//     allocCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//     allocCreateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

//     core::ArrStatic<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> cmdBuffers;
//     if (vkAllocateCommandBuffers(logicalDevice, &allocCreateInfo, cmdBuffers.data()) != VK_SUCCESS) {
//         return FAILED_TO_ALLOCATE_VULKAN_COMMAND_BUFFER_ERREXPR;
//     }

//     return cmdBuffers;
// }

// void recordCommandBuffer(VkCommandBuffer cmdBuffer, u32 imageIdx,
//                          const RenderPipeline& renderPipeline,
//                          const FrameBufferList& frameBuffers,
//                          const Swapchain& swapchain) {
//     VkCommandBufferBeginInfo beginInfo{};
//     beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//     beginInfo.flags = 0;
//     beginInfo.pInheritanceInfo = nullptr;

//     Assert(vkBeginCommandBuffer(cmdBuffer, &beginInfo) == VK_SUCCESS);

//     VkRenderPassBeginInfo renderPassInfo{};
//     renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//     renderPassInfo.renderPass = renderPipeline.renderPass;
//     renderPassInfo.framebuffer = frameBuffers[imageIdx];
//     renderPassInfo.renderArea.offset = {0, 0};
//     renderPassInfo.renderArea.extent = swapchain.extent;

//     VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // BLACK
//     renderPassInfo.clearValueCount = 1;
//     renderPassInfo.pClearValues = &clearColor;

//     {
//         vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//         defer { vkCmdEndRenderPass(cmdBuffer); };

//         vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipeline.graphicsPipeline);

//         VkViewport viewport{};
//         viewport.x = 0.0f;
//         viewport.y = 0.0f;
//         viewport.width = f32(swapchain.extent.width);
//         viewport.height = f32(swapchain.extent.height);
//         viewport.minDepth = 0.0f;
//         viewport.maxDepth = 1.0f;
//         vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

//         VkRect2D scissor{};
//         scissor.offset = {0, 0};
//         scissor.extent = swapchain.extent;
//         vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

//         vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
//     }

//     Assert(vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS);
// }


// } // namespace

namespace {

VulkanContext g_vkctx;

// EXPERIMENTAL SECTION BEGIN
void createRenderPipeline();
void createFrameBuffers(core::Memory<VkFramebuffer> outFrameBuffers);
void createCommandBuffers(core::Memory<VkCommandBuffer> cmdBuffers);
void recordCommandBuffer(u32 imageIdx);
void createSemaphores(core::Memory<VkSemaphore> outSemaphores);
void createFences(core::Memory<VkFence> outFences);
// EXPERIMENTAL SECTION END

}

core::expected<AppError> Renderer::init(const RendererInitInfo& info) {
    core::setLoggerTag(VULKAN_VALIDATION_TAG, appLogTagsToCStr(VULKAN_VALIDATION_TAG));

    g_vkctx.device = core::Unpack(VulkanDevice::create(info), "Failed to create a device");
    g_vkctx.swapchain = core::Unpack(VulkanSwapchain::create(g_vkctx));

    // Create example shader
    {
        VulkanShader::CreateFromFileInfo info = {
            core::sv(STLV_ASSETS "/shaders/shader.vert.spirv"),
            core::sv(STLV_ASSETS "/shaders/shader.frag.spirv")
        };
        g_vkctx.shader = VulkanShader::createGraphicsShaderFromFile(info, g_vkctx);
    }

    // EXPERIMENTAL SECTION BEGIN
    {
        const addr_size imageCount = g_vkctx.swapchain.imageViews.len();
        g_vkctx.maxFramesInFlight = imageCount;

        createRenderPipeline();
        g_vkctx.frameBuffers.replaceWith(VkFramebuffer{}, g_vkctx.maxFramesInFlight);
        createFrameBuffers(g_vkctx.frameBuffers.mem());
        g_vkctx.cmdBuffers.replaceWith(VkCommandBuffer{}, g_vkctx.maxFramesInFlight);
        createCommandBuffers(g_vkctx.cmdBuffers.mem());
        g_vkctx.inFlightFences.replaceWith(VkFence{}, g_vkctx.maxFramesInFlight);
        createFences(g_vkctx.inFlightFences.mem());
        g_vkctx.imageAvailableSemaphores.replaceWith(VkSemaphore{}, g_vkctx.maxFramesInFlight);
        createSemaphores(g_vkctx.imageAvailableSemaphores.mem());
        g_vkctx.renderFinishedSemaphores.replaceWith(VkSemaphore{}, g_vkctx.maxFramesInFlight);
        createSemaphores(g_vkctx.renderFinishedSemaphores.mem());
    }
    // EXPERIMENTAL SECTION END

    return {};
}

void Renderer::drawFrame() {
    // IMPORTANT: Logical Steps for drawing a frame
    //  * Wait for the previous frame to finish
    //  * Acquire an image from the swap chain
    //  * Record a command buffer which draws the scene onto that image
    //  * Submit the recorded command buffer
    //  * Present the swap chain image

    auto& currentFrame = g_vkctx.currentFrame;
    auto& maxFramesInFlight = g_vkctx.maxFramesInFlight;
    auto& device = g_vkctx.device;
    auto& inFlightFence = g_vkctx.inFlightFences[currentFrame];
    auto& imageAvailableSemaphore = g_vkctx.imageAvailableSemaphores[currentFrame];
    auto& renderFinishedSemaphore = g_vkctx.renderFinishedSemaphores[currentFrame];
    auto& cmdBuffer = g_vkctx.cmdBuffers[g_vkctx.currentFrame];
    auto& swapchain = g_vkctx.swapchain;
    auto& graphicsQueue = g_vkctx.device.graphicsQueue;
    auto& presentQueue = g_vkctx.device.presentQueue;

    VK_MUST(vkWaitForFences(device.logicalDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX));
    VK_MUST(vkResetFences(device.logicalDevice, 1, &inFlightFence));

    u32 imageIdx;
    // TODO: Needs different error handling!
    VK_MUST(vkAcquireNextImageKHR(device.logicalDevice, swapchain.handle, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIdx));

    // Record Commands
    {
        VK_MUST(vkResetCommandBuffer(cmdBuffer, 0));
        recordCommandBuffer(imageIdx);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VK_MUST(vkQueueSubmit(graphicsQueue.handle, 1, &submitInfo, inFlightFence));
    }

    // Present
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        constexpr addr_size signalSemaphoresLen = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]);

        presentInfo.waitSemaphoreCount = signalSemaphoresLen;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains[] = { swapchain.handle };
        constexpr addr_size swapchainsLen = sizeof(swapchains) / sizeof(swapchains[0]);
        presentInfo.swapchainCount = swapchainsLen;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIdx;
        presentInfo.pResults = nullptr;

        // TODO: Needs different error handling!
        VK_MUST(vkQueuePresentKHR(presentQueue.handle, &presentInfo));
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void Renderer::resizeTarget(u32 width, u32 height) {

}

void Renderer::shutdown() {
    VK_MUST(vkDeviceWaitIdle(g_vkctx.device.logicalDevice));

    // EXPERIMENTAL SECTION:
    {
        for (addr_size i = 0; i < g_vkctx.inFlightFences.len(); i++)
            vkDestroyFence(g_vkctx.device.logicalDevice, g_vkctx.inFlightFences[i], nullptr);
        for (addr_size i = 0; i < g_vkctx.imageAvailableSemaphores.len(); i++)
            vkDestroySemaphore(g_vkctx.device.logicalDevice, g_vkctx.imageAvailableSemaphores[i], nullptr);
        for (addr_size i = 0; i < g_vkctx.renderFinishedSemaphores.len(); i++)
            vkDestroySemaphore(g_vkctx.device.logicalDevice, g_vkctx.renderFinishedSemaphores[i], nullptr);

        if (g_vkctx.cmdBuffersPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(g_vkctx.device.logicalDevice, g_vkctx.cmdBuffersPool, nullptr);
        }

        for (addr_size i = 0; i < g_vkctx.frameBuffers.len(); i++) {
            vkDestroyFramebuffer(g_vkctx.device.logicalDevice, g_vkctx.frameBuffers[i], nullptr);
        }
        g_vkctx.frameBuffers.clear();

        if (g_vkctx.pipeline != VK_NULL_HANDLE) {
            logInfoTagged(RENDERER_TAG, "Destroying VkPipeline");
            vkDestroyPipeline(g_vkctx.device.logicalDevice, g_vkctx.pipeline, nullptr);
        }

        if (g_vkctx.pipelineLayout != VK_NULL_HANDLE) {
            logInfoTagged(RENDERER_TAG, "Destroying Pipeline Layout");
            vkDestroyPipelineLayout(g_vkctx.device.logicalDevice, g_vkctx.pipelineLayout, nullptr);
        }

        if (g_vkctx.renderPass != VK_NULL_HANDLE) {
            logInfoTagged(RENDERER_TAG, "Destroying Render Pass");
            vkDestroyRenderPass(g_vkctx.device.logicalDevice, g_vkctx.renderPass, nullptr);
        }

        VulkanShader::destroy(g_vkctx.shader, g_vkctx.device.logicalDevice);
    }

    VulkanSwapchain::destroy(g_vkctx.swapchain, g_vkctx.device);
    VulkanDevice::destroy(g_vkctx.device);
}

namespace {

void createRenderPipeline() {
    auto& device = g_vkctx.device;
    auto& swapchain = g_vkctx.swapchain;
    VkShaderModule vertexShaderModule = g_vkctx.shader.stages[0].shaderModule;
    VkShaderModule fragmentShaderModule = g_vkctx.shader.stages[1].shaderModule;
    auto& pipelineLayout = g_vkctx.pipelineLayout;
    auto& renderPass = g_vkctx.renderPass;
    auto& pipeline = g_vkctx.pipeline;

    // Create Pipeline
    {
        VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
        fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageCreateInfo.module = fragmentShaderModule;
        fragShaderStageCreateInfo.pName = VulkanShader::SHADERS_ENTRY_FUNCTION;

        VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
        vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageCreateInfo.module = vertexShaderModule;
        vertShaderStageCreateInfo.pName = VulkanShader::SHADERS_ENTRY_FUNCTION;

        auto shaderStagesCreateInfo = core::createArrStatic(
            vertShaderStageCreateInfo,
            fragShaderStageCreateInfo
        );

        // Create Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
        vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
        vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
        vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
        vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

        // Create Pipeline Layout
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        VK_MUST(vkCreatePipelineLayout(device.logicalDevice,
                                       &pipelineLayoutCreateInfo,
                                       nullptr,
                                       &pipelineLayout));
        logInfoTagged(RENDERER_TAG, "Pipeline Layout created");

        // Create Dynamic state
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };
        constexpr addr_size dynamicStatesLen = sizeof(dynamicStates) / sizeof(dynamicStates[0]);

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = dynamicStatesLen;
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;

        // Create Input Assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
        inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

        // Create Viewports and scissors
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = f32(swapchain.extent.width);
        viewport.height = f32(swapchain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissors{};
        scissors.offset = {0, 0};
        scissors.extent = swapchain.extent;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissors;

        // Create Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
        rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizerCreateInfo.depthClampEnable = VK_FALSE;
        rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizerCreateInfo.lineWidth = 1.0f;
        rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizerCreateInfo.depthBiasClamp = 0.0f;
        rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

        // Create Multisampler
        VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
        multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
        multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingCreateInfo.minSampleShading = 1.0f;
        multisamplingCreateInfo.pSampleMask = nullptr;
        multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

        // Color Blending
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                   VK_COLOR_COMPONENT_G_BIT |
                                                   VK_COLOR_COMPONENT_B_BIT |
                                                   VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = VK_FALSE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
        colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendingCreateInfo.attachmentCount = 1;
        colorBlendingCreateInfo.pAttachments = &colorBlendAttachmentState;
        colorBlendingCreateInfo.blendConstants[0] = 0.0f;
        colorBlendingCreateInfo.blendConstants[1] = 0.0f;
        colorBlendingCreateInfo.blendConstants[2] = 0.0f;
        colorBlendingCreateInfo.blendConstants[3] = 0.0f;

        // Creating Render Pass
        {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = swapchain.imageFormat;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0; // array consists of a single VkAttachmentDescription, so its index is 0.
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkRenderPassCreateInfo renderPassCreateInfo{};
            renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassCreateInfo.attachmentCount = 1;
            renderPassCreateInfo.pAttachments = &colorAttachment;
            renderPassCreateInfo.subpassCount = 1;
            renderPassCreateInfo.pSubpasses = &subpass;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            renderPassCreateInfo.dependencyCount = 1;
            renderPassCreateInfo.pDependencies = &dependency;

            VK_MUST(vkCreateRenderPass(device.logicalDevice,
                                       &renderPassCreateInfo,
                                       nullptr,
                                       &renderPass));
            logInfoTagged(RENDERER_TAG, "Render Pass created");
        }

        // Creating Graphics Pipeline
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = u32(shaderStagesCreateInfo.len());
        pipelineCreateInfo.pStages = shaderStagesCreateInfo.data();
        pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
        pipelineCreateInfo.pDepthStencilState = nullptr;
        pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.layout = pipelineLayout;
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        VK_MUST(vkCreateGraphicsPipelines(device.logicalDevice,
                                          VK_NULL_HANDLE,
                                          1,
                                          &pipelineCreateInfo,
                                          nullptr,
                                          &pipeline));

        logInfoTagged(RENDERER_TAG, "Graphics Pipeline created");
    }
}

void createFrameBuffers(core::Memory<VkFramebuffer> outFrameBuffers) {
    auto& logicalDevice = g_vkctx.device.logicalDevice;
    auto& swapchain = g_vkctx.swapchain;
    auto& renderPass = g_vkctx.renderPass;

    Assert(outFrameBuffers.len() == swapchain.imageViews.len(), "Sanity check failed");

    for (size_t i = 0; i < swapchain.imageViews.len(); i++) {
        VkImageView attachments[] = {
            swapchain.imageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchain.extent.width;
        framebufferInfo.height = swapchain.extent.height;
        framebufferInfo.layers = 1;

        VK_MUST(vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &outFrameBuffers[i]));
    }
}

void createCommandBuffers(core::Memory<VkCommandBuffer> cmdBuffers) {
    auto& device = g_vkctx.device;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device.graphicsQueue.idx;

    auto& cmdBuffersPool = g_vkctx.cmdBuffersPool;
    VK_MUST(vkCreateCommandPool(device.logicalDevice, &poolInfo, nullptr, &cmdBuffersPool));

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmdBuffersPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = u32(cmdBuffers.len());

    VK_MUST(vkAllocateCommandBuffers(device.logicalDevice, &allocInfo, cmdBuffers.data()));
}

void recordCommandBuffer(u32 imageIdx) {
    auto& renderPass = g_vkctx.renderPass;
    auto& frameBuffer = g_vkctx.frameBuffers[imageIdx];
    auto& swapchain = g_vkctx.swapchain;
    auto& cmdBuffer = g_vkctx.cmdBuffers[imageIdx];
    auto& graphicsPipeline = g_vkctx.pipeline;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    VK_MUST(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = frameBuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain.extent;

    VkClearValue clearValue{};
    clearValue.color = { 0.3f, 0.6f, 0.9f, 1.0f };

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    // Begin Render Pass
    {
        vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        defer { vkCmdEndRenderPass(cmdBuffer); };

        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = f32(swapchain.extent.width);
        viewport.height = f32(swapchain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchain.extent;
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
    }

    VK_MUST(vkEndCommandBuffer(cmdBuffer));
}

void createSemaphores(core::Memory<VkSemaphore> outSemaphores) {
    auto& device = g_vkctx.device;

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (addr_size i = 0; i < outSemaphores.len(); i++) {
        VK_MUST(vkCreateSemaphore(device.logicalDevice, &semaphoreCreateInfo, nullptr, &outSemaphores[i]));
    }
}

void createFences(core::Memory<VkFence> outFences) {
    auto& device = g_vkctx.device;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (addr_size i = 0; i < outFences.len(); i++) {
        VK_MUST(vkCreateFence(device.logicalDevice, &fenceCreateInfo, nullptr, &outFences[i]));
    }
}

} // namespace
