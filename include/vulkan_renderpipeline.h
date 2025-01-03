#pragma once

#include <app_error.h>
#include <basic.h>
#include <vulkan_include.h>

struct RenderPipeline {
    struct CreateInfo {
        VkDevice logicalDevice;
        core::StrView vertShaderPath;
        core::StrView fragShaderPath;
    };

    [[nodiscard]] static core::expected<RenderPipeline, AppError> create(CreateInfo&& info);
    static void destroy(VkDevice logicalDevice, RenderPipeline& renderPipeline);

    core::ArrList<u8> fragShaderBytes;
    core::ArrList<u8> vertShaderBytes;
};
