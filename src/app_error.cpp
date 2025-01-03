#include <app_error.h>

AppError AppError::createPltErr(PlatformError::Type t, const char* msg) {
    AppError res;
    res.type = AppError::Type::PLATFORM_ERROR;
    res.pltErr.errMsg = msg;
    res.pltErr.type = t;
    return res;
}

AppError AppError::createRendErr(RendererError e) {
    AppError res;
    res.rendErr = e;
    res.type = AppError::Type::RENDERER_ERROR;
    return res;
}

bool AppError::isOk() {
    return this->type == AppError::Type::OK;
}

namespace {

constexpr const char* pltErrorToCStr(const PlatformError& e) {
    return e.errMsg ? e.errMsg : "unknown";
}

constexpr const char* rendErrorToCStr(RendererError e) {
    switch (e) {
        case RendererError::FAILED_TO_CREATE_VULKAN_INSTANCE:
            return "Failed to create Vulkan instance";
        case RendererError::FAILED_TO_GET_VULKAN_VERSION:
            return "Failed to get Vulkan version";
        case RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_EXTENSION_PROPERTIES:
            return "Failed to enumerate Vulkan Instance extension properties";
        case RendererError::FAILED_TO_ENUMERATE_VULKAN_INSTANCE_LAYER_PROPERTIES:
            return "Failed to enumerate Vulkan Instance layer properties";
        case RendererError::FAILED_TO_CREATE_INSTANCE_MISSING_REQUIRED_EXT:
            return "Failed to create Vulkan instance due to missing required extension";
        case RendererError::FAILED_TO_CREATE_VULKAN_DEBUG_MESSENGER:
            return "Failed to create Vulkan Debug Messenger";
        case RendererError::FAILED_TO_FIND_GPUS_WITH_VULKAN_SUPPORT:
            return "Failed to find GPUs with Vulkan support";
        case RendererError::FAILED_TO_FIND_GPU_WITH_REQUIRED_FEATURES:
            return "Failed to find GPU with support for required features";
        case RendererError::FAILED_TO_CREATE_LOGICAL_DEVICE:
            return "Failed to create Vulkan logical device";
        case RendererError::FAILED_TO_QUERY_FOR_PRESENT_QUEUE_SUPPORT:
            return "Failed to query for Preset Queue support";
        case RendererError::FAILED_TO_ENUMERATE_VULKAN_DEVICE_EXTENSION_PROPERTIES:
            return "Failed to enumerate Vulkan Device Extension properties";
        case RendererError::FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES:
            return "Failed to get physical device surface capabilities";
        case RendererError::FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_FORMATS:
            return "Failed to get physical device surface formats";
        case RendererError::FAILED_TO_GET_PHYSICAL_DEVICE_SURFACE_PRESENT_MODES:
            return "Failed to get physical device surface present modes";
        case RendererError::FAILED_TO_CREATE_SWAPCHAIN:
            return "Failed to create Swapchain";
        case RendererError::FAILED_TO_GET_SWAPCHAIN_IMAGES:
            return "Failed to get Swapchain Images";
        case RendererError::FAILED_TO_CREATE_SWAPCHAIN_IMAGE_VIEW:
            return "Failed to create Swapchain Image View";
        case RendererError::FAILED_TO_CREATE_VULKAN_SHADER_MODULE:
            return "Failed to create vulkan Shader Module";
    }
    return "unknown";
}

} // namespace

const char* AppError::toCStr() {
    switch (this->type) {
        case Type::OK: return "ok";
        case Type::PLATFORM_ERROR: return pltErrorToCStr(this->pltErr);
        case Type::RENDERER_ERROR: return rendErrorToCStr(this->rendErr);
    }
    return "unknown";
}
