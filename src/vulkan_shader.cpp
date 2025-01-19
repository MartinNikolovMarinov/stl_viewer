#include <app_logger.h>
#include <vulkan_renderer.h>

namespace {

core::AtomicU32 nextShaderId = 1;

};

core::expected<VulkanShaderStage, AppError> VulkanShaderStage::createFromFile(VkDevice logicalDevice,
                                                                              core::StrView path,
                                                                              Type stageType) {
    core::ArrList<u8> bytes;
    if (auto res = core::fileReadEntire(path.data(), bytes); res.hasErr()) {
        char errBuf[core::MAX_SYSTEM_ERR_MSG_SIZE];
        Assert(core::pltErrorDescribe(res.err(), errBuf), "Failed to describe platform error");
        logErrTagged(RENDERER_TAG, "Failed to load shader file, path: %s, reason: %s", path.data(), errBuf);
        return core::unexpected(createPltErr(PlatformError::Type::FAILED_TO_LOAD_SHADER,
                                             "Failed to load shader file"));
    }

    VulkanShaderStage ret;
    ret.id = nextShaderId.fetch_add(1);
    ret.stageType = stageType;

    // Save the shader bytes.
    {
        [[maybe_unused]] addr_size ignored;
        addr_size shaderBytesLen;
        ret.shaderBytes = bytes.release(shaderBytesLen, ignored);
        ret.shaderBytesSize = u32(shaderBytesLen);
    }

    // Create the shader module
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = ret.shaderBytesSize;
        createInfo.pCode = reinterpret_cast<const u32*>(ret.shaderBytes);

        if (
            VkResult vres = vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &ret.shaderModule);
            vres != VK_SUCCESS
        ) {
            return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_VULKAN_SHADER_MODULE));
        }
    }

    logInfoTagged(RENDERER_TAG, "Created Shader Stage: id=%u, type=%u", ret.id, ret.stageType);

    // Debug trace log the source code of the loaded shader file.
#if STLV_DEBUG
    if (core::getLogLevel() <= core::LogLevel::L_DEBUG) {
        core::StrBuilder modifiedPath = path.slice(0, path.len() - core::cstrLen(".spirv"));
        modifiedPath.append(".debug.src"_sv);

        core::ArrList<u8> shaderSrc;
        if (auto res = core::fileReadEntire(modifiedPath.view().data(), shaderSrc); res.hasErr()) {
            char errBuf[core::MAX_SYSTEM_ERR_MSG_SIZE];
            Assert(core::pltErrorDescribe(res.err(), errBuf), "Failed to describe platform error");
            logErrTagged(RENDERER_TAG, "Failed to load shader debug source, path: %s, reason: %s", path.data(), errBuf);
            // Don't crash because of this
        }
        else {
            logDebugTagged(RENDERER_TAG, "Contents of '%s':\n%s", modifiedPath.view().data(), shaderSrc.data());
        }
    }
#endif

    return ret;
}

void VulkanShaderStage::destroy(VulkanShaderStage& stage, VkDevice logicalDevice) {
    logInfoTagged(RENDERER_TAG, "Destroying Shader Stage (id=%u, type=%u)", stage.id, stage.stageType);

    if (stage.shaderBytes != nullptr) {
        core::getAllocator(core::DEFAULT_ALLOCATOR_ID).free(stage.shaderBytes, stage.shaderBytesSize, stage.shaderBytesSize);
        stage.shaderBytes = nullptr;
        stage.shaderBytesSize = 0;
    }

    defer { stage = {}; };

    if (logicalDevice == VK_NULL_HANDLE) {
        // This is probably a bug.
        logErrTagged(RENDERER_TAG, "Trying to destory a shader stage with an undefined logical device.");
        return;
    }

    if (stage.shaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(logicalDevice, stage.shaderModule, nullptr);
        stage.shaderModule = VK_NULL_HANDLE;
    }
}

VulkanShader VulkanShader::createGraphicsShaderFromFile(const CreateFromFileInfo& info,
                                                        const VulkanContext& vkctx) {
    logInfoTagged(RENDERER_TAG, "Creating a Graphics Shader:");

    auto& device = vkctx.device;
    auto& swapchain = vkctx.swapchain;

    VulkanShader ret;

    // Vertex and Fragment stages are required.
    VulkanShaderStage vertexStage = core::Unpack(VulkanShaderStage::createFromFile(device.logicalDevice,
                                                                                   info.vertexShaderPath,
                                                                                   VulkanShaderStage::VERTEX));
    VulkanShaderStage fragmentStage = core::Unpack(VulkanShaderStage::createFromFile(device.logicalDevice,
                                                                                     info.fragmetShaderPath,
                                                                                     VulkanShaderStage::FRAGMENT));

    ret.stages.push(vertexStage);
    ret.stages.push(fragmentStage);

    // Create Pipeline
    VulkanPipeline pipeline;
    {
        VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
        fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageCreateInfo.module = fragmentStage.shaderModule;
        fragShaderStageCreateInfo.pName = SHADERS_ENTRY_FUNCTION;

        VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
        vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageCreateInfo.module = vertexStage.shaderModule;
        vertShaderStageCreateInfo.pName = SHADERS_ENTRY_FUNCTION;

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
                                       &pipeline.pipelineLayout));
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
                                       &pipeline.renderPass));
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
        pipelineCreateInfo.layout = pipeline.pipelineLayout;
        pipelineCreateInfo.renderPass = pipeline.renderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        VK_MUST(vkCreateGraphicsPipelines(device.logicalDevice,
                                          VK_NULL_HANDLE,
                                          1,
                                          &pipelineCreateInfo,
                                          nullptr,
                                          &pipeline.handle));

        logInfoTagged(RENDERER_TAG, "Graphics Pipeline created");
    }

    ret.pipeline = std::move(pipeline);

    return ret;
}

void VulkanShader::destroy(VulkanShader& shader, VkDevice logicalDevice) {
    for (addr_size i = 0; i < shader.stages.len(); i++) {
        auto& stage = shader.stages[i];
        VulkanShaderStage::destroy(stage, logicalDevice);
    }

    defer { shader = {}; };

    if (logicalDevice == VK_NULL_HANDLE) {
        // This is probably a bug.
        logErrTagged(RENDERER_TAG, "Trying to destroy a Vulkan Shader with logical device equal to null");
        return;
    }

    if (shader.pipeline.handle != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying VkPipeline");
        vkDestroyPipeline(logicalDevice, shader.pipeline.handle, nullptr);
    }

    if (shader.pipeline.pipelineLayout != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Pipeline Layout");
        vkDestroyPipelineLayout(logicalDevice, shader.pipeline.pipelineLayout, nullptr);
    }

    if (shader.pipeline.renderPass != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Render Pass");
        vkDestroyRenderPass(logicalDevice, shader.pipeline.renderPass, nullptr);
    }

    logInfoTagged(RENDERER_TAG, "Vulkan Shader Destryoed");
}
