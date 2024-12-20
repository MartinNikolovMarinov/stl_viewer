#pragma once

// IMPORTANT: The platform layer should not use basic.h or anything from core since there are collisions with some of
//            the platforms internal definitions (like defer).

#include <core_types.h>
#include <core_assert.h>
#include <app_error.h>
#include <user_input.h>

using namespace coretypes;

typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

struct Platform {
    [[nodiscard]] static AppError init(const char* windowTitle, i32 windowWidth, i32 windowHeight);
    [[nodiscard]] static AppError pollEvent(PlatformEvent& ev, bool block = false);

    static void requiredVulkanExtsCount(i32& count);
    static void requiredVulkanExts(const char** extensions);
    [[nodiscard]] static AppError createVulkanSurface(VkInstance instance, VkSurfaceKHR& surface);
};
