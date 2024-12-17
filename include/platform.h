#pragma once

// IMPORTANT: The platform layer should not use basic.h or anything from core since there are collisions with some of
//            the platforms internal definitions.

#include <core_types.h>

using namespace coretypes;

typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

struct Platform {
    enum struct Error : i32 {
        SUCCESS,
        FAILED_TO_CREATE_SURFACE,
        SENTINEL
    };
    static constexpr const char* errToCStr(Error err);
    static constexpr bool isOk(Error err);

    static Error init(const char* windowTitle, i32 windowWidth, i32 windowHeight);
    static Error start();

    static void requiredVulkanExtsCount(i32& count);
    static void requiredVulkanExts(const char** extensions);
    static Error createVulkanSurface(VkInstance instance, VkSurfaceKHR surface);
};

constexpr const char* Platform::errToCStr(Platform::Error err) {
    switch (err)
    {
        case Platform::Error::FAILED_TO_CREATE_SURFACE: return "Failed To Create Surface";

        case Platform::Error::SUCCESS: return "success";
        case Platform::Error::SENTINEL: break;
    }
    return "unknown";
}

constexpr bool Platform::isOk(Platform::Error err) {
    return err == Platform::Error::SUCCESS;
}
