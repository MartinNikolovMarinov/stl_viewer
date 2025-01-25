#pragma once

#include <basic.h>
#include <app_error.h>

enum RendererBackendType : u8 {
    NONE,
    VULKAN
};

struct RendererInitInfo {
    struct VulkanInfo {
        core::Memory<const char*> requiredDeviceExtensions;
        core::Memory<const char*> optionalDeviceExtensions;
        core::Memory<const char*> requiredInstanceExtensions;
        core::Memory<const char*> optionalInstanceExtensions;
        core::Memory<const char*> layers;
    };

    const char* appName = nullptr;
    bool vSyncOn = false;
    RendererBackendType backendType = RendererBackendType::NONE;
    union {
        VulkanInfo vk;
    } backend;

    static RendererInitInfo create(const char* appName, bool vSyncOn);
};

struct Renderer {
    [[nodiscard]] static core::expected<AppError> init(const RendererInitInfo& info);
    static void drawFrame();
    static void resizeTarget(i32 width, i32 height);
    static void shutdown();
};
