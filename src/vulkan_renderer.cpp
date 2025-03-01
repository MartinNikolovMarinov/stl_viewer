#include <app_logger.h>
#include <platform.h>
#include <renderer.h>
#include <vulkan_renderer.h>

namespace {

VulkanContext g_vkctx;

// EXPERIMENTAL SECTION BEGIN
void createRenderPipeline();
void createFrameBuffers(core::Memory<VkFramebuffer> outFrameBuffers);
void createCommandBuffers(core::Memory<VkCommandBuffer> cmdBuffers);
void recordCommandBuffer(VkCommandBuffer cmdBuffer, VkFramebuffer frameBuffer);
void createSemaphores(core::Memory<VkSemaphore> outSemaphores);
void createFences(core::Memory<VkFence> outFences);
void recreateSwapchain();

void createExampleScene();
// EXPERIMENTAL SECTION END

}

core::expected<AppError> Renderer::init(const RendererInitInfo& info) {
    core::setLoggerTag(VULKAN_VALIDATION_TAG, appLogTagsToCStr(VULKAN_VALIDATION_TAG));

    g_vkctx.device = core::Unpack(VulkanDevice::create(info), "Failed to create a device");
    g_vkctx.swapchain = core::Unpack(VulkanSwapchain::create(g_vkctx));

    // Create example shader
    {
        VulkanShader::CreateFromFileInfo shaderCreateInfo = {
            core::sv(STLV_ASSETS "/shaders/mesh_shader.vert.spirv"),
            core::sv(STLV_ASSETS "/shaders/mesh_shader.frag.spirv")
        };
        g_vkctx.shader = VulkanShader::createGraphicsShaderFromFile(shaderCreateInfo, g_vkctx);
    }

    // EXPERIMENTAL SECTION BEGIN
    {
        u32 imageCount = u32(g_vkctx.swapchain.imageViews.len());
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

    // Prepare scene
    {
        createExampleScene();
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
    auto& swapchain = g_vkctx.swapchain;
    auto& graphicsQueue = g_vkctx.device.graphicsQueue;
    auto& presentQueue = g_vkctx.device.presentQueue;

    VK_MUST(vkWaitForFences(device.logicalDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX));

    // Acquire next image from swapchian
    u32 imageIdx;
    {
        VkResult vkres = vkAcquireNextImageKHR(device.logicalDevice,
                                               swapchain.handle,
                                               UINT64_MAX, // TODO2: probably should have some reasonable max
                                               imageAvailableSemaphore,
                                               VK_NULL_HANDLE,
                                               &imageIdx);

        if (vkres == VK_ERROR_OUT_OF_DATE_KHR || vkres == VK_SUBOPTIMAL_KHR || g_vkctx.frameBufferResized) {
            g_vkctx.frameBufferResized = false;
            recreateSwapchain();
            return;
        }

        Panic(vkres == VK_SUCCESS, "Failed to Acquire next image from swapchain.");
    }

    VK_MUST(vkResetFences(device.logicalDevice, 1, &inFlightFence));

    // Record Commands
    {
        auto& cmdBuffer = g_vkctx.cmdBuffers[currentFrame];
        auto& frameBuffer = g_vkctx.frameBuffers[imageIdx];
        VK_MUST(vkResetCommandBuffer(cmdBuffer, 0));
        recordCommandBuffer(cmdBuffer, frameBuffer);

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

        VkResult vkres = vkQueuePresentKHR(presentQueue.handle, &presentInfo);
        if (vkres == VK_ERROR_OUT_OF_DATE_KHR || vkres == VK_SUBOPTIMAL_KHR || g_vkctx.frameBufferResized) {
            g_vkctx.frameBufferResized = false;
            recreateSwapchain();
        }
        else {
            Panic(vkres == VK_SUCCESS, "Failed present image.");
        }
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void Renderer::resizeTarget(i32 width, i32 height) {
    logInfoTagged(RENDERER_TAG, "Window Resized to (w={}, h={})", width, height);
    // TODO: I probably need to set this for Windows.
    // g_vkctx.frameBufferResized = true;
}

void Renderer::shutdown() {
    VK_MUST(vkDeviceWaitIdle(g_vkctx.device.logicalDevice));

    // EXPERIMENTAL SECTION:
    {
        for (addr_size i =0; i < g_vkctx.meshes.len(); i++) {
            Mesh2D::destroy(g_vkctx.device, g_vkctx.meshes[i]);
        }

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
    auto& surface = g_vkctx.device.surface;
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

        // Create Pipeline Layout
        auto bindingDescription = Mesh2D::getBindingDescription();
        auto attributeDescrption = Mesh2D::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
        vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
        vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputCreateInfo.vertexAttributeDescriptionCount = u32(attributeDescrption.len());
        vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescrption.data();

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
        viewport.width = f32(surface.capabilities.extent.width);
        viewport.height = f32(surface.capabilities.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissors{};
        scissors.offset = {0, 0};
        scissors.extent = surface.capabilities.extent;

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
            colorAttachment.format = surface.capabilities.format.format;
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
    auto& surface = g_vkctx.device.surface;

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
        framebufferInfo.width = surface.capabilities.extent.width;
        framebufferInfo.height = surface.capabilities.extent.height;
        framebufferInfo.layers = 1;

        VK_MUST(vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &outFrameBuffers[i]));
    }
}

void createCommandBuffers(core::Memory<VkCommandBuffer> cmdBuffers) {
    auto& device = g_vkctx.device;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = u32(device.graphicsQueue.idx);

    auto& cmdBuffersPool = g_vkctx.cmdBuffersPool;
    VK_MUST(vkCreateCommandPool(device.logicalDevice, &poolInfo, nullptr, &cmdBuffersPool));

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmdBuffersPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = u32(cmdBuffers.len());

    VK_MUST(vkAllocateCommandBuffers(device.logicalDevice, &allocInfo, cmdBuffers.data()));
}

void recordCommandBuffer(VkCommandBuffer cmdBuffer, VkFramebuffer frameBuffer) {
    auto& renderPass = g_vkctx.renderPass;
    auto& graphicsPipeline = g_vkctx.pipeline;
    auto& surface = g_vkctx.device.surface;
    auto& meshes = g_vkctx.meshes;

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
    renderPassInfo.renderArea.extent = surface.capabilities.extent;

    VkClearValue clearValue{};
    clearValue.color = { { 0.3f, 0.6f, 0.9f, 1.0f } };

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
        viewport.width = f32(surface.capabilities.extent.width);
        viewport.height = f32(surface.capabilities.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = surface.capabilities.extent;
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        // TODO: record all vertices with one command in bulk.
        for (addr_size i = 0; i < meshes.len(); i++) {
            VkBuffer vertexBuffers[] = { meshes[i].vertexBuffer };
            VkDeviceSize offsets[] = {0};

            vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdDraw(cmdBuffer, u32(meshes[i].vertexByteSize()), 1, 0, 0);
        }
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

void recreateSwapchain() {
    auto& device = g_vkctx.device;
    auto& swapchain = g_vkctx.swapchain;
    auto& surface = g_vkctx.device.surface;
    bool vSyncOn = device.vSyncOn;

    VK_MUST(vkDeviceWaitIdle(device.logicalDevice));

    // Destroy
    {
        for (addr_size i = 0; i < g_vkctx.frameBuffers.len(); i++) {
            vkDestroyFramebuffer(g_vkctx.device.logicalDevice, g_vkctx.frameBuffers[i], nullptr);
        }
        g_vkctx.frameBuffers.replaceWith(VkFramebuffer{}, g_vkctx.frameBuffers.len());
        VulkanSwapchain::destroy(swapchain, device);
    }

    // Query Surface Capabilities
    {
        VulkanSurface::Capabilities capabilities = VulkanSurface::queryCapabilities(surface, device.physicalDevice);
        surface.capabilities = core::Unpack(VulkanSurface::pickCapabilities(capabilities, vSyncOn),
                                            "Failed to query for new surface capabilities");
    }

    // Create
    {
        swapchain = core::Unpack(VulkanSwapchain::create(g_vkctx));
        createFrameBuffers(g_vkctx.frameBuffers.mem());
    }
}

void createExampleScene() {
    auto& meshes = g_vkctx.meshes;
    auto& device = g_vkctx.device;

    // Prepare Meshes
    {
        Mesh2D quadMesh;

        // Left triangle:
        quadMesh.bindingData.push({ core::v(0.98f, 0.98f), core::v(1.0f, 0.0f, 0.0f, 1.0f) });
        quadMesh.bindingData.push({ core::v(-0.98f, 0.98f), core::v(0.0f, 1.0f, 0.0f, 1.0f) });
        quadMesh.bindingData.push({ core::v(-0.98f, -0.98f), core::v(0.0f, 0.0f, 1.0f, 1.0f) });

        // Right triangle:
        quadMesh.bindingData.push({ core::v(0.98f, 0.98f), core::v(1.0f, 0.0f, 0.0f, 1.0f) });
        quadMesh.bindingData.push({ core::v(-0.98f, -0.98f), core::v(0.0f, 0.0f, 1.0f, 1.0f) });
        quadMesh.bindingData.push({ core::v(0.98f, -0.98f), core::v(0.0f, 1.0f, 0.0f, 1.0f) });

        // Create Vertex Buffer
        VkBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = quadMesh.vertexByteSize();
        vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_MUST(vkCreateBuffer(device.logicalDevice, &vertexBufferInfo, nullptr, &quadMesh.vertexBuffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device.logicalDevice, quadMesh.vertexBuffer, &memRequirements);

        auto findMemoryType = [](u32 typeFilter, VkMemoryPropertyFlags properties) -> u32 {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(g_vkctx.device.physicalDevice, &memProperties);
            for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
                bool isSupported = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;
                if ((typeFilter & (1 << i)) && isSupported) {
                    return i;
                }
            }

            Assert(false, "Failed to find memory type");
            return 0;
        };

        VkMemoryAllocateInfo vertexAllocInfo{};
        vertexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vertexAllocInfo.allocationSize = memRequirements.size;
        vertexAllocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        VK_MUST(vkAllocateMemory(device.logicalDevice, &vertexAllocInfo, nullptr, &quadMesh.vertexBufferMemory));

        vkBindBufferMemory(device.logicalDevice, quadMesh.vertexBuffer, quadMesh.vertexBufferMemory, 0);

        void* data;
        vkMapMemory(device.logicalDevice, quadMesh.vertexBufferMemory, 0, vertexBufferInfo.size, 0, &data);
        core::memcopy(reinterpret_cast<char*>(data),
                      reinterpret_cast<const char*>(quadMesh.bindingData.data()),
                      addr_size(vertexBufferInfo.size) * sizeof(quadMesh.bindingData));
        vkUnmapMemory(device.logicalDevice, quadMesh.vertexBufferMemory);

        meshes.push(std::move(quadMesh));
    }
}

} // namespace
