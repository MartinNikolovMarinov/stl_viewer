#pragma once

#include <basic.h>
#include <app_error.h>

struct ApplicationInfo {
    const char* appName;
    const char* windowTitle;
    i32 initWindowWidth;
    i32 initWindowHeight;
};

struct Application {
    static bool isRunning();
    [[nodiscard]] static core::expected<AppError> init(const ApplicationInfo& appInfo);
    [[nodiscard]] static core::expected<AppError> start();
    static void shutdown();
};
