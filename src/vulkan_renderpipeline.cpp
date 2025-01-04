#include <app_logger.h>
#include <vulkan_renderpipeline.h>

constexpr const char* VERTEX_SHADER_ENTRY_FUNCTION = "main";
constexpr const char* FRAGMENT_SHADER_ENTRY_FUNCTION = "main";

namespace {

core::expected<VkShaderModule, AppError> createShaderModule(VkDevice logicalDevice, const core::ArrList<u8>& bytes) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytes.len();
    createInfo.pCode = reinterpret_cast<const u32*>(bytes.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (
        VkResult vres = vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_VULKAN_SHADER_MODULE));
    }

    return shaderModule;
}

} // namespace

core::expected<RenderPipeline, AppError> RenderPipeline::create(CreateInfo&& info) {
    RenderPipeline ret;

    // Read Fragment Shader
    if (auto res = core::fileReadEntire(info.fragShaderPath.data(), ret.fragShaderBytes); res.hasErr()) {
        char errBuf[core::MAX_SYSTEM_ERR_MSG_SIZE];
        Assert(core::pltErrorDescribe(res.err(), errBuf));
        logErrTagged(RENDERER_TAG, "Failed to load fragment shader, path: %s, reason: %s",
                    info.fragShaderPath.data(), errBuf);
        return core::unexpected(createPltErr(PlatformError::Type::FAILED_TO_LOAD_SHADER,
                                             "Failed to load fragment shader"));
    }

    // Read Vertex Shader
    if (auto res = core::fileReadEntire(info.vertShaderPath.data(), ret.vertShaderBytes); res.hasErr()) {
        char errBuf[core::MAX_SYSTEM_ERR_MSG_SIZE];
        Assert(core::pltErrorDescribe(res.err(), errBuf));
        logErrTagged(RENDERER_TAG, "Failed to open vertex shader, path: %s, reason: %s",
                    info.vertShaderPath.data(), errBuf);
        return core::unexpected(createPltErr(PlatformError::Type::FAILED_TO_LOAD_SHADER,
                                             "Failed to load vertex shader"));
    }

    // Create Fragment Shader Module
    VkShaderModule fragShaderModule = VK_NULL_HANDLE;
    {
        auto res = createShaderModule(info.logicalDevice, ret.fragShaderBytes);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        fragShaderModule = res.value();
        logInfoTagged(RENDERER_TAG, "Fragment Shader Module created");
    }
    defer { vkDestroyShaderModule(info.logicalDevice, fragShaderModule, nullptr); };

    // Create Vertex Shader Module
    VkShaderModule vertShaderModule = VK_NULL_HANDLE;
    {
        auto res = createShaderModule(info.logicalDevice, ret.vertShaderBytes);
        if (res.hasErr()) {
            return core::unexpected(res.err());
        }
        vertShaderModule = res.value();
        logInfoTagged(RENDERER_TAG, "Vertex Shader Module created");
    }
    defer { vkDestroyShaderModule(info.logicalDevice, vertShaderModule, nullptr); };

    // Create Shader Stage
    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
    fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageCreateInfo.module = fragShaderModule;
    fragShaderStageCreateInfo.pName = FRAGMENT_SHADER_ENTRY_FUNCTION;

    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
    vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageCreateInfo.module = vertShaderModule;
    vertShaderStageCreateInfo.pName = VERTEX_SHADER_ENTRY_FUNCTION;

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

    if (
        VkResult vres = vkCreatePipelineLayout(info.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &ret.pipelineLayout);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_VULKAN_PIPELINE_LAYOUT));
    }
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
    viewport.width = f32(info.swapchainExtent.width);
    viewport.height = f32(info.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissors{};
    scissors.offset = {0, 0};
    scissors.extent = info.swapchainExtent;

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
        colorAttachment.format = info.swapchainImageFormat;
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

        if (
            VkResult vres = vkCreateRenderPass(info.logicalDevice, &renderPassCreateInfo, nullptr, &ret.renderPass);
            vres != VK_SUCCESS
        ) {
            return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_VULKAN_RENDER_PASS));
        }

        logInfoTagged(RENDERER_TAG, "Render Pass created");
    }

    // Creating Graphics Pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = shaderStagesCreateInfo.len();
    pipelineCreateInfo.pStages = shaderStagesCreateInfo.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = ret.pipelineLayout;
    pipelineCreateInfo.renderPass = ret.renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (
        VkResult vres = vkCreateGraphicsPipelines(info.logicalDevice,
                                                 VK_NULL_HANDLE,
                                                 1,
                                                 &pipelineCreateInfo,
                                                 nullptr,
                                                 &ret.graphicsPipeline);
        vres != VK_SUCCESS
    ) {
        return core::unexpected(createRendErr(RendererError::FAILED_TO_CREATE_VULKAN_GRAPHICS_PIPELINE));
    }

    logInfoTagged(RENDERER_TAG, "Graphics Pipeline created");

    return ret;
}

void RenderPipeline::destroy(VkDevice logicalDevice, RenderPipeline& renderPipeline) {
    defer { renderPipeline = {}; };

    if (logicalDevice == VK_NULL_HANDLE) {
        // This is probably a bug.
        logErrTagged(RENDERER_TAG, "Trying to destroy a Render Pipeline with logical device equal to null");
        return;
    }

    if (renderPipeline.graphicsPipeline != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Graphics Pipeline");
        vkDestroyPipeline(logicalDevice, renderPipeline.graphicsPipeline, nullptr);
    }

    if (renderPipeline.pipelineLayout != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Pipeline Layout");
        vkDestroyPipelineLayout(logicalDevice, renderPipeline.pipelineLayout, nullptr);
    }

    if (renderPipeline.renderPass != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Render Pass");
        vkDestroyRenderPass(logicalDevice, renderPipeline.renderPass, nullptr);
    }
}
