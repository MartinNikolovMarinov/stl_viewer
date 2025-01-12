#pragma once

#include <basic.h>
#include <app_error.h>

struct RendererInitInfo {
    const char* appName;
};

struct Renderer {
    [[nodiscard]] static core::expected<AppError> init(const RendererInitInfo& info);
    static void drawFrame();
    static void resizeTarget(u32 width, u32 height);
    static void shutdown();
};
