#pragma once

// NOTE: Do not include the whole basic.h here!
#include <core_types.h>

using namespace coretypes;

struct PlatformError {
    enum struct Type : i32 {
        FAILED_TO_INITIALIZE_CORE_LOGGER,

        FAILED_TO_CREATE_X11_DISPLAY,
        FAILED_TO_CREATE_X11_WINDOW,
        FAILED_TO_CREATE_X11_KHR_XLIB_SURFACE,

        FAILED_TO_REGISTER_WIN32_WINDOW,
        FAILED_TO_CREATE_WIN32_WINDOW,
        FAILED_TO_CREATE_WIN32_KHR_SURFACE,
        FAILED_TO_POLL_FOR_WIN32_EVENT,

        FAILED_TO_CREATE_OSX_WINDOW,
        FAILED_TO_CREATE_OSX_METAL_LAYER,
        FAILED_TO_CREATE_OSX_KHR_XLIB_SURFACE,
    };

    const char* errMsg; // static memory!
    Type type;
};

enum struct RendererError : i32 {
    FAILED_TO_CREATE_VULKAN_INSTANCE,
    FAILED_TO_GET_VULKAN_VERSION,
    FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES,
    FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES,
    FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT,
    FAILED_TO_CREATE_VULKAN_DEBUG_MESSENGER,
    FAILED_TO_FIND_GPUS_WITH_VULKAN_SUPPORT,
    FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES,
    FAILED_TO_CREATE_LOGICAL_DEVICE,
    FAILED_TO_QUERY_FOR_PRESENT_QUEUE_SUPPORT,
    FAILED_TO_ENUMERATE_VULKAN_DEVICE_EXTENSION_PROPERTIES,
    FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES,
    FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_FORMATS,
    FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_PRESENT_MODES,
};

struct AppError {
    enum struct Type : i32 {
        OK,
        PLATFORM_ERROR,
        RENDERER_ERROR,
    };

    constexpr AppError() : dummy(true), type(Type::OK) {}

    static AppError createPltErr(PlatformError::Type t, const char* msg);
    static AppError createRendErr(RendererError e);

    union {
        bool dummy;
        PlatformError pltErr;
        RendererError rendErr;
    };
    Type type;

    bool isOk();
    const char* toCStr();
};

#define APP_OK AppError()

constexpr auto createPltErr = AppError::createPltErr;
constexpr auto createRendErr = AppError::createRendErr;
