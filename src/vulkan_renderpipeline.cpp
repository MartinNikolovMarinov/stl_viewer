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

    // Shader stage creation
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = FRAGMENT_SHADER_ENTRY_FUNCTION;

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = VERTEX_SHADER_ENTRY_FUNCTION;

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    return ret;
}

void RenderPipeline::destroy(VkDevice, RenderPipeline& renderPipeline) {
    defer { renderPipeline = {}; };
}
