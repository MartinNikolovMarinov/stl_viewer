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
    };

    const char* errMsg; // static memory!
    Type type;
};

enum struct RendererError : i32 {
    FAILED_TO_CREATE_VULKAN_INSTANCE,
    FAILED_TO_GET_VULKAN_VERSION,
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
