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

    return ret;
}

void VulkanShader::destroy(VulkanShader& shader, VkDevice logicalDevice) {
    for (addr_size i = 0; i < shader.stages.len(); i++) {
        auto& stage = shader.stages[i];
        VulkanShaderStage::destroy(stage, logicalDevice);
    }

    defer { shader = {}; };

    logInfoTagged(RENDERER_TAG, "Vulkan Shader Destryoed");
}
