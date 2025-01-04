#pragma once

#include <basic.h>
#include <app_error.h>

struct RendererInitInfo {
    const char* appName;
};

struct Renderer {
    [[nodiscard]] static core::expected<AppError> init(const RendererInitInfo& info);
    static void drawFrame();
    static void shutdown();
};
